#include "jf.h"
#include "json_parse.hpp"
#include "stdio.h"

/*
    make a dif function comparing nodes
    load jf_tree back into a file format (yaml, xml, json)

    recursively load a file tree into memory (filtering out json data, yml data, etc)
    actively monitor each file and evey time it is changed, commit a snapshot

    UI :)
*/

void jf_print_indent(int depth) {
    for (int i = 0; i < depth; ++i)
        printf("  ");
}

enum jf_TreeDiff {
    JF_DIFF_STALE,   // unchanged between two trees
    JF_DIFF_ADDED,   // key appears in tree b but not tree a
    JF_DIFF_REMOVED, // key appears in tree a but not tree b
    JF_DIFF_CHANGED  // value @ key is different between tree 1 and tree 2
};

struct jf_DiffNode;

const char* jf_diff_type_str(jf_TreeDiff diff) {
    switch (diff) {
        case JF_DIFF_ADDED:   return "ADDED";
        case JF_DIFF_REMOVED: return "REMOVED";
        case JF_DIFF_CHANGED: return "CHANGED";
        case JF_DIFF_STALE:   return "STALE";
        default:              return "UNKNOWN";
    }
}

const char* jf_type_str(jf_Type node) {
    switch (node ) {
        case JF_NULL:   return "NULL";
        case JF_STRING: return "STRING";
        case JF_NUMBER: return "NUMBER";
        case JF_BOOL:   return "BOOL";
        case JF_OBJECT: return "OBJECT";
        case JF_ARRAY:  return "ARRAY";
        default:        return "UNKNOWN";
    }
}


void print_key(const char* key, size_t fixed_len) {
    size_t len = strlen(key);

    putchar('[');

    // Print key and pad with '*'
    if (len < fixed_len) {
        printf("%s", key);
        
        for (size_t i = len; i < fixed_len; ++i) {
            putchar('.');
        }

    // Print truncated key with '..' at the end
    } else {
        for (size_t i = 0; i < fixed_len; ++i) {
            if (i >= fixed_len - 2) {
                putchar('.');
            } else {
                putchar(key[i]);
            }
        }
    }

    putchar(']');
}

void jf_print_value_str(jf_Node* node) {
    if (!node) {
        printf("NULL");
        return;
    }

    switch( node->type ) {
        case JF_NULL:   printf("NULL"); return;
        case JF_STRING: printf("%s", node->s_value.str); return;
        case JF_NUMBER: printf("%f", node->n_value); return;
        case JF_OBJECT: printf("<OBJECT>"); return;
        case JF_ARRAY:  printf("<ARRAY>"); return;
        case JF_BOOL:   printf("%s", (node->b_value) ? "TRUE" : "FALSE"); return;
        default:        printf("<UNKNOWN>"); return;
    }
}

struct jf_DiffNode {
    jf_TreeDiff type;
    jf_Bool key_allocated;

    jf_String* key;
    jf_Node* node_a; // val A
    jf_Node* node_b; // val B

    jf_DiffNode* child; // linked list containing the dif of a child node
    jf_DiffNode* next;  // linked list to next node in B tree
};

void jf_print_diff_pair(jf_DiffNode* node, int indent) {
    /*
    +   type: value   
    -   type: value
    ?   type: value -> type: value
    */

    jf_Node* a = node->node_a;
    jf_Node* b = node->node_b;

    // changed
    if (a && b) {
        printf("? ");
        print_key((node->key) ? node->key->str : "unknown", 8);
        jf_print_indent(indent);

        // a val
        printf("   %s: ", jf_type_str(a->type));
        jf_print_value_str(a);
        
        printf(" -> ");
        // b val
        printf("%s: ", jf_type_str(b->type));
        jf_print_value_str(b);

        printf("\n");
    }

    // removed
    else if (a && !b) {
        printf("- ");
        print_key((node->key) ? node->key->str : "unknown", 8);
        jf_print_indent(indent);

        // a val
        printf("   %s: ", jf_type_str(a->type));
        jf_print_value_str(a);
        
        printf("\n");
    }

    // added
    else if (!a && b) {
        printf("+ ");
        print_key((node->key) ? node->key->str : "unknown", 8);
        jf_print_indent(indent);

        // a val
        printf("   %s: ", jf_type_str(b->type));
        jf_print_value_str(b);

        printf("\n");
    }

    // printf("invalid\n");
}

void jf_print_diff_node(jf_DiffNode* node, int indent) {
    while (node) {
        if ((node->node_a || node->node_b)) {
        }

        jf_print_diff_pair(node, indent);

        if (node->child) {
            jf_print_diff_node(node->child, indent + indent);
        }

        node = node->next;
    }
}

jf_Error jf_alloc_diff(jf_DiffNode** node, jf_Node* a, jf_Node* b) {
    *node = (jf_DiffNode*) jf_alloc(sizeof(jf_DiffNode));
    if (!node) { return JF_NO_MEM; }

    (*node)->type = JF_DIFF_STALE;
    (*node)->node_a = a;
    (*node)->node_b = b;
    (*node)->child = NULL;
    (*node)->next = NULL;
    (*node)->key = NULL;
    (*node)->key_allocated = JF_FALSE;

    return JF_SUCCESS;
}

jf_Error jf_diff_alloc_key(jf_DiffNode* node) {
    if (node->key != NULL) {
        return JF_NO_MEM;
    }

    node->key = (jf_String*) jf_alloc(sizeof(jf_String));
    node->key->allocated = JF_FALSE;
    node->key_allocated = JF_TRUE;
    return JF_SUCCESS;
}

jf_Error jf_free_diff(jf_DiffNode* diff) {
    if (!diff) { return JF_NO_REF; }

    jf_DiffNode* next   = diff->next;
    jf_DiffNode* child  = diff->child;
    jf_String* key = diff->key;

    jf_free(diff);

    jf_Error err;

    if (child)  { err = jf_free_diff(child); }
    if (next)   { err = jf_free_diff(next);  }
    if (diff->key_allocated) { // internally allocated key 
        err = jf_string_free(key);
        jf_free(key);
    }

    return err;
}

jf_Error jf_diff_attach_child(jf_DiffNode* head, jf_DiffNode* child) {

    if (head->child) {
        head->child->next = child;
    }

    head->child = child;

    return JF_SUCCESS;
}

jf_Error jf_diff_attach_next(jf_DiffNode** head, jf_DiffNode* next) {
    (*head)->next = next;
    (*head) = next;

    return JF_SUCCESS;
}

jf_Bool jf_diff_updated(jf_DiffNode* diff) {
    while (diff) {
        if (diff->type != JF_DIFF_STALE) {
            return JF_TRUE;
        }

        diff = diff->next;
    }

    return JF_FALSE;
}

jf_Bool jf_diff_contains_key(jf_DiffNode* diff, jf_String* key) {
    while (diff) {
        if (jf_string_compare(diff->key, key)) {
            return JF_TRUE;
        }

        diff = diff->next;
    }

    return JF_FALSE;
}

jf_Error jf_compare_node_diff(jf_DiffNode* tail, jf_Node* a, jf_Node* b) {
    if (!a || !b || a->type != JF_OBJECT || b->type != JF_OBJECT || !tail) {
        return JF_NO_REF;
    }

    jf_Error err;
    jf_DiffNode* head = tail;
    jf_KeyValue* entries_a = a->o_value.entries;
    size_t entries_a_size = a->o_value.used;
    jf_KeyValue* entries_b = b->o_value.entries;
    size_t entries_b_size = b->o_value.used;

    // parse entries a
    for (size_t i = 0; i < entries_a_size; ++i) {
        jf_Bool match = JF_FALSE;

        jf_Node* a_node = entries_a[i].value;
        jf_String* a_key = &entries_a[i].key;

        jf_Node* b_node = NULL;
        jf_String* b_key = NULL;

        // compare keys from A to B
        for (size_t j = 0; j < entries_b_size; ++j) {
            b_key = &entries_b[j].key;
            b_node = entries_b[j].value;
            match = jf_string_compare(a_key, b_key);
            if (match) break; 
        }

        if (!match) { b_node = NULL; }

        // move to next item in linked list
        jf_DiffNode* diff;
        if (err = jf_alloc_diff(&diff, a_node, b_node)) { return err; };
        if (err = jf_diff_attach_next(&tail, diff)) { return err; };

        // assign key
             if (b_node != NULL) { diff->key = b_key; }
        else if (a_node != NULL) { diff->key = a_key; }
    }

    // parse entries b
    for (size_t i = 0; i < entries_b_size; ++i) {
        jf_Bool match = JF_FALSE;

        jf_Node* a_node = NULL;
        jf_String* a_key = NULL;

        jf_Node* b_node = entries_b[i].value;
        jf_String* b_key = &entries_b[i].key;

        // compare keys from B to A
        for (size_t j = 0; j < entries_a_size; ++j) {
            a_key = &entries_a[j].key;
            a_node = entries_a[j].value;
            match = jf_string_compare(a_key, b_key);
            if (match) break; 
        }

        // verify keys and match (cannot have dupes)
        if (!match) { a_node = NULL; }
        else if (jf_diff_contains_key(head, b_key)) { continue; }

        // move to next item in linked list
        jf_DiffNode* diff;
        if (err = jf_alloc_diff(&diff, a_node, b_node)) { return err; };
        if (err = jf_diff_attach_next(&tail, diff)) { return err; };
    
        // assign key
             if (a_node != NULL) { diff->key = a_key; }
        else if (b_node != NULL) { diff->key = b_key; }
    }

    return JF_SUCCESS;
}

jf_Error jf_parse_node_layer_diff(jf_DiffNode* head);

jf_Error jf_compare_array_diff(jf_DiffNode* tail, jf_Array* a, jf_Array* b) {
    if (!a || !b || !tail) { return JF_NO_REF; }

    jf_Error err;

    size_t min_size = JF_MATH_MIN(a->size, b->size);
    size_t max_size = JF_MATH_MAX(a->size, b->size);

    // parse shared indices
    for (size_t i = 0; i < min_size; ++i) {
        jf_Node* node_a = a->elements[i];
        jf_Node* node_b = b->elements[i];

        jf_DiffNode* diff;
        if (err = jf_alloc_diff(&diff, node_a, node_b)) { return err; }
        if (err = jf_diff_alloc_key(diff))              { return err; }
        if (err = jf_string_from_number(diff->key, i))  { return err; }
        
        tail->next = diff;
        tail = diff;
        
        if (node_a->type != node_b->type) {
            diff->type = JF_DIFF_CHANGED;
            continue;
        }

        if (node_a->type == JF_OBJECT) {
            // Recurse into object
            jf_DiffNode* child = NULL;
            if (err = jf_alloc_diff(&child, NULL, NULL)) { return err; };
            if (err = jf_compare_node_diff(child, node_a, node_b)) { return err; };
            if (err = jf_parse_node_layer_diff(child)) { return err; };

            diff->child = child;
            diff->type = jf_diff_updated(child) ? JF_DIFF_CHANGED : JF_DIFF_STALE;
            continue;
        }

        if (node_a->type == JF_ARRAY) {
            // Recurse into array
            jf_DiffNode* child = NULL;
            if (err = jf_alloc_diff(&child, NULL, NULL)) { return err; };
            if (err = jf_compare_array_diff(child, &node_a->a_value, &node_b->a_value)) { return err; };
            if (err = jf_parse_node_layer_diff(child)) { return err; };

            diff->child = child;
            diff->type = jf_diff_updated(child) ? JF_DIFF_CHANGED : JF_DIFF_STALE;
            continue;
        }

        if (!jf_node_compare(node_a, node_b)) {
            diff->type = JF_DIFF_CHANGED;
            continue;
        }
    }

    // parse remaining elements
    for (size_t i = min_size; i < max_size; ++i) {
        jf_Node* node_a = i < a->used ? a->elements[i] : NULL;
        jf_Node* node_b = i < b->used ? b->elements[i] : NULL;

        jf_DiffNode* diff = NULL;
        if (err = jf_alloc_diff(&diff, node_a, node_b)) { return err; }
        if (err = jf_diff_alloc_key(diff))              { return err; }
        if (err = jf_string_from_number(diff->key, i))  { return err; }

        diff->type = (node_a == NULL) ? JF_DIFF_ADDED : JF_DIFF_REMOVED;

        tail->next = diff;
        tail = diff;
    }

    return JF_SUCCESS;
}

jf_Error jf_parse_node_layer_diff(jf_DiffNode* head) {
    if (!head) { return JF_NO_REF; }

    int cnt = 0;

    while (head) {
        ++cnt;
        if (head->node_a && head->node_b) {
            if (head->node_a->type != head->node_b->type) {
                head->type = JF_DIFF_CHANGED;
                goto next;
            }

            if (head->node_a->type == JF_ARRAY) {

                // recurse diff
                jf_Error err;
                jf_DiffNode* child;

                if (err = jf_alloc_diff(&child, NULL, NULL)) { return err; };
                if (err = jf_compare_array_diff(child, &head->node_a->a_value, &head->node_b->a_value)) { return err; };
                if (err = jf_diff_attach_child(head, child)) { return err; };
                head->type = jf_diff_updated(child) ? JF_DIFF_CHANGED : JF_DIFF_STALE;

                goto next;
            }

            if (head->node_a->type == JF_OBJECT) {

                // recurse diff
                jf_Error err;
                jf_DiffNode* child;

                if (err = jf_alloc_diff(&child, NULL, NULL)) { return err; };
                if (err = jf_compare_node_diff(child, head->node_a, head->node_b)) { return err; }
                if (err = jf_parse_node_layer_diff(child)) { return err; }
                if (err = jf_diff_attach_child(head, child)) { return err; }
                head->type = jf_diff_updated(child) ? JF_DIFF_CHANGED : JF_DIFF_STALE;

                goto next;
            }

            head->type = jf_node_compare(head->node_a, head->node_b) ? JF_DIFF_STALE : JF_DIFF_CHANGED;
        }

        // current layer key comparison
        else if ( head->node_a && !head->node_b) { head->type = JF_DIFF_REMOVED; }
        else if (!head->node_a &&  head->node_b) { head->type = JF_DIFF_ADDED;   }

        next:
        head = head->next;
    }

    return JF_SUCCESS;
}

int main() {
    jf_Error err;

    jf_start();

    jf_Node* node_1 = nullptr;
    err = jf_parse_from_json_file(&node_1, "./test1.json");

    jf_Node* node_2 = nullptr;
    err = jf_parse_from_json_file(&node_2, "./test2.json");

    // jf_print_node(node_1);
    // jf_print_node(node_2);

    jf_DiffNode* diff;
    jf_print_error(jf_alloc_diff(&diff, NULL, NULL));
    jf_print_error(jf_compare_node_diff(diff, node_1, node_2));
    jf_print_error(jf_parse_node_layer_diff(diff));

    jf_print_diff_node(diff, 2);

    jf_free_diff(diff);
    jf_node_free(node_1);
    jf_node_free(node_2);
    jf_finish();
    return 0;
}
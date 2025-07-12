#include "jf.h"
#include <cstdlib>
#include "memory.h"
#include "string.h"

int __jf_heap_count__ = 0; 
int __jf_heap_count_alloc__ = 0; 
int __jf_heap_count_free__ = 0;

/*
    STRINGS
*/

jf_Error jf_string_alloc(jf_String* str, const char* data, size_t len) {
    str->str = (char*) jf_alloc(len + 1);
    str->allocated = JF_TRUE;
    
    if (!str->str) { return JF_NO_MEM; }
    memcpy(str->str, data, len);
    str->str[len] = 0;
    str->len = len;

    return JF_SUCCESS;
}

jf_Error jf_string_free(jf_String* str) {
    if (!str->allocated) { return JF_SUCCESS; }

    jf_free(str->str);
    str->len = 0;
    return JF_SUCCESS;
}

jf_Bool jf_string_compare(jf_String* str_a, jf_String* str_b) {
    if (!str_a || !str_a->str || !str_b || !str_b->str) {
        return JF_FALSE;
    }

    str_a->len = strlen(str_a->str);
    str_b->len = strlen(str_b->str);

    if (str_a->len != str_b->len) {
        return JF_FALSE;
    }

    return (jf_Bool) (strncmp(
        str_a->str, 
        str_b->str, 
        JF_MATH_MIN(str_a->len, str_b->len)
    ) == 0);
}

jf_Error jf_string_copy(jf_String* str_a, jf_String* str_b) {
    if (!str_a || !str_a->str || !str_b || !str_b->str) {
        return JF_NO_REF;
    }

    if (str_a->len < str_b->len) {
        str_a->str = (char*) realloc(str_a->str, str_b->len + 1);
    }

    memcpy(str_a->str, str_b->str, str_b->len);
    str_a->len = str_b->len;

    return JF_SUCCESS;
}

jf_Error jf_string_from_number(jf_String* str, double num) {
    if (!str) return JF_NO_REF;

    char buffer[JF_STRING_MAX_NUMBER] = { 0 };

    // Use "%.17g" to ensure round-trippable representation
    int len = snprintf(buffer, sizeof(buffer), "%.17g", num);
    if (len < 0 || len >= JF_STRING_MAX_NUMBER) {
        return JF_INDEX_OUT_OF_BOUNDS;
    }

    return jf_string_alloc(str, buffer, (size_t)len);
}

/*
    KEY VALUE
*/

jf_Error jf_key_value_alloc(jf_KeyValue* kv, jf_String key) {
    if (!kv) return JF_NO_REF;

    jf_Error err = jf_string_alloc(&kv->key, key.str, key.len);
    if (err != JF_SUCCESS) return err;

    kv->value = NULL;
    return JF_SUCCESS;
}

jf_Error jf_key_value_free(jf_KeyValue* kv) {
    jf_Error err;

    if (!kv) { return JF_NO_REF; }
    err = jf_string_free(&kv->key);
    if (err != JF_SUCCESS) return err;

    err = jf_node_free(kv->value);
    if (err != JF_SUCCESS) return err;

    return JF_SUCCESS;
}

/*
    OBJECTS
*/

jf_Error jf_object_alloc(jf_Object* obj, size_t count) {
    if (!obj) { return JF_NO_REF; }

    obj->entries = (jf_KeyValue*) jf_calloc(sizeof(jf_KeyValue), count);
    if (!obj->entries) { return JF_NO_MEM; }

    obj->used = 0;
    obj->size = count;

    return JF_SUCCESS;
}

jf_Error jf_object_free(jf_Object* obj) {
    jf_Error err;

    if (!obj) { return JF_NO_REF; }

    for (size_t i = 0; i < obj->used; ++i) {
        jf_key_value_free(&obj->entries[i]);
    }

    jf_free(obj->entries);
    obj->size = 0;
    obj->used = 0;

    return JF_SUCCESS;
}

/*
    ARRAYS
*/

jf_Error jf_array_alloc(jf_Array* arr, size_t size) {
    if (!arr) return JF_NO_REF;

    arr->elements = (jf_Node**) jf_calloc(size, sizeof(jf_Node*));
    if (!arr->elements) return JF_NO_MEM;

    arr->used = 0;
    arr->size = size;
    return JF_SUCCESS;
}

jf_Error jf_array_free(jf_Array* arr) {
    if (!arr || !arr->elements) return JF_NO_REF;

    for (size_t i = 0; i < arr->size; ++i) {
        if (arr->elements[i]) {
            jf_node_free(arr->elements[i]);
        }
    }

    jf_free(arr->elements);
    arr->elements = NULL;
    arr->size = 0;
    arr->used = 0;

    return JF_SUCCESS;
}

jf_Bool jf_array_contains(jf_Array* arr, jf_Node* node) {
    if (!arr) {
        return JF_FALSE;
    }

    for (int i = 0; i < arr->used; ++i) {
        if (arr->elements[i]->type == node->type && jf_node_compare(arr->elements[i], node)) {
            return JF_TRUE;
        }
    }

    return JF_FALSE;
}

jf_Bool jf_array_compare(jf_Array* a, jf_Array* b) {
    if (a->size != b->size || a->used != b->used) {
        return JF_FALSE;
    }

    for (size_t i = 0; i < a->used; ++i) {
        if (a->elements[i]->type != b->elements[i]->type) {
            printf("array cmp failed 0\n");
            return JF_FALSE;
        }

        if (!jf_node_compare(a->elements[i], b->elements[i])) {
            printf("array cmp failed 1\n");
            return JF_FALSE;
        }
    }

    return JF_TRUE;
}

/*
    NODES
*/

jf_Error jf_node_alloc(jf_Node** node) {
    if (*node) { return JF_NO_MEM; }
    *node = (jf_Node*) jf_alloc(sizeof(jf_Node));
    if (!(*node)) { return JF_NO_MEM; }

    (*node)->type = JF_NULL;
    (*node)->next = nullptr;
    
    return JF_SUCCESS;
}

jf_Error jf_node_free(jf_Node* node) {
    jf_Error err = JF_SUCCESS;
    if (!node) { return JF_NO_REF; }


    switch (node->type) {
        case (JF_STRING) : err = jf_string_free(&node->s_value); break;
        case (JF_OBJECT) : err = jf_object_free(&node->o_value); break;
        case (JF_ARRAY)  : err = jf_array_free (&node->a_value); break;
    }

    jf_Node* next = node->next;
    jf_free(node);
    if (next) {
        return jf_node_free(node->next);
    }

    return JF_SUCCESS;
}

jf_Bool jf_node_compare(jf_Node* a, jf_Node* b) {
    if (a->type != b->type) {
        return JF_FALSE;
    }

    // compares objects
    if (a->type == JF_OBJECT) { // ew
        if (a->o_value.size != b->o_value.size || a->o_value.used != b->o_value.used) {
            return JF_FALSE;
        }

        for (int i = 0; i < a->o_value.size; ++i) {
            jf_KeyValue* entry_a = & a->o_value.entries[i];
            jf_KeyValue* entry_b = & b->o_value.entries[i];

            if (!jf_string_compare(&entry_a->key, &entry_b->key)) {
                return JF_FALSE;
            }

            if (!jf_node_compare(entry_a->value, entry_b->value)) {
                return JF_FALSE;
            }
        }
    }

    // compare primitives (and arrays)
    switch (a->type) {
        case JF_NULL:   return JF_TRUE;
        case JF_BOOL:   return (jf_Bool) (a->b_value == b->b_value);
        case JF_NUMBER: return (jf_Bool) (a->n_value == b->n_value);
        case JF_STRING: return jf_string_compare(&a->s_value, &b->s_value);
        case JF_ARRAY:  return jf_array_compare(&a->a_value, &b->a_value);
    }

    return JF_FALSE;
}

/*
    diffing (timeline comparisions)
*/

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
    node->key_allocated  = JF_TRUE;
    return JF_SUCCESS;
}

jf_Error jf_free_diff(jf_DiffNode* diff) {
    if (!diff) { return JF_NO_REF; }

    jf_DiffNode* next   = diff->next;
    jf_DiffNode* child  = diff->child;
    jf_String*   key    = diff->key;

    jf_Error err;
    
    if (child)  { err = jf_free_diff(child); }
    if (next)   { err = jf_free_diff(next);  }
    if (diff->key_allocated) { // internally allocated key 
        err = jf_string_free(key);
        jf_free(key);
    }

    jf_free(diff);

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

jf_Error jf_compare_object_diff(jf_DiffNode* tail, jf_Object* a, jf_Object* b) {
    if (!a || !tail) {
        return JF_NO_REF;
    }

    if (b == NULL) {
        return jf_one_sided_object_diff(tail, a, JF_DIFF_ADDED);
    }

    jf_Error err;
    jf_DiffNode* head = tail;
    jf_KeyValue* entries_a = a->entries;
    size_t entries_a_size = a->used;
    jf_KeyValue* entries_b = b->entries;
    size_t entries_b_size = b->used;

    // parse entries a, checks for new
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
        if (err = jf_diff_attach_next(&tail, diff))     { return err; };

        // assign key
             if (b_node != NULL) { diff->key = b_key; }
        else if (a_node != NULL) { diff->key = a_key; }
    }

    // parse entries b, checks for item removal
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

    return jf_parse_node_layer_diff(head);
}

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
            if (err = jf_alloc_diff(&child, NULL, NULL))                                 { return err; };
            if (err = jf_compare_object_diff(child, &node_a->o_value, &node_b->o_value)) { return err; };
            // if (err = jf_parse_node_layer_diff(child))                                   { return err; };

            diff->child = child;
            diff->type = jf_diff_updated(child) ? JF_DIFF_CHANGED : JF_DIFF_STALE;

            continue;
        }

        if (node_a->type == JF_ARRAY) {
            // Recurse into array
            jf_DiffNode* child = NULL;
            if (err = jf_alloc_diff(&child, NULL, NULL))                                { return err; };
            if (err = jf_compare_array_diff(child, &node_a->a_value, &node_b->a_value)) { return err; };
            if (err = jf_parse_node_layer_diff(child))                                  { return err; };

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

jf_Error jf_one_sided_object_diff(jf_DiffNode* head, jf_Object* node, jf_TreeDiff type) {
    if (!head || !node) { return JF_NO_REF; }

    jf_Error err;
    jf_KeyValue* entries = node->entries;
    size_t count = node->used;

    for (size_t i = 0; i < count; ++i) {
        jf_String* entry_key  = &entries[i].key;
        jf_Node* entry_node  = entries[i].value;

        jf_DiffNode* child;
        if (err = jf_alloc_diff(&child, entry_node, NULL)) { return err; } ;
        child->key = &entries[i].key;

        if (err = jf_parse_node_layer_diff(child)) { return err; }
        jf_diff_attach_next(&head, child);
    }

    return JF_SUCCESS;
}

jf_Error jf_one_sided_array_diff(jf_DiffNode* head, jf_Array* array, jf_TreeDiff type) {
    if (!head || !array) { return JF_NO_REF; }

    jf_Error err;
    jf_Node** elements = array->elements;
    size_t used = array->used;

    for (size_t i = 0; i < used; ++i) {
        jf_Node* current_node = elements[i];

        jf_DiffNode* child;
        if (err = jf_alloc_diff(&child, current_node, NULL)) { return err; }
        if (err = jf_diff_alloc_key(child))                  { return err; }
        if (err = jf_string_from_number(child->key, i))      { return err; }

        jf_parse_node_layer_diff(child);

        jf_diff_attach_next(&head, child);
    }

    return JF_SUCCESS;
}

jf_Error jf_recurse_one_sided_nodes(jf_DiffNode* head, jf_Node* reference_node) {
    if (!head || !reference_node) { return JF_NO_REF; }
    if (reference_node->type != JF_OBJECT && reference_node->type != JF_ARRAY) { return JF_SUCCESS; }

    jf_Error err;

    jf_DiffNode* child;
    if (err = jf_alloc_diff(&child, NULL, NULL)) { return err; };

    // recurse one sided objects
    if (reference_node->type == JF_OBJECT) {
        if (err = jf_one_sided_object_diff(child, &reference_node->o_value, head->type))
        { return err; }
    }

    // recurse one sided arrays
    if (reference_node->type == JF_ARRAY) {
        if (err = jf_one_sided_array_diff(child, &reference_node->a_value, head->type))
        { return err; }
    }

    if (err = jf_diff_attach_child(head, child)) { return err; }

    return JF_SUCCESS;
}

jf_Error jf_parse_node_layer_diff(jf_DiffNode* head) {
    if (!head) { return JF_NO_REF; }
    
    jf_Error err;

    while (head) {
        if (head->node_a && head->node_b) {
            if (head->node_a->type != head->node_b->type) {
                head->type = JF_DIFF_CHANGED;
                goto next;
            }

            if (head->node_a->type == JF_ARRAY) {

                // recurse diff
                jf_DiffNode* child;
                jf_Array* array_a = &head->node_a->a_value;
                jf_Array* array_b = &head->node_b->a_value;

                if (err = jf_alloc_diff(&child, NULL, NULL))              { return err; };
                if (err = jf_compare_array_diff(child, array_a, array_b)) { return err; };
                if (err = jf_diff_attach_child(head, child))              { return err; };
                head->type = jf_diff_updated(child) ? JF_DIFF_CHANGED : JF_DIFF_STALE;

                goto next;
            }

            if (head->node_a->type == JF_OBJECT) {

                // recurse diff
                jf_DiffNode* child;
                jf_Object* object_a = &head->node_a->o_value;
                jf_Object* object_b = &head->node_b->o_value;

                if (err = jf_alloc_diff(&child, NULL, NULL))                 { return err; };
                if (err = jf_compare_object_diff(child, object_a, object_b)) { return err; }
                // if (err = jf_parse_node_layer_diff(child))                   { return err; }
                if (err = jf_diff_attach_child(head, child))                 { return err; }
                head->type = jf_diff_updated(child) ? JF_DIFF_CHANGED : JF_DIFF_STALE;

                goto next;
            }

            head->type = jf_node_compare(head->node_a, head->node_b) ? JF_DIFF_STALE : JF_DIFF_CHANGED;

            goto next;
        }

        // current layer key comparison
        if (head->node_a) { 
            head->type = JF_DIFF_REMOVED; 
            jf_recurse_one_sided_nodes(head, head->node_a);
        }

        if (head->node_b) { 
            head->type = JF_DIFF_ADDED;
            jf_recurse_one_sided_nodes(head, head->node_b);
        }

        next:
        head = head->next;
    }

    return JF_SUCCESS;
}

/*
    logging and strings
*/

void jf_print_error(jf_Error err) {
    switch (err) {
        case (JF_SUCCESS): printf("ERROR: JF_SUCCESS\n"); return;
        case (JF_NO_MEM): printf("ERROR: JF_NO_MEM\n"); return;
        case (JF_NO_REF): printf("ERROR: JF_NO_REF\n"); return;
        case (JF_INDEX_OUT_OF_BOUNDS): printf("ERROR: JF_INDEX_OUT_OF_BOUNDS\n"); return;
        case (JF_INVALID_SYNTAX): printf("ERROR: JF_INVALID_SYNTAX\n"); return;
        case (JF_INVALID_ESCAPE): printf("ERROR: JF_INVALID_ESCAPE\n"); return;
        case (JF_UNEXPECTED_EOF): printf("ERROR: JF_UNEXPECTED_EOF\n"); return;
        case (JF_INVALID_TYPE): printf("ERROR: JF_INVALID_TYPE\n"); return;
        case (JF_INVALID_FILE_PATH): printf("ERROR: JF_INVALID_FILE_PATH\n"); return;
        default: printf("ERROR: UNKNOWN\n"); return;
    }
}

void jf_print_diff_pair(jf_DiffNode* node, int indent, int count) {
    /*
    +   type: value   
    -   type: value
    ?   type: value -> type: value
    */

    jf_Node* a = node->node_a;
    jf_Node* b = node->node_b;

    // changed
    if (a && b) {
        if (node->type != JF_DIFF_STALE) {
            printf("? ");
        } else {
            printf("  ");
        }
        print_key((node->key) ? node->key->str : "unknown", JF_MAX_NAME_LEN);
        jf_print_indent(indent * count);

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
        print_key((node->key) ? node->key->str : "unknown", JF_MAX_NAME_LEN);
        jf_print_indent(indent * count);

        // a val
        printf("   %s: ", jf_type_str(a->type));
        jf_print_value_str(a);
        
        printf("\n");
    }

    // added
    else if (!a && b) {
        printf("+ ");
        print_key((node->key) ? node->key->str : "unknown", JF_MAX_NAME_LEN);
        jf_print_indent(indent);

        // a val
        printf("   %s: ", jf_type_str(b->type));
        jf_print_value_str(b);

        printf("\n");
    }

    // printf("invalid\n");
}

void jf_print_diff_node(jf_DiffNode* node, int indent, int count) {
    while (node) {
        jf_print_diff_pair(node, indent, count);

        if (node->child) {
            jf_print_diff_node(node->child, indent, count + 1);
        }

        node = node->next;
    }
}

void jf_print_indent(int depth) {
    for (int i = 0; i < depth; ++i)
        printf("  ");
}

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


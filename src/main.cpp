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

void jf_print_node(jf_Node* node, int depth = 0) {
    if (!node) {
        jf_print_indent(depth);
        printf("null\n");
        return;
    }

    switch (node->type) {
        case JF_NULL:
            jf_print_indent(depth);
            printf("null");
            break;

        case JF_BOOL:
            jf_print_indent(depth);
            printf(node->b_value == JF_TRUE ? "true\n" : "false");
            break;

        case JF_NUMBER:
            jf_print_indent(depth);
            printf("%f", node->n_value);
            break;

        case JF_STRING:
            jf_print_indent(depth);
            printf("\"%.*s\"", (int)node->s_value.len, node->s_value.str);
            break;

        case JF_ARRAY: {
            jf_print_indent(depth);
            printf("[\n");

            for (size_t i = 0; i < node->a_value.used; ++i) {
                jf_print_node(node->a_value.elements[i], depth + 1);
                if (i < node->a_value.used - 1)
                    printf(",");
                printf("\n");
            }

            jf_print_indent(depth);
            printf("]");
            break;
        }

        case JF_OBJECT: {
            jf_print_indent(depth);
            printf("{\n");

            for (size_t i = 0; i < node->o_value.used; ++i) {
                jf_KeyValue* kv = &node->o_value.entries[i];

                jf_print_indent(depth + 1);
                printf("\"%.*s\": ", (int)kv->key.len, kv->key.str);
                jf_print_node(kv->value, depth + 1);

                if (i < node->o_value.used - 1)
                    printf(",");
                printf("\n");
            }

            jf_print_indent(depth);
            printf("}");
            break;
        }

        default:
            jf_print_indent(depth);
            printf("/* unknown type */\n");
            break;
    }

    // Print linked sibling nodes (if any)
    if (node->next) {
        printf(",\n");
        jf_print_node(node->next, depth);
    }
}

enum jf_TreeDiff {
    JF_DIFF_STALE,   // unchanged between two trees
    JF_DIFF_ADDED,   // key appears in tree b but not tree a
    JF_DIFF_REMOVED, // key appears in tree a but not tree b
    JF_DIFF_CHANGED  // value @ key is different between tree 1 and tree 2
};

struct jf_DiffNode {
    jf_TreeDiff diff;

    jf_Node* node_a; // val A
    jf_Node* node_b; // val B

    jf_DiffNode* next;
};

jf_Error jf_alloc_diff(jf_DiffNode** tail, jf_TreeDiff type, jf_Node* a, jf_Node* b) {
    jf_DiffNode* new_node = (jf_DiffNode*) jf_alloc(sizeof(jf_DiffNode));
    if (!new_node) {
        return JF_NO_MEM;
    }

    new_node->diff = type;
    new_node->node_a = a;
    new_node->node_b = b;

    if (*tail) { (*tail)->next = new_node; }
    *tail = new_node;

    return JF_SUCCESS;
}

jf_Error jf_free_diff(jf_TreeDiff* diff) {
    // IMPL
    return JF_SUCCESS;
}

jf_Bool jf_compare_primitives(jf_Node* a, jf_Node* b) {
    if (!a || !b || a->type != b->type) {
        return JF_FALSE;
    }

    switch (a->type) {
        case JF_NULL:   return JF_TRUE;
        case JF_BOOL:   return (jf_Bool) (a->b_value == b->b_value);
        case JF_NUMBER: return (jf_Bool) (a->n_value == b->n_value);
        case JF_STRING: return jf_string_compare(&a->s_value, &b->s_value);
    }

    return JF_FALSE;
}

jf_Error jf_compare_nodes(jf_DiffNode* diff, jf_Node* a, jf_Node* b) {
    if (!diff) { return JF_NO_REF; }
    
    jf_DiffNode* tail = diff;

    return JF_SUCCESS;
}

int main() {
    jf_Error err;

    jf_start();

    jf_Node* node_1 = nullptr;
    err = jf_parse_from_json_file(&node_1, "./test1.json");

    jf_Node* node_2 = nullptr;
    err = jf_parse_from_json_file(&node_2, "./test2.json");

    jf_print_node(node_1);
    jf_print_node(node_2);

    jf_node_free(node_1);
    jf_node_free(node_2);
    jf_finish();
    return 0;
}

/*
    node 1, node 2 

    add
        - when var (key) is in node 2 but not node 1
    change
        - when var (val) is different in node 2 than node 1
    remove
        - when var (key) is in node 1 but not node 2
    stale
        - unchanged
*/
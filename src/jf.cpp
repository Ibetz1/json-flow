#include "jf.h"
#include <cstdlib>
#include "memory.h"
#include "string.h"
#include "json_parse.hpp"

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
            return JF_FALSE;
        }

        if (!jf_node_compare(a->elements[i], b->elements[i])) {
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

jf_Error jf_diff_alloc(jf_DiffNode** node, jf_Node* a, jf_Node* b) {
    *node = (jf_DiffNode*) jf_alloc(sizeof(jf_DiffNode));
    if (!node) { return JF_NO_MEM; }

    (*node)->type = JF_DIFF_STALE;
    (*node)->node_a = a;
    (*node)->node_b = b;
    (*node)->child = NULL;
    (*node)->next = NULL;
    (*node)->key = NULL;
    (*node)->key_allocated = JF_FALSE;
    (*node)->shallow_child = JF_FALSE;
    (*node)->shallow_list  = JF_FALSE;

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

jf_Error jf_diff_free(jf_DiffNode* diff) {
    if (!diff) { return JF_NO_REF; }

    jf_DiffNode* next   = diff->next;
    jf_DiffNode* child  = diff->child;
    jf_String*   key    = diff->key;

    jf_Error err;
    
    if (child && !diff->shallow_child) { err = jf_diff_free(child); }
    if (next  && !diff->shallow_list)  { err = jf_diff_free(next);  }
    if (diff->key_allocated) { // internally allocated key 
        err = jf_string_free(key);
        jf_free(key);
    }

    jf_free(diff);

    return err;
}

jf_Error jf_force_diff_state(jf_DiffNode* head, jf_TreeDiff state) {
    jf_Error err;

    while (head) {
        head->type = state;

        if (head->child) {
            if (err = jf_force_diff_state(head->child, state)) { return err; }
        }

        head = head->next;
    }

    return JF_SUCCESS;
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

        if (diff->child && jf_diff_updated(diff->child)) {
            return JF_TRUE;
        }

        diff = diff->next;
    }

    return JF_FALSE;
}

jf_Bool jf_diff_match_key(jf_DiffNode* diff, jf_String* key, jf_DiffNode** child) {
    while (diff && key) {
        if (jf_string_compare(diff->key, key)) {
            if (child != NULL) { *child = diff; }
            return JF_TRUE;
        }

        diff = diff->next;
    }

    return JF_FALSE;
}

jf_Error jf_diff_filter_type(jf_DiffNode** filtered, jf_DiffNode* to_filter, const jf_TreeDiff* types, size_t type_count) {
    if (!filtered || !to_filter) return JF_NO_REF;

    jf_Error err;

    if ((err = jf_diff_alloc(filtered, NULL, NULL)) != JF_SUCCESS) return err;
    jf_DiffNode* tail = *filtered;
    jf_DiffNode* cur = to_filter;

    while (cur) {
        jf_Bool match = JF_FALSE;
        
        if (cur->type == JF_DIFF_UNKNOWN) {
            jf_Bool match = JF_TRUE;
        } else {
            for (size_t i = 0; i < type_count; ++i) {
                if (cur->type == types[i]) {
                    match = JF_TRUE;
                    break;
                }
            }
        }

        if (match) {
            // Allocate a new node
            jf_DiffNode* new_node;
            if ((err = jf_diff_alloc(&new_node, cur->node_a, cur->node_b)) != JF_SUCCESS) return err;
            new_node->type = cur->type;
            new_node->shallow_child = JF_TRUE;
            new_node->key = cur->key;

            // Reference the original children without filtering
            new_node->child = cur->child;

            // Append to filtered list
            if ((err = jf_diff_attach_next(&tail, new_node)) != JF_SUCCESS) return err;
        }

        cur = cur->next;
    }

    return JF_SUCCESS;
}

jf_Error jf_diff_filter_path(jf_DiffNode* diff, jf_DiffNode** out, jf_String* path, size_t path_len) {
    if (!diff || !path) return JF_NO_REF;

    jf_Error err;
    jf_Bool match = JF_FALSE;

    if (path_len > 0) {
        match = jf_diff_match_key(diff, &path[0], &diff); // sets diff to matched diff

        if (match) {
            if (path_len > 1) {
                if (!diff->child) { return JF_SUCCESS; }
                return jf_diff_filter_path(diff->child, out, &path[1], path_len - 1);
            } else {
                if (jf_diff_alloc(out, diff->node_a, diff->node_b)) { return JF_NO_MEM; }
                (*out)->shallow_child = JF_TRUE;
                (*out)->shallow_list = JF_TRUE;
                (*out)->key = diff->key;
                (*out)->child = diff->child;
                (*out)->type = diff->type;
            }
        }
    }

    return JF_SUCCESS;
}

jf_Error jf_compare_object_diff(jf_DiffNode* tail, jf_Object* a, jf_Object* b) {
    if (!tail || (a == NULL && b == NULL)) { return JF_NO_REF; }

    if (b == NULL) { return jf_one_sided_object_diff(tail, a, JF_DIFF_ADDED); }
    if (a == NULL) { return jf_one_sided_object_diff(tail, b, JF_DIFF_REMOVED); }

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
        if (err = jf_diff_alloc(&diff, a_node, b_node)) { return err; };
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
        else if (jf_diff_match_key(head, b_key)) { continue; }

        // move to next item in linked list
        jf_DiffNode* diff;
        if (err = jf_diff_alloc(&diff, a_node, b_node)) { return err; };
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
        if (err = jf_diff_alloc(&diff, node_a, node_b)) { return err; }
        if (err = jf_diff_alloc_key(diff))              { return err; }
        if (err = jf_string_from_number(diff->key, i))  { return err; }

        jf_diff_attach_next(&tail, diff);
        
        if (node_a->type != node_b->type) {
            diff->type = JF_DIFF_CHANGED;
            continue;
        }

        if (node_a->type == JF_OBJECT) {
            // Recurse into object
            jf_DiffNode* child = NULL;
            if (err = jf_diff_alloc(&child, NULL, NULL))                                 { return err; };
            if (err = jf_compare_object_diff(child, &node_a->o_value, &node_b->o_value)) { return err; };
            diff->child = child;
            // jf_diff_attach_child(diff, child);

            diff->type = jf_diff_updated(child) ? JF_DIFF_CHANGED : JF_DIFF_STALE;

            continue;
        }

        if (node_a->type == JF_ARRAY) {
            // Recurse into array
            jf_DiffNode* child = NULL;
            if (err = jf_diff_alloc(&child, NULL, NULL))                                { return err; };
            if (err = jf_compare_array_diff(child, &node_a->a_value, &node_b->a_value)) { return err; };
            if (err = jf_parse_node_layer_diff(child))                                  { return err; };
            // jf_diff_attach_child(diff, child);
            
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
        if (err = jf_diff_alloc(&diff, node_a, node_b)) { return err; }
        if (err = jf_diff_alloc_key(diff))              { return err; }
        if (err = jf_string_from_number(diff->key, i))  { return err; }

        diff->type = (node_a == NULL) ? JF_DIFF_ADDED : JF_DIFF_REMOVED;

        tail->next = diff;
        tail = diff;
    }

    return JF_SUCCESS;
}

jf_Error jf_one_sided_object_diff(jf_DiffNode* tail, jf_Object* node, jf_TreeDiff type) {
    if (!tail || !node) { return JF_NO_REF; }

    jf_Error err;
    jf_DiffNode* head = tail;
    jf_KeyValue* entries = node->entries;
    size_t count = node->used;

    for (size_t i = 0; i < count; ++i) {
        jf_String* entry_key  = &entries[i].key;
        jf_Node* entry_node  = entries[i].value;

        jf_DiffNode* child;
        if (err = jf_diff_alloc(&child, entry_node, NULL)) { return err; } ;
        child->key = &entries[i].key;
        child->type = type;

        if (err = jf_parse_node_layer_diff(child)) { return err; }

        jf_diff_attach_next(&tail, child);
    }

    jf_force_diff_state(head, type);
    return JF_SUCCESS;
}

jf_Error jf_one_sided_array_diff(jf_DiffNode* tail, jf_Array* array, jf_TreeDiff type) {
    if (!tail || !array) { return JF_NO_REF; }

    jf_Error err;
    jf_DiffNode* head = tail;
    jf_Node** elements = array->elements;
    size_t used = array->used;

    for (size_t i = 0; i < used; ++i) {
        jf_Node* current_node = elements[i];

        jf_DiffNode* child;
        if (err = jf_diff_alloc(&child, current_node, NULL)) { return err; }
        if (err = jf_diff_alloc_key(child))                  { return err; }
        if (err = jf_string_from_number(child->key, i))      { return err; }

        jf_parse_node_layer_diff(child);

        jf_diff_attach_next(&tail, child);
    }

    jf_force_diff_state(head, type);
    return JF_SUCCESS;
}

jf_Error jf_recurse_one_sided_nodes(jf_DiffNode* head, jf_Node* reference_node) {
    if (!head || !reference_node) { return JF_NO_REF; }
    if (reference_node->type != JF_OBJECT && reference_node->type != JF_ARRAY) { return JF_SUCCESS; }

    jf_Error err;

    jf_DiffNode* child;
    if (err = jf_diff_alloc(&child, NULL, NULL)) { return err; };

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

    // if (err = jf_force_diff_state(head, head->type)) { return err; }
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

                if (err = jf_diff_alloc(&child, NULL, NULL))              { return err; };
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

                if (err = jf_diff_alloc(&child, NULL, NULL))                 { return err; };
                if (err = jf_compare_object_diff(child, object_a, object_b)) { return err; }
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



// TODO fix to allocate timeline & buffers in 1 alloc
jf_Error jf_timeline_context_alloc(jf_TimelineContext** context, size_t num_entries) {
    *context = (jf_TimelineContext*) jf_alloc(sizeof(jf_TimelineContext));

    (*context)->files = (jf_String*)    jf_calloc(sizeof(jf_String),    num_entries);
    (*context)->nodes = (jf_Node**)     jf_calloc(sizeof(jf_Node*),     num_entries);
    (*context)->diffs = (jf_DiffNode**) jf_calloc(sizeof(jf_DiffNode*), num_entries);
    (*context)->size = num_entries;

    return JF_SUCCESS;
}

jf_Error jf_timeline_context_free(jf_TimelineContext* context) {
    jf_Error error;

    for (int i = 0; i < context->size; ++i) {
        if (context->nodes[i]) {
            if (error = jf_node_free(context->nodes[i])) { return error; };
        }
    }

    for (int i = 0; i < context->size; ++i) {
        if (context->diffs[i]) {
            if (error = jf_diff_free(context->diffs[i])) { return error; };
        }

        if (&context->files[i]) {
            if (error = jf_string_free(&context->files[i])) { return error; };
        }
    }
    
    jf_free(context->files);
    jf_free(context->nodes);
    jf_free(context->diffs);
    jf_free(context);

    return JF_SUCCESS;
}

// ALLOCATES STRINGS
jf_Error jf_timeline_context_add_files(jf_TimelineContext* context, const jf_String* files) {
    jf_Error error;

    for (size_t i = 0; i < context->size; ++i) {
        if (error = jf_string_alloc(&context->files[i], files[i].str, files[i].len)) { return error; };
    }

    return JF_SUCCESS;
}

/*
    timeine - contains parsed diffs in order
*/

jf_Error jf_timeline_alloc(jf_Timeline** timeline) {
    *timeline = (jf_Timeline*) jf_alloc(sizeof(jf_Timeline));
    
    (*timeline)->version      = 0;
    (*timeline)->free_entries = JF_FALSE;
    (*timeline)->entry        = NULL;
    (*timeline)->prev         = NULL;
    (*timeline)->next         = NULL;

    return JF_SUCCESS;
}

jf_Error jf_timeline_free(jf_Timeline* timeline) {
    jf_Error err;
    
    if (timeline->free_entries && timeline->entry) {
        if (err = jf_diff_free(timeline->entry)) { return err; }
    }

    jf_free(timeline);

    jf_Timeline* next = timeline->next;
    if (next) { return jf_timeline_free(next); }

    return JF_SUCCESS;
}

// will link the back most node of b to the front most node of a
jf_Error jf_timeline_attach(jf_Timeline* a, jf_Timeline* b) {
    if (!a || !b) { return JF_NO_REF; }
    
    jf_Timeline* cur_front = a;
    jf_Timeline* cur_back  = b;

    // find back and front most nodes
    while (cur_front->next) { cur_front = cur_front->next; }
    while (cur_back->prev) { cur_back = cur_back->prev; }

    // link nodes front and back
    cur_front->next = cur_back;
    cur_back->prev = cur_front;
    
    return JF_SUCCESS;
}

jf_Error jf_timeline_build_from_file_names(jf_Timeline** timeline, jf_TimelineContext* context) {
    jf_Error err;

    if (!timeline || !context) { return JF_NO_REF; }

    if (err = jf_timeline_alloc(timeline)) { return err; };
    jf_Timeline* current_timeline = *timeline;
    
    for (int i = 0; i < context->size; ++i) {
        jf_Node* node = NULL;
        jf_DiffNode* diff = NULL;
        jf_Timeline* new_timeline = NULL;
        
        if (err = jf_parse_from_json_file(&node, context->files[i])) { return err; }
        if (err = jf_diff_alloc(&diff, NULL, NULL)) { return err; }
        
        // first node
        if (i == 0) {
            if (err = jf_compare_object_diff(diff, &node->o_value, NULL)) { return err; };
            
        // every node after
        } else {
            if (err = jf_compare_object_diff(diff, &context->nodes[i - 1]->o_value, &node->o_value)) { return err; };
            if (err = jf_timeline_alloc(&new_timeline));
        }

        if (new_timeline != NULL) {
            if (err = jf_timeline_attach(current_timeline, new_timeline)) { return err; }
            while (current_timeline->next) {
                current_timeline = current_timeline->next;
            }
        }
        
        current_timeline->version = i;
        current_timeline->entry = diff;

        context->nodes[i] = node;
        context->diffs[i] = diff;
    }

    return JF_SUCCESS;
}

jf_Error jf_timeline_filter_path(jf_Timeline* main_timeline, jf_Timeline** filtered, jf_String* path, size_t path_len) {
    jf_Error err;
    if ((err = jf_timeline_alloc(filtered))) return err;
    (*filtered)->free_entries = JF_TRUE;

    jf_DiffNode* matched = NULL;
    jf_Timeline* current_filtered = *filtered;
    jf_Timeline* current_timeline = main_timeline;

    jf_Timeline* last_valid = NULL;
    size_t num_matches = 0;

    while (current_timeline) {
        if (err = jf_diff_filter_path(current_timeline->entry, &matched, path, path_len)) { return err; }

        if (matched && jf_diff_updated(matched)) {
            current_filtered->entry = matched;
            current_filtered->version = num_matches++;

            // Preemptively allocate next only if another timeline node exists
            if (current_timeline->next) {
                jf_Timeline* next_filter;
                if ((err = jf_timeline_alloc(&next_filter))) return err;
                next_filter->free_entries = JF_TRUE;

                jf_timeline_attach(current_filtered, next_filter);
                last_valid = current_filtered;
                current_filtered = next_filter;
            }
        }

        current_timeline = current_timeline->next;
    }

    // If the last allocated timeline node has no match, free it
    if (current_filtered && current_filtered->entry == NULL && last_valid) {
        last_valid->next = NULL;
        jf_timeline_free(current_filtered);
    }

    return JF_SUCCESS;
}

/*
    logging and strings
*/

void jf_print_error(jf_Error err) {
    switch (err) {
        case (JF_SUCCESS): printf("JF-RET: JF_SUCCESS\n"); return;
        case (JF_NO_MEM): printf("JF-RET: JF_NO_MEM\n"); return;
        case (JF_NO_REF): printf("JF-RET: JF_NO_REF\n"); return;
        case (JF_INDEX_OUT_OF_BOUNDS): printf("JF-RET: JF_INDEX_OUT_OF_BOUNDS\n"); return;
        case (JF_INVALID_SYNTAX): printf("JF-RET: JF_INVALID_SYNTAX\n"); return;
        case (JF_INVALID_ESCAPE): printf("JF-RET: JF_INVALID_ESCAPE\n"); return;
        case (JF_UNEXPECTED_EOF): printf("JF-RET: JF_UNEXPECTED_EOF\n"); return;
        case (JF_INVALID_TYPE): printf("JF-RET: JF_INVALID_TYPE\n"); return;
        case (JF_INVALID_FILE_PATH): printf("JF-RET: JF_INVALID_FILE_PATH\n"); return;
        default: printf("JF-RET: UNKNOWN\n"); return;
    }
}

void jf_print_diff_action(jf_TreeDiff type) {
    switch (type) {
        case (JF_DIFF_ADDED)   : printf("+ "); break;
        case (JF_DIFF_REMOVED) : printf("- "); break;
        case (JF_DIFF_STALE)   : printf("  "); break;
        case (JF_DIFF_CHANGED) : printf("? "); break;
    }
}

jf_TreeDiff jf_match_diff_action(jf_Node* a, jf_Node* b) {
    if (a && b) { 
        if (jf_node_compare(a, b)) {
            return JF_DIFF_STALE; 
        }

        return JF_DIFF_CHANGED;
    }
    if (a) { return JF_DIFF_REMOVED; }
    if (b) { return JF_DIFF_ADDED; }

    return JF_DIFF_UNKNOWN;
}

void jf_print_diff_pair(const jf_DiffNode* node, int indent, int count) {
    /*
    +   type: value   
    -   type: value
    ?   type: value -> type: value
    */

    jf_Node* a = node->node_a;
    jf_Node* b = node->node_b;

    // changed
    if (a && b) {
        jf_TreeDiff action = jf_match_diff_action(node->node_a, node->node_b);

        jf_print_diff_action(node->type);
        // jf_print_diff_action(action);

        print_key((node->key) ? node->key->str : "unknown", JF_MAX_NAME_LEN);
        jf_print_indent(indent * count);

        // a val
        printf("   %s: ", jf_type_str(a->type));
        jf_print_value_str(a);
    
        if (action != JF_DIFF_STALE) {
            printf(" -> ");
            // b val
            printf("%s: ", jf_type_str(b->type));
            jf_print_value_str(b);
        }

        printf("\n");
    }

    // removed
    else if (a) {
        jf_print_diff_action(node->type);
        // jf_print_diff_action(jf_match_diff_action(node->node_a, node->node_b));
        print_key((node->key) ? node->key->str : "unknown", JF_MAX_NAME_LEN);
        jf_print_indent(indent * count);

        // a val
        printf("   %s: ", jf_type_str(a->type));
        jf_print_value_str(a);
        
        printf("\n");
    }

    // added
    else if (b) {
        jf_print_diff_action(node->type);
        // jf_print_diff_action(jf_match_diff_action(node->node_a, node->node_b));
        print_key((node->key) ? node->key->str : "unknown", JF_MAX_NAME_LEN);
        jf_print_indent(indent);

        // a val
        printf("   %s: ", jf_type_str(b->type));
        jf_print_value_str(b);

        printf("\n");
    }

    // printf("invalid\n");
}

void jf_print_diff_node(const jf_DiffNode* node, int indent, int count) {
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


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
    
    if (!str->str) { return JF_NO_MEM; }
    memcpy(str->str, data, len);
    str->len = len;

    return JF_SUCCESS;
}

jf_Error jf_string_free(jf_String* str) {
    jf_free(str->str);
    str->len = 0;
    return JF_SUCCESS;
}

jf_Bool jf_string_compare(jf_String* str_a, jf_String* str_b) {
    if (str_a->len != str_b->len) {
        return JF_FALSE;
    }

    if (!str_a || !str_a->str || !str_b || !str_b->str) {
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

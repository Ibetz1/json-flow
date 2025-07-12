/*
    Json Flow API
    ianbetz.22.4@gmail.com
    Ian Betz
    7/11/2025
*/

#ifndef _JF_HPP
#define _JF_HPP

#include "stdio.h"
#include <cstdlib>


/*
    logging
*/

#if defined(_MSC_VER)
#   define JF_INLINE static __forceinline
#elif defined(__GNUC__) || defined(__clang__)
#   define JF_INLINE static inline __attribute__((always_inline))
#else
#   define JF_INLINE static inline
#endif

/*
    heap logging
*/
#ifdef JF_LOG_HEAP_EVENTS
#   define JF_HEAP_LOG(fmt, ...) printf("HEAP: " fmt "\n", ##__VA_ARGS__)
#else
#   define JF_HEAP_LOG(fmt, ...)
#endif

/*
    heap debug tracking
*/

#ifdef JF_DEBUG_HEAP
    extern int __jf_heap_count_alloc__;
    extern int __jf_heap_count_free__;
#   define JF_HEAP_TRACK() printf("\nALLOC_CALLS=%i :: FREE_CALLS=%i :: UNFREED=%i\n\n", __jf_heap_count_alloc__, __jf_heap_count_free__, __jf_heap_count_alloc__ - __jf_heap_count_free__);

    JF_INLINE void* jf_alloc(size_t size)                 { ++__jf_heap_count_alloc__; return malloc(size);         }
    JF_INLINE void* jf_calloc(size_t size1, size_t size2) { ++__jf_heap_count_alloc__; return calloc(size1, size2); }
    JF_INLINE void  jf_free(void* ptr)                    { ++__jf_heap_count_free__ ; return free(ptr);            }

    JF_INLINE int jf_start() {
        return 0;
    }

    JF_INLINE int jf_finish() {
        JF_HEAP_TRACK();
        return 0;
    }
#else
#   define __JF_HEAP_ALLOC__
#   define __JF_HEAP_FREE__

    JF_INLINE void* jf_alloc(size_t size)                 { return malloc(size);         }
    JF_INLINE void* jf_calloc(size_t size1, size_t size2) { return calloc(size1, size2); }
    JF_INLINE void  jf_free(void* ptr)                    { return free(ptr);            }

    JF_INLINE int jf_start() { 
        return 0; 
    }

    JF_INLINE int jf_finish() { 
        return 0; 
    }
#endif

/*
    semi-verbose debug logging
*/
#ifdef JF_LOG_DEBUG_EVENTS
#   define JF_DEBUG_LOG(fmt, ...) printf("DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#   define JF_DEBUG_LOG(fmt, ...)
#endif

/*
    low-verbose debug logging
*/
#ifndef JF_LOG_DISABLE
#   define JF_LOG(fmt, ...) printf("INFO: " fmt "\n", ##__VA_ARGS__)
#else
#   define JF_LOG(fmt, ...)
#endif

/*
    math
*/

#define JF_MATH_MIN(A,B) (((A)<(B))?(A):(B))
#define JF_MATH_MAX(A,B) (((A)>(B))?(A):(B))

#define JF_STRING(str, size) { (char*) str, size }
#define JF_STRING_STATIC(str) { (char*) str, sizeof(str) }
#define JF_STRING_CONST(str) { (char*) str, strlen(str) }
#define JF_STRING_MAX_NUMBER 0x20

typedef double jf_Number;
struct jf_Node;
struct jf_KeyValue;
struct jf_Object;
struct jf_Array;
struct jf_String;

/*
    types
*/
enum jf_Type {
    JF_NULL,
    JF_BOOL,
    JF_NUMBER,
    JF_STRING,
    JF_OBJECT,
    JF_ARRAY
};

enum jf_Error {
    JF_SUCCESS,
    JF_NO_MEM,
    JF_NO_REF,
    JF_INDEX_OUT_OF_BOUNDS,
    JF_INVALID_SYNTAX,
    JF_INVALID_ESCAPE,
    JF_UNEXPECTED_EOF,
    JF_INVALID_TYPE,
    JF_INVALID_FILE_PATH
};

enum jf_Bool {
    JF_FALSE,
    JF_TRUE
};

/*
    wrapper because std::string ew
*/
struct jf_String {
    char* str;
    size_t len;
    jf_Bool allocated;
};

jf_Error jf_string_alloc(jf_String* str, const char* data, size_t len);
jf_Error jf_string_free(jf_String* str);
jf_Bool  jf_string_compare(jf_String* str_a, jf_String* str_b);
jf_Error jf_string_copy(jf_String* str_a, jf_String* str_b);
jf_Error jf_string_from_number(jf_String* str, double num);


/*
    assigns a key to a node
*/
struct jf_KeyValue {
    jf_String key;
    jf_Node* value;
};

jf_Error jf_key_value_alloc(jf_KeyValue* kv, jf_String key);
jf_Error jf_key_value_free(jf_KeyValue* kv);

/*
    wrapper for if_Node, contains a generic
*/
struct jf_Object { // fixed allocator
    size_t size;
    size_t used;
    jf_KeyValue* entries;
};

jf_Error jf_object_alloc(jf_Object* obj, size_t count);
jf_Error jf_object_free(jf_Object* obj);

/*
    multile generic json types
*/
struct jf_Array { // fixed allocator
    size_t size;
    size_t used;
    jf_Node** elements;
};

jf_Error jf_array_alloc(jf_Array* array, size_t size);
jf_Error jf_array_free(jf_Array* array);
jf_Bool  jf_array_compare(jf_Array* a, jf_Array* b);
jf_Bool  jf_array_contains(jf_Array* arr, jf_Node* val);

/*
    generic json type
*/
struct jf_Node {
    jf_Type type;

    union {
        jf_Number   n_value;
        jf_Object   o_value;
        jf_Array    a_value;
        jf_Bool     b_value;
        jf_String   s_value;
    };

    jf_Node* next;
};

jf_Error jf_node_alloc(jf_Node** obj);
jf_Error jf_node_free(jf_Node* obj);
jf_Bool  jf_node_compare(jf_Node* node_a, jf_Node* node_b);

/*
    logging
*/
void jf_print_value_str(jf_Node* node);

void jf_print_error(jf_Error err);

#endif
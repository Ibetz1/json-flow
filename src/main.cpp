#include "jf.h"
#include "json_parse.hpp"
#include "stdio.h"

/*
    create a timeline of difs which can zoom in on node keys
    - create current timeline entry renderer
    - create overall node timeline renderer

    load jf_tree back into a file format (yaml, xml, json)

    recursively load a file tree into memory (filtering out json data, yml data, etc)
    actively monitor each file and evey time it is changed, commit a snapshot

    UI :)
*/

struct jf_Timeline {
    jf_DiffNode* entry;
    jf_Timeline* prev;
    jf_Timeline* next;
};

jf_Error jf_timeline_alloc(jf_Timeline* timeline, jf_DiffNode* entry) {
    timeline->entry = NULL;
    timeline->prev  = NULL;
    timeline->next  = NULL;
    return JF_SUCCESS;
}

jf_Error jf_timeline_free(jf_Timeline* timeline) {
    jf_Timeline* next;
    
    if (next) {
        return jf_timeline_free(next);
    }

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

#define jf_timeline_front_insert(a, b) ({ jf_timeline_attach(a, b); })
#define jf_timeline_back_insert (a, b) ({ jf_timeline_attach(b, a); })

jf_Error jf_timeline_build_from_file_names(jf_Timeline* timeline, jf_String* files, size_t count) {
    return JF_SUCCESS;
}

jf_Error jf_timeline_build_from_node_name(jf_Timeline* timeline, jf_String name) {
    return JF_SUCCESS;
}



int main() {
    jf_Error err;

    jf_start();

    jf_String timeline_files[] = {
        JF_STRING_STATIC("./test_timeline/test1.json"),
        JF_STRING_STATIC("./test_timeline/test2.json"),
        JF_STRING_STATIC("./test_timeline/test3.json"),
    };

    jf_Node* node_1 = nullptr;
    err = jf_parse_from_json_file(&node_1, timeline_files[0]);

    jf_Node* node_2 = nullptr;
    err = jf_parse_from_json_file(&node_2, timeline_files[1]);

    // jf_print_node(node_1);
    // jf_print_node(node_2);

    jf_DiffNode* diff;
    jf_print_error(jf_alloc_diff(&diff, NULL, NULL));
    jf_print_error(jf_compare_object_diff(diff, &node_1->o_value,&node_2->o_value));
    // jf_print_error(jf_compare_object_diff(diff, &node_1->o_value, NULL));
    // jf_print_error(jf_compare_object_diff(diff, NULL, &node_1->o_value));
    
    jf_print_diff_node(diff, 2);

    printf("finished\n");

    jf_free_diff(diff);
    jf_node_free(node_1);
    jf_node_free(node_2);
    jf_finish();
    return 0;
}
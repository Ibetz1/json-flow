#include "json_parse.hpp"

jf_Error jf_from_json(const json& j, jf_Node** out) {
    jf_Error err;

    err = jf_node_alloc(out);
    if (err != JF_SUCCESS) return err;

    jf_Node* node = *out;

    if (j.is_null()) {
        node->type = JF_NULL;
    } 

    else if (j.is_boolean()) {
        node->type = JF_BOOL;
        node->b_value = j.get<bool>() ? JF_TRUE : JF_FALSE;
    } 

    else if (j.is_number()) {
        node->type = JF_NUMBER;
        node->n_value = j.get<jf_Number>();
    } 

    else if (j.is_string()) {
        node->type = JF_STRING;
        const std::string& s = j.get_ref<const std::string&>();
        err = jf_string_alloc(&node->s_value, s.c_str(), s.size());
        if (err != JF_SUCCESS) return err;
    }

    else if (j.is_object()) {
        node->type = JF_OBJECT;
        err = build_object(j, &node->o_value);
        if (err != JF_SUCCESS) return err;
    }

    else if (j.is_array()) {
        node->type = JF_ARRAY;
        err = build_array(j, &node->a_value);
        if (err != JF_SUCCESS) return err;
    }

    else {
        return JF_INVALID_TYPE;
    }

    return JF_SUCCESS;
}

jf_Error build_object(const json& j_obj, jf_Object* out_obj) {
    size_t count = j_obj.size();
    jf_Error err = jf_object_alloc(out_obj, count);
    if (err != JF_SUCCESS) return err;

    size_t index = 0;
    for (auto it = j_obj.begin(); it != j_obj.end(); ++it) {
        jf_KeyValue* kv = &out_obj->entries[index++];

        err = jf_key_value_alloc(kv, JF_STRING(it.key().c_str(), it.key().size()));
        if (err != JF_SUCCESS) return err;

        err = jf_from_json(it.value(), &kv->value);
        if (err != JF_SUCCESS) return err;

        out_obj->used++;
    }

    return JF_SUCCESS;
}

jf_Error build_array(const json& j_arr, jf_Array* out_arr) {
    size_t count = j_arr.size();
    jf_Error err = jf_array_alloc(out_arr, count);
    if (err != JF_SUCCESS) return err;

    for (size_t i = 0; i < count; ++i) {
        jf_Node* child = nullptr;
        err = jf_from_json(j_arr[i], &child);
        if (err != JF_SUCCESS) return err;

        out_arr->elements[i] = child;
        out_arr->used++;
    }

    return JF_SUCCESS;
}

jf_Error jf_parse_from_json_file(jf_Node** node, const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        perror("fopen");
        return JF_INVALID_FILE_PATH;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char* buffer = (char*) jf_alloc(size + 1);
    if (!buffer) {
        fclose(f);
        return JF_NO_MEM;
    }

    size_t read = fread(buffer, 1, size, f);
    buffer[read] = '\0';

    fclose(f);
    json parsed = json::parse(buffer);
    jf_free(buffer);

    return jf_from_json(parsed, node);
}
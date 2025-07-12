#ifndef _JSON_PARSE_HPP
#define _JSON_PARSE_HPP

#include "parsers/json.hpp"
#include "jf.h"

using json = nlohmann::json;

jf_Error jf_from_json(const json& j, jf_Node** out);

jf_Error build_object(const json& j_obj, jf_Object* out_obj);

jf_Error build_array(const json& j_arr, jf_Array* out_arr);

jf_Error jf_parse_from_json_file(jf_Node** node, jf_String path);

#endif
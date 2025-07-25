/*
    Ian Betz
    7/13/2025
    ianbetz.22.4@gmail.com

    fair warning to all potential readers of this source code,
    this abomination was written on a 35+ hour coding bender, 
    read at your own discretion.
    
    trying to understand the diff parser, ui code, or file system could lead to the following symptoms:
        - severe brain damage
        - time dilation
        - death

    YOU HAVE BEEN WARNED!
*/

#include "platform/tinyfiledialogs.h"
#include "jf.h"
#include "json_parse.hpp"
#include "stdio.h"
#include "array"
#include <sstream>
#include "string"
#include <iostream>
#include "fstream"
#include "set"

#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

/*
    - add in new file tracking for projects
*/ 

#include <filesystem>
#include <vector>
#include <string>
namespace fs = std::filesystem;

jf_TreeDiff jf_diff_get_main_type(jf_DiffNode* root) {
    if (!root) {
        printf("invalid ref");
        return JF_DIFF_UNKNOWN;
    }

    size_t counts[JF_DIFF_UNKNOWN + 1] = { 0 };

    while (root) {
        counts[root->type]++;
        root = root->next;
    }

    // Find the type with the highest count
    jf_TreeDiff max_type = JF_DIFF_STALE;
    size_t max_count = 0;

    for (int i = 0; i < JF_DIFF_UNKNOWN; ++i) {
        if (counts[i] > max_count) {
            max_count = counts[i];
            max_type = (jf_TreeDiff)i;
        }
    }

    return max_type;
};

std::map<std::string, std::string> get_project_folders(std::string base_folder_path) {
    std::map<std::string, std::string> folders;

    printf("get project folders\n");

    // --- Discover .tml folders once ---
    if (folders.empty()) {
        for (const auto& dir_entry : fs::directory_iterator(base_folder_path)) {
            if (!dir_entry.is_directory()) continue;

            std::string folder_name = dir_entry.path().filename().string();
            std::string folder_path = dir_entry.path().string();
            if (folder_name.find(".tml") != std::string::npos) {
                folders.insert({folder_name, folder_path});
            }
        }
    }

    return folders;
}

std::vector<std::string> get_json_files_in_folder(const std::string& folder_path) {
    std::vector<std::string> json_files;

    for (const auto& entry : fs::directory_iterator(folder_path)) {
        if (!entry.is_regular_file()) continue;

        if (entry.path().extension() == ".json") {
            json_files.push_back(entry.path().string());
        }
    }

    return json_files;
}

size_t count_json_files_recurse(const std::string& root_folder) {
    return get_json_files_in_folder(root_folder).size();
}

std::vector<std::string> find_json_files_recurse(const std::string& folder_path) {
    if (folder_path.empty()) return {};

    std::vector<std::string> result;

    for (const auto& entry : fs::recursive_directory_iterator(folder_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            result.push_back(entry.path().string());
        }
    }

    return result;
}

// this is a disgrace (thank good ol GPT)
void copy_json_files_to_project_structure(const std::vector<std::string>& json_paths, const std::string& dest_dir) {
    try {
        fs::create_directories(dest_dir); // ensure the destination directory exists

        for (const auto& json_path_str : json_paths) {
            fs::path json_path(json_path_str);

            if (!fs::exists(json_path) || json_path.extension() != ".json") {
                std::cerr << "Skipping invalid JSON file: " << json_path << "\n";
                continue;
            }

            std::string folder_name = json_path.stem().string() + ".tml";
            fs::path folder_path = fs::path(dest_dir) / folder_name;
            fs::create_directories(folder_path);

            // Generate timestamped filename
            std::time_t now = std::time(nullptr);
            std::string timestamp_filename = std::to_string(now) + ".json";
            fs::path target_file = folder_path / timestamp_filename;

            // Copy contents
            std::ifstream src(json_path, std::ios::binary);
            std::ofstream dst(target_file, std::ios::binary);
            dst << src.rdbuf();

            std::cout << "Copied: " << json_path << " -> " << target_file << "\n";
        }
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
    }
}

bool draw_fullwidth_buttons(
    const std::map<std::string, std::string>& items, 
    std::function<void(const std::string& p, const std::string& f)> on_clicked
) {
    bool clicked = false;

    float width = ImGui::GetContentRegionAvail().x;

    for (auto& [name, f] : items ) {
        if (ImGui::Button(name.c_str(), ImVec2(width, 0))) {
            on_clicked(f, name);
            clicked = true;
        }
    }

    return clicked;
}

std::string get_value_string(jf_Node* node) {
    switch (node->type) {
        case (JF_STRING) : return std::string(node->s_value.str);
        case JF_NUMBER: {
            std::ostringstream oss;
            oss.precision(10); // enough precision to retain meaningful digits
            oss << std::fixed << node->n_value;
            std::string num_str = oss.str();

            // Trim trailing zeros
            size_t dot = num_str.find('.');
            if (dot != std::string::npos) {
                size_t end = num_str.find_last_not_of('0');
                if (end != std::string::npos) {
                    if (num_str[end] == '.') {
                        num_str = num_str.substr(0, end); // trim dot too
                    } else {
                        num_str = num_str.substr(0, end + 1);
                    }
                }
            }

            return num_str;
        }
        case (JF_OBJECT) : return "<object>";
        case (JF_ARRAY)  : return "<array>";
        case (JF_NULL)   : return "null";
        case (JF_BOOL)   : return (node->b_value) ? "true" : "false";
    }

    return "unknown";
}

ImU32 calculate_diff_color(jf_TreeDiff type) {
    ImU32 square_color = IM_COL32(255, 255, 255, 128); // default gray
    switch (type) {
        case JF_DIFF_CHANGED: square_color = IM_COL32(255, 255, 0, 255);   break;  // yellow/orange
        case JF_DIFF_ADDED:   square_color = IM_COL32(128, 255, 128, 255); break;  // green
        case JF_DIFF_REMOVED: square_color = IM_COL32(255, 128, 128, 255); break;  // red
        case JF_DIFF_STALE:   square_color = IM_COL32(255, 255, 255, 64);  break; // dark gray
    }

    return square_color;
}

bool is_valid_json_string(const std::string& str) {
    try {
        auto _ = nlohmann::json::parse(str); // explicitly use the result
        return true;
    } catch (...) {
        return false;
    }
}

std::string fnv1a_hash_str(const std::string& str) {
    constexpr uint64_t FNV_OFFSET = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME  = 1099511628211ULL;

    uint64_t hash = FNV_OFFSET;
    for (char c : str) {
        hash ^= static_cast<unsigned char>(c);
        hash *= FNV_PRIME;
    }

    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

std::string fnv1a_hash_file(const std::string& path) {
    constexpr uint64_t FNV_OFFSET = 14695981039346656037ULL;
    constexpr uint64_t FNV_PRIME  = 1099511628211ULL;

    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return "";

    uint64_t hash = FNV_OFFSET;
    char c;
    while (in.get(c)) {
        hash ^= static_cast<unsigned char>(c);
        hash *= FNV_PRIME;
    }

    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}


static bool path_updated = false;
static std::vector<std::string> selected_node_path = {};
static std::vector<jf_TreeDiff> diff_filters = { };
static std::vector<jf_Type>     type_filters = { };
static std::string get_full_node_path() {
    std::string full_path;
    for (size_t i = 0; i < selected_node_path.size(); ++i) {
        full_path += selected_node_path[i];
        if (i < selected_node_path.size() - 1) full_path += ".";
    }
    return full_path;
}

static bool compare_node_paths(const std::vector<std::string>& a, const std::vector<std::string>& b) {
    if (a.size() != b.size()) return false;

    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) return false;
    }

    return true;
}

void render_timeline_summary(jf_Timeline* timeline, jf_Timeline* display_node, std::function<void(jf_Timeline* selected)> on_pressed) {
    if (!timeline) return;

    constexpr float square_size = 16.0f;
    constexpr float spacing = 4.0f;

    jf_Timeline* current = timeline;
    while (current) {
        jf_TreeDiff type = jf_diff_get_main_type(current->entry);
        ImU32 entry_color = calculate_diff_color(type);

        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImVec2 max = ImVec2(pos.x + square_size, pos.y + square_size);

        std::string id = "##timeline_button_" + std::to_string((uintptr_t)current);

        if (ImGui::InvisibleButton(id.c_str(), ImVec2(square_size, square_size))) {
            on_pressed(current);
        }

        ImGui::GetWindowDrawList()->AddRectFilled(pos, max, entry_color, 3.0f);

        if (current->version == display_node->version) {
            ImVec2 center = ImVec2(
                pos.x + square_size * 0.5f,
                pos.y + square_size * 0.5f
            );

            float dot_radius = square_size * 0.25f; // adjust size as needed
            ImGui::GetWindowDrawList()->AddCircleFilled(center, dot_radius, IM_COL32(0, 0, 0, 128));
        }

        ImGui::SameLine(0.0f, spacing); // move to next square
        current = current->next;
    }

    // Ensure next line starts below the timeline
    ImGui::NewLine();
}

void render_diff_tree(
    jf_DiffNode* root,
    const std::vector<std::string>& prefix = {},
    int depth = 1, 
    bool parent_hovering = false, 
    bool parent_selected = false,
    std::vector<std::string>& path = *(new std::vector<std::string>())
) {
    if (!root) { return; }

    if (path.empty() && !prefix.empty()) {
        path = prefix;
    }

    const float indentation_depth = 20.f;
    const float box_padding = 4.f;
    const float box_spacing = 8.f;
    const float icon_width = 12.f;

    ImGui::Indent(depth * indentation_depth);
    bool hovering = false;

    while (root) {
        bool node_selected = false;

        if (
            root->key && 
            root->key->str
        ) {

            // Push current key into path
            path.push_back(root->key->str);

            // fuck std::lib
            bool diff_matches = std::find(diff_filters.begin(), diff_filters.end(), root->type) != diff_filters.end();
            bool type_matches = false;

            if (root->node_a && !type_matches) {
                type_matches = std::find(type_filters.begin(), type_filters.end(), root->node_a->type) != type_filters.end();
            }

            if (root->node_b && !type_matches) {
                type_matches = std::find(type_filters.begin(), type_filters.end(), root->node_b->type) != type_filters.end();
            }

            ImVec2 pos = ImGui::GetCursorScreenPos();
            float height = ImGui::GetFrameHeight();
            float full_width = ImGui::GetContentRegionAvail().x;
            
            std::string unique_id = "##" + std::string(root->key->str) + std::to_string((uintptr_t)root);
            bool pressed = ImGui::InvisibleButton(unique_id.c_str(), ImVec2(full_width, height));
            hovering = parent_hovering || ImGui::IsItemHovered();
            
            ImU32 bg_col;

            // gray out if unmatched
            if (type_matches && diff_matches) {
                if (pressed) {
                    selected_node_path = path;
                    path_updated = true;
                }

                node_selected = compare_node_paths(path, selected_node_path);
                pressed = pressed || parent_selected || node_selected;
                bg_col = pressed ? ImGui::GetColorU32(ImGuiCol_ButtonActive)
                                    : hovering ? ImGui::GetColorU32(ImGuiCol_ButtonHovered)
                                                : ImGui::GetColorU32(ImGuiCol_Button);
            } else {
                bg_col = IM_COL32(16, 16, 16, 255);
            }

            ImVec2 window_left = ImGui::GetWindowPos();
            window_left.x += ImGui::GetStyle().WindowPadding.x;

            float x_cursor = window_left.x;
            float x_indent = pos.x;
            
            float square_size = ImGui::GetFrameHeight() * 0.5f;
            ImVec2 square_min(x_cursor, pos.y + (ImGui::GetFrameHeight() - square_size) * 0.5f);
            ImVec2 square_max(square_min.x + square_size, square_min.y + square_size);

            ImGui::GetWindowDrawList()->AddRectFilled(square_min, square_max, calculate_diff_color(root->type), 2.0f);

            auto render_clipped_box = [&](const char* text, float width) {
                ImVec2 box_min(x_indent, pos.y);
                ImVec2 box_max(x_indent + width, pos.y + height);
                ImGui::GetWindowDrawList()->AddRectFilled(box_min, box_max, bg_col, box_max.y / 8);
                ImGui::PushClipRect(box_min, box_max, true);

                ImVec2 text_size = ImGui::CalcTextSize(text);
                ImVec2 text_pos = ImVec2(
                    box_min.x + (width - text_size.x) * 0.5f,
                    box_min.y + (height - text_size.y) * 0.5f
                );

                ImGui::GetWindowDrawList()->AddText(text_pos, ImGui::GetColorU32(ImGuiCol_Text), text);
                ImGui::PopClipRect();
                x_indent += width + box_spacing;
            };

            int box_count = 1; // always at least the key box
            if (root->node_a) box_count += 2;
            if (root->node_b && root->type != JF_DIFF_STALE) box_count += 2;

            float box_width = (full_width - (box_spacing * (box_count - 1))) / box_count;
            x_indent = pos.x; // reset x_indent for this row

            render_clipped_box(root->key->str, box_width);

            if (root->node_a) {
                render_clipped_box(jf_type_str(root->node_a->type), box_width);
                render_clipped_box(get_value_string(root->node_a).c_str(), box_width);
            }

            if (root->node_b && root->type != JF_DIFF_STALE) {
                render_clipped_box(jf_type_str(root->node_b->type), box_width);
                render_clipped_box(get_value_string(root->node_b).c_str(), box_width);
            }

            if (root->child) {
                ImGui::Unindent(depth * indentation_depth);
                render_diff_tree(root->child, prefix, depth + 1, hovering, node_selected || parent_selected, path);
                ImGui::Indent(depth * indentation_depth);
            }

            path.pop_back();
        }

        if (!parent_hovering) hovering = false;
        root = root->next;
    }

    ImGui::Unindent(depth * indentation_depth);
}

struct Project {
    size_t last_file_count = 0;
    std::string project_name = "pick a project";
    std::string selected_name = "pick a timeline";
    std::string selected_path = "";
    std::string project_path = "";
    std::string originating_path = "";
    std::set<std::string> tracked_files = {};
    std::map<std::string, std::string> tracked_hashes = {};
    std::map<std::string, std::string> project_folders = {};

    void create(std::string folder) {
        printf("creating project from folder: %s\n", folder.c_str());
        std::vector<std::string> found_files = find_json_files_recurse(folder);
        for (std::string f : found_files) {
            tracked_files.insert(f);
        }

        project_name = fs::path(folder).filename().string();
        project_path = "./projects/" + project_name;
        fs::create_directory(project_path);
        copy_json_files_to_project_structure(found_files, project_path);
        project_name = fs::path(project_path).filename().string();
        project_folders.clear();
        project_folders = get_project_folders(project_path);
        originating_path = folder;
        compute_latest_file_hashes();
        save();
    }

    void import(std::string path) {
        std::ifstream in(path + "/project.json");
        if (!in.is_open()) return;

        json j;
        in >> j;

        selected_path    = j.value("selected_path",    "");
        project_path     = j.value("project_path",     "");
        originating_path = j.value("originating_path", "");
        last_file_count  = j.value("last_file_count",   0);
        project_name     = j.value("project_name",     "pick a project");
        selected_name    = j.value("selected_name",    "pick a timeline");
        tracked_files    = j.value("tracked_files",    std::set<std::string>{});
        project_folders  = j.value("project_folders",  std::map<std::string, std::string>{});
        tracked_hashes   = j.value("tracked_hashes",   std::map<std::string, std::string>{});
    }

    void save() {
        json j;
        j["project_path"] = project_path;
        j["project_name"] = project_name;
        j["selected_name"] = selected_name;
        j["tracked_files"] = tracked_files;
        j["project_folders"] = project_folders;
        j["tracked_hashes"] = tracked_hashes;
        j["selected_path"] = selected_path;
        j["last_file_count"] = last_file_count;
        j["originating_path"] = originating_path;

        std::ofstream out(project_path + "/project.json");
        out << j.dump(4); // pretty print with indent
    }

    // this is really bad but its ok
    void compute_latest_file_hashes() {
        tracked_hashes.clear();

        for (auto& path : tracked_files) {
            std::string file_data;
            std::string file_hash;

            // if file doesnt exist hash empty json
            std::ifstream in(path, std::ios::in | std::ios::binary);
            if (!in) {
                file_data = "{}";
            } else {
                std::ostringstream ss;
                ss << in.rdbuf();
                file_data = ss.str();
            }

            tracked_hashes[path] = file_hash;
        }
    }

    bool check_timeline() {
        if (project_path.empty()) return false;
        if (originating_path.empty()) return false;

        bool updated = false;

        // look for new files
        // WHAT THE FUCK EVEN IS THIS IAN? - ian
        fs::path path_to_watch(originating_path);
        if (fs::exists(path_to_watch) && fs::is_directory(path_to_watch)) {
            std::vector<std::string> existing_files = find_json_files_recurse(originating_path);
            size_t new_file_count = existing_files.size();
            
            if (new_file_count != last_file_count) {
                for (const std::string& new_path : existing_files) {
                    if (!tracked_files.count(new_path)) {
                        printf("new file found %s\n", new_path.c_str());

                        fs::path fpath = fs::path(new_path);
                        std::string name = fpath.replace_extension(".tml").filename().string();
                        std::string local_path = project_path + "/" + name;

                        fs::create_directory(local_path);

                        tracked_files.insert(new_path);
                        project_folders.insert({name, local_path});
                        last_file_count = new_file_count;
                        updated = true;
                    }
                }
            }
        }

        for (auto& path : tracked_files) {
            std::string filename = fs::path(path).filename().string();
            std::string folder_name = fs::path(path).stem().string() + ".tml";
            std::string timeline_dir = project_path + "/" + folder_name;
            fs::create_directory(timeline_dir);

            if (!fs::exists(timeline_dir) || !fs::is_directory(timeline_dir)) {
                continue;
            }

            std::string file_data;
            std::string file_hash;

            // if file doesnt exist hash empty json
            std::ifstream in(path, std::ios::in | std::ios::binary);
            if (!in) {
                file_data = "{}";
            } else {
                std::ostringstream ss;
                ss << in.rdbuf();
                file_data = ss.str();
            }

            if (!is_valid_json_string(file_data)) {
                continue;
            }

            file_hash = fnv1a_hash_str(file_data);

            if (tracked_hashes[path] != file_hash) {
                tracked_hashes[path] = file_hash;
                fs::path file_path = fs::path(timeline_dir) / (std::to_string(std::time(nullptr)) + ".json");
                std::string file_path_str = file_path.string();
                std::ofstream out(file_path, std::ios::out | std::ios::binary);
                if (out) {
                    out.write(file_data.data(), file_data.size());
                    updated = true;
                } else {
                    std::cerr << "Failed to write to file: " << file_path << "\n";
                }
            }
        }

        if (updated) { save(); }
        return updated;
    }
};

void update_diff_tree(
    Project& project, 
    jf_TimelineContext*& timeline_context, 
    jf_Timeline*& timeline,
    jf_Timeline*& display_node,
    jf_Timeline*& timeline_filtered
) {
    jf_start();


    if (timeline_context != NULL) {
        jf_timeline_context_free(timeline_context);
        timeline_context = NULL;
    }

    if (timeline != NULL) {
        jf_timeline_free(timeline);
        timeline_context = NULL;
    }

    if (timeline_filtered != NULL) {
        jf_timeline_free(timeline_filtered);
        timeline_filtered = NULL;
    }

    if (fs::exists(project.selected_path)) {
        std::vector<std::string> files = get_json_files_in_folder(project.selected_path);

        if (!files.empty()) {
            // build timeline context
            jf_timeline_context_alloc(&timeline_context, files.size());
            for (int i = 0; i < files.size(); ++i) {
                jf_string_alloc(&timeline_context->files[i], files[i].c_str(), files[i].size());
            }

            // build the actual timeline
            jf_timeline_build_from_file_names(&timeline, timeline_context);

            jf_Timeline* cur = timeline;

            while (cur && cur->next) { cur = cur->next; }
            display_node = cur;
        }
    }

    jf_finish();
}

struct Session {
    std::string project_path = "";

    void save() {
        json j;
        j["project_path"] = project_path;

        std::ofstream out("./session.json");
        out << j.dump(4); // pretty print with indent
    }

    void load() {
        std::ifstream in("./session.json");
        if (!in.is_open()) return;

        try {
            json j;
            in >> j;

            project_path = j.value("project_path", "");
        } catch(...) {
            return;
        }

    }
};

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1200, 900, "Json Flow", nullptr, nullptr);
    
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    
    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    Session session;
    session.load();
    printf("sessios path=%s\n", session.project_path.c_str());

    Project current_project;
    current_project.import(session.project_path);

    jf_TimelineContext* timeline_context = NULL;
    jf_Timeline* timeline = NULL;
    jf_Timeline* display_node = NULL;
    jf_Timeline* timeline_filtered = NULL;

    ImVec4* colors = ImGui::GetStyle().Colors;

    // Change button background colors
    colors[ImGuiCol_Button]        = ImVec4(1.f, 1.f, 1.f, 0.05f); // Normal
    colors[ImGuiCol_ButtonHovered] = ImVec4(1.f, 1.f, 1.f, 0.2f); // Hover
    colors[ImGuiCol_ButtonActive]  = ImVec4(1.f, 1.f, 1.f, 0.4f); // Clicked
    colors[ImGuiCol_WindowBg]      = ImVec4(0.1f, 0.1f, 0.1f, 1.0f); // background

    static bool diff_filter_added   = true;
    static bool diff_filter_removed = false;
    static bool diff_filter_changed = true;
    static bool diff_filter_stale   = true;
    static bool type_filter_object  = true;
    static bool type_filter_array   = true;
    static bool type_filter_string  = true;
    static bool type_filter_null    = true;
    static bool type_filter_number  = true;
    static bool type_filter_bool    = true;

    while (!glfwWindowShouldClose(window)) {
        if (current_project.check_timeline()) {
            update_diff_tree(current_project, timeline_context, timeline, display_node, timeline_filtered);

            if (selected_node_path.size() > 0) {
                path_updated = true;
            }
        }

        diff_filters.clear();
        type_filters.clear();

        // this should be in a data structure
        if (diff_filter_added)   diff_filters.push_back(JF_DIFF_ADDED);
        if (diff_filter_removed) diff_filters.push_back(JF_DIFF_REMOVED);
        if (diff_filter_changed) diff_filters.push_back(JF_DIFF_CHANGED);
        if (diff_filter_stale)   diff_filters.push_back(JF_DIFF_STALE);

        if (type_filter_object) type_filters.push_back(JF_OBJECT);
        if (type_filter_array)  type_filters.push_back(JF_ARRAY);
        if (type_filter_string) type_filters.push_back(JF_STRING);
        if (type_filter_null)   type_filters.push_back(JF_NULL);
        if (type_filter_number) type_filters.push_back(JF_NUMBER);
        if (type_filter_bool)   type_filters.push_back(JF_BOOL);

        if (path_updated) {
            jf_start();

            if (timeline_filtered != NULL) {
                jf_timeline_free(timeline_filtered);
                timeline_filtered = NULL;
            }

            jf_String* current_path = (jf_String*) jf_calloc(sizeof(jf_String), selected_node_path.size());

            for (int i = 0; i < selected_node_path.size(); ++i) {
                current_path[i] = JF_STRING(selected_node_path[i].c_str(), selected_node_path[i].length());
            }

            jf_print_error(jf_timeline_filter_path(timeline, &timeline_filtered, current_path, selected_node_path.size()));

            jf_free(current_path);
            jf_finish();

            path_updated = false;
        }
        
        glfwPollEvents();

        // Start new frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Panel sizes
        float third_h = display_h / 3.0f;
        float two_third_h = display_h - third_h;

        float third_w = display_w / 3.0f;
        float two_third_w = display_w - third_w;

        // bottom left panel
        ImGui::SetNextWindowPos(ImVec2(0, two_third_h));
        ImGui::SetNextWindowSize(ImVec2(display_w, third_h));
        ImGui::Begin("Entry Changes", nullptr, 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_HorizontalScrollbar
        );

        if (timeline_filtered != NULL) {
            jf_Timeline* cur = timeline_filtered;

            float full_width = ImGui::GetContentRegionAvail().x;
            float box_width = full_width / 5.0f;
            float box_height = ImGui::GetContentRegionAvail().y * 0.9f; // fixed height

            while (cur) { // up to 5 boxes
                ImGui::PushID(cur->version);

                ImGui::BeginChild(
                    "TimelineBox",
                    ImVec2(box_width, box_height),
                    true
                );

                std::vector<std::string> truncated_path = selected_node_path;
                truncated_path.pop_back();

                ImGui::Text("Version %d", cur->version);
                ImGui::Separator();
                render_diff_tree(cur->entry, truncated_path);

                ImGui::EndChild();
                ImGui::PopID();

                ImGui::SameLine(0, 8.0f); // small gap between boxes

                cur = cur->next;
            }
        }

        ImGui::End();

        // top left panel
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(third_w, two_third_h));
        ImGui::Begin(current_project.project_name.c_str(), nullptr, 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_MenuBar
        );

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("Project")) {
                if (ImGui::MenuItem("Open Oproject")) {
                    std::string cwd = std::filesystem::current_path().string();
                    const char* folder = tinyfd_selectFolderDialog("Select a folder", cwd.c_str());
                    if (folder) {
                        current_project.import(folder);
                        session.project_path = current_project.project_path;
                        session.save();
                    }
                }

                if (ImGui::MenuItem("New Project")) {
                    const char* folder = tinyfd_selectFolderDialog("Select a folder", nullptr);
                    if (folder) {
                        current_project.create(folder);
                        session.project_path = current_project.project_path;
                        session.save();
                    }
                }

                if (ImGui::MenuItem("Delete Project")) {
                    ImGui::OpenPopup("ConfirmDeleteProject");
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Filters")) {
                ImGui::Checkbox("Diff: Added",   &diff_filter_added);
                ImGui::Checkbox("Diff: Removed", &diff_filter_removed);
                ImGui::Checkbox("Diff: Changed", &diff_filter_changed);
                ImGui::Checkbox("Diff: Stale",   &diff_filter_stale);
                ImGui::Checkbox("Type: String",  &type_filter_string);
                ImGui::Checkbox("Type: Bool",    &type_filter_bool);
                ImGui::Checkbox("Type: Number",  &type_filter_number);
                ImGui::Checkbox("Type: Null",    &type_filter_null);
                ImGui::Checkbox("Type: Object",  &type_filter_object);
                ImGui::Checkbox("Type: Array",   &type_filter_array);
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        // render_project_files("./timelines/");
        draw_fullwidth_buttons(current_project.project_folders, [&](const std::string& path, const std::string& name) { // onpressed
            selected_node_path.clear();
            current_project.selected_name = name;
            current_project.selected_path = path;
            current_project.save();

            update_diff_tree(current_project, timeline_context, timeline, display_node, timeline_filtered);
        });

        ImGui::End();



        // timeline panel
        ImGui::SetNextWindowPos(ImVec2(third_w, 0));
        ImGui::SetNextWindowSize(ImVec2(two_third_w, two_third_h));
        ImGui::Begin(current_project.selected_name.c_str(), nullptr, 
            ImGuiWindowFlags_NoResize | 
            ImGuiWindowFlags_NoMove | 
            ImGuiWindowFlags_NoCollapse | 
            ImGuiWindowFlags_HorizontalScrollbar
        );

        if (timeline_context != NULL && timeline != NULL) {
            render_timeline_summary(timeline, display_node, [&](jf_Timeline* selected) {
                display_node = selected;
            });

            if (display_node != NULL) {
                render_diff_tree(display_node->entry);
            }

        }

        ImGui::End();
        ImGui::Render();
        
        glViewport(0, 0, display_w, display_w);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
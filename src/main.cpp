#include "jf.h"
#include "json_parse.hpp"
#include "stdio.h"

#include <GLFW/glfw3.h>
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"


/*
    load jf_tree back into a file format (yaml, xml, json)

    recursively load a file tree into memory (filtering out json data, yml data, etc)
    actively monitor each file and evey time it is changed, commit a snapshot

    UI :)
*/

int main() {
    // Init GLFW
    if (!glfwInit()) return -1;

    // Request OpenGL 3.3 core (optional but recommended)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "ImGui + GLFW", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    // Initialize backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Simple Window");
        ImGui::Text("No glad needed!");
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
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

// int main() {
//     jf_Error err;

//     jf_start();

//     // jf_String timeline_files[] = {
//     //     JF_STRING_STATIC("./test_timeline/test1.json"),
//     //     JF_STRING_STATIC("./test_timeline/test2.json"),
//     //     JF_STRING_STATIC("./test_timeline/test3.json"),
//     // };

//     // jf_String test_path[] = {
//     //     JF_STRING_STATIC("flags"),
//     //     JF_STRING_STATIC("debug_mode"),
//     //     JF_STRING_STATIC("1"),
//     // };

//     // jf_TimelineContext* context = NULL;
//     // jf_Timeline* timeline = NULL;

//     // jf_print_error(jf_timeline_context_alloc(&context, 3));
//     // jf_print_error(jf_timeline_context_add_files(context, timeline_files));

//     // jf_print_error(jf_timeline_build_from_file_names(&timeline, context));

//     // // jf_DiffNode* test_diff = jf_diff_filter_path(timeline->entry, test_path, 1);
//     // // jf_print_diff_node(test_diff, 1);

//     // jf_Timeline* filtered_timeline = NULL;
//     // jf_timeline_filter_path(timeline, &filtered_timeline, test_path, 2);

//     // jf_Timeline* cur = filtered_timeline;
//     // while (cur) {
//     //     printf("---------- version %i ----------\n", cur->version);

//     //     jf_print_diff_node(cur->entry, 2);

//     //     printf("--------------------------------\n\n");

//     //     cur = cur->next;
//     // }

//     // jf_timeline_free(filtered_timeline);

//     // jf_print_error(jf_timeline_free(timeline));
//     // jf_print_error(jf_timeline_context_free(context));

//     jf_finish();
//     return 0;
// }
#include "runscope/runscope_v2.hpp"
#include "runscope/process_manager.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <thread>
#include <chrono>

void simulation_work()
{
    RUNSCOPE_PROFILE_FUNCTION();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
}

void physics_calculation()
{
    RUNSCOPE_PROFILE_FUNCTION();
    std::this_thread::sleep_for(std::chrono::milliseconds(8));
    simulation_work();
}

void render_scene()
{
    RUNSCOPE_PROFILE_FUNCTION();
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
}

void game_frame()
{
    RUNSCOPE_PROFILE_SCOPE("game_frame");
    physics_calculation();
    render_scene();
}

static void glfw_error_callback(int error, const char* description)
{
    std::cerr << "GLFW Error " << error << ": " << description << "\n";
}

void show_error_dialog(const std::string& error_message, bool& show_error_flag)
{
    ImGui::OpenPopup("Error");
    if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error:");
        ImGui::TextWrapped("%s", error_message.c_str());
        ImGui::Separator();

        if (ImGui::Button("OK", ImVec2(120, 0)))
        {
            show_error_flag = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    static bool popup_open = true;
    if (!popup_open)
    {
        ImGui::OpenPopup("Error");
        popup_open = true;
    }
}

int main()
{
    glfwSetErrorCallback(glfw_error_callback);

    if (!glfwInit())
    {
        return 1;
    }

#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    const char* glsl_version = "#version 150";
#else
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    const char* glsl_version = "#version 330";
#endif

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "RunScope - Professional Profiler", nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    auto& profiler = runscope::core::ProfilerEngine::getInstance();
    auto& process_mgr = runscope::ProcessManager::getInstance();

    process_mgr.register_process("simulation_work");
    process_mgr.register_process("physics_calculation");
    process_mgr.register_process("render_scene");
    process_mgr.register_process("game_frame");

    profiler.begin_session("MainSession");

    runscope::ui::ProfilerUI ui;

    bool run_simulation = true;
    int frame_count = 0;
    std::string error_message;
    bool show_error = false;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (show_error)
        {
            show_error_dialog(error_message, show_error);
        }

        std::vector<runscope::core::ProfileEntry> entries;

        try
        {
            if (run_simulation)
            {
                game_frame();
                ++frame_count;
                entries = profiler.get_entries();

                for (const auto& entry : entries)
                {
                    process_mgr.update_statistics(entry.name, entry.duration_ms());
                }
            }
            else
            {
                auto& attacher = ui.get_process_attacher();
                if (attacher.is_attached() && attacher.is_sampling())
                {
                    entries = attacher.get_sampled_entries();
                }
                else
                {
                    entries = profiler.get_entries();
                }
            }

            ui.render(entries);

            ImGui::Begin("Status", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
            {
                auto& attacher = ui.get_process_attacher();

                if (attacher.is_attached() && attacher.is_sampling())
                {
                    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "PROFILING REAL PROCESS DATA");
                    ImGui::Text("Attached to PID: %u", attacher.attached_pid());
                }
                else if (run_simulation)
                {
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Running Local Simulation");
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No active profiling");
                }

                ImGui::Separator();
                ImGui::Checkbox("Run Local Simulation", &run_simulation);
                ImGui::Text("Frame Count: %d", frame_count);

                if (ImGui::Button("Clear All Data"))
                {
                    profiler.clear();
                    process_mgr.clear_statistics();
                    frame_count = 0;
                }
            }
            ImGui::End();
        }
        catch (const std::runtime_error& ex)
        {
            error_message = ex.what();
            show_error = true;
            std::cerr << "Runtime error in frame: " << ex.what() << "\n";
        }
        catch (const std::exception& ex)
        {
            error_message = std::string("Standard exception: ") + ex.what();
            show_error = true;
            std::cerr << "Standard exception in frame: " << ex.what() << "\n";
        }
        catch (...)
        {
            error_message = "Unknown exception occurred";
            show_error = true;
            std::cerr << "Unknown exception in frame\n";
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    profiler.end_session();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
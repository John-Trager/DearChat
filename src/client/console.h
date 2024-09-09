#pragma once
#include "imgui.h"
#include "imgui_stdlib.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <stdio.h>
#define GL_SILENCE_DEPRECATION
#if defined(IMGUI_IMPL_OPENGL_ES2)
    #include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <functional>

class Console {
public:

    Console(std::function<void(const std::string&)> submitCallback) 
    : submitCallback_(submitCallback) 
    {}


    void AddLog(const std::string& message) {
        log_.emplace_back(message);
        scrollToBottom_ = true;  // Automatically scroll to the bottom when a message is added
    }

    void Draw(const std::string& title, bool* open) {
        ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::Begin(title.c_str(), open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove |
                                          ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | 
                                          ImGuiWindowFlags_NoCollapse);

        // Scrollable region for the log
        if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true)) {
            for (const std::string& message : log_) {
                ImGui::TextUnformatted(message.c_str());
            }

            // Scroll to the bottom if new text was added
            if (scrollToBottom_) {
                ImGui::SetScrollHereY(1.0f);
                scrollToBottom_ = false;
            }
        }
        ImGui::EndChild();

        // Input box
        ImGui::Separator();

        if (ImGui::InputText("##input", &inputBuffer_, ImGuiInputTextFlags_EnterReturnsTrue)) {
            submitCallback_(inputBuffer_); // call the callback function
            Submit();
            ImGui::SetKeyboardFocusHere(-1);
        }

        ImGui::End();
    }

private:
    void Submit() {
        if (!inputBuffer_.empty()) {
            std::string messageLine = "[ME] " + inputBuffer_;
            AddLog(messageLine);  // Add the input text to the log
            inputBuffer_ = "";     // Clear the input buffer
        }
    }

    std::string inputBuffer_;                 // Input buffer
    std::vector<std::string> log_;            // Log to store messages
    bool scrollToBottom_ = false;             // Scroll flag
    std::function<void(const std::string&)> submitCallback_;  // Functor to handle submit action
};
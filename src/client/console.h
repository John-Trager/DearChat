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

using CallbackFunc = std::function<void(const std::string&)>;

class Console {
public:

    
    Console(CallbackFunc sendMsgCallback, CallbackFunc joinRoomCallback, CallbackFunc createRoomCallback)
    : sendMsgCallback_(sendMsgCallback)
    , joinRoomCallback_(joinRoomCallback)
    , createRoomCallback_(createRoomCallback)
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
                                          ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar |
                                          ImGuiWindowFlags_NoBringToFrontOnFocus);

        // --- the menu bar and conditional pop up windows ---

        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::MenuItem("Join Room")) {
                showRoomJoinWindow_ = true;
            }
            if (ImGui::MenuItem("Create Room")) {
                showRoomCreateWindow_ = true;
            }
            ImGui::EndMenuBar();
        }

        if (showRoomJoinWindow_) {
            ImGui::SetNextWindowSize(ImVec2(240, 100), ImGuiCond_Appearing);
            ImGui::Begin("Join Room", &showRoomJoinWindow_, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
            ImGui::InputText("Room Name", &roomNameBuffer_, ImGuiInputTextFlags_EnterReturnsTrue);
            if (ImGui::Button("Join")) {
                joinRoomCallback_(roomNameBuffer_);
                showRoomJoinWindow_ = false;
                roomNameBuffer_ = ""; // Clear the input buffer
            }
            ImGui::End();
        }

        if (showRoomCreateWindow_) {
            ImGui::SetNextWindowSize(ImVec2(270, 100), ImGuiCond_Appearing);
            ImGui::Begin("Create Room", &showRoomCreateWindow_, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
            ImGui::InputText("Create Name", &roomNameBuffer_, ImGuiInputTextFlags_EnterReturnsTrue);
            if (ImGui::Button("Create")) {
                createRoomCallback_(roomNameBuffer_);
                showRoomCreateWindow_ = false;
                roomNameBuffer_ = ""; // Clear the input buffer
            }
            ImGui::End();
        }

        // --- the console text region and input box ---

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
            sendMsgCallback_(inputBuffer_); // call the callback function
            SubmitMsg();
            ImGui::SetKeyboardFocusHere(-1);
        }

        ImGui::End();
    }

private:
    void SubmitMsg() {
        if (!inputBuffer_.empty()) {
            std::string messageLine = "[ME] " + inputBuffer_;
            AddLog(messageLine);  // Add the input text to the log
            inputBuffer_ = "";     // Clear the input buffer
        }
    }

    std::string inputBuffer_;                 // Input buffer
    std::vector<std::string> log_;            // Log to store messages
    bool scrollToBottom_ = false;             // Scroll flag
    CallbackFunc sendMsgCallback_;  // Functor to handle submit action
    
    bool showRoomJoinWindow_ = false;         // Flag to show the room join window
    std::string roomNameBuffer_;              // Buffer for room name input
    CallbackFunc joinRoomCallback_; // Functor to handle room join action

    bool showRoomCreateWindow_ = false;         // Flag to show the create join window
    CallbackFunc createRoomCallback_; // Functor to handle room creation action
};
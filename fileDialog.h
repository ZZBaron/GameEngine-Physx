#pragma once
#include "GameEngine.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

namespace fs = std::filesystem;

class FileDialog {
private:
     inline static std::string currentPath;
     inline static std::vector<std::string> selectedFiles;
     inline static std::string selectedFile;
     inline static bool showHidden;
     inline static std::vector<std::string> allowedExtensions;

public:
    static bool showFileDialog(const char* title, std::vector<std::string> extensions = { ".glb", ".fbx" }) {
        allowedExtensions = extensions;
        if (currentPath.empty()) {
            currentPath = fs::current_path().string();
        }

        ImGui::SetNextWindowSize(ImVec2(600, 400), ImGuiCond_FirstUseEver);
        bool open = true;
        if (ImGui::Begin(title, &open, ImGuiWindowFlags_NoCollapse)) {
            // Path navigation bar
            char pathBuffer[512];
            strcpy(pathBuffer, currentPath.c_str());
            if (ImGui::InputText("Path", pathBuffer, sizeof(pathBuffer))) {
                if (fs::exists(pathBuffer)) {
                    currentPath = pathBuffer;
                }
            }

            // Parent directory button
            if (ImGui::Button("..")) {
                fs::path parent = fs::path(currentPath).parent_path();
                if (!parent.empty()) {
                    currentPath = parent.string();
                }
            }

            ImGui::SameLine();
            ImGui::Checkbox("Show Hidden", &showHidden);

            // File/Directory list
            ImGui::BeginChild("Files", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
            for (const auto& entry : fs::directory_iterator(currentPath)) {
                const auto& path = entry.path();
                std::string filename = path.filename().string();

                // Skip hidden files unless shown
                if (!showHidden && filename[0] == '.') {
                    continue;
                }

                // Skip files that don't match allowed extensions
                if (!entry.is_directory()) {
                    bool hasValidExtension = allowedExtensions.empty();
                    for (const auto& ext : allowedExtensions) {
                        if (path.extension() == ext) {
                            hasValidExtension = true;
                            break;
                        }
                    }
                    if (!hasValidExtension) continue;
                }

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                if (selectedFile == path.string()) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                ImGui::TreeNodeEx(filename.c_str(), flags);
                if (ImGui::IsItemClicked()) {
                    if (entry.is_directory()) {
                        currentPath = path.string();
                    }
                    else {
                        selectedFile = path.string();
                    }
                }
            }
            ImGui::EndChild();

            // Bottom bar with select/cancel buttons
            bool result = false;
            if (ImGui::Button("Select", ImVec2(120, 0))) {
                result = true;
                open = false;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                selectedFile.clear();
                open = false;
            }

            ImGui::End();
            return result;
        }
        return false;
    }

    static const std::string& getSelectedFile() { return selectedFile; }
    static void clearSelection() { selectedFile.clear(); }
};


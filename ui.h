// ui.h
#pragma once
#include "GameEngine.h"
#include "font.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "scene.h"
#include "fileDialog.h"
#include "modelImporter.h"
#include "ImGuiFileDialog.h"



 // Helper function to find PhysXBody for a node
 inline std::shared_ptr<PhysXBody> findPhysicsBody(Scene& scene, std::shared_ptr<Node> node) {
     for (const auto& body : scene.physicsWorld.bodies) {
         if (body->node == node) {
             return body;
         }
     }
     return nullptr;
 }

// Function and variable declarations
void toggleMenu();
void initImGui(GLFWwindow* window);
void cleanupImGui();

// forward declerations
// class Scene;
extern Scene scene; // Declare the external scene variable that appears in GameEngine.cpp


class MenuSystem {
private:
    static MenuSystem* instance;
    bool isOpen;
    int activeTab;
    ImFont* menuFont;  // Store custom font

    // Input fields for sphere creation
    float sphereRadius;
    glm::vec3 sphereCenter;

    // for selected items in the menu
    std::shared_ptr<Material> selectedMaterial = nullptr;


public:
    static MenuSystem& getInstance() {
        if (!instance) {
            instance = new MenuSystem();
        }
        return *instance;
    }

    MenuSystem() : isOpen(false), activeTab(0), sphereRadius(1.0f), sphereCenter(0.0f), menuFont(nullptr) {}

    bool initializeFont() {
        ImGuiIO& io = ImGui::GetIO();

        // First try to load custom font
        const char* fontPath = "fonts/Roboto/Roboto-Regular.ttf";
        if (FILE* file = fopen(fontPath, "rb")) {
            fclose(file);
            menuFont = io.Fonts->AddFontFromFileTTF(fontPath, 16.0f);
        }

        // If custom font loading failed, use default font
        if (!menuFont) {
            std::cout << "Warning: Could not load custom font. Using default font instead.\n";
            menuFont = io.Fonts->AddFontDefault();
        }

        // Build font atlas
        unsigned char* pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        return menuFont != nullptr;
    }

    void toggle() {
        isOpen = !isOpen;
    }

    bool isMenuOpen() const { return isOpen; }

    void render() {
        if (!isOpen) return;

        // Begin ImGui window
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Get screen dimensions
        int display_w, display_h;
        GLFWwindow* window = glfwGetCurrentContext();
        glfwGetFramebufferSize(window, &display_w, &display_h);

        // Set fixed menu size (50% of screen width, 90% of screen height)
        float menuWidth = display_w * 0.5f;
        float menuHeight = display_h * 0.9f;

        // Center the menu
        ImGui::SetNextWindowPos(ImVec2((display_w - menuWidth) * 0.5f, (display_h - menuHeight) * 0.5f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(menuWidth, menuHeight), ImGuiCond_Always);

        // Set menu style
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.2f, 0.2f, 0.95f)); // Dark gray background
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f)); // White text

        // Push custom font
        ImGui::PushFont(menuFont);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoCollapse;


        if (ImGui::Begin("Game Menu", &isOpen, window_flags)) {
            // Style the tabs
            ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));

            if (ImGui::BeginTabBar("MenuTabs", ImGuiTabBarFlags_None)) {
                if (ImGui::BeginTabItem("Scene")) {
                    // Scene tab content
                    ImGui::BeginChild("SceneTab", ImVec2(0, 0), true);
                    ImGui::Text("View and edit scene properties");
                    ImGui::Separator();


                    if (ImGui::TreeNode("Objects")) {
                        static std::shared_ptr<Node> selectedNode = scene.selectedNodes[0]; // choose first to display
                        static bool showCreateWindow = false;
                        static bool showFileDialog = false;
                        static glm::vec3 importPosition(0.0f);

                        // Create and Delete buttons at the top
                        ImGui::BeginGroup();
                        if (ImGui::Button("Create Object", ImVec2(120, 25))) {
                            showCreateWindow = true;
                        }

                        ImGui::SameLine();
                        ImGui::BeginDisabled(selectedNode == nullptr);
                        if (ImGui::Button("Delete Object", ImVec2(120, 25))) {
                            // Remove from physics world if it has physics
                            auto physBody = findPhysicsBody(scene, selectedNode);
                            if (physBody) {
                                auto& bodies = scene.physicsWorld.bodies;
                                bodies.erase(std::remove(bodies.begin(), bodies.end(), physBody), bodies.end());
                            }

                            // Remove from scene nodes
                            auto& nodes = scene.sceneNodes;
                            nodes.erase(std::remove(nodes.begin(), nodes.end(), selectedNode), nodes.end());

                            selectedNode = nullptr;
                        }
                        ImGui::EndDisabled();
                        ImGui::EndGroup();
                        ImGui::Separator();

                        // Objects Table
                        if (ImGui::BeginTable("Objects Table", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                            ImGui::TableSetupColumn("ID");
                            ImGui::TableSetupColumn("Name");
                            ImGui::TableSetupColumn("Type");
                            ImGui::TableSetupColumn("Position");
                            ImGui::TableHeadersRow();

                            for (size_t i = 0; i < scene.sceneNodes.size(); i++) {
                                auto& node = scene.sceneNodes[i];
                                ImGui::TableNextRow();
                                ImGui::TableNextColumn();

                                char label[32];
                                sprintf(label, "%zu", i);
                                if (ImGui::Selectable(label, selectedNode == node, ImGuiSelectableFlags_SpanAllColumns)) {
                                    selectedNode = node;
                                    selectedMaterial = nullptr;
                                }

                                ImGui::TableNextColumn();
                                ImGui::Text("%s", node->name.empty() ? "Unnamed" : node->name.c_str());

                                ImGui::TableNextColumn();
                                ImGui::Text("%s", findPhysicsBody(scene, node) ? "Physics Object" : "Static Object");

                                ImGui::TableNextColumn();
                                glm::vec3 pos = node->getWorldPosition();
                                ImGui::Text("%.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
                            }
                            ImGui::EndTable();
                        }

                        // Create Object Window
                        if (showCreateWindow) {
                            ImGui::SetNextWindowSize(ImVec2(400, 600), ImGuiCond_FirstUseEver);
                            if (ImGui::Begin("Create Object", &showCreateWindow)) {
                                static int selectedType = 0;
                                static const char* types[] = { "Sphere", "Box", "Cylinder" };
                                static glm::vec3 position(0.0f);
                                static bool isDynamic = true;
                                static float sphereRadius = 1.0f;
                                static float boxDimensions[3] = { 1.0f, 1.0f, 1.0f };
                                static float cylinderRadius = 0.5f;
                                static float cylinderHeight = 1.0f;

                                ImGui::Combo("Type", &selectedType, types, IM_ARRAYSIZE(types));
                                ImGui::DragFloat3("Position", &position.x, 0.1f);
                                ImGui::Checkbox("Dynamic (Physics)", &isDynamic);

                                switch (selectedType) {
                                case 0: // Sphere
                                    ImGui::DragFloat("Radius", &sphereRadius, 0.1f, 0.1f, 10.0f);
                                    break;
                                case 1: // Box
                                    ImGui::DragFloat3("Dimensions", boxDimensions, 0.1f, 0.1f, 10.0f);
                                    break;
                                case 2: // Cylinder
                                    ImGui::DragFloat("Radius", &cylinderRadius, 0.1f, 0.1f, 10.0f);
                                    ImGui::DragFloat("Height", &cylinderHeight, 0.1f, 0.1f, 10.0f);
                                    break;
                                }

                                if (ImGui::Button("Create", ImVec2(120, 30))) {
                                    std::shared_ptr<Node> newNode;
                                    switch (selectedType) {
                                    case 0:
                                        newNode = std::make_shared<SphereNode>(sphereRadius);
                                        break;
                                    case 1:
                                        newNode = std::make_shared<BoxNode>(
                                            boxDimensions[0], boxDimensions[1], boxDimensions[2]);
                                        break;
                                    case 2:
                                        newNode = std::make_shared<CylinderNode>(
                                            cylinderRadius, cylinderHeight);
                                        break;
                                    }

                                    newNode->setWorldPosition(position);

                                    if (isDynamic) {
                                        auto physBody = std::make_shared<PhysXBody>(newNode, false);
                                        scene.addPhysicsBody(physBody);
                                    }
                                    else {
                                        scene.addNode(newNode);
                                    }

                                    showCreateWindow = false;
                                }

                                ImGui::SameLine();
                                if (ImGui::Button("Import Model...", ImVec2(120, 30))) {
                                    showFileDialog = true;
                                    importPosition = position; // Store position for imported model
                                }

                                ImGui::End();
                            }
                        }

                        
                        // File Dialog Window
                        if (showFileDialog) {
                            IGFD::FileDialogConfig config;
                            config.path = ".";
                            config.countSelectionMax = 1;
                            config.flags = ImGuiFileDialogFlags_Modal;

                            // Get the main viewport size
                            ImGuiViewport* viewport = ImGui::GetMainViewport();
                            ImVec2 viewportSize = viewport->Size;

                            // Set the dialog size to 80% of the viewport size
                            ImVec2 dialogSize(viewportSize.x * 0.8f, viewportSize.y * 0.8f);

                            ImGuiFileDialog::Instance()->OpenDialog(
                                "ImportModelDialog",
                                "Choose Model File",
                                ".glb,.fbx,{.glb,.fbx}",
                                config);
                        }

                        if (ImGuiFileDialog::Instance()->Display(
                            "ImportModelDialog",
                            ImGuiWindowFlags_NoCollapse,
                            ImVec2(400, 300),    // minimum size
                            ImVec2(FLT_MAX, FLT_MAX)))  // maximum size (unlimited)
                            {
                            if (ImGuiFileDialog::Instance()->IsOk()) {
                                std::string selectedFile = ImGuiFileDialog::Instance()->GetFilePathName();
                                if (!selectedFile.empty()) {
                                    ModelImporter importer;
                                    auto importedNode = importer.importGLB(selectedFile);
                                    if (importedNode) {
                                        importedNode->setWorldPosition(importPosition);
                                        scene.addNode(importedNode);
                                        showCreateWindow = false;
                                    }
                                    else {
                                        // Show error message
                                        ImGui::OpenPopup("Import Error");
                                    }
                                }
                                showFileDialog = false;
                            }
                            ImGuiFileDialog::Instance()->Close();
                        }

                        // Import Error Popup
                        if (ImGui::BeginPopupModal("Import Error", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
                            ImGui::Text("Failed to import model!");
                            ImGui::Text("Error: %s", ModelImporter().getLastError().c_str());
                            if (ImGui::Button("OK", ImVec2(120, 0))) {
                                ImGui::CloseCurrentPopup();
                            }
                            ImGui::EndPopup();
                        }
                        

                        if (selectedNode) {
                            ImGui::SetNextWindowPos(ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowWidth(), ImGui::GetWindowPos().y));
                            ImGui::Begin("Object Properties", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

                            if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                                glm::vec3 worldPos = selectedNode->getWorldPosition();
                                float position[3] = { worldPos.x, worldPos.y, worldPos.z };

                                // If the position is modified, update the node
                                if (ImGui::DragFloat3("World Position", position, 0.1f)) {
                                    selectedNode->setWorldPosition(glm::vec3(position[0], position[1], position[2]));
                                }

                                ImGui::Text("Local Transform:");
                                ImGui::Indent();
                                ImGui::Text("Translation: %.2f, %.2f, %.2f",
                                    selectedNode->localTranslation.x,
                                    selectedNode->localTranslation.y,
                                    selectedNode->localTranslation.z);

                                glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(selectedNode->localRotation));
                                ImGui::Text("Rotation (degrees): %.2f, %.2f, %.2f",
                                    eulerAngles.x, eulerAngles.y, eulerAngles.z);

                                ImGui::Text("Scale: %.2f, %.2f, %.2f",
                                    selectedNode->localScale.x,
                                    selectedNode->localScale.y,
                                    selectedNode->localScale.z);
                                ImGui::Unindent();
                            }

                            if (selectedNode->mesh && ImGui::CollapsingHeader("Materials", ImGuiTreeNodeFlags_DefaultOpen)) {
                                if (ImGui::BeginTable("Materials Table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                                    ImGui::TableSetupColumn("ID");
                                    ImGui::TableSetupColumn("Name");
                                    ImGui::TableSetupColumn("Base Color");
                                    ImGui::TableHeadersRow();

                                    for (size_t i = 0; i < selectedNode->mesh->materials.size(); i++) {
                                        auto& material = selectedNode->mesh->materials[i];
                                        ImGui::TableNextRow();
                                        ImGui::TableNextColumn();

                                        char label[32];
                                        sprintf(label, "%zu", i);
                                        if (ImGui::Selectable(label, selectedMaterial == material, ImGuiSelectableFlags_SpanAllColumns)) {
                                            selectedMaterial = material;
                                        }

                                        ImGui::TableNextColumn();
                                        ImGui::Text("%s", material->name.empty() ? "Unnamed" : material->name.c_str());

                                        ImGui::TableNextColumn();
                                        ImGui::ColorButton("##color", ImVec4(
                                            material->baseColor.r,
                                            material->baseColor.g,
                                            material->baseColor.b,
                                            1.0f
                                        ));
                                    }
                                    ImGui::EndTable();
                                }

                                if (selectedMaterial) {
                                    ImGui::Separator();
                                    ImGui::Text("Material Properties:");
                                    ImGui::Indent();

                                    // Base properties
                                    float baseColor[3] = {
                                        selectedMaterial->baseColor.r,
                                        selectedMaterial->baseColor.g,
                                        selectedMaterial->baseColor.b
                                    };
                                    if (ImGui::ColorEdit3("Base Color", baseColor)) {
                                        selectedMaterial->baseColor = glm::vec3(baseColor[0], baseColor[1], baseColor[2]);
                                    }

                                    float metallic = selectedMaterial->metallic;
                                    if (ImGui::SliderFloat("Metallic", &metallic, 0.0f, 1.0f)) {
                                        selectedMaterial->metallic = metallic;
                                    }

                                    float roughness = selectedMaterial->roughness;
                                    if (ImGui::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) {
                                        selectedMaterial->roughness = roughness;
                                    }

                                    float emission[3] = {
                                        selectedMaterial->emission.r,
                                        selectedMaterial->emission.g,
                                        selectedMaterial->emission.b
                                    };
                                    if (ImGui::ColorEdit3("Emission", emission)) {
                                        selectedMaterial->emission = glm::vec3(emission[0], emission[1], emission[2]);
                                    }

                                    float emissionStrength = selectedMaterial->emissionStrength;
                                    if (ImGui::SliderFloat("Emission Strength", &emissionStrength, 0.0f, 10.0f)) {
                                        selectedMaterial->emissionStrength = emissionStrength;
                                    }

                                    if (!selectedMaterial->textureMaps.empty()) {
                                        ImGui::Text("\nTexture Maps:");
                                        ImGui::BeginTable("Texture Maps", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg);
                                        ImGui::TableSetupColumn("Type");
                                        ImGui::TableSetupColumn("ID");
                                        ImGui::TableSetupColumn("UV Set");
                                        ImGui::TableSetupColumn("Tiling");
                                        ImGui::TableHeadersRow();

                                        for (const auto& [mapType, textureMap] : selectedMaterial->textureMaps) {
                                            ImGui::TableNextRow();

                                            ImGui::TableNextColumn();
                                            ImGui::Text("%s", mapType.c_str());

                                            ImGui::TableNextColumn();
                                            ImGui::Text("%u", textureMap.textureId);

                                            ImGui::TableNextColumn();
                                            ImGui::Text("%s", textureMap.uvSet.c_str());

                                            ImGui::TableNextColumn();
                                            ImGui::Text("%.2f, %.2f", textureMap.tiling.x, textureMap.tiling.y);
                                        }
                                        ImGui::EndTable();
                                    }
                                    else {
                                        ImGui::Text("\nNo texture maps assigned");
                                    }
                                

                                    ImGui::Unindent();
                                }
                            }

                            // Show physics properties if the node has a physics body
                            auto physBody = findPhysicsBody(scene, selectedNode);
                            if (physBody && ImGui::CollapsingHeader("Physics", ImGuiTreeNodeFlags_DefaultOpen)) {
                                ImGui::Text("Mass: %.2f", physBody->getMass());

                                glm::vec3 vel = physBody->getVelocity();
                                ImGui::Text("Velocity: %.2f, %.2f, %.2f", vel.x, vel.y, vel.z);

                                glm::vec3 angVel = physBody->getAngularVelocity();
                                ImGui::Text("Angular Velocity: %.2f, %.2f, %.2f",
                                    angVel.x, angVel.y, angVel.z);

                                glm::vec3 linearMomentum = vel * physBody->getMass();
                                ImGui::Text("Linear Momentum: %.2f, %.2f, %.2f",
                                    linearMomentum.x, linearMomentum.y, linearMomentum.z);

                                glm::vec3 angularMomentum = physBody->getAngularMomentum();
                                ImGui::Text("Angular Momentum: %.2f, %.2f, %.2f",
                                    angularMomentum.x, angularMomentum.y, angularMomentum.z);
                            }

                            ImGui::End();
                        }

                        ImGui::TreePop();
                    }


                    if (ImGui::TreeNode("Physics")) {
                        // bool play
                        bool play = scene.play;
                        if (ImGui::Button(play ? "Pause" : "Play", ImVec2(120, 30))) {
                            scene.play = !play;
                            // Add your play/pause logic here
                            // For example: togglePlay();
                        }
                        ImGui::TreePop();

                    }

                    if (ImGui::TreeNode("Camera Settings")) {
                        if (scene.activeCamera) {
                            // Position
                            glm::vec3 camPos = scene.activeCamera->cameraPos;
                            if (ImGui::DragFloat3("Position", &camPos.x, 0.1f)) {
                                scene.activeCamera->setCameraPos(camPos);
                            }

                            // Front vector (view direction)
                            glm::vec3 camFront = scene.activeCamera->cameraFront;
                            if (ImGui::DragFloat3("View Direction", &camFront.x, 0.01f, -1.0f, 1.0f)) {
                                scene.activeCamera->setCameraFront(glm::normalize(camFront));
                            }

                            // Camera speed
                            float speed = scene.activeCamera->cameraSpeed;
                            if (ImGui::DragFloat("Movement Speed", &speed, 0.001f, 0.001f, 0.1f)) {
                                scene.activeCamera->cameraSpeed = speed;
                            }

                            // Camera sensitivity
                            float sensitivity = scene.activeCamera->sensitivity;
                            if (ImGui::DragFloat("Mouse Sensitivity", &sensitivity, 0.01f, 0.01f, 1.0f)) {
                                scene.activeCamera->sensitivity = sensitivity;
                            }

                            // Display current yaw and pitch (read-only)
                            ImGui::Text("Yaw: %.2f", scene.activeCamera->yaw);
                            ImGui::Text("Pitch: %.2f", scene.activeCamera->pitch);
                        }
                        else {
                            ImGui::Text("No active camera");
                        }
                        ImGui::TreePop();
                    }


                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Render")) {
                    ImGui::BeginChild("RenderTab", ImVec2(0, 0), true);

                    // Global render settings
                    if (ImGui::CollapsingHeader("Render Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                        bool drawObjs = scene.drawObjects;
                        if (ImGui::Checkbox("Draw Objects", &drawObjs)) {
                            scene.drawObjects = drawObjs;
                        }

                        bool drawWire = scene.drawWireframes;
                        if (ImGui::Checkbox("Draw Wireframes", &drawWire)) {
                            scene.drawWireframes = drawWire;
                        }
                    }

                    // Shadow mapping section
                    if (ImGui::CollapsingHeader("Shadow Maps", ImGuiTreeNodeFlags_DefaultOpen)) {
                        bool shadowsEnabled = scene.shadowRenderer.shadowsEnabled;
                        if (ImGui::Checkbox("Enable Shadows", &shadowsEnabled)) {
                            scene.shadowRenderer.toggleShadows(shadowsEnabled);
                        }

                        // Shadow map properties
                        float nearPlane = scene.shadowRenderer.getNearPlane();
                        float farPlane = scene.shadowRenderer.getFarPlane();
                        if (ImGui::DragFloat("Shadow Near Plane", &nearPlane, 0.1f, 0.1f, farPlane)) {
                            scene.shadowRenderer.setShadowProperties(nearPlane, farPlane);
                        }
                        if (ImGui::DragFloat("Shadow Far Plane", &farPlane, 0.1f, nearPlane, 100.0f)) {
                            scene.shadowRenderer.setShadowProperties(nearPlane, farPlane);
                        }

                        // Display each shadow map's details
                        ImGui::Text("\nShadow Maps:");
                        for (int i = 0; i < scene.spotLights.size(); i++) {
                            if (ImGui::TreeNode((void*)(intptr_t)i, "Shadow Map %d", i)) {
                                ShadowMap shadowMap = scene.shadowRenderer.getShadowMap(i);

                                // Display shadow map properties
                                ImGui::Text("FBO ID: %u", shadowMap.depthMapFBO);
                                ImGui::Text("Texture ID: %u", shadowMap.depthMap);
                                ImGui::Text("Resolution: %ux%u", shadowMap.shadowWidth, shadowMap.shadowHeight);

                                // Display light space matrix
                                glm::mat4 lightSpaceMatrix = scene.shadowRenderer.getLightSpaceMatrix(i);
                                if (ImGui::TreeNode("Light Space Matrix")) {
                                    for (int row = 0; row < 4; row++) {
                                        ImGui::Text("%.2f, %.2f, %.2f, %.2f",
                                            lightSpaceMatrix[row][0],
                                            lightSpaceMatrix[row][1],
                                            lightSpaceMatrix[row][2],
                                            lightSpaceMatrix[row][3]);
                                    }
                                    ImGui::TreePop();
                                }

                                // Light properties affecting the shadow map
                                auto& light = scene.spotLights[i];
                                glm::vec3 lightPos = light->getWorldPosition();
                                glm::vec3 lightDir = light->direction;

                                ImGui::Text("\nLight Properties:");
                                ImGui::Text("Position: %.2f, %.2f, %.2f", lightPos.x, lightPos.y, lightPos.z);
                                ImGui::Text("Direction: %.2f, %.2f, %.2f", lightDir.x, lightDir.y, lightDir.z);
                                ImGui::Text("Inner Cutoff: %.2f", light->innerCutoff);
                                ImGui::Text("Outer Cutoff: %.2f", light->outerCutoff);
                                ImGui::Text("Intensity: %.2f", light->intensity);

                                ImGui::TreePop();
                            }
                        }
                    }

                    // Main shader program info
                    if (ImGui::CollapsingHeader("Shader Programs")) {
                        GLuint mainProgram = scene.shadowRenderer.getMainShaderProgram();
                        GLuint depthProgram = scene.shadowRenderer.getDepthShaderProgram();

                        ImGui::Text("Main Shader Program ID: %u", mainProgram);
                        ImGui::Text("Depth Shader Program ID: %u", depthProgram);
                    }

                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Simulation")) {
                    ImGui::BeginChild("SimulationTab", ImVec2(0, 0), true);

                    ImGui::Text("Simulation Controls");
                    ImGui::Separator();

                    static bool isPlaying = false;
                    if (ImGui::Button(isPlaying ? "Pause" : "Play", ImVec2(120, 30))) {
                        isPlaying = !isPlaying;
                        // Add your play/pause logic here
                        // For example: togglePlay();
                    }

                    ImGui::SameLine();
                    if (ImGui::Button("Reset", ImVec2(120, 30))) {
                        // Add your reset logic here
                    }

                    static float gravity = -9.81f;
                    ImGui::DragFloat("Gravity", &gravity, 0.1f, -20.0f, 20.0f);

                    static float timeScale = 1.0f;
                    ImGui::DragFloat("Time Scale", &timeScale, 0.1f, 0.1f, 10.0f);

                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Debug")) {
                    ImGui::BeginChild("DebugTab", ImVec2(0, 0), true);

                    ImGui::Text("Debug Information");
                    ImGui::Separator();

                    extern float deltaTime_sys;

                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Delta Time: %.3f", deltaTime_sys);

                    ImGui::EndChild();
                    ImGui::EndTabItem();
                }

              

                ImGui::EndTabBar();
            }
            ImGui::PopStyleColor(3); // Pop tab colors
        }
        ImGui::End();

        // Pop styles and font
        ImGui::PopFont();
        ImGui::PopStyleColor(2); // Pop window background and text colors

        // Render ImGui
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }
};



extern bool isMenuOpen;

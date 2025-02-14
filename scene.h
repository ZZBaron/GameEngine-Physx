#pragma once
#include "GameEngine.h"
#include "object3D.h"
#include "camera.h"
#include "PhysXWorld.h"
#include "shadowRenderer.h"
#include "player.h"
#include "UVviewer.h"
#include "vender/imgui/imgui.h"
#include "vender/imgui/backends/imgui_impl_glfw.h"
#include "vender/imgui/backends/imgui_impl_opengl3.h"
#include "background.h"


class Scene {
public:

    // screen and window
    // 
    // Screen dimensions
    int screenWidth = 1792;
    int screenHeight = 1008;


    // Camera management
    std::shared_ptr<Camera> activeCamera;
    std::vector<std::shared_ptr<Camera>> cameras;

    //background
    std::shared_ptr<Skybox> skybox;


    // object nodes
    // scene should control which nodes are drawn on camera (view frustum culling)
    // but these are all the nodes in the scene including the culled ones and the ones in physicsWorld
    std::vector<std::shared_ptr<Node>> sceneNodes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodeRegistry;

    //which nodes are selected
    std::vector<std::shared_ptr<Node>> selectedNodes;


    // physics world
    PhysXWorld physicsWorld;

    // phyiscs params
    bool play = false; // play sim/animation
    bool gravityEnabled = true; // enable gravity
    bool collisionEnabled = true; // enable collision detection

    ShadowRenderer shadowRenderer;

    //draw flags
    bool drawWireframes = false;
    bool drawObjects = true;
    bool drawControlsoverlay = true;

    glm::vec3 ambientLight;
    std::vector<std::shared_ptr<SpotLight>> spotLights;
    std::vector<std::shared_ptr<SunLight >> sunLights;

    // for first person player mode
    Player player;
    bool playerMode = false;

    // for mapping uv's
    UVViewer uvViewer;
    bool showUVs = false;

    // Animation system (for controlling all the animations playing in scene)
    AnimationSystem animationSystem;

    Scene() : ambientLight(0.1f, 0.1f, 0.1f) {

        // Initialize the background member
        skybox = std::make_shared<Skybox>();

        //configure default background
        //background->setColor(glm::vec3(0.2f, 0.3f, 0.3f));


        // Create and set up default camera
        auto defaultCamera = std::make_shared<Camera>("Default");
        cameras.push_back(defaultCamera);
        activeCamera = defaultCamera;

        auto defaultLight = std::make_shared<SpotLight>();
        defaultLight->name = "defaultLight";
        defaultLight->setWorldPosition(glm::vec3(0.0f, 3.0f, 0.0f));
        defaultLight->intensity = 5.0f;
        defaultLight->direction = glm::normalize(glm::vec3(0.0f, -1.0f, 2.0f));
        addSpotLight(defaultLight);

        auto light2 = std::make_shared<SpotLight>();
        light2->name = "light2";
        light2->setWorldPosition(glm::vec3(0.0f, 3.0f, 0.0f));
        light2->intensity = 10.0f;
        light2->direction = glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f));
        light2->innerCutoff = glm::cos(glm::radians(25.0f));    // Inner angle of 25 degrees
        light2->outerCutoff = glm::cos(glm::radians(35.0f));    // Outer angle of 35 degrees
        addSpotLight(light2);
    }

    void setup() {
        //setup background
        skybox->setup();

        // Load cubemap textures
        std::vector<std::string> faces{
            getProjectRoot() + "/textures/cubemap_nx.png", // -x (left)
            getProjectRoot() + "/textures/cubemap_px.png", // +x (right)
            getProjectRoot() + "/textures/cubemap_py.png", // +y (top)
            getProjectRoot() + "/textures/cubemap_ny.png", // -y (bottom)
            getProjectRoot() + "/textures/cubemap_pz.png", // +z (front)
            getProjectRoot() + "/textures/cubemap_nz.png" // -z (back)
        };



        skybox->loadCubemap(faces);

        glClearColor(0.0f,  0.0f, 0.0f, 0.0f);
        glEnable(GL_DEPTH_TEST);
        glShadeModel(GL_SMOOTH);

        // Initialize shadow rendering
        shadowRenderer.initialize();

        // uvViewer.initialize();

        //new frame imgui??
    }

    // Node Management
    void addNode(std::shared_ptr<Node> node, const std::string& name = "") {
        sceneNodes.push_back(node);
        if (!name.empty()) {
            nodeRegistry[name] = node;
        }

        // Add all child nodes recursively
        for (const auto& child : node->children) {
            addNode(child);
        }
    }

    std::shared_ptr<Node> getNode(const std::string& name) {
        auto it = nodeRegistry.find(name);
        return (it != nodeRegistry.end()) ? it->second : nullptr;
    }

    void removeNode(const std::string& name) {
        auto node = getNode(name);
        if (node) {
            // Remove from registry
            nodeRegistry.erase(name);

            // Remove from scene nodes
            auto it = std::find(sceneNodes.begin(), sceneNodes.end(), node);
            if (it != sceneNodes.end()) {
                sceneNodes.erase(it);
            }
        }
    }

    // Physics Management
    void addPhysicsBody(std::shared_ptr<PhysXBody> body, const std::string& name = "") {
        physicsWorld.addBody(body);
        if (body->node) {
            addNode(body->node, name);
        }
    }

    // Camera Management
    void setActiveCamera(size_t index) {
        if (index < cameras.size()) {
            activeCamera = cameras[index];
        }
    }

    void addCamera(std::shared_ptr<Camera> camera) {
        cameras.push_back(camera);
    }


    void addSpotLight(std::shared_ptr<SpotLight> spotLight) {
        spotLights.push_back(spotLight);
        shadowRenderer.addSpotLight(spotLight);
        addNode(spotLight); // for light visualizer (sphere)

    }

    void addSunLight(std::shared_ptr<SunLight> sunLight) {
        sunLights.push_back(sunLight);
        addNode(sunLight); // for light visualizer (sphere)

    }

    // Scene Update and Rendering
    void update(float deltaTime) {
        if (play) {

            // Update animations
            animationSystem.update(deltaTime);

            // Update physics
            physicsWorld.updateSimulation(deltaTime);

            // Update scene graph
            for (auto& node : sceneNodes) {
                node->updateWorldTransform();
            }

        }
    }

    void render() {
        if (!activeCamera) return;

        // Get camera matrices
        glm::mat4 view = activeCamera->getViewMatrix();
        glm::mat4 projection = activeCamera->getProjectionMatrix();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        // Separate opaque and transparent objects
        std::vector<std::shared_ptr<Node>> opaqueNodes;
        std::vector<std::shared_ptr<Node>> transparentNodes;

        for (const auto& node : sceneNodes) {
            if (node->mesh && node->visible) {
                bool isTransparent = false;
                for (const auto& material : node->mesh->materials) {
                    if (material && material->alpha < 1.0f) {
                        isTransparent = true;
                        break;
                    }
                }
                if (isTransparent) {
                    transparentNodes.push_back(node);
                }
                else {
                    opaqueNodes.push_back(node);
                }
            }
        }

        // 1. First render shadow map

        shadowRenderer.renderShadowPass(opaqueNodes);  // Only render opaque objects to shadow map

        // 2. Reset viewport and render opaque objects with shadows
        glViewport(0, 0, screenWidth, screenHeight);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);

        if (drawObjects) {

            shadowRenderer.prepareMainPass(view, projection, activeCamera->cameraPos);

            shadowRenderer.renderMainPass(opaqueNodes, view, projection);

            // 3. Render transparent objects with special settings
            if (!transparentNodes.empty()) {
                // Sort transparent objects back-to-front
                std::sort(transparentNodes.begin(), transparentNodes.end(),
                    [&](const std::shared_ptr<Node>& a, const std::shared_ptr<Node>& b) {
                        glm::vec3 cameraPos = activeCamera->cameraPos;
                        float distA = glm::length(cameraPos - a->getWorldPosition());
                        float distB = glm::length(cameraPos - b->getWorldPosition());
                        return distA > distB;  // Sort back to front
                    });

                // Enable blending for transparent objects
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                glDepthMask(GL_FALSE);


                shadowRenderer.renderMainPass(transparentNodes, view, projection);

                // Reset states
                glDepthMask(GL_TRUE);
                glDisable(GL_BLEND);

            }
        }

        if (drawWireframes) {
            drawWireFrames();
        }

        if (drawControlsoverlay) {
            //drawControlsOverlay();
        }

        // Render skybox last for better performance
        if (skybox) {
            glDepthFunc(GL_LEQUAL);
            skybox->render(view, projection);
            glDepthFunc(GL_LESS);
        }

        glUseProgram(shadowRenderer.getMainShaderProgram());
    }

    void drawWireFrames() {
        // Save current shader program
        GLint currentProgram;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

        // Disable the main shader and switch to a basic shader for wireframes
        glUseProgram(0);

        // Save current polygon mode
        GLint previousPolygonMode[2];
        glGetIntegerv(GL_POLYGON_MODE, previousPolygonMode);

        // Enable wireframe mode to call each drawWireframe
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        // Set up matrices in fixed-function pipeline
        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(glm::value_ptr(activeCamera->getProjectionMatrix()));

        

        // Draw each wireframe
        for (const auto& node : sceneNodes) {
            if (node->mesh && node->visible) {
                glMatrixMode(GL_MODELVIEW);
                glLoadMatrixf(glm::value_ptr(activeCamera->getViewMatrix() * node->worldTransform));
                node->mesh->drawWireframe();
            }
        }

        // Restore original states
        glPolygonMode(GL_FRONT, previousPolygonMode[0]);
        glPolygonMode(GL_BACK, previousPolygonMode[1]);

        // Restore previous shader program
        glUseProgram(currentProgram);
    }

    // Animation control methods
    void playAction(const std::string& actionName, const std::string& nodeName,
        AnimationSystem::PlaybackMode mode = AnimationSystem::PlaybackMode::LOOP) {
        auto node = getNode(nodeName);
        if (!node) return;

        // Find the action in the node's mesh
        if (auto animatedMesh = std::dynamic_pointer_cast<AnimatedMesh>(node->mesh)) {
            for (const auto& action : animatedMesh->actions) {
                if (action.name == actionName) {
                    // Create a unique name for this action instance
                    std::string uniqueName = nodeName + "_" + actionName;
                    animationSystem.playAction(uniqueName,
                        std::make_shared<Action>(action),
                        node,  
                        mode);
                    return;
                }
            }
        }
    }

    void stopAction(const std::string& actionName, const std::string& nodeName) {
        std::string uniqueName = nodeName + "_" + actionName;
        animationSystem.stopAction(uniqueName);
    }

    void pauseAction(const std::string& actionName, const std::string& nodeName) {
        std::string uniqueName = nodeName + "_" + actionName;
        animationSystem.pauseAction(uniqueName);
    }

    void resumeAction(const std::string& actionName, const std::string& nodeName) {
        std::string uniqueName = nodeName + "_" + actionName;
        animationSystem.resumeAction(uniqueName);
    }

    void stopAllActions() {
        animationSystem.stopAllActions();
    }

    // Get list of available actions for a node
    std::vector<std::string> getAvailableActions(const std::string& nodeName) {
        std::vector<std::string> actions;
        auto node = getNode(nodeName);
        if (node) {
            if (auto animatedMesh = std::dynamic_pointer_cast<AnimatedMesh>(node->mesh)) {
                for (const auto& action : animatedMesh->actions) {
                    actions.push_back(action.name);
                }
            }
        }
        return actions;
    }

    void debugAnimations() {
        std::cout << "\n=== Scene Animations Debug ===\n";
        for (const auto& [name, activeAction] : animationSystem.getActiveActions()) {
            std::cout << "Action: " << name << std::endl;
            std::cout << "  Target Node: " << (activeAction.targetNode ? activeAction.targetNode->name : "none") << std::endl;
            std::cout << "  Playing: " << (activeAction.isPlaying ? "yes" : "no") << std::endl;
            std::cout << "  Weight: " << activeAction.weight << std::endl;
            std::cout << "  Speed: " << activeAction.speed << std::endl;
            std::cout << "  Mode: " << static_cast<int>(activeAction.mode) << std::endl;
        }
    }

    void togglePlayer() {
        playerMode = !playerMode;
        
        if (playerMode) {
            activeCamera = player.camera;
        }
        else {
            //default camera
            activeCamera = cameras[0];
        }


    }

    void drawControlsOverlay() {
        // Draw controls overlay
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_Always);
        ImGui::SetNextWindowBgAlpha(0.3f);
        ImGui::Begin("Controls", nullptr,
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_AlwaysAutoResize |
            ImGuiWindowFlags_NoSavedSettings |
            ImGuiWindowFlags_NoNav);

        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Controls:");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "WASD - Camera Movement");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Space - Toggle Camera Control");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "P - Toggle Menu");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "L - Toggle Play/Pause");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "G - Toggle Sphere Generation");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "O - Toggle Wireframes");
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "` - Toggle Console");

        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // selections
    // Add these new methods
    void addSelectedNode(std::shared_ptr<Node> node) {
        if (node && std::find(selectedNodes.begin(), selectedNodes.end(), node) == selectedNodes.end()) {
            selectedNodes.push_back(node);
        }
    }

    void clearSelection() {
        selectedNodes.clear();
    }

    void removeSelectedNode(std::shared_ptr<Node> node) {
        auto it = std::find(selectedNodes.begin(), selectedNodes.end(), node);
        if (it != selectedNodes.end()) {
            selectedNodes.erase(it);
        }
    }

    bool isNodeSelected(std::shared_ptr<Node> node) const {
        return std::find(selectedNodes.begin(), selectedNodes.end(), node) != selectedNodes.end();
    }

};
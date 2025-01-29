// EngineCore.h
#pragma once
#include "GameEngine.h"
#include "scene.h"
#include "ui.h"
#include "console.h"
#include "input.h"
#include "PhysXManager.h"

class EngineCore {
private:
    static EngineCore* instance;

    // Core systems
    GLFWwindow* window;
    std::vector<std::shared_ptr<Scene>> scenes;
    std::shared_ptr<Scene> activeScene;

    // 2d scene

    // Time management

    //For real time
    float deltaTime;

    // for simulations
    float simTime;
    float simSpeed;

    // Window properties
    int screenWidth;
    int screenHeight;

    // Menu System for the core
    MenuSystem menuSystem;

    //Will have regular viewport, and 

    // Private constructor for singleton
    EngineCore() :
        window(nullptr),
        deltaTime(0.0f),
        simTime(0.0f),
        simSpeed(1.0f),
        screenWidth(1792),
        screenHeight(1008) {}

public:
    static EngineCore& getInstance() {
        if (!instance) {
            instance = new EngineCore();
        }
        return *instance;
    }

    bool initialize() {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }

        // Create window
        window = glfwCreateWindow(screenWidth, screenHeight, "Game Engine", NULL, NULL);
        if (!window) {
            glfwTerminate();
            return false;
        }

        glfwMakeContextCurrent(window);

        // Initialize GLEW
        if (glewInit() != GLEW_OK) {
            return false;
        }

        // Initialize subsystems
        initializeSubsystems();

        return true;
    }

    void initializeSubsystems() {
        // Initialize ImGui
        initImGui(window);

        // Initialize PhysX
        PhysXManager::getInstance().initialize();

        // Initialize input system
        glfwSetKeyCallback(window, key_callback);
        glfwSetCursorPosCallback(window, mouse_callback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        // Create default scene
        auto defaultScene = std::make_shared<Scene>();
        addScene(defaultScene);
        setActiveScene(0);
    }

    void run() {
        while (!glfwWindowShouldClose(window)) {
            // Update time
            updateTime();

            // Process input
            processInput(window);

            // Update active scene
            if (activeScene) {
                activeScene->update(deltaTime);
            }

            // Render
            render();

            // Swap buffers
            glfwSwapBuffers(window);
            glfwPollEvents();
        }
    }

    void cleanup() {
        // Cleanup ImGui
        cleanupImGui();

        // Cleanup PhysX
        PhysXManager::getInstance().cleanup();

        // Cleanup GLFW
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    // Scene Management
    void addScene(std::shared_ptr<Scene> scene) {
        scenes.push_back(scene);
    }

    void setActiveScene(size_t index) {
        if (index < scenes.size()) {
            activeScene = scenes[index];
        }
    }

    std::shared_ptr<Scene> getActiveScene() {
        return activeScene;
    }

    // Getters
    GLFWwindow* getWindow() { return window; }
    float getDeltaTime() { return deltaTime; }
    float getSimTime() { return simTime; }
    int getScreenWidth() { return screenWidth; }
    int getScreenHeight() { return screenHeight; }

private:
    void updateTime() {
        static float lastFrame = 0.0f;
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        simTime += deltaTime * simSpeed;
    }

    void render() {
        if (activeScene) {
            // Clear the screen
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Start ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Render scene
            activeScene->render();

            // Render ImGui
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
    }
};

// Define the static instance
EngineCore* EngineCore::instance = nullptr;
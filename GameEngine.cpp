// GameEngine.cpp : Defines the entry point for the application.
//

#include "GameEngine.h"
#include "render.h"
#include "ui.h"
#include "input.h"
#include "camera.h"
#include "shape.h"
#include "PhysXManager.h"
#include "PhysXBody.h"
#include "PhysXWorld.h"
#include "font.h"
#include "console.h"
#include "debugRenderer.h" // for rendering depth map to quad
#include "shader.h"
#include "misc_funcs.h"
#include "randomGen.h"

// Specify shader paths

std::string textVertex_shaderPATH_string = getProjectRoot() + "/text_vertex_shader.glsl";
std::string textFragment_shaderPATH_string = getProjectRoot() + "/text_fragment_shader.glsl";
const char* textVertex_shaderPATH = textVertex_shaderPATH_string.c_str();
const char* textFragment_shaderPATH = textFragment_shaderPATH_string.c_str();

std::string depthVertexShaderPATH_string = getProjectRoot() + "/depth_vertex_shader.glsl";
std::string depthFragmentShaderPATH_string = getProjectRoot() + "/depth_fragment_shader.glsl";
const char* depthVertexShaderPATH = depthVertexShaderPATH_string.c_str();
const char* depthFragmentShaderPATH = depthFragmentShaderPATH_string.c_str();

std::string shadowVetexShaderPATH_string = getProjectRoot() + "/shadow_vertex_shader.glsl";
std::string shadowFragmentShaderPATH_string = getProjectRoot() + "/shadow_fragment_shader.glsl";
const char* shadowVetexShaderPATH = shadowVetexShaderPATH_string.c_str();
const char* shadowFragmentShaderPATH = shadowFragmentShaderPATH_string.c_str();


// Screen dimensions
int screenWidth = 1792;
int screenHeight = 1008;

bool consoleActive = false; // for console toggle
float simSpeed = 1.0f; // for controlling simulation time factor
bool play = false; // play sim/animation
bool gravityEnabled = true; // enable gravity
bool collisionEnabled = true; // enable collision detection
bool genSpheres = false; // generate spheres

//simulation params
bool simulationMode = true; // generate physics with set deltaTime and then play like a movie at custom speed
float simulationPlaybackTime = 0.0f;
float simulationPlaybackSpeed = 1.0f;

std::chrono::steady_clock::time_point startTime_sys;
float deltaTime_sys = 0.0f; // Time difference between frames (sys because might be different than deltaTime_sim)
float deltaTime_sim = 0.0f; // Time difference between frames for simulation


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//Global var
GLuint depthShaderProgram;
GLuint shadowShaderProgram;
GLuint shaderProgram;
GLuint textShaderProgram;
GLuint testShaderProgram;
GLuint debugDepthShaderProgram; // for rendering depth map to quad
GLuint shaders; // change shaders if needed
bool shadowsEnabled = true; // enable shadows
glm::vec3 lightPos(0.0f, 15.0f, 0.0f); // Example light position, dist from earth to sun is 150.4*10^9 m
glm::vec3 lightColor(1.0f, 1.0f, 1.0f); // White light
glm::mat4 lightProjection, lightView;
glm::mat4 lightSpaceMatrix;
float near_plane = 0.1f, far_plane = 50.0f; // for shadow mapping
float luminousPower = 1.0f; // Luminous flux of the light source

bool trackLight = false; // Make camera same as light view


// Physics world
PhysXWorld physicsWorld;

// Non-physics shapes (like the lightsource)
std::vector<std::shared_ptr<Shape>> shapes;

// for debugging
glm::vec3 spherePos;
glm::vec3 sphereCOM;
glm::vec3 sphereCentroid;

// for selected objects
std::shared_ptr<PhysXBody> selectedRB = nullptr; // Initialize selected rigid body pointer

GLuint depthMap;
GLuint depthMapFBO;
// higher values of SHADOW_WIDTH and SHADOW_HEIGHT will give better quality shadows
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

void depthMapSetup() {
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);

    // Debug: Check the generated FBO and texture IDs
    std::cout << "Generated FBO: " << depthMapFBO << ", Depth Map Texture: " << depthMap << std::endl;


    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

    // Check for texture creation errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cout << "Error creating depth texture: " << err << std::endl;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // IMPORTANT: Set viewport size for depth map rendering
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glEnable(GL_DEPTH_TEST);  // Enable depth testing
    glDepthFunc(GL_LESS);     // Set depth function to less than
    glDepthMask(GL_TRUE);     // Enable depth writing

    // Print depth testing state
    GLint depthTest, depthFunc, depthMask;
    glGetIntegerv(GL_DEPTH_TEST, &depthTest);
    glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
    glGetBooleanv(GL_DEPTH_WRITEMASK, (GLboolean*)&depthMask);

    std::cout << "Depth Test enabled: " << (depthTest ? "yes" : "no") << std::endl;
    std::cout << "Depth Func: " << depthFunc << std::endl;
    std::cout << "Depth Mask: " << (depthMask ? "yes" : "no") << std::endl;

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);


    // Check framebuffer status right after attachment
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer is not complete after setup! Status: " << status << std::endl;
    }

    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    // Verify attachment
    GLint attachmentType;
    glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE, &attachmentType);
    if (attachmentType != GL_TEXTURE) {
        std::cout << "Depth attachment is not a texture!" << std::endl;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void setupScene() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glShadeModel(GL_SMOOTH);

    //Set up depth map and shadows
    depthMapSetup();

    Shader depthShader(depthVertexShaderPATH, depthFragmentShaderPATH);
    depthShaderProgram = depthShader.getShaderProgram();

    debugDepthShaderProgram = initDebugDepthShader(); // for rendering depth map to quad

    Shader shadowShader(shadowVetexShaderPATH, shadowFragmentShaderPATH);
    shadowShaderProgram = shadowShader.getShaderProgram();

    // Create and compile shaders for text rendering
    textShaderProgram = createShaderProgram(textVertex_shaderPATH, textFragment_shaderPATH);


    // Set up projection matrix for text rendering
    glm::mat4 textProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
    glUseProgram(textShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(textProjection));

}

// Singularites in lightView matrix made creating this function necessary
glm::mat4 createLightView(const glm::vec3& lightPos, const glm::vec3& targetPos, const glm::vec3& upVector) {
    // Calculate the direction vector from light to target
    glm::vec3 direction = targetPos - lightPos;

    // Check if direction is parallel to up vector (their cross product is near zero)
    glm::vec3 right = glm::cross(direction, upVector);
    if (glm::length(right) < 0.0001f) {
        // If they're parallel, use a different up vector
        // If light is above target (common case), use world Z as temp up
        // If light is below target, use negative world Z as temp up
        glm::vec3 tempUp = (direction.y >= 0.0f) ?
            glm::vec3(0.0f, 0.0f, 1.0f) :
            glm::vec3(0.0f, 0.0f, -1.0f);

        return glm::lookAt(lightPos, targetPos, tempUp);
    }

    // Normal case - use the provided up vector
    return glm::lookAt(lightPos, targetPos, upVector);
}

// function definitions before main


int main() {

    std::cout << "GLM Version: "
        << GLM_VERSION_MAJOR << "."
        << GLM_VERSION_MINOR << "."
        << GLM_VERSION_PATCH << std::endl;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    //
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "OpenGL Game Engine", NULL, NULL);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // what is the exe dir?
    std::cout << getProjectRoot() << std::endl;

    glfwMakeContextCurrent(window);

    // Set up viewport
    glViewport(0, 0, screenWidth, screenHeight);

    // Set GLFW input mode to capture the mouse cursor
    glfwSetCursorPosCallback(window, mouse_callback);

    // Set the key callback
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Initialize GLEW after creating the context
    if (glewInit() != GLEW_OK) {
        return -1;
    }

    const char* glVersion = (const char*)glGetString(GL_VERSION);
    const char* glVendor = (const char*)glGetString(GL_VENDOR);
    const char* glRenderer = (const char*)glGetString(GL_RENDERER);

    std::cout << "OpenGL Version: " << glVersion << std::endl;
    std::cout << "GPU Vendor: " << glVendor << std::endl;
    std::cout << "GPU Renderer: " << glRenderer << std::endl;

    setupScene();
    camstate = false; // enable camera movement, default value is false
    initializeConsole(); // Initialize console for text input
    initImGui(window);

    // Initialize FreeType for font (after glfw and glew)
    std::string fontPATH_string = getProjectRoot() + "/fonts/Roboto/Roboto-Regular.ttf";
    const char* fontPATH = fontPATH_string.c_str();
    initFreeType(fontPATH);

    startTime_sys = std::chrono::steady_clock::now(); // Initialize start time

    // Initialize PhysX
    PhysXManager::getInstance().initialize();


    // Create physics objects
    auto ground = std::make_shared<RectPrism>(glm::vec3(0.0f, -3.0f, 0.0f), 10.0f, 1.0f, 10.0f);
    // ground->rotate(glm::vec3(0.0f, 0.0f, 1.0f), glm::pi<float>()*(45.0f/180.0f));
    auto groundBody = std::make_shared<PhysXBody>(ground, true);
    physicsWorld.addBody(groundBody);

    auto sphere = std::make_shared<Sphere>(glm::vec3(0.0f, 5.0f, 0.0f), 1.0f, 20, 20);
    auto sphereBody = std::make_shared<PhysXBody>(sphere, true);
    physicsWorld.addBody(sphereBody);

    selectedRB = sphereBody; //maintains live reference

    auto lightVisualizer = std::make_shared<Sphere>(lightPos, 0.1f, 10, 10);
    lightVisualizer->color = glm::vec3(1.0f, 1.0f, 0.0f); // Yellow color for visibility
    lightVisualizer->isEmissive = true;
    shapes.push_back(lightVisualizer);

    // Define your bounding box for sphere generation
    glm::vec3 boxMin(1.0f, 1.0f, 1.0f);  // Adjust as needed
    glm::vec3 boxMax(2.0f, 2.0f, 2.0f);   // Adjust as needed

    //generateRandomSpheres(shapes, bodies, boxMin, boxMax, 0.1f, 10, 10, 100);
    //generateRandomBoxes(shapes, bodies, boxMin, boxMax, 0.1f, 0.1f, 0.1f, 10);

    float shapeGenerationInterval = 0.1f;  // Generate spheres every 1 second
    float timeSinceLastGeneration = 0.0f;



    // set up inital cam view
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
    glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniform3fv(glGetUniformLocation(shadowShaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));
    updateCamera(shadowShaderProgram); // Update the camera view

    // Main loop
    while (!glfwWindowShouldClose(window)) {


        //Manage system time and simulation time
        auto currentTime = std::chrono::steady_clock::now(); // Get current time
        std::chrono::duration<float> elapsed = currentTime - startTime_sys; // Calculate elapsed time
        deltaTime_sys = elapsed.count(); // Store elapsed time
        deltaTime_sim = simSpeed * deltaTime_sys; // is this right? use sys for now
        startTime_sys = currentTime; // Update start time for next frame

        processInput(window);  // Process keyboard input

        // Update light position
        //lightPos = glm::vec3(5.0f * cos(0.1f * glfwGetTime()), 10.0f, 5.0f * sin(0.1f * glfwGetTime()));
        //lightVisualizer->translate(lightPos - lightVisualizer->center);


         // Generate random spheres
         timeSinceLastGeneration += deltaTime_sys;
         if (timeSinceLastGeneration >= shapeGenerationInterval && genSpheres) {
             //physicsWorld.debug(); 
             generateRandomSpheres(physicsWorld, boxMin, boxMax, 0.1f, 10, 10, 1);  // Generate 1 sphere
             // generateRandomBoxes(shapes, bodies, boxMin, boxMax, 0.1f, 0.1f, 0.1f, 1);
             //physicsWorld.debug();
             timeSinceLastGeneration = 0.0f;
         }

        // for shadows and depth map
        glUseProgram(depthShaderProgram);
        lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, near_plane, far_plane);
        glm::vec3 lightTarget(0.0f, 0.0f, 0.0f);  // Where the light is looking at
        glm::vec3 upVector(0.0f, 1.0f, 0.0f);     // World up vector

        //lightView = glm::lookAt(lightPos, lightTarget, upVector);

        // avoids singularites
        lightView = createLightView(
            lightPos,                      // Exact light position
            lightTarget,  // Exact target
            upVector   // Exact up vector
        );
        lightSpaceMatrix = lightProjection * lightView;

        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

        // Enable depth testing and writing for this framebuffer
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        // Set the clear depth
        glClearDepth(1.0f);

        ////new code: debug depth map (always white) -> (always black)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;


        glClear(GL_DEPTH_BUFFER_BIT);

        // Use your depth map shader and render scene
        if (shadowsEnabled) {
            for (size_t i = 0; i < physicsWorld.bodies.size(); ++i) {
                physicsWorld.bodies[i]->drawShadow(depthShaderProgram, lightSpaceMatrix);
            }
        }

        // draw other objects for shadow map here

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // render scene as normal using the generated depth/shadow map
        glViewport(0, 0, screenWidth, screenHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        projection = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
        view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        // Use the shadow shader program
        shaders = shadowShaderProgram;
        glUseProgram(shaders);
        glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shadowShaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));
        glUniform3fv(glGetUniformLocation(shadowShaderProgram, "lightColor"), 1, glm::value_ptr(lightColor));

        glActiveTexture(GL_TEXTURE1);  // Use texture unit 1 for shadow map
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(glGetUniformLocation(shadowShaderProgram, "shadowMap"), 1);
        // Update light position uniform
        glUniform3fv(glGetUniformLocation(shadowShaderProgram, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniformMatrix4fv(glGetUniformLocation(shadowShaderProgram, "lightSpaceMatrix"), 1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        //set light flux
        glUniform1f(glGetUniformLocation(shaders, "luminousPower"), luminousPower);

        if (camstate == true) {
            updateCamera(shaders); // Update the camera view

        }

        if (trackLight) {
            view = glm::lookAt(lightPos, glm::vec3(0.0f), glm::vec3(0.0, 1.0, 0.0));
        }

        //updatePhysicsWorld(deltaTime_sys); // Update physics world
        // 
        // Step PhysX simulation
        physicsWorld.updateSimulation(deltaTime_sys);



        //Animation
        // Create a quaternion for rotation (e.g., degrees around the z-axis)
        // glm::quat groundRotation = glm::angleAxis(glm::radians((float) glfwGetTime()), glm::vec3(0.0f, 1.0f, 0.0f));
        // sphereRB1.rotate(groundRotation);


        // Render objects
        drawAxes(view, projection); // world axes

        //draw physics rigid bodies in the physicsWorld
        for (size_t i = 0; i < physicsWorld.bodies.size(); ++i) {
            physicsWorld.bodies[i]->draw(shaders, view, projection, lightSpaceMatrix, depthMap);

        }


        // draw non physics shapes
        for (size_t i = 0; i < shapes.size(); ++i) {
            shapes[i]->draw(shaders, view, projection, lightSpaceMatrix, depthMap);

            //local axex
            shapes[i]->drawLocalAxes(view, projection);
        }

        // debug render depth map
        glViewport(0, 0, screenWidth / 4, screenHeight / 4); // Render to a quarter of the screen
        renderDepthMapToQuad(depthMap, lightSpaceMatrix, near_plane, far_plane);


        //draw menu after objects
        // Inside the main loop, just before glfwSwapBuffers(window):
        if (MenuSystem::getInstance().isMenuOpen()) {
            MenuSystem::getInstance().render();
        }

        // render console
        if (Console::getInstance().isVisible()) {
            Console::getInstance().render();
        }

        /*if (glGetError() != GL_NO_ERROR) {
            std::cerr << "OpenGL error: " << glGetError() << std::endl;
        }*/

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Clean up PhysX
    PhysXManager::getInstance().cleanup();

    //clean up imgui
    cleanupImGui();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

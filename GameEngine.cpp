// GameEngine.cpp : Defines the entry point for the application.
//

#include "GameEngine.h"
#include "render.h"
#include "ui.h"
#include "input.h"
#include "camera.h"
#include "PhysXManager.h"
#include "PhysXBody.h"
#include "PhysXWorld.h"
#include "font.h"
#include "console.h"
#include "debugRenderer.h" // for rendering depth map to quad
#include "shader.h"
#include "misc_funcs.h"
#include "randomGen.h"
#include "shadowRenderer.h"
#include "primitveNodes.h"
#include "scene.h"
#include "modelImporter.h"
#include "curveParameterization.h"
#include "Nodes\tube.h"
#include "Nodes\bin.h"
#include "surfaceParameterization.h"


// Specify shader paths

std::string textVertex_shaderPATH_string = getProjectRoot() + "/text_vertex_shader.glsl";
std::string textFragment_shaderPATH_string = getProjectRoot() + "/text_fragment_shader.glsl";
const char* textVertex_shaderPATH = textVertex_shaderPATH_string.c_str();
const char* textFragment_shaderPATH = textFragment_shaderPATH_string.c_str();

// Screen dimensions
int screenWidth = 1792;
int screenHeight = 1008;

bool consoleActive = false; // for console toggle
float simSpeed = 1.0f; // for controlling simulation time factor
bool genSpheres = false; // generate spheres

//simulation params
bool simulationMode = false; // generate physics with set deltaTime and then play like a movie at custom speed
float simulationPlaybackTime = 0.0f;
float simulationPlaybackSpeed = 1.0f;

std::chrono::steady_clock::time_point startTime_sys;
float deltaTime_sys = 0.0f; // Time difference between frames (sys because might be different than deltaTime_sim)
float deltaTime_sim = 0.0f; // Time difference between frames for simulation


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//debugging
GLuint debugDepthShaderProgram; // for rendering depth map to quad

//text shaders
GLuint textShaderProgram;

// light prop
bool trackLight = false; // Make camera same as light view

// for selected nodes
std::shared_ptr<Node> selectedNode = nullptr; // Initialize selected Node pointer

Scene scene;

void setupScene() {
    scene.setup();


    debugDepthShaderProgram = initDebugDepthShader(); // for rendering depth map to quad

    // Create and compile shaders for text rendering
    textShaderProgram = createShaderProgram(textVertex_shaderPATH, textFragment_shaderPATH);


    // Set up projection matrix for text rendering
    glm::mat4 textProjection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);
    glUseProgram(textShaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(textShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(textProjection));


    //new frame imgui??

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

    std::cout << "Project root = " << getProjectRoot() << std::endl;

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

    auto groundNode = std::make_shared<BoxNode>(10.0f, 0.5f, 10.0f);
    groundNode->name = "ground";
    std::cout << "node pos before = " << vec3_to_string(groundNode->getWorldPosition()) << std::endl;
    groundNode->setWorldPosition(glm::vec3(0.0f, -1.5f, 0.0f));
    std::cout << "node pos after = " << vec3_to_string(groundNode->getWorldPosition()) << std::endl;
    auto groundBody = std::make_shared<PhysXBody>(groundNode, true);
    std::cout << "body pos = " << vec3_to_string(groundBody->getPosition()) << std::endl;

    scene.addPhysicsBody(groundBody);


    //selectedRB = sphereBody; //maintains live reference

    // test blender import
    // Import a model with textures

    std::string modelPath = getProjectRoot() + "/blender/simple room.glb";

    ModelImporter importer;
    bool showUVs = false;

    auto model = importer.importGLB(modelPath);
    if (model) {
        //model->localScale = glm::vec3(0.01f, 0.01f, 0.01f);
        //model->updateWorldTransform();
        // for (const auto& child : model->children) {
        //     child->mesh->flipNormals();
        // }
        scene.addNode(model);



        // for viewing uv's
        //scene.uvViewer.setupMeshUVs(model->mesh);

        showUVs = false;
    }
    else {
        std::cout << "Import failed: " << importer.getLastError() << std::endl;
    }

    // auto aniModel = importer.importGLB(getProjectRoot() + "/blender/test ani.glb");
    // if (aniModel) {
    //     scene.addNode(aniModel);
    // }
    // 
    // std::cout << "\n --- Before Animation Check ---" << std::endl;
    // // Play an animation - with proper casting
    // if (auto animatedMesh = std::dynamic_pointer_cast<AnimatedMesh>(aniModel->mesh)) {
    //     std::cout << "\n ---- Model has animated mesh ----" << std::endl;
    //     // Print available animations
    //     for (const auto& action : animatedMesh->actions) {
    //         std::cout << "Found animation: " << action.name << " (duration: " << action.duration << "s)" << std::endl;
    //     }
    // 
    //     // Play the first animation
    //     scene.animationSystem.playAction("character_Walk",
    //         std::make_shared<Action>(animatedMesh->actions[0]),
    //         model,
    //         AnimationSystem::PlaybackMode::LOOP);
    // }

    // auto manModel = importer.importGLB(getProjectRoot() + "/blender/Base Character.glb");
    // if (manModel) {
    //     scene.addNode(manModel);
    // }


    //tube stuff
    // Using parametric curve
    auto curve = CurveParameterization(
        [](float t) { return glm::vec3(t, sin(t), cos(t)); },
        0.0f, 2.0f * glm::pi<float>()
    );

    TubeNode::TubeParameters params;
    params.radialSegments = 16;  // Higher resolution around circumference
    params.lengthSegments = 64;  // Higher resolution along length
    auto tubeFromCurve = std::make_shared<TubeNode>(curve, 0.1f, params);

    // Using points
    std::vector<glm::vec3> points = {
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(1.0f, 1.0f, 0.0f),
        glm::vec3(2.0f, 0.0f, 1.0f)
    };
    auto tubeFromPoints = std::make_shared<TubeNode>(points, 0.1f);

    //scene.addNode(tubeFromCurve);

    //testing surfaces
    // Create wavy ground using multiple sine waves for interesting terrain
    auto wavyGroundSurface = SurfaceParameterization(
        [](float u, float v) -> glm::vec3 {
            // Scale u,v to create a larger ground plane (-10 to 10 in x,z)
            float x = (u - 0.5f) * 200.0f;
            float z = (v - 0.5f) * 200.0f;

            // Helper function to compute height at any point
            auto getHeight = [](float x, float z) -> float {
                float y = 0.0f;
                // Simplified wave pattern for clearer lighting
                y += sin(x * 0.5f) * 1.0f;          // X wave
                y += sin(z * 0.5f) * 1.0f;          // Z wave
                y += sin(x * 1.2f + z * 1.2f) * 0.3f;  // Diagonal wave
                return y;
                };

            // Get the height at current point
            float y = getHeight(x, z);

            // Calculate exact normals using partial derivatives (dy/dx and dy/dz)
            const float epsilon = 0.01f;  // Small step for numerical derivatives
            float hL = getHeight(x - epsilon, z);  // Left point
            float hR = getHeight(x + epsilon, z);  // Right point
            float hD = getHeight(x, z - epsilon);  // Down point
            float hU = getHeight(x, z + epsilon);  // Up point

            // Calculate partial derivatives
            float dydx = (hR - hL) / (2.0f * epsilon);  // dy/dx
            float dydz = (hU - hD) / (2.0f * epsilon);  // dy/dz

            return glm::vec3(x, y, z);
        }
    );

    // Create surface parameters with higher resolution for smoother waves
    ParametricSurfaceNode::SurfaceParameters surfaceParams;
    surfaceParams.uSegments = 500;  // Increase resolution
    surfaceParams.vSegments = 500;
    surfaceParams.generateNormals = true;  // Important for lighting
    surfaceParams.generateUVs = false;      // For texturing

    // Create the node
    auto wavyGroundNode = std::make_shared<ParametricSurfaceNode>(wavyGroundSurface, surfaceParams);
    wavyGroundNode->mesh->flipNormals();
    wavyGroundNode->setWorldPosition(glm::vec3(0.0f, -5.0f, 0.0f));

    // Create a material with a blue-green color
    auto groundMaterial = std::make_shared<Material>();
    groundMaterial->baseColor = glm::vec3(0.2f, 0.5f, 0.7f);  // Blue-green color
    groundMaterial->roughness = 0.7f;  // More matte appearance
    groundMaterial->metallic = 0.01f;   // Non-metallic
    groundMaterial->specular = 0.35f;   // Slight specular highlight

    wavyGroundNode->mesh->materials[0] = groundMaterial;

    // Add to scene
    //scene.addNode(wavyGroundNode);


    //testing binBody
    auto binNode = std::make_shared<BinNode>(4.0f, 3.0f, 4.0f);
    binNode->setWorldPosition(glm::vec3(0.0f, 5.0f, 0.0f));

    // Create glass material
    auto glassMaterial = std::make_shared<Material>();
    glassMaterial->baseColor = glm::vec3(0.2f, 0.3f, 0.4f);  // Light blue tint to make it visible
    glassMaterial->transmission = 0.9f;  // transparent
    glassMaterial->ior = 1.52f;         // Typical glass IOR
    glassMaterial->roughness = 0.01f;    // Smooth surface
    glassMaterial->specular = 1.0f;      // Increased for more visible reflections
    glassMaterial->alpha = 0.2f;         // More transparent
    glassMaterial->metallic = 0.0f;      // Make sure it's not metallic
    glassMaterial->emission = glm::vec3(0.0f); // Make sure it's not emissive

    // Apply to all walls
    binNode->setMaterial(glassMaterial);

    auto binBody = std::make_shared<BinBody>(binNode, true);  // true for static
    //scene.addPhysicsBody(binBody, "my_bin");

    // Define bounding box for sphere generation
    glm::vec3 boxMin(0.0f, 0.0f, 0.0f);  
    glm::vec3 boxMax(1.0f, 1.0f, 1.0f);   
    boxMin += glm::vec3(0.0f, 5.0f, 0.0f);
    boxMax += glm::vec3(0.0f, 5.0f, 0.0f);

    float shapeGenerationInterval = 0.1f;  // Generate spheres every _ seconds
    float timeSinceLastGeneration = 0.0f;


    // Main loop
    while (!glfwWindowShouldClose(window)) {


        //Manage system time and simulation time
        auto currentTime = std::chrono::steady_clock::now(); // Get current time
        std::chrono::duration<float> elapsed = currentTime - startTime_sys; // Calculate elapsed time
        deltaTime_sys = elapsed.count(); // Store elapsed time
        deltaTime_sim = simSpeed * deltaTime_sys; // is this right? use sys for now
        startTime_sys = currentTime; // Update start time for next frame

        processInput(window);  // Process keyboard and mouse input
        
        scene.update(deltaTime_sys); // update animations, physics, etc.

         // Generate random spheres
         timeSinceLastGeneration += deltaTime_sys;
         if (timeSinceLastGeneration >= shapeGenerationInterval && genSpheres) {
             //physicsWorld.debug(); 
             generateRandomSpheres(scene, boxMin, boxMax, 0.1f, 10, 10, 1);  // Generate 1 sphere
             // generateRandomBoxes(shapes, bodies, boxMin, boxMax, 0.1f, 0.1f, 0.1f, 1);
             //physicsWorld.debug();
             timeSinceLastGeneration = 0.0f;
         }

         //render visible nodes with shadows
         scene.render();

        // Draw world axes
        drawAxes(scene.activeCamera->getViewMatrix(), scene.activeCamera->getProjectionMatrix()); 

        // Debug visualization: render depth map to quad
        // glViewport(0, 0, screenWidth / 4, screenHeight / 4);
        // renderDepthMapToQuad(scene.shadowRenderer.getShadowMap(1).depthMap, scene.shadowRenderer.getLightSpaceMatrix(1), scene.shadowRenderer.getNearPlane(), scene.shadowRenderer.getFarPlane());
        // 
        // glViewport(0, 0, screenWidth, screenHeight);


        // uv debug
        // if (showUVs) {  
        //     GLuint textureId = model->mesh->materials[0]->textureMaps["baseColor"].textureId;
        //     scene.uvViewer.render(textureId);
        // }

        

        //draw menu after objects
        // Inside the main loop, just before glfwSwapBuffers(window):
        if (MenuSystem::getInstance().isMenuOpen()) {
            MenuSystem::getInstance().render();
        }

        // render console
        if (Console::getInstance().isVisible()) {
            Console::getInstance().render();
        }

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

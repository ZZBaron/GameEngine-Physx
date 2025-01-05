#pragma once
#include "GameEngine.h"
#include "object3D.h"
#include "camera.h"
#include "PhysXWorld.h"
#include "shadowRenderer.h"


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


    // object nodes
	// scene should control which nodes are drawn on camera (view frustum culling)
	// but these are all the nodes in the scene including the culled ones and the ones in physicsWorld
	std::vector<std::shared_ptr<Node>> sceneNodes;
	std::unordered_map<std::string, std::shared_ptr<Node>> nodeRegistry;


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

	glm::vec3 ambientLight;
	std::vector<std::shared_ptr<Node>> lights;

	Scene() : ambientLight(0.1f, 0.1f, 0.1f) {
		// Create and set up default camera
		auto defaultCamera = std::make_shared<Camera>();
		cameras.push_back(defaultCamera);
		activeCamera = defaultCamera;

        shadowRenderer.shadowsEnabled = false;
	}

    // Draw


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

    void addLight(std::shared_ptr<Node> light) {

        std::cout << "\n=== Scene::addLight() ===\n";
        std::cout << "Light node position before adding: " <<
            light->getWorldPosition().x << ", " <<
            light->getWorldPosition().y << ", " <<
            light->getWorldPosition().z << "\n";

        std::cout << "Light node transform before adding:\n";
        for (int i = 0; i < 4; i++) {
            std::cout << "[ ";
            for (int j = 0; j < 4; j++) {
                std::cout << light->worldTransform[i][j] << " ";
            }
            std::cout << "]\n";
        }


        lights.push_back(light);
        addNode(light);

        std::cout << "Light node position after adding: " <<
            light->getWorldPosition().x << ", " <<
            light->getWorldPosition().y << ", " <<
            light->getWorldPosition().z << "\n";

    }

    // Scene Update and Rendering
    void update(float deltaTime) {
        if (play) {
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

        // glClearColor(0.2f, 0.2f, 0.2f, 1.0f); Can be useful for debug to clearColor
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 1. Shadow pass
        shadowRenderer.renderShadowPass(sceneNodes);

        // Set viewport back after shadows
        glViewport(0, 0, screenWidth, screenHeight);


        shadowRenderer.prepareMainPass(view, projection, activeCamera->cameraPos);

        if (drawObjects) {
            shadowRenderer.renderMainPass(sceneNodes, view, projection);
        }

        if (drawWireframes) {
            drawWireFrames();
        }

        glUseProgram(shadowRenderer.getMainShaderProgram());

        //draw axes
    }

    void drawWireFrames() {
        // Wireframe pass
        glUseProgram(0);  // Disable shaders

        glMatrixMode(GL_PROJECTION);
        glLoadMatrixf(glm::value_ptr(activeCamera->getProjectionMatrix()));

        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glm::value_ptr(activeCamera->getViewMatrix()));

        for (const auto& node : sceneNodes) {
            if (node->mesh) {
                glPushMatrix();
                glMultMatrixf(glm::value_ptr(node->worldTransform));
                node->mesh->drawWireframe();
                glPopMatrix();
            }
        }
    }

};
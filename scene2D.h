// Scene2D.h
#pragma once
#include "GameEngine.h"
#include "Camera2D.h"
#include "Renderer2D.h"

class Scene2D {
private:
    std::unique_ptr<Renderer2D> renderer;
    std::shared_ptr<Camera2D> camera;
    std::vector<std::shared_ptr<Node2D>> nodes;

public:
    void initialize() {
        renderer = std::make_unique<Renderer2D>();
        renderer->initialize();

        camera = std::make_shared<Camera2D>("MainCamera");
    }

    void render() {
        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT);

        // Set view and projection matrices
        renderer->setViewProjection(
            camera->getViewMatrix(),
            camera->getProjectionMatrix()
        );

        // Begin sprite batch
        renderer->beginBatch();

        // Draw all visible nodes
        for (const auto& node : nodes) {
            if (node->visible && node->sprite) {
                renderer->drawSprite(*node->sprite, node->worldTransform);
            }
        }

        // Flush remaining sprites
        renderer->flushBatch();
    }

    void addNode(std::shared_ptr<Node2D> node) {
        nodes.push_back(node);
    }

    std::shared_ptr<Camera2D> getCamera() const {
        return camera;
    }
};
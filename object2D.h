// Node2D.h
#pragma once
#include "GameEngine.h"


class Sprite {
public:
    GLuint texture;
    glm::vec2 size;        // Width and height of the sprite
    glm::vec4 color;       // Tint color
    glm::vec2 origin;      // Origin point for rotation (normalized 0-1)
    glm::vec4 sourceRect;  // Source rectangle for sprite sheets (x, y, width, height)

    Sprite() :
        texture(0),
        size(100.0f, 100.0f),
        color(1.0f),
        origin(0.5f),
        sourceRect(0.0f, 0.0f, 1.0f, 1.0f) {
        generateQuad();
    }

    void generateQuad() {
        // Generate a simple quad for the sprite
        float vertices[] = {
            // Positions          // Texture coords
            -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
             0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
             0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
            -0.5f,  0.5f, 0.0f,  0.0f, 1.0f
        };

        unsigned int indices[] = {
            0, 1, 2,
            2, 3, 0
        };

        // Create buffers...
        // (Buffer creation code similar to the 3D mesh, but simplified for 2D)
    }

    void draw(GLuint shaderProgram) {
        // Bind texture and draw the quad
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        // Draw using the shader program...
    }
};



class Node2D {
public:
    std::string name;
    Node2D* parent;
    std::vector<std::shared_ptr<Node2D>> children;

    // 2D Transform data
    glm::vec2 position;
    float rotation;  // Single angle for 2D rotation (in radians)
    glm::vec2 scale;
    glm::mat4 worldTransform;

    // Visibility
    bool visible;

    // Optional components
    std::shared_ptr<class Sprite> sprite;  // 2D sprite instead of 3D mesh

    Node2D() :
        parent(nullptr),
        position(0.0f),
        rotation(0.0f),
        scale(1.0f),
        worldTransform(1.0f),
        visible(true) {}

    virtual ~Node2D() = default;

    void updateWorldTransform() {
        // Build local transform
        glm::mat4 localTransform = glm::mat4(1.0f);
        localTransform = glm::translate(localTransform, glm::vec3(position, 0.0f));
        localTransform = glm::rotate(localTransform, rotation, glm::vec3(0.0f, 0.0f, 1.0f));
        localTransform = glm::scale(localTransform, glm::vec3(scale, 1.0f));

        // Combine with parent transform
        if (parent) {
            worldTransform = parent->worldTransform * localTransform;
        }
        else {
            worldTransform = localTransform;
        }

        // Update children
        for (auto& child : children) {
            child->updateWorldTransform();
        }
    }

    void setWorldPosition(const glm::vec2& worldPos) {
        if (parent) {
            // Convert world position to local space
            glm::vec2 parentWorldPos = glm::vec2(parent->worldTransform[3]);
            glm::vec2 localPos = (worldPos - parentWorldPos) /
                glm::vec2(parent->scale);
            position = localPos;
        }
        else {
            position = worldPos;
        }
        updateWorldTransform();
    }

    glm::vec2 getWorldPosition() const {
        return glm::vec2(worldTransform[3]);
    }
};
// Camera2D.h
#pragma once
#include "GameEngine.h"

class Camera2D {
public:
    std::string name;

    // 2D position (x, y) instead of 3D position
    glm::vec2 position = glm::vec2(0.0f);
    float zoom = 1.0f;  // Replace 3D perspective with 2D zoom

    // Screen parameters
    int screenWidth = 1792;
    int screenHeight = 1008;

    Camera2D(std::string name) : name(name) {}

    // Get view matrix for 2D
    glm::mat4 getViewMatrix() {
        glm::mat4 view = glm::mat4(1.0f);
        view = glm::translate(view, glm::vec3(-position.x, -position.y, 0.0f));
        view = glm::scale(view, glm::vec3(zoom, zoom, 1.0f));
        return view;
    }

    // Get orthographic projection matrix instead of perspective
    glm::mat4 getProjectionMatrix() {
        return glm::ortho(0.0f,
            static_cast<float>(screenWidth),
            static_cast<float>(screenHeight),
            0.0f,
            -1.0f, 1.0f);
    }

    void setPosition(const glm::vec2& newPos) {
        position = newPos;
    }

    void setZoom(float newZoom) {
        zoom = glm::max(0.1f, newZoom);  // Prevent negative or zero zoom
    }
};

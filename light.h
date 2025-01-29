// Light.h
#pragma once
#include "GameEngine.h"
#include "object3D.h"
#include "primitveNodes.h"

//general light class (not explicitly used)
class Light : public Node {
public:
    glm::vec3 color = glm::vec3(1.0f);
    float intensity = 1.0f;

    Light() {
        castsShadows = false; //light visualizer should not  receive shadows, light should pass completely through
        receivesShadows = false;
    }

    virtual ~Light() = default;

};

//requires cubemap
class PointLight : public Light {
public:
    float lightRadius = 10.0f;      // Light influence radius
    float constant = 1.0f;    // Attenuation factors
    float linear = 0.09f;
    float quadratic = 0.032f;

    PointLight() {
        type = NodeType::PointLight;
    }

    float calculateAttenuation(float distance) const {
        return 1.0f / (constant + linear * distance + quadratic * distance * distance);
    }
};

//perspective projection
class SpotLight : public PointLight {
public:
    float innerCutoff;  // Cosine of inner cone angle
    float outerCutoff;  // Cosine of outer cone angle
    glm::vec3 direction;

    SpotLight() :
        innerCutoff(glm::cos(glm::radians(12.5f))),
        outerCutoff(glm::cos(glm::radians(17.5f))),
        direction(0.0f, -1.0f, 0.0f) {
        type = NodeType::SpotLight;
    }
};

//orthographic projection
class SunLight : public Light { 
public:
    glm::vec3 direction;  // Direction of the sunlight (should be normalized)
    float ambientStrength = 0.1f;  // Strength of ambient light
    float shadowBias = 0.005f;     // Bias for shadow calculations

    // Size of the orthographic frustum for shadow mapping
    float left = -10.0f;
    float right = 10.0f;
    float bottom = -10.0f;
    float top = 10.0f;
    float near = 1.0f;
    float far = 50.0f;

    SunLight() : direction(0.0f, -1.0f, 0.0f) {
        type = NodeType::SunLight;

        // Sun is typically brighter than other lights
        intensity = 2.0f;

        // Default color is slightly warm (like sunlight)
        color = glm::vec3(1.0f, 0.95f, 0.8f);
    }

    // Get the orthographic projection matrix for shadow mapping
    glm::mat4 getOrthographicProjection() const {
        return glm::ortho(left, right, bottom, top, near, far);
    }

    // Get the view matrix for shadow mapping
    glm::mat4 getLightViewMatrix() const {
        // Use the light's position as the eye position
        glm::vec3 lightPos = getWorldPosition();
        // Look in the direction of the light rays
        return glm::lookAt(
            lightPos,
            lightPos + direction,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }

    // Update the orthographic frustum size based on scene bounds
    void updateFrustumToFitScene(const glm::vec3& sceneMin, const glm::vec3& sceneMax) {
        // Calculate the scene bounds in light space
        glm::mat4 lightView = getLightViewMatrix();
        glm::vec4 corners[8];
        corners[0] = lightView * glm::vec4(sceneMin.x, sceneMin.y, sceneMin.z, 1.0f);
        corners[1] = lightView * glm::vec4(sceneMin.x, sceneMin.y, sceneMax.z, 1.0f);
        corners[2] = lightView * glm::vec4(sceneMin.x, sceneMax.y, sceneMin.z, 1.0f);
        corners[3] = lightView * glm::vec4(sceneMin.x, sceneMax.y, sceneMax.z, 1.0f);
        corners[4] = lightView * glm::vec4(sceneMax.x, sceneMin.y, sceneMin.z, 1.0f);
        corners[5] = lightView * glm::vec4(sceneMax.x, sceneMin.y, sceneMax.z, 1.0f);
        corners[6] = lightView * glm::vec4(sceneMax.x, sceneMax.y, sceneMin.z, 1.0f);
        corners[7] = lightView * glm::vec4(sceneMax.x, sceneMax.y, sceneMax.z, 1.0f);

        // Find the bounds in light space
        glm::vec3 mins(std::numeric_limits<float>::max());
        glm::vec3 maxs(std::numeric_limits<float>::lowest());
        for (const auto& corner : corners) {
            mins.x = std::min(mins.x, corner.x);
            mins.y = std::min(mins.y, corner.y);
            mins.z = std::min(mins.z, corner.z);
            maxs.x = std::max(maxs.x, corner.x);
            maxs.y = std::max(maxs.y, corner.y);
            maxs.z = std::max(maxs.z, corner.z);
        }

        // Update the orthographic frustum
        left = mins.x;
        right = maxs.x;
        bottom = mins.y;
        top = maxs.y;
        near = -maxs.z;  // Convert to positive near plane
        far = -mins.z;   // Convert to positive far plane

        // Add some padding
        float padding = 1.1f;
        left *= padding;
        right *= padding;
        bottom *= padding;
        top *= padding;
    }
}; 
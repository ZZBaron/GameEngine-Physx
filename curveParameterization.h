#pragma once
#include "GameEngine.h"
#include "object3D.h"
#include <functional>
#include <vector>

// Class to represent a parametric curve in 3D space
class CurveParameterization {
public:
    // Function type for the parametric curve: R -> R^3
    using ParametricFunction = std::function<glm::vec3(float)>;

private:
    ParametricFunction func;
    float startParam;
    float endParam;

public:
    CurveParameterization(ParametricFunction f, float start = 0.0f, float end = 1.0f)
        : func(f), startParam(start), endParam(end) {}

    // Evaluate the curve at parameter t
    glm::vec3 evaluate(float t) const {
        // Clamp t to [startParam, endParam]
        t = glm::clamp(t, startParam, endParam);
        return func(t);
    }

    // Compute tangent using finite differences
    glm::vec3 evaluateTangent(float t, float h = 0.0001f) const {
        glm::vec3 next = evaluate(t + h);
        glm::vec3 prev = evaluate(t - h);
        return glm::normalize(next - prev);
    }

    float getStart() const { return startParam; }
    float getEnd() const { return endParam; }
};

// Represents a complete orthonormal frame (tangent, normal, binormal)
struct Frame {
    glm::vec3 tangent;
    glm::vec3 normal;
    glm::vec3 binormal;

    Frame() : tangent(1, 0, 0), normal(0, 1, 0), binormal(0, 0, 1) {}

    Frame(const glm::vec3& t, const glm::vec3& n, const glm::vec3& b)
        : tangent(t), normal(n), binormal(b) {}

    // Parallel transport the frame along a curve
    void transport(const glm::vec3& newTangent) {
        // Compute rotation from old tangent to new tangent
        glm::vec3 axis = glm::cross(tangent, newTangent);
        float cosAngle = glm::dot(tangent, newTangent);

        // Handle cases where vectors are parallel or anti-parallel
        if (glm::length(axis) < 0.000001f) {
            if (cosAngle > 0) return; // vectors are parallel, no change needed
            // vectors are anti-parallel, rotate 180° around any perpendicular axis
            axis = glm::normalize(glm::cross(tangent, glm::vec3(0, 1, 0)));
            if (glm::length(axis) < 0.000001f)
                axis = glm::normalize(glm::cross(tangent, glm::vec3(1, 0, 0)));
        }

        // Normalize axis and compute rotation angle
        axis = glm::normalize(axis);
        float angle = acos(glm::clamp(cosAngle, -1.0f, 1.0f));

        // Build rotation matrix
        glm::mat3 rotation = glm::rotate(glm::mat4(1.0f), angle, axis);

        // Apply rotation to frame
        normal = rotation * normal;
        binormal = rotation * binormal;
        tangent = newTangent;

        // Ensure orthonormality
        normal = glm::normalize(normal);
        binormal = glm::normalize(binormal);
        tangent = glm::normalize(tangent);
    }
};
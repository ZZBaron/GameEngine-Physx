#pragma once
#include "GameEngine.h"
#include "object3D.h"
#include <functional>
#include <vector>

// Class to represent a parametric surface in 3D space
class SurfaceParameterization {
public:
    // Function type for the parametric surface: R² -> R³
    // Takes u,v parameters and returns a 3D point
    using ParametricFunction = std::function<glm::vec3(float, float)>;

private:
    ParametricFunction func;
    float uStart, uEnd;  // Parameter bounds for u
    float vStart, vEnd;  // Parameter bounds for v

public:
    SurfaceParameterization(ParametricFunction f,
        float u_start = 0.0f, float u_end = 1.0f,
        float v_start = 0.0f, float v_end = 1.0f)
        : func(f)
        , uStart(u_start), uEnd(u_end)
        , vStart(v_start), vEnd(v_end) {}

    // Evaluate the surface at parameters (u,v)
    glm::vec3 evaluate(float u, float v) const {
        // Clamp parameters to bounds
        u = glm::clamp(u, uStart, uEnd);
        v = glm::clamp(v, vStart, vEnd);
        return func(u, v);
    }

    // Compute partial derivatives using finite differences
    glm::vec3 evaluatePartialU(float u, float v, float h = 0.0001f) const {
        glm::vec3 next = evaluate(u + h, v);
        glm::vec3 prev = evaluate(u - h, v);
        return glm::normalize(next - prev);
    }

    glm::vec3 evaluatePartialV(float u, float v, float h = 0.0001f) const {
        glm::vec3 next = evaluate(u, v + h);
        glm::vec3 prev = evaluate(u, v - h);
        return glm::normalize(next - prev);
    }

    // Compute surface normal at a point
    glm::vec3 evaluateNormal(float u, float v, float h = 0.0001f) const {
        glm::vec3 du = evaluatePartialU(u, v, h);
        glm::vec3 dv = evaluatePartialV(u, v, h);
        return glm::normalize(glm::cross(du, dv));
    }

    // Getters for parameter bounds
    float getUStart() const { return uStart; }
    float getUEnd() const { return uEnd; }
    float getVStart() const { return vStart; }
    float getVEnd() const { return vEnd; }
};

// Class to represent a surface constructed from a grid of control points
class ControlPointSurface {
private:
    std::vector<std::vector<glm::vec3>> controlPoints;
    int uDegree, vDegree;  // Polynomial degrees for interpolation

    // Evaluate basis functions (using de Casteljau's algorithm for simplicity)
    float evaluateBasis(float t, int i, int degree, const std::vector<float>& knots) const {
        if (degree == 0) {
            return (t >= knots[i] && t < knots[i + 1]) ? 1.0f : 0.0f;
        }

        float alpha1 = 0.0f, alpha2 = 0.0f;

        if ((knots[i + degree] - knots[i]) != 0.0f) {
            alpha1 = (t - knots[i]) / (knots[i + degree] - knots[i]);
        }

        if ((knots[i + degree + 1] - knots[i + 1]) != 0.0f) {
            alpha2 = (knots[i + degree + 1] - t) / (knots[i + degree + 1] - knots[i + 1]);
        }

        return alpha1 * evaluateBasis(t, i, degree - 1, knots) +
            alpha2 * evaluateBasis(t, i + 1, degree - 1, knots);
    }

public:
    ControlPointSurface(const std::vector<std::vector<glm::vec3>>& points,
        int u_degree = 3, int v_degree = 3)
        : controlPoints(points)
        , uDegree(u_degree)
        , vDegree(v_degree) {}

    // Convert to a parametric surface
    SurfaceParameterization toParametricSurface() const {
        return SurfaceParameterization(
            [this](float u, float v) -> glm::vec3 {
                // Simple bilinear interpolation for now
                // Could be extended to full NURBS evaluation
                int maxU = controlPoints.size() - 1;
                int maxV = controlPoints[0].size() - 1;

                float fu = u * maxU;
                float fv = v * maxV;

                int u0 = static_cast<int>(fu);
                int v0 = static_cast<int>(fv);
                int u1 = std::min(u0 + 1, maxU);
                int v1 = std::min(v0 + 1, maxV);

                float alphau = fu - u0;
                float alphav = fv - v0;

                glm::vec3 p00 = controlPoints[u0][v0];
                glm::vec3 p10 = controlPoints[u1][v0];
                glm::vec3 p01 = controlPoints[u0][v1];
                glm::vec3 p11 = controlPoints[u1][v1];

                return (1 - alphau) * (1 - alphav) * p00 +
                    alphau * (1 - alphav) * p10 +
                    (1 - alphau) * alphav * p01 +
                    alphau * alphav * p11;
            }
        );
    }
};

// Class to represent a surface mesh generated from a parameterization
class ParametricSurfaceNode : public Node {
public:
    struct SurfaceParameters {
        int uSegments = 32;    // Number of segments in u direction
        int vSegments = 32;    // Number of segments in v direction
        bool generateUVs = true;  // Whether to generate UV coordinates
        bool generateNormals = true;  // Whether to generate normals
    };

private:
    void generateMesh(const SurfaceParameterization& param, const SurfaceParameters& params) {
        mesh = std::make_shared<Mesh>();

        float uStep = (param.getUEnd() - param.getUStart()) / params.uSegments;
        float vStep = (param.getVEnd() - param.getVStart()) / params.vSegments;

        // Generate vertices
        for (int i = 0; i <= params.uSegments; ++i) {
            for (int j = 0; j <= params.vSegments; ++j) {
                float u = param.getUStart() + i * uStep;
                float v = param.getVStart() + j * vStep;

                // Position
                glm::vec3 pos = param.evaluate(u, v);
                mesh->positions.push_back(pos);

                // Normal
                if (params.generateNormals) {
                    glm::vec3 normal = param.evaluateNormal(u, v);
                    mesh->normals.push_back(normal);
                }

                // UV coordinates
                if (params.generateUVs) {
                    float uCoord = static_cast<float>(i) / params.uSegments;
                    float vCoord = static_cast<float>(j) / params.vSegments;
                    mesh->uvSets["map1"].push_back(glm::vec2(uCoord, vCoord));
                }
            }
        }

        // Generate indices
        for (int i = 0; i < params.uSegments; ++i) {
            for (int j = 0; j < params.vSegments; ++j) {
                int current = i * (params.vSegments + 1) + j;
                int next = current + 1;
                int bottom = current + (params.vSegments + 1);
                int bottomNext = bottom + 1;

                // First triangle
                mesh->indices.push_back(current);
                mesh->indices.push_back(bottom);
                mesh->indices.push_back(next);

                // Second triangle
                mesh->indices.push_back(next);
                mesh->indices.push_back(bottom);
                mesh->indices.push_back(bottomNext);
            }
        }

        mesh->setupBuffers();
    }

public:
    ParametricSurfaceNode(const SurfaceParameterization& param,
        const SurfaceParameters& params = SurfaceParameters()) {
        generateMesh(param, params);
    }

    // Constructor for control point surface
    ParametricSurfaceNode(const ControlPointSurface& surface,
        const SurfaceParameters& params = SurfaceParameters()) {
        generateMesh(surface.toParametricSurface(), params);
    }
};

// Example surface generators
namespace SurfaceExamples {
    // Create a plane
    inline SurfaceParameterization createPlane(float width = 1.0f, float height = 1.0f) {
        return SurfaceParameterization(
            [width, height](float u, float v) -> glm::vec3 {
                return glm::vec3(
                    (u - 0.5f) * width,
                    0.0f,
                    (v - 0.5f) * height
                );
            }
        );
    }

    // Create a sphere
    inline SurfaceParameterization createSphere(float radius = 1.0f) {
        return SurfaceParameterization(
            [radius](float u, float v) -> glm::vec3 {
                float phi = u * 2.0f * glm::pi<float>();
                float theta = v * glm::pi<float>();
                return glm::vec3(
                    radius * sin(theta) * cos(phi),
                    radius * cos(theta),
                    radius * sin(theta) * sin(phi)
                );
            }
        );
    }

    // Create a torus
    inline SurfaceParameterization createTorus(float majorRadius = 1.0f, float minorRadius = 0.25f) {
        return SurfaceParameterization(
            [majorRadius, minorRadius](float u, float v) -> glm::vec3 {
                float phi = u * 2.0f * glm::pi<float>();
                float theta = v * 2.0f * glm::pi<float>();
                return glm::vec3(
                    (majorRadius + minorRadius * cos(theta)) * cos(phi),
                    minorRadius * sin(theta),
                    (majorRadius + minorRadius * cos(theta)) * sin(phi)
                );
            }
        );
    }
};
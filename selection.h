// selection.h
#pragma once
#include "GameEngine.h"
#include "camera.h"
#include <limits>
#include "PhysXBody.h"
#include "PhysXWorld.h"
#include "object3D.h"
#include "scene.h"

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;

    Ray(const glm::vec3& o, const glm::vec3& d) : origin(o), direction(glm::normalize(d)) {}
};

class SelectionSystem {
private:
    std::shared_ptr<Camera> camera;
    static SelectionSystem* instance;
    std::shared_ptr<Node> selectedNode;

public:
    static SelectionSystem& getInstance();

    void setCamera(std::shared_ptr<Camera> cam) { camera = cam; }

    std::shared_ptr<Node> getSelectedNode() const { return selectedNode; }

    Ray screenToWorldRay(double mouseX, double mouseY, int screenWidth, int screenHeight,
        Scene& scene);

    bool handleSelection(double mouseX, double mouseY, int screenWidth, int screenHeight, Scene& scene, bool additive);

    void drawRay(Ray ray, float length, Scene& scene);

private:
    SelectionSystem() : selectedNode(nullptr) {}

    struct IntersectionResult {
        bool hit;
        float distance;
        IntersectionResult() : hit(false), distance(std::numeric_limits<float>::max()) {}
    };

    IntersectionResult rayIntersectMesh(const Ray& ray, const std::shared_ptr<Node>& node);
    IntersectionResult rayMeshIntersection(const Ray& localRay, const std::shared_ptr<Mesh>& mesh);
    bool triangleIntersection(const Ray& ray, const glm::vec3& v0, const glm::vec3& v1,
        const glm::vec3& v2, float& t);
};

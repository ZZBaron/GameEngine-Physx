// selection.h
#pragma once
#include "GameEngine.h"
#include "camera.h"
#include <limits>
#include "PhysXBody.h"
#include "PhysXWorld.h"
#include "object3D.h"

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;

    Ray(const glm::vec3& o, const glm::vec3& d) : origin(o), direction(glm::normalize(d)) {}
};

class SelectionSystem {
private:
    std::shared_ptr<Camera> camera;
    static SelectionSystem* instance;
    std::shared_ptr<PhysXBody> selectedBody;

public:
    static SelectionSystem& getInstance();
    std::shared_ptr<PhysXBody> getSelectedBody() const { return selectedBody; }

    Ray screenToWorldRay(double mouseX, double mouseY, int screenWidth, int screenHeight,
        const glm::mat4& projection, const glm::mat4& view);

    bool handleSelection(double mouseX, double mouseY, int screenWidth, int screenHeight,
        const glm::mat4& projection, const glm::mat4& view, PhysXWorld& physicsWorld);

private:
    SelectionSystem() : selectedBody(nullptr) {}

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

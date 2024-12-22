// selection.h
#pragma once
#include "GameEngine.h"
#include "camera.h"
#include <limits>
#include "PhysXBody.h"
#include "PhysXWorld.h"

// Forward declarations
extern PhysXWorld physicsWorld;
extern std::shared_ptr<PhysXBody> selectedRB;

// Ray structure for intersection testing
struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;

    Ray(const glm::vec3& o, const glm::vec3& d) : origin(o), direction(glm::normalize(d)) {}
};

class SelectionSystem {
private:
    static SelectionSystem* instance;

public:
    static SelectionSystem& getInstance();

    // Declare functions but don't define them here
    static Ray screenToWorldRay(double mouseX, double mouseY, int screenWidth, int screenHeight,
        const glm::mat4& projection, const glm::mat4& view);

    static bool raySphereIntersect(const Ray& ray, const Sphere* sphere, float& t);
    static bool rayBoxIntersect(const Ray& ray, const RectPrism* box, float& t);
    std::shared_ptr<PhysXBody> findIntersectedBody(const Ray& ray, PhysXWorld physicsWorld);

private:
    SelectionSystem() {} // Private constructor for singleton
};
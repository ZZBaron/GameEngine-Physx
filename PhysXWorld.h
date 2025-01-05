// PhysXWorld.h
#pragma once
#include <vector>
#include <memory>
#include "PhysXBody.h"

class PhysXWorld {
public:
    std::vector<std::shared_ptr<PhysXBody>> bodies;

    void addBody(std::shared_ptr<PhysXBody> body) {
        bodies.push_back(body);
    }

    void updateSimulation(float deltaTime) {
        PhysXManager::getInstance().simulate(deltaTime);

        // Update all nodes from physics state
        for (auto& body : bodies) {
            body->updateNode();
        }
    }

    void debug() {
        std::cout << "Physics bodies in world: " << bodies.size() << std::endl;
    }
};
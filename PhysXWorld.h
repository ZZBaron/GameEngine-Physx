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

        // Update all shapes
        for (auto& body : bodies) {
            body->updateShape();
        }
    }

    void debug() {
        std::cout << "size of bodies = " << bodies.size() << std::endl;
    }
};
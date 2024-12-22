// PhysXSimulation.h
#pragma once
#include "PhysXManager.h"
#include "PhysXBody.h"
#include <vector>

class PhysXSimulation {
private:
    std::vector<std::shared_ptr<PhysXBody>> bodies;
    float simulationDuration;
    float timeStep;

public:
    PhysXSimulation(float duration, float step) : simulationDuration(duration), timeStep(step) {}

    void addBody(std::shared_ptr<PhysXBody> body) {
        bodies.push_back(body);
    }

    void simulate() {
        float currentTime = 0.0f;
        while (currentTime < simulationDuration) {
            PhysXManager::getInstance().simulate(timeStep);
            currentTime += timeStep;
        }
    }

    const std::vector<std::shared_ptr<PhysXBody>>& getBodies() const {
        return bodies;
    }
};
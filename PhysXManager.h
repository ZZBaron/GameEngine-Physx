// PhysXManager.h
#pragma once

#include <PxPhysicsAPI.h>
#include "GameEngine.h"


using namespace physx;

class PhysXManager {
private:
    static PhysXManager* instance;
    PxDefaultAllocator allocator;
    PxDefaultErrorCallback errorCallback;
    PxFoundation* foundation;
    PxPhysics* physics;
    PxDefaultCpuDispatcher* dispatcher;
    PxScene* scene;
    PxPvd* pvd; // Physics visual debugger


public:
    // Singleton method
    static PhysXManager& getInstance() {
        if (instance == nullptr) {
            instance = new PhysXManager();
        }
        return *instance;
    }

    bool initialize() {
        // Create foundation
        foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);
        if (!foundation) {
            return false;
        }

        // Create PVD (PhysX Visual Debugger)
        pvd = PxCreatePvd(*foundation);
        PxPvdTransport* transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
        pvd->connect(*transport, PxPvdInstrumentationFlag::eALL);

        // Create physics
        physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), true, pvd);
        if (!physics) {
            return false;
        }


        // Create scene
        PxSceneDesc sceneDesc(physics->getTolerancesScale());
        sceneDesc.gravity = PxVec3(0.0f, -9.8f, 0.0f);
        dispatcher = PxDefaultCpuDispatcherCreate(2);
        sceneDesc.cpuDispatcher = dispatcher;
        sceneDesc.filterShader = PxDefaultSimulationFilterShader;
        scene = physics->createScene(sceneDesc);

        return true;
    }

    void cleanup() {
        PX_RELEASE(scene);
        PX_RELEASE(dispatcher);
        PX_RELEASE(physics);
        PX_RELEASE(pvd);
        PX_RELEASE(foundation);
    }

    PxPhysics* getPhysics() { return physics; }
    PxScene* getScene() { return scene; }

    // Simulation step
    void simulate(float deltaTime) {
        scene->simulate(deltaTime);
        scene->fetchResults(true);
    }

private:
    PhysXManager() : foundation(nullptr), physics(nullptr), dispatcher(nullptr), scene(nullptr), pvd(nullptr) {}
    ~PhysXManager() { cleanup(); }
};


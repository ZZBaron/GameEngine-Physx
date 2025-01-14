// PhysXBody2D.h
#pragma once
#include <PxPhysicsAPI.h>
#include "GameEngine.h"
#include "object2D.h"
#include "PhysXManager.h"


using namespace physx;

class PhysXBody2D {
public:
    PxRigidActor* actor;
    std::shared_ptr<Node2D> node;
    bool isStatic;
    float depth = 0.1f;  // Small depth for 2D physics

    PhysXBody2D(std::shared_ptr<Node2D> nodePtr, bool staticBody = false)
        : node(nodePtr), isStatic(staticBody) {
        createActor();
    }

    void createActor() {
        PxPhysics* physics = PhysXManager::getInstance().getPhysics();

        // Create transform (in XY plane)
        PxTransform transform(
            PxVec3(node->position.x, node->position.y, 0.0f),
            PxQuat(PxIdentity)  // No rotation initially
        );

        // Create the actor
        if (isStatic) {
            actor = physics->createRigidStatic(transform);
        }
        else {
            PxRigidDynamic* dynamicActor = physics->createRigidDynamic(transform);

            // Lock movement and rotation to 2D
            PxRigidDynamic* body = dynamicActor;
            body->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_LINEAR_Z, true);
            body->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_X, true);
            body->setRigidDynamicLockFlag(PxRigidDynamicLockFlag::eLOCK_ANGULAR_Y, true);

            actor = dynamicActor;
        }

        // Create material
        PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.6f);

        // Create shape based on sprite bounds
        if (node->sprite) {
            // Create box shape using sprite dimensions
            PxBoxGeometry geometry(
                node->sprite->size.x * 0.5f,  // half-width
                node->sprite->size.y * 0.5f,  // half-height
                depth * 0.5f                  // small depth for 2D
            );

            PxShape* shape = physics->createShape(geometry, *material);
            actor->attachShape(*shape);
        }

        // Add to physics scene
        PhysXManager::getInstance().getScene()->addActor(*actor);
    }

    void updateNode() {
        if (!actor || isStatic) return;

        // Get transform from physics
        PxTransform transform = actor->getGlobalPose();

        // Update node position (only X and Y)
        node->position.x = transform.p.x;
        node->position.y = transform.p.y;

        // Update node rotation (only around Z axis)
        float angle = atan2f(2.0f * (transform.q.w * transform.q.z), 1.0f - 2.0f * transform.q.z * transform.q.z);
        node->rotation = angle;

        // Update transform
        node->updateWorldTransform();
    }

    void applyForce(const glm::vec2& force) {
        if (PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>()) {
            dynamicActor->addForce(PxVec3(force.x, force.y, 0.0f));
        }
    }

    void setLinearVelocity(const glm::vec2& velocity) {
        if (PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>()) {
            dynamicActor->setLinearVelocity(PxVec3(velocity.x, velocity.y, 0.0f));
        }
    }

    glm::vec2 getLinearVelocity() const {
        if (PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>()) {
            PxVec3 vel = dynamicActor->getLinearVelocity();
            return glm::vec2(vel.x, vel.y);
        }
        return glm::vec2(0.0f);
    }

    void setAngularVelocity(float omega) {
        if (PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>()) {
            // Only allow rotation around Z axis
            dynamicActor->setAngularVelocity(PxVec3(0.0f, 0.0f, omega));
        }
    }

    float getAngularVelocity() const {
        if (PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>()) {
            return dynamicActor->getAngularVelocity().z;
        }
        return 0.0f;
    }
};
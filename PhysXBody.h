// PhysXBody.h
#pragma once
#include "shape.h"
#include <PxPhysicsAPI.h>
#include "PhysXManager.h"

using namespace physx;

enum class CoordSystem { // makes rotational methods look prettier
    World,
    Local
};


class PhysXBody {
private:
    PxRigidActor* actor;
    std::shared_ptr<Shape> shape;
    bool isStatic;

public:
    PhysXBody(std::shared_ptr<Shape> shapePtr, bool staticBody = false)
        : shape(shapePtr), isStatic(staticBody) {
        createActor();
    }

    void createActor() {
        PxPhysics* physics = PhysXManager::getInstance().getPhysics();

        // Create geometry based on shape type
        PxGeometry* geometry = nullptr;
        if (auto sphere = dynamic_cast<Sphere*>(shape.get())) {
            geometry = new PxSphereGeometry(sphere->radius);
        }
        else if (auto box = dynamic_cast<RectPrism*>(shape.get())) {
            geometry = new PxBoxGeometry(box->sideLength_a / 2, box->sideLength_b / 2, box->sideLength_c / 2);
        }

        if (!geometry) return;

        // Create transform
        PxTransform transform(
            PxVec3(shape->center.x, shape->center.y, shape->center.z),
            PxQuat(shape->orientation.x, shape->orientation.y, shape->orientation.z, shape->orientation.w)
        );

        // Create actor
        if (isStatic) {
            actor = physics->createRigidStatic(transform);
        }
        else {
            PxRigidDynamic* dynamicActor = physics->createRigidDynamic(transform);
            // Set mass for dynamic bodies
            PxRigidBodyExt::setMassAndUpdateInertia(*dynamicActor, 1.0f);
            actor = dynamicActor;
        }

        // Create material
        PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.6f); // Static friction, dynamic friction, restitution

        // Create shape and attach it to the actor
        PxShape* shape = physics->createShape(*geometry, *material);
        actor->attachShape(*shape);

        // Add actor to scene
        PhysXManager::getInstance().getScene()->addActor(*actor);

        delete geometry;
    }

    void updateShape() {
        if (!actor) return;

        // Update shape's transform from PhysX
        PxTransform transform = actor->getGlobalPose();
        shape->center = glm::vec3(transform.p.x, transform.p.y, transform.p.z);
        shape->orientation = glm::quat(transform.q.w, transform.q.x, transform.q.y, transform.q.z);
        shape->updateModelMatrix();
    }

    void draw(GLuint shaderProgram, glm::mat4 view, glm::mat4 projection, const glm::mat4& lightSpaceMatrix, GLuint depthMap) {
        shape->draw(shaderProgram, view, projection, lightSpaceMatrix, depthMap);
    }

    void drawShadow(GLuint depthShaderProgram, const glm::mat4& lightSpaceMatrix) {
        shape->drawShadow(depthShaderProgram, lightSpaceMatrix);
    }

    PxRigidActor* getActor() { return actor; }

    std::shared_ptr<Shape> getShape() const {
        return shape;
    }

    glm::vec3 getPosition() {
        if (!actor) return glm::vec3(0.0f); // Return zero vector if actor doesn't exist

        PxTransform transform = actor->getGlobalPose();
        return glm::vec3(transform.p.x, transform.p.y, transform.p.z);
    }

    float getMass() {
        // We need to cast to PxRigidDynamic since only dynamic bodies have mass
        if (PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>()) {
            return dynamicActor->getMass();
        }
        return 0.0f; // Static bodies have infinite mass, return 0 to indicate this
    }

    glm::vec3 getVelocity() {
        if (PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>()) {
            PxVec3 vel = dynamicActor->getLinearVelocity();
            return glm::vec3(vel.x, vel.y, vel.z);
        }
        return glm::vec3(0.0f); // Static bodies have no velocity
    }
    
    glm::vec3 getAngularVelocity(CoordSystem coords = CoordSystem::World) {
        if (PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>()) {
            PxVec3 worldAngVel = dynamicActor->getAngularVelocity();

            if (coords == CoordSystem::World) {
                return glm::vec3(worldAngVel.x, worldAngVel.y, worldAngVel.z);
            }
            else {
                PxTransform transform = dynamicActor->getGlobalPose();
                glm::quat orientation(transform.q.w, transform.q.x, transform.q.y, transform.q.z);
                return glm::inverse(glm::mat3_cast(orientation)) * glm::vec3(worldAngVel.x, worldAngVel.y, worldAngVel.z);
            }
        }
        return glm::vec3(0.0f);
    }

    glm::vec3 getAngularMomentum(CoordSystem coords = CoordSystem::World) {
        if (PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>()) {
            if (coords == CoordSystem::World) {
                glm::mat3 I_world = getInertiaTensor(CoordSystem::World);
                glm::vec3 omega = getAngularVelocity(CoordSystem::World);
                return I_world * omega;
            }
            else {
                glm::mat3 I_local = getInertiaTensor(CoordSystem::Local);
                glm::vec3 omega_local = getAngularVelocity(CoordSystem::Local);
                return I_local * omega_local;
            }
        }
        return glm::vec3(0.0f);
    }

    glm::mat3 getInertiaTensor(CoordSystem coords = CoordSystem::World) {
        if (PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>()) {
            // Get local (principal) inertia tensor
            PxVec3 diagonalInertia = dynamicActor->getMassSpaceInertiaTensor();
            glm::mat3 I_local(
                diagonalInertia.x, 0.0f, 0.0f,
                0.0f, diagonalInertia.y, 0.0f,
                0.0f, 0.0f, diagonalInertia.z
            );

            if (coords == CoordSystem::Local) {
                return I_local;
            }
            else {
                PxTransform transform = dynamicActor->getGlobalPose();
                glm::quat orientation(transform.q.w, transform.q.x, transform.q.y, transform.q.z);
                glm::mat3 R = glm::mat3_cast(orientation);
                return R * I_local * glm::transpose(R);
            }
        }
        return glm::mat3(0.0f);
    }



};
// PhysXBody.h
#pragma once
#include "object3D.h"
#include "primitveNodes.h"
#include <PxPhysicsAPI.h>
#include "PhysXManager.h"


using namespace physx;

enum class CoordSystem {
    World,
    Local
};

struct BoundingSphere {
    glm::vec3 boundingSphereCenter; // needs to be updated along with the actual node position
    float boundingSphereRadius; // only updated as actual mesh changes
};

class PhysXBody {
public:
    PxRigidActor* actor;
    std::shared_ptr<Node> node;
    bool isStatic;

    // bounding sphere for broad phase collisions
    BoundingSphere boundingSphere;

    PhysXBody(std::shared_ptr<Node> nodePtr, bool staticBody = false)
        : node(nodePtr), isStatic(staticBody) {
        node->updateWorldTransform();  // Add this line
        createActor();
        updateBoundingSphere();
    }

    void createActor() {
        std::cout << "\n=== PhysXBody::createActor() ===\n";
        std::cout << "Node world transform before physics init:\n";
        for (int i = 0; i < 4; i++) {
            std::cout << "[ ";
            for (int j = 0; j < 4; j++) {
                std::cout << node->worldTransform[i][j] << " ";
            }
            std::cout << "]\n";
        }

        std::cout << "Node world position: " <<
            node->worldTransform[3][0] << ", " <<
            node->worldTransform[3][1] << ", " <<
            node->worldTransform[3][2] << "\n";

        PxPhysics* physics = PhysXManager::getInstance().getPhysics();

        // Create geometry based on mesh type
        PxGeometry* geometry = createGeometryFromMesh();
        if (!geometry) return;

        // Create transform from node's world transform
        glm::vec3 position = glm::vec3(node->worldTransform[3]);
        glm::quat orientation = glm::quat_cast(glm::mat3(node->worldTransform));

        PxTransform transform(
            PxVec3(position.x, position.y, position.z),
            PxQuat(orientation.x, orientation.y, orientation.z, orientation.w)
        );

        // Create actor
        if (isStatic) {
            actor = physics->createRigidStatic(transform);
        }
        else {
            PxRigidDynamic* dynamicActor = physics->createRigidDynamic(transform);
            PxRigidBodyExt::updateMassAndInertia(*dynamicActor, 1.0f);
            actor = dynamicActor;
        }

        // Create material
        PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.6f);

        // Create shape and attach to actor
        PxShape* shape = physics->createShape(*geometry, *material);
           actor->attachShape(*shape);

        PhysXManager::getInstance().getScene()->addActor(*actor);

        delete geometry;
    }

    void updateNode() {
        if (!actor) return;

        // Update node transform from PhysX only if this is a dynamic body
        if (!isStatic) {
            PxTransform transform = actor->getGlobalPose();

            // Convert to glm
            glm::vec3 position(transform.p.x, transform.p.y, transform.p.z);
            glm::quat rotation(transform.q.w, transform.q.x, transform.q.y, transform.q.z);



            // Update only local transform components
            node->localTranslation = position;
            node->localRotation = rotation;

            // Let the scene graph update the world transform
            node->updateWorldTransform();

        }


    }

   

    // Physics-related methods from the original PhysXBody
    PxRigidActor* getActor() { return actor; }
    std::shared_ptr<Node> getNode() const { return node; }

    glm::vec3 getPosition() {
        if (!actor) return glm::vec3(0.0f);
        PxTransform transform = actor->getGlobalPose();
        return glm::vec3(transform.p.x, transform.p.y, transform.p.z);
    }

    float getMass() {
        if (PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>()) {
            return dynamicActor->getMass();
        }
        return 0.0f;
    }

    glm::vec3 getVelocity() {
        if (PxRigidDynamic* dynamicActor = actor->is<PxRigidDynamic>()) {
            PxVec3 vel = dynamicActor->getLinearVelocity();
            return glm::vec3(vel.x, vel.y, vel.z);
        }
        return glm::vec3(0.0f);
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

private:
    PxGeometry* createGeometryFromMesh() {
        if (!node->mesh) return nullptr;

        switch (node->type) {
            case NodeType::Sphere: {
                auto sphereNode = static_cast<SphereNode*>(node.get());
                return new PxSphereGeometry(sphereNode->radius);
            }

            case NodeType::Box: {
                auto boxNode = static_cast<BoxNode*>(node.get());
                return new PxBoxGeometry(
                    boxNode->width * 0.5f,
                    boxNode->height * 0.5f,
                    boxNode->depth * 0.5f
                );
            }

            case NodeType::Default:
            default: {
                if (node->mesh) {
                    // Fall back to computing bounding box for unknown types
                    glm::vec3 halfExtents = computeBoxHalfExtents(node->mesh.get());
                    return new PxBoxGeometry(halfExtents.x, halfExtents.y, halfExtents.z);
                }
                return nullptr;
            }
        }
    }

    PxGeometry* createTriangleMeshGeometry() {
        if (!node->mesh) return nullptr;

        PxPhysics* physics = PhysXManager::getInstance().getPhysics();

        // Create triangle mesh descriptor
        PxTriangleMeshDesc meshDesc;
        meshDesc.points.count = node->mesh->positions.size();
        meshDesc.points.stride = sizeof(glm::vec3);
        meshDesc.points.data = node->mesh->positions.data();

        meshDesc.triangles.count = node->mesh->indices.size() / 3;
        meshDesc.triangles.stride = 3 * sizeof(unsigned int);
        meshDesc.triangles.data = node->mesh->indices.data();

        // Set up cooking parameters
        PxCookingParams params(physics->getTolerancesScale());

        // Use correct preprocessing flags
        params.meshPreprocessParams = static_cast<PxMeshPreprocessingFlags>(
            PxMeshPreprocessingFlag::eWELD_VERTICES |
            PxMeshPreprocessingFlag::eFORCE_32BIT_INDICES
            );

        params.meshWeldTolerance = 0.001f; // Weld close vertices
        params.buildGPUData = true;        // Enable GPU acceleration if available

        // Cook the mesh
        PxDefaultMemoryOutputStream writeBuffer;
        PxTriangleMeshCookingResult::Enum result;
        bool status = PxCookTriangleMesh(params, meshDesc, writeBuffer, &result);

        if (!status) {
            std::cout << "Failed to cook triangle mesh!" << std::endl;
            return nullptr;
        }

        // Create the triangle mesh
        PxDefaultMemoryInputData readBuffer(writeBuffer.getData(), writeBuffer.getSize());
        PxTriangleMesh* triMesh = physics->createTriangleMesh(readBuffer);

        if (!triMesh) {
            std::cout << "Failed to create triangle mesh!" << std::endl;
            return nullptr;
        }

        // Create geometry with proper scaling
        glm::vec3 scale = node->localScale;
        PxMeshScale meshScale(PxVec3(scale.x, scale.y, scale.z));
        return new PxTriangleMeshGeometry(triMesh, meshScale);
    }

    void updateBoundingSphere() {
        if (node->mesh->positions.empty()) return;

        glm::vec3 center = computeBoundingSphereCenter(node->mesh.get());

        float radius = 0.0f;
        for (const auto& pos : node->mesh->positions) {
            float dist = glm::length(pos - center);
            radius = glm::max(radius, dist);
        }

        
    }

    glm::vec3 computeBoundingSphereCenter(Mesh* mesh) {
        if (mesh->positions.empty()) return glm::vec3(0.0f, 0.0f, 0.0f);

        glm::vec3 center = glm::vec3(0.0f);
        for (const auto& pos : mesh->positions) {
            center += pos;
        }
        center /= static_cast<float>(mesh->positions.size());
        return center;
    }

    bool computeBoundingSphere(Mesh* mesh, glm::vec3& center, float& radius) {
        if (mesh->positions.empty()) return false;

        center = glm::vec3(0.0f);
        for (const auto& pos : mesh->positions) {
            center += pos;
        }
        center /= static_cast<float>(mesh->positions.size());

        radius = 0.0f;
        for (const auto& pos : mesh->positions) {
            float dist = glm::length(pos - center);
            radius = glm::max(radius, dist);
        }

        return true;
    }

    glm::vec3 computeBoxHalfExtents(Mesh* mesh) {
        if (mesh->positions.empty()) return glm::vec3(1.0f);

        glm::vec3 minExtents(FLT_MAX);
        glm::vec3 maxExtents(-FLT_MAX);

        for (const auto& pos : mesh->positions) {
            minExtents = glm::min(minExtents, pos);
            maxExtents = glm::max(maxExtents, pos);
        }

        return (maxExtents - minExtents) * 0.5f;
    }
};
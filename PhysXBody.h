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

enum class GeometryType {
    Sphere,
    Box,
    Capsule,
    Mesh
};

class PhysXBody {
public:
    PxRigidActor* actor;
    std::shared_ptr<Node> node;
    bool isStatic;

    std::vector<std::shared_ptr<Node>> compoundParts; // For compound bodies

    // bounding sphere for broad phase collisions
    BoundingSphere boundingSphere;

    //default constructor
    PhysXBody() : actor(nullptr), node(nullptr), isStatic(false) {}


    PhysXBody(std::shared_ptr<Node> nodePtr, bool staticBody = false, bool useMesh=true)
        : node(nodePtr), isStatic(staticBody) {
        // node->updateWorldTransform();  

        if (useMesh) {
            createGeometryFromMesh();
            createActor();
		}
        updateBoundingSphere();
    }

    // Constructor for compound bodies
    PhysXBody(std::shared_ptr<Node> rootNode, const std::vector<std::shared_ptr<Node>>& parts, bool staticBody = false)
        : node(rootNode), compoundParts(parts), isStatic(staticBody) {
        node->updateWorldTransform();
        createCompoundActor();
    }

    void createSphereGeometry(float radius) {
		geometry = std::make_shared<PxSphereGeometry>(radius);
	}

    void createBoxGeometry(float width, float height, float depth) {
        geometry = std::make_shared<PxBoxGeometry>(width * 0.5f, height * 0.5f, depth * 0.5f);
    }

    void createCapsuleGeometry(float radius, float halfHeight) {
		geometry = std::make_shared<PxCapsuleGeometry>(radius, halfHeight);
	}

    void createActor() {

        PxPhysics* physics = PhysXManager::getInstance().getPhysics();
        
        // geometry should be defined before creating the actor
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

            // setSolverIterationCounts(minPositionIters, minVelocityIters)
            // - minPositionIters: Number of iterations for resolving position constraints (4-8 typical)
            //   Higher values = more accurate collision resolution but more CPU cost
            //   Lower values = faster but less stable stacking/resting contacts
            // - minVelocityIters: Number of iterations for resolving velocity constraints (1-2 typical)
            //   Higher values = more accurate friction/restitution but more CPU cost
            //   Most scenarios work fine with just 1-2 velocity iterations
            dynamicActor->setSolverIterationCounts(4, 1);  // Default values, more stable for many-body contacts

            // Add linear and angular damping to dissipate energy
            dynamicActor->setLinearDamping(0.5f);    // Range 0-1, helps calm linear vibration
            dynamicActor->setAngularDamping(0.5f);   // Range 0-1, helps calm rotational vibration

            PxRigidBodyExt::updateMassAndInertia(*dynamicActor, 1.0f);
            actor = dynamicActor;
        }

        // Create material
        PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.6f);

        // Create shape and attach to actor
        PxShape* shape = physics->createShape(*geometry, *material);
           actor->attachShape(*shape);

        PhysXManager::getInstance().getScene()->addActor(*actor);

    }

    void createCompoundActor() {
        PxPhysics* physics = PhysXManager::getInstance().getPhysics();

        // Create transform from root node's world transform
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
            actor = dynamicActor;
        }

        // Create material
        PxMaterial* material = physics->createMaterial(0.5f, 0.5f, 0.6f);

        // Attach shapes for each part
        for (const auto& part : compoundParts) {
            attachNodeShape(part.get(), material);
        }

        // Update mass properties for dynamic bodies
        if (!isStatic && actor->is<PxRigidDynamic>()) {
            PxRigidBodyExt::updateMassAndInertia(*actor->is<PxRigidDynamic>(), 1.0f);
        }

        PhysXManager::getInstance().getScene()->addActor(*actor);
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

    void createGeometryFromMesh() {

        if (!node->mesh) {
            geometry = nullptr;
            return;
        }

        switch (node->type) {
        case NodeType::Sphere: {
            auto sphereNode = static_cast<SphereNode*>(node.get());
            geometry = std::make_shared<PxSphereGeometry>(sphereNode->radius);
            return;
        }

        case NodeType::Box: {
            auto boxNode = static_cast<BoxNode*>(node.get());
            geometry = std::make_shared<PxBoxGeometry>(
                boxNode->width * 0.5f,
                boxNode->height * 0.5f,
                boxNode->depth * 0.5f
            );

            return;
        }

        case NodeType::Default:
        default: {
            if (node->mesh) {
                // Fall back to computing bounding box for unknown types
                glm::vec3 halfExtents = computeBoxHalfExtents(node->mesh.get());
                geometry = std::make_shared<PxBoxGeometry>(halfExtents.x, halfExtents.y, halfExtents.z);
                return;
            }

            return;
        }
        }
    }

   


    // Physics-related methods from the original PhysXBody
    PxRigidActor* getActor() { return actor; }
    std::shared_ptr<Node> getNode() const { return node; }

    std::shared_ptr<PxGeometry> getGeometry() const { return geometry; }

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
    std::shared_ptr<PxGeometry> geometry;


    void attachNodeShape(Node* node, PxMaterial* material) {
        if (!node) return;

        PxPhysics* physics = PhysXManager::getInstance().getPhysics();
        PxGeometry* geometry = nullptr;

        // Get local transform relative to root
        glm::mat4 relativeTransform = glm::inverse(this->node->worldTransform) * node->worldTransform;
        glm::vec3 localPos = glm::vec3(relativeTransform[3]);
        glm::quat localRot = glm::quat_cast(glm::mat3(relativeTransform));
        glm::vec3 localScale = node->localScale; // Consider scale if needed

        // Create appropriate geometry based on node type
        switch (node->type) {
        case NodeType::Sphere: {
            auto sphereNode = static_cast<SphereNode*>(node);
            geometry = new PxSphereGeometry(sphereNode->radius);
            break;
        }
        case NodeType::Box: {
            auto boxNode = static_cast<BoxNode*>(node);
            geometry = new PxBoxGeometry(
                boxNode->width * 0.5f,
                boxNode->height * 0.5f,
                boxNode->depth * 0.5f
            );
            break;
        }
        case NodeType::Cylinder: {
            // Add cylinder support if needed
            break;
        }
        default:
            // Default to bounding box for unknown types
            if (node->mesh) {
                glm::vec3 halfExtents = computeBoxHalfExtents(node->mesh.get());
                geometry = new PxBoxGeometry(halfExtents.x, halfExtents.y, halfExtents.z);
            }
            break;
        }

        if (geometry) {
            // Create and attach shape with local transform
            PxTransform localTransform(
                PxVec3(localPos.x, localPos.y, localPos.z),
                PxQuat(localRot.x, localRot.y, localRot.z, localRot.w)
            );

            PxShape* shape = physics->createShape(*geometry, *material);
            shape->setLocalPose(localTransform);
            actor->attachShape(*shape);

            delete geometry;
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

// Helper class for creating compound bodies
class CompoundBodyBuilder {
public:
    static std::shared_ptr<PhysXBody> createCompoundBody(
        std::shared_ptr<Node> rootNode,
        const std::vector<std::shared_ptr<Node>>& parts,
        bool isStatic = false)
    {
        return std::make_shared<PhysXBody>(rootNode, parts, isStatic);
    }
};
#pragma once
#include "GameEngine.h"
#include "scene.h"
#include "PhysXWorld.h"
#include "primitveNodes.h"

// Function to generate a random float between min and max
float randomFloat(float min, float max) {
    static std::default_random_engine generator(std::chrono::system_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<float> distribution(min, max);
    return distribution(generator);
}

// Function to generate a random color
glm::vec3 randomColor() {
    return glm::vec3(randomFloat(0.0f, 1.0f), randomFloat(0.0f, 1.0f), randomFloat(0.0f, 1.0f));
}

void generateRandomSpheres(Scene& scene,
    const glm::vec3& boxMin,
    const glm::vec3& boxMax,
    float radius,
    int numSlices,
    int numStacks,
    int count,
    float mass = 1.0f) {
    for (int i = 0; i < count; ++i) {
        // Generate random position within the bounding box
        glm::vec3 position(
            randomFloat(boxMin.x + radius, boxMax.x - radius),
            randomFloat(boxMin.y + radius, boxMax.y - radius),
            randomFloat(boxMin.z + radius, boxMax.z - radius)
        );

        // Create sphere

        auto sphereNode = std::make_shared<SphereNode>(radius, numSlices, numStacks);
        sphereNode->setWorldPosition(position);

        // Set random color for the mesh
        if (sphereNode->mesh) {
            sphereNode->mesh->materials[0]->baseColor = randomColor();
            // Update the mesh buffers to reflect the color change
            //sphereNode->mesh->updateBuffers();
        }

        // Create physics body for the sphere
        auto sphereBody = std::make_shared<PhysXBody>(sphereNode, false);  // false = dynamic body
        scene.addPhysicsBody(sphereBody);

      

    }
}

// void generateRandomBoxes(Scene& scene,
//     const glm::vec3& boundBoxMin,
//     const glm::vec3& boundBoxMax,
//     const float sideLength_a,
//     const float sideLength_b,
//     const float sideLength_c,
//     int count,
//     float mass = 1.0f) {
//     RectPrism boundingBox(boundBoxMin,boundBoxMax);
// 
//     for (int i = 0; i < count; ++i) {
//         // Generate random position within the bounding box
//         glm::vec3 position(
//             randomFloat(boundBoxMin.x + sideLength_a, boundBoxMax.x - sideLength_a),
//             randomFloat(boundBoxMin.y + sideLength_b, boundBoxMax.y - sideLength_b),
//             randomFloat(boundBoxMin.z + sideLength_c, boundBoxMax.z - sideLength_c)
//         );
// 
//         auto boxNode = std::make_shared<BoxNode>(sideLength_c, sideLength_b, sideLength_c);
//         boxNode->setWorldPosition(position);
// 
//     
// 
//         // Set random color for the mesh
//         if (boxNode->mesh) {
//             glm::vec4 randomVertexColor(randomColor(), 1.0f);  // Random RGB + alpha 1.0
//             for (size_t i = 0; i < boxNode->mesh->colors.size(); i++) {
//                 boxNode->mesh->colors[i] = randomVertexColor;
//             }
//             // Update the mesh buffers to reflect the color change
//             //boxNode->mesh->updateBuffers();
//         }
// 
//         // Create physics body for the sphere
//         auto boxBody = std::make_shared<PhysXBody>(boxNode, false);  // false = dynamic body
//         scene.addPhysicsBody(boxBody);
//     }
// }
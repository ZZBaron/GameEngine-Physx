#pragma once
#include "PhysXBody.h"

class BinNode : public Node {
public:
    std::shared_ptr<BoxNode> bottom;
    std::shared_ptr<BoxNode> frontWall;
    std::shared_ptr<BoxNode> backWall;
    std::shared_ptr<BoxNode> leftWall;
    std::shared_ptr<BoxNode> rightWall;

    BinNode(float width, float height, float depth, float wallThickness = 0.1f) {
        // Create the bottom with full width and depth
        bottom = std::make_shared<BoxNode>(width - 2.0f*wallThickness, wallThickness, depth);
        bottom->localTranslation = glm::vec3(0.0f, -height / 2.0f - wallThickness/ 2.0, 0.0f);
        addChild(bottom);

        // Create front wall 
        frontWall = std::make_shared<BoxNode>(width - 2 * wallThickness, height, wallThickness);
        frontWall->localTranslation = glm::vec3(0.0f, 0.0f, depth / 2.0f - wallThickness / 2.0f);
        addChild(frontWall);

        // Create back wall 
        backWall = std::make_shared<BoxNode>(width - 2 * wallThickness, height, wallThickness);
        backWall->localTranslation = glm::vec3(0.0f, 0.0f, -depth / 2.0f + wallThickness / 2.0f);
        addChild(backWall);

        // Create left wall - longer than front and back
        leftWall = std::make_shared<BoxNode>(wallThickness, height, depth - 2 * wallThickness);
        leftWall->localTranslation = glm::vec3(-width / 2.0f + 1.5f*wallThickness, 0.0f, 0.0f);
        addChild(leftWall);

        // Create right wall - longer than front and back
        rightWall = std::make_shared<BoxNode>(wallThickness, height, depth - 2 * wallThickness);
        rightWall->localTranslation = glm::vec3(width / 2.0f - 1.5f*wallThickness, 0.0f, 0.0f);
        addChild(rightWall);

        // Update transforms
        updateWorldTransform();
    }

    // Set material for all walls
    void setMaterial(std::shared_ptr<Material> material) {
        bottom->mesh->materials = { material };
        frontWall->mesh->materials = { material };
        backWall->mesh->materials = { material };
        leftWall->mesh->materials = { material };
        rightWall->mesh->materials = { material };
    }
};

// New BinBody that uses the compound body system
class BinBody : public PhysXBody {
public:
    std::shared_ptr<BinNode> binNode;

    BinBody(std::shared_ptr<BinNode> bin, bool isStatic = true)
        : PhysXBody(bin, collectParts(bin), isStatic), binNode(bin) {}

    // Factory method to create a bin with physics
    static std::shared_ptr<BinBody> createBin(float width, float height, float depth,
        float wallThickness = 0.1f,
        bool isStatic = true) {
        auto bin = std::make_shared<BinNode>(width, height, depth, wallThickness);
        return std::make_shared<BinBody>(bin, isStatic);
    }

private:
    static std::vector<std::shared_ptr<Node>> collectParts(std::shared_ptr<BinNode> bin) {
        std::vector<std::shared_ptr<Node>> parts;
        parts.push_back(bin->bottom);
        parts.push_back(bin->frontWall);
        parts.push_back(bin->backWall);
        parts.push_back(bin->leftWall);
        parts.push_back(bin->rightWall);
        return parts;
    }
};

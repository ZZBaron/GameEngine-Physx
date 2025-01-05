#pragma once
#include "GameEngine.h"
#include "object3D.h"
#include "textureManager.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>

class ModelImporter {
private:
    Assimp::Importer importer;
    std::string lastError;
    std::string modelDirectory;

    // Helper struct for texture mapping
    struct TextureMapping {
        aiTextureType aiType;
        std::string engineType;

        TextureMapping(aiTextureType type, const std::string& engType)
            : aiType(type), engineType(engType) {}
    };

    glm::mat4 aiToGlmMatrix(const aiMatrix4x4& aiMat) {
        return glm::mat4(
            aiMat.a1, aiMat.b1, aiMat.c1, aiMat.d1,
            aiMat.a2, aiMat.b2, aiMat.c2, aiMat.d2,
            aiMat.a3, aiMat.b3, aiMat.c3, aiMat.d3,
            aiMat.a4, aiMat.b4, aiMat.c4, aiMat.d4
        );
    }

    std::shared_ptr<Material> processMaterial(aiMaterial* material, const aiScene* scene) {

        std::cout << "\n=== Processing Material ===" << std::endl;
        std::cout << "Material name: " << (material->GetName().C_Str()) << std::endl;

        auto newMaterial = std::make_shared<Material>();

        if (!material) { 
            std::cout << "Material not found" << std::endl;
            return newMaterial; 
        }

        // Try standard properties first
        aiColor4D baseColor(1.0f);
        if (material->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS) {
            std::cout << "Found base color via AI_MATKEY_BASE_COLOR" << std::endl;
            newMaterial->baseColor = glm::vec3(baseColor.r, baseColor.g, baseColor.b);
        }
        else if (material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == AI_SUCCESS) {
            std::cout << "Found base color via AI_MATKEY_COLOR_DIFFUSE" << std::endl;
            newMaterial->baseColor = glm::vec3(baseColor.r, baseColor.g, baseColor.b);
        }
        else {
            std::cout << "Standard color properties not found, trying PBR..." << std::endl;

            // Try PBR properties
            aiMaterialProperty* baseColorProp;
            if (material->Get("$mat.gltf.pbrMetallicRoughness.baseColorFactor", 0, 0, baseColorProp) == AI_SUCCESS) {
                std::cout << "Found PBR base color factor" << std::endl;
                if (baseColorProp->mDataLength >= sizeof(float) * 3) {
                    float* colorData = reinterpret_cast<float*>(baseColorProp->mData);
                    newMaterial->baseColor = glm::vec3(colorData[0], colorData[1], colorData[2]);
                }
            }
            else {
                std::cout << "No PBR base color found either" << std::endl;
            }
        }

        std::cout << "Final baseColor = " << vec3_to_string(newMaterial->baseColor) << std::endl;

        // Dump all material properties for debugging
        for (unsigned int i = 0; i < material->mNumProperties; i++) {
            aiMaterialProperty* prop = material->mProperties[i];
            std::cout << "Property " << i << ": " << prop->mKey.C_Str()
                << " (type=" << prop->mType
                << ", size=" << prop->mDataLength << ")" << std::endl;
        }

        // Subsurface
        float subsurface;
        if (material->Get(AI_MATKEY_TRANSMISSION_FACTOR, subsurface) == AI_SUCCESS) {
            newMaterial->subsurface = subsurface;
        }

        // Subsurface Color
        aiColor3D subsurfaceColor;
        if (material->Get(AI_MATKEY_COLOR_TRANSPARENT, subsurfaceColor) == AI_SUCCESS) {
            newMaterial->subsurfaceColor = glm::vec3(subsurfaceColor.r, subsurfaceColor.g, subsurfaceColor.b);
        }

        // Metallic
        material->Get(AI_MATKEY_METALLIC_FACTOR, newMaterial->metallic);

        // Specular
        float specular;
        if (material->Get(AI_MATKEY_SHININESS_STRENGTH, specular) == AI_SUCCESS) {
            newMaterial->specular = specular;
        }

        // Roughness
        material->Get(AI_MATKEY_ROUGHNESS_FACTOR, newMaterial->roughness);

        // IOR
        material->Get(AI_MATKEY_REFRACTI, newMaterial->ior);

        // Transmission
        material->Get(AI_MATKEY_TRANSMISSION_FACTOR, newMaterial->transmission);

        // Emission
        aiColor3D emission(0.0f);
        if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emission) == AI_SUCCESS) {
            newMaterial->emission = glm::vec3(emission.r, emission.g, emission.b);
        }
        float emissionStrength;
        if (material->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissionStrength) == AI_SUCCESS) {
            newMaterial->emissionStrength = emissionStrength;
        }

        // Alpha
        float opacity;
        if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
            newMaterial->alpha = opacity;
        }

        // Process textures
        std::vector<TextureMapping> textureMappings;
        textureMappings.push_back(TextureMapping(aiTextureType_BASE_COLOR, "baseColor"));
        textureMappings.push_back(TextureMapping(aiTextureType_NORMAL_CAMERA, "normal"));
        textureMappings.push_back(TextureMapping(aiTextureType_METALNESS, "metallic"));
        textureMappings.push_back(TextureMapping(aiTextureType_DIFFUSE_ROUGHNESS, "roughness"));
        textureMappings.push_back(TextureMapping(aiTextureType_EMISSION_COLOR, "emission"));
        textureMappings.push_back(TextureMapping(aiTextureType_OPACITY, "alpha"));
        textureMappings.push_back(TextureMapping(aiTextureType_AMBIENT_OCCLUSION, "occlusion"));

        for (const auto& mapping : textureMappings) {
            aiString texPath;
            if (material->GetTexture(mapping.aiType, 0, &texPath) == AI_SUCCESS) {
                Material::TextureMap texMap;

                // Handle embedded textures
                const aiTexture* embeddedTex = scene->GetEmbeddedTexture(texPath.C_Str());
                if (embeddedTex) {
                    texMap.textureId = TextureManager::getInstance().LoadFromMemory(
                        reinterpret_cast<unsigned char*>(embeddedTex->pcData),
                        embeddedTex->mWidth * (embeddedTex->mHeight == 0 ? 1 : embeddedTex->mHeight),
                        embeddedTex->achFormatHint
                    );
                }
                else {
                    texMap.textureId = TextureManager::getInstance().LoadTexture(
                        (std::filesystem::path(modelDirectory) / texPath.C_Str()).string(),
                        mapping.engineType
                    );
                }

                // Get UV transform
                aiUVTransform transform;
                if (material->Get(AI_MATKEY_UVTRANSFORM(mapping.aiType, 0), transform) == AI_SUCCESS) {
                    texMap.offset = glm::vec2(transform.mTranslation.x, transform.mTranslation.y);
                    texMap.tiling = glm::vec2(transform.mScaling.x, transform.mScaling.y);
                }

                newMaterial->textureMaps[mapping.engineType] = texMap;
            }
        }

        return newMaterial;
    }

    std::shared_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene) {
        auto newMesh = std::make_shared<Mesh>(false);

        // Process vertices
        for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
            newMesh->positions.push_back(glm::vec3(
                mesh->mVertices[i].x,
                mesh->mVertices[i].y,
                mesh->mVertices[i].z
            ));

            if (mesh->HasNormals()) {
                newMesh->normals.push_back(glm::vec3(
                    mesh->mNormals[i].x,
                    mesh->mNormals[i].y,
                    mesh->mNormals[i].z
                ));
            }

            if (mesh->HasTextureCoords(0)) {
                newMesh->uvSets["map1"].push_back(glm::vec2(
                    mesh->mTextureCoords[0][i].x,
                    mesh->mTextureCoords[0][i].y
                ));
            }

            if (mesh->HasVertexColors(0)) {
                newMesh->colors.push_back(glm::vec4(
                    mesh->mColors[0][i].r,
                    mesh->mColors[0][i].g,
                    mesh->mColors[0][i].b,
                    mesh->mColors[0][i].a
                ));
            }
            else {
                newMesh->colors.push_back(glm::vec4(1.0f));
            }
        }

        // Process indices
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            aiFace face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                newMesh->indices.push_back(face.mIndices[j]);
            }
        }

        // Process material
        if (mesh->mMaterialIndex >= 0) {
            newMesh->materials.push_back(processMaterial(scene->mMaterials[mesh->mMaterialIndex], scene));
        }

        newMesh->setupBuffers();
        return newMesh;
    }

    void processNode(aiNode* node, const aiScene* scene, std::shared_ptr<Node> engineNode) {
        // Process transforms
        glm::mat4 transform = aiToGlmMatrix(node->mTransformation);
        engineNode->localTranslation = glm::vec3(transform[3]);
        engineNode->localRotation = glm::quat(transform);
        engineNode->localScale = glm::vec3(1.0f);

        // Process meshes
        if (node->mNumMeshes > 0) {
            engineNode->mesh = processMesh(scene->mMeshes[node->mMeshes[0]], scene);
        }

        // Process children
        for (unsigned int i = 0; i < node->mNumChildren; i++) {
            auto childNode = std::make_shared<Node>();
            childNode->name = node->mChildren[i]->mName.C_Str();
            engineNode->addChild(childNode);
            processNode(node->mChildren[i], scene, childNode);
        }
    }

public:
    std::shared_ptr<Node> importGLB(const std::string& path) {
        modelDirectory = std::filesystem::path(path).parent_path().string();

        unsigned int flags = aiProcess_Triangulate |
            aiProcess_GenSmoothNormals |
            aiProcess_CalcTangentSpace |
            aiProcess_JoinIdenticalVertices |
            aiProcess_FlipUVs |
            aiProcess_PreTransformVertices;

        const aiScene* scene = importer.ReadFile(path, flags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            lastError = importer.GetErrorString();
            return nullptr;
        }

        auto rootNode = std::make_shared<Node>();
        rootNode->name = "GLB_Root";
        processNode(scene->mRootNode, scene, rootNode);
        rootNode->updateWorldTransform();
        return rootNode;
    }

    const std::string& getLastError() const { return lastError; }
};
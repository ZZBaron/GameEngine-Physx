#pragma once
#include "GameEngine.h"
#include "object3D.h"
#include "misc_funcs.h"
#include "textureManager.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "animation.h"

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

    void debugMesh(aiMesh* originalMesh, const aiScene* scene, const std::shared_ptr<Mesh>& processedMesh) {
        std::cout << "\n=== Mesh Processing Debug Output ===" << std::endl;
        std::cout << "Original mesh name: " << originalMesh->mName.C_Str() << std::endl;

        // Compare vertex counts
        std::cout << "\nVertex Count Verification:" << std::endl;
        std::cout << "Original vertices: " << originalMesh->mNumVertices << std::endl;
        std::cout << "Processed vertices: " << processedMesh->positions.size() << std::endl;

        // Debug UV data conversion
        if (originalMesh->HasTextureCoords(0)) {
            std::cout << "\nUV Coordinate Verification:" << std::endl;
            std::cout << "Original UV channel count: " << originalMesh->GetNumUVChannels() << std::endl;
            std::cout << "Processed UV sets: " << processedMesh->uvSets.size() << std::endl;

            // Print first few vertices with their UVs for verification
            const unsigned int maxDebugVertices = 5;
            const unsigned int debugVertexCount = std::min(maxDebugVertices, originalMesh->mNumVertices);
            for (unsigned int i = 0; i < debugVertexCount; i++) {
                std::cout << "\nVertex " << i << ":" << std::endl;

                // Original data
                aiVector3D origPos = originalMesh->mVertices[i];
                aiVector3D origUV = originalMesh->mTextureCoords[0][i];
                std::cout << "Original:" << std::endl;
                std::cout << "  Position: (" << origPos.x << ", " << origPos.y << ", " << origPos.z << ")" << std::endl;
                std::cout << "  UV: (" << origUV.x << ", " << origUV.y << ")" << std::endl;

                // Processed data
                const glm::vec3& procPos = processedMesh->positions[i];
                const glm::vec2& procUV = processedMesh->uvSets["map1"][i];
                std::cout << "Processed:" << std::endl;
                std::cout << "  Position: (" << procPos.x << ", " << procPos.y << ", " << procPos.z << ")" << std::endl;
                std::cout << "  UV: (" << procUV.x << ", " << procUV.y << ")" << std::endl;
            }
        }

        // Debug face/index conversion
        std::cout << "\nFace/Index Verification:" << std::endl;
        std::cout << "Original faces: " << originalMesh->mNumFaces << std::endl;
        std::cout << "Processed indices: " << processedMesh->indices.size() / 3 << " triangles" << std::endl;

        // Print first few triangles for verification
        const unsigned int maxDebugFaces = 40;
        const unsigned int debugFaceCount = std::min(maxDebugFaces, originalMesh->mNumFaces);
        for (unsigned int i = 0; i < debugFaceCount; i++) {
            const aiFace& origFace = originalMesh->mFaces[i];
            std::cout << "\nTriangle " << i << ":" << std::endl;

            // Original indices
            std::cout << "Original indices: [";
            for (unsigned int j = 0; j < origFace.mNumIndices; j++) {
                std::cout << origFace.mIndices[j];
                if (j < origFace.mNumIndices - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;

            // Processed indices
            std::cout << "Processed indices: [";
            for (unsigned int j = 0; j < 3; j++) {
                std::cout << processedMesh->indices[i * 3 + j];
                if (j < 2) std::cout << ", ";
            }
            std::cout << "]" << std::endl;

            // Print corresponding UV coordinates for this face
            if (originalMesh->HasTextureCoords(0)) {
                std::cout << "UV coordinates for this triangle:" << std::endl;
                for (unsigned int j = 0; j < 3; j++) {
                    unsigned int vertexIndex = processedMesh->indices[i * 3 + j];
                    const glm::vec2& uv = processedMesh->uvSets["map1"][vertexIndex];
                    std::cout << "  v" << j << ": (" << uv.x << ", " << uv.y << ")" << std::endl;
                }
            }
        }

        // Material verification
        if (originalMesh->mMaterialIndex >= 0) {
            aiMaterial* origMaterial = scene->mMaterials[originalMesh->mMaterialIndex];
            std::cout << "\nMaterial Verification:" << std::endl;
            std::cout << "Original material name: " << origMaterial->GetName().C_Str() << std::endl;

            if (!processedMesh->materials.empty()) {
                auto& procMaterial = processedMesh->materials[0];
                std::cout << "Processed material:" << std::endl;
                std::cout << "Base Color: ("
                    << procMaterial->baseColor.r << ", "
                    << procMaterial->baseColor.g << ", "
                    << procMaterial->baseColor.b << ")" << std::endl;

                // Debug texture assignments
                std::cout << "\nTexture assignments:" << std::endl;
                for (const auto& [mapType, texMap] : procMaterial->textureMaps) {
                    std::cout << mapType << " texture ID: " << texMap.textureId << std::endl;
                    std::cout << "  UV Set: " << texMap.uvSet << std::endl;
                    std::cout << "  Offset: (" << texMap.offset.x << ", " << texMap.offset.y << ")" << std::endl;
                    std::cout << "  Tiling: (" << texMap.tiling.x << ", " << texMap.tiling.y << ")" << std::endl;
                }
            }
        }

        std::cout << "\n=== End of Mesh Processing Debug Output ===" << std::endl;
    }

    std::shared_ptr<Material> processMaterial(aiMaterial* material, const aiScene* scene) {
        if (!material) {
            std::cout << "Warning: Null material provided" << std::endl;
            return std::make_shared<Material>();
        }

        std::cout << "\n=== Processing Material ===" << std::endl;
        std::cout << "Material name: " << material->GetName().C_Str() << std::endl;

        // Debug code to print all material properties
        aiString name;
        material->Get(AI_MATKEY_NAME, name);
        std::cout << "Material name: " << name.C_Str() << std::endl;

        for (unsigned int i = 0; i < material->mNumProperties; ++i) {
            aiMaterialProperty* prop = material->mProperties[i];
            std::cout << "Property: " << prop->mKey.C_Str() << std::endl;
        }


        auto newMaterial = std::make_shared<Material>();

        // Process base color
        aiColor4D baseColor(1.0f);
        if (material->Get(AI_MATKEY_BASE_COLOR, baseColor) == AI_SUCCESS) {
            std::cout << "Found base color via AI_MATKEY_BASE_COLOR" << std::endl;
            newMaterial->baseColor = glm::vec3(baseColor.r, baseColor.g, baseColor.b);
        }
        else if (material->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor) == AI_SUCCESS) {
            std::cout << "Found base color via AI_MATKEY_COLOR_DIFFUSE" << std::endl;
            newMaterial->baseColor = glm::vec3(baseColor.r, baseColor.g, baseColor.b);
        }

        // Process PBR properties
        float metallicFactor = 0.0f;
        if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor) == AI_SUCCESS) {
            std::cout << "Found metallic factor via AI_MATKEY_METALLIC_FACTOR" << std::endl;
            newMaterial->metallic = metallicFactor;
        }

        float roughnessFactor = 0.5f;
        if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor) == AI_SUCCESS) {
            std::cout << "Found roughness factor via AI_MATKEY_ROUGHNESS_FACTOR" << std::endl;
            newMaterial->roughness = roughnessFactor;
        }

        // Process transmission
        float transmission = 0.0f;
        if (material->Get(AI_MATKEY_TRANSMISSION_FACTOR, transmission) == AI_SUCCESS) {
            std::cout << "Found transmission factor via AI_MATKEY_TRANSMISSION_FACTOR" << std::endl;
            newMaterial->transmission = transmission;
        }

        // Process alpha/transparency 
        float opacity = 1.0f;
        if (material->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS) {
            std::cout << "Found opacity via AI_MATKEY_OPACITY" << std::endl;
            newMaterial->alpha = opacity;
        }

        // Process emission
        aiColor3D emission(0.0f);
        if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emission) == AI_SUCCESS) {
            std::cout << "Found emission color via AI_MATKEY_COLOR_EMISSIVE" << std::endl;
            newMaterial->emission = glm::vec3(emission.r, emission.g, emission.b);
        }

        float emissionStrength = 0.0f;
        if (material->Get(AI_MATKEY_EMISSIVE_INTENSITY, emissionStrength) == AI_SUCCESS) {
            std::cout << "Found emissive intensity via AI_MATKEY_EMISSIVE_INTENSITY" << std::endl;
            newMaterial->emissionStrength = emissionStrength;
        }

        // Process IOR
        float ior = 1.45f;
        if (material->Get(AI_MATKEY_REFRACTI, ior) == AI_SUCCESS) {
            std::cout << "Found IOR via AI_MATKEY_REFRACTI" << std::endl;
            newMaterial->ior = ior;
        }

        // Define texture mappings with their properties
        struct TextureMapping {
            aiTextureType aiType;
            std::string engineType;
            bool sRGB;
            Material::TextureMap::ColorSpace defaultColorSpace;
            Material::TextureMap::Interpolation defaultInterpolation;
            Material::TextureMap::Extension defaultExtension;
            Material::TextureMap::AlphaMode defaultAlphaMode;
            Material::TextureMap::Projection defaultProjection;
        };

        std::vector<TextureMapping> textureMappings = {
            // Base Color / Albedo
            {aiTextureType_BASE_COLOR, "baseColor", true,
             Material::TextureMap::ColorSpace::sRGB,
             Material::TextureMap::Interpolation::Linear,
             Material::TextureMap::Extension::Repeat,
             Material::TextureMap::AlphaMode::Straight,
             Material::TextureMap::Projection::Flat},
            
            // Handle both GLTF and traditional normal maps
            {aiTextureType_NORMALS, "normal", false,
             Material::TextureMap::ColorSpace::Linear,
             Material::TextureMap::Interpolation::Linear,
             Material::TextureMap::Extension::Repeat,
             Material::TextureMap::AlphaMode::Straight,
             Material::TextureMap::Projection::Flat},
            
            {aiTextureType_NORMAL_CAMERA, "normal", false,
             Material::TextureMap::ColorSpace::Linear,
             Material::TextureMap::Interpolation::Linear,
             Material::TextureMap::Extension::Repeat,
             Material::TextureMap::AlphaMode::Straight,
             Material::TextureMap::Projection::Flat},
            
            // Metallic maps (handle both GLTF and traditional)
            // {aiTextureType_METALNESS, "metallic", false,
            //  Material::TextureMap::ColorSpace::Linear,
            //  Material::TextureMap::Interpolation::Linear,
            //  Material::TextureMap::Extension::Repeat,
            //  Material::TextureMap::AlphaMode::Straight,
            //  Material::TextureMap::Projection::Flat},
            
            // Roughness maps (handle both GLTF and traditional)
            {aiTextureType_DIFFUSE_ROUGHNESS, "roughness", false,
             Material::TextureMap::ColorSpace::Linear,
             Material::TextureMap::Interpolation::Linear,
             Material::TextureMap::Extension::Repeat,
             Material::TextureMap::AlphaMode::Straight,
             Material::TextureMap::Projection::Flat},
            
            // Emission maps
            {aiTextureType_EMISSIVE, "emission", true,
             Material::TextureMap::ColorSpace::sRGB,
             Material::TextureMap::Interpolation::Linear,
             Material::TextureMap::Extension::Repeat,
             Material::TextureMap::AlphaMode::Straight,
             Material::TextureMap::Projection::Flat},
            
            // Ambient Occlusion
            {aiTextureType_AMBIENT_OCCLUSION, "occlusion", false,
             Material::TextureMap::ColorSpace::Linear,
             Material::TextureMap::Interpolation::Linear,
             Material::TextureMap::Extension::Repeat,
             Material::TextureMap::AlphaMode::Straight,
             Material::TextureMap::Projection::Flat},
        };

        // Process textures
        for (const auto& mapping : textureMappings) {
            aiString texPath;
            if (material->GetTexture(mapping.aiType, 0, &texPath) == AI_SUCCESS) {
                Material::TextureMap texMap;
                texMap.colorSpace = mapping.defaultColorSpace;
                texMap.interpolation = mapping.defaultInterpolation;
                texMap.extension = mapping.defaultExtension;
                texMap.alphaMode = mapping.defaultAlphaMode;
                texMap.projection = mapping.defaultProjection;

                // Process texture properties from material
                int mapMode;
                if (material->Get(AI_MATKEY_MAPPINGMODE_U(mapping.aiType, 0), mapMode) == AI_SUCCESS) {
                    switch (mapMode) {
                    case aiTextureMapMode_Wrap:
                        texMap.extension = Material::TextureMap::Extension::Repeat;
                        break;
                    case aiTextureMapMode_Clamp:
                        texMap.extension = Material::TextureMap::Extension::Extend;
                        break;
                    case aiTextureMapMode_Decal:
                        texMap.extension = Material::TextureMap::Extension::Clip;
                        break;
                    }
                }

                // Process UV transforms
                aiUVTransform transform;
                if (material->Get(AI_MATKEY_UVTRANSFORM(mapping.aiType, 0), transform) == AI_SUCCESS) {
                    texMap.offset = glm::vec2(transform.mTranslation.x, transform.mTranslation.y);
                    texMap.tiling = glm::vec2(transform.mScaling.x, transform.mScaling.y);
                }
                else {
                    texMap.offset = glm::vec2(0.0f);
                    texMap.tiling = glm::vec2(1.0f);
                }

                // Handle embedded textures
                const aiTexture* embeddedTex = scene->GetEmbeddedTexture(texPath.C_Str());
                if (embeddedTex) {
                    std::cout << "Processing embedded texture for " << mapping.engineType << std::endl;
                    texMap.textureId = TextureManager::getInstance().LoadFromMemory(
                        reinterpret_cast<unsigned char*>(embeddedTex->pcData),
                        embeddedTex->mWidth * (embeddedTex->mHeight == 0 ? 1 : embeddedTex->mHeight),
                        embeddedTex->achFormatHint,
                        texMap
                    );
                }
                else {
                    // Construct full path for external texture
                    std::filesystem::path fullPath = std::filesystem::path(modelDirectory) / texPath.C_Str();
                    std::cout << "Loading external texture: " << fullPath.string() << " for " << mapping.engineType << std::endl;

                    texMap.textureId = TextureManager::getInstance().LoadTexture(
                        fullPath.string(),
                        mapping.engineType,
                        texMap
                    );
                }

                // Store the texture map
                newMaterial->textureMaps[mapping.engineType] = texMap;
            }
        }

#ifdef _DEBUG
        // Debug output
        std::cout << "\nProcessed material properties:" << std::endl;
        std::cout << "Base Color: " << vec3_to_string(newMaterial->baseColor) << std::endl;
        std::cout << "Metallic: " << newMaterial->metallic << std::endl;
        std::cout << "Roughness: " << newMaterial->roughness << std::endl;
        std::cout << "IOR: " << newMaterial->ior << std::endl;
        std::cout << "Alpha: " << newMaterial->alpha << std::endl;

        if (!newMaterial->textureMaps.empty()) {
            std::cout << "\nLoaded textures:" << std::endl;
		}

        for (const auto& [type, texMap] : newMaterial->textureMaps) {
            std::cout << "\nTexture type: " << type << std::endl;
            std::cout << "Texture ID: " << texMap.textureId << std::endl;
            std::cout << "Color Space: " << static_cast<int>(texMap.colorSpace) << std::endl;
            std::cout << "Interpolation: " << static_cast<int>(texMap.interpolation) << std::endl;
            std::cout << "Extension: " << static_cast<int>(texMap.extension) << std::endl;
            std::cout << "Projection: " << static_cast<int>(texMap.projection) << std::endl;
            std::cout << "Alpha Mode: " << static_cast<int>(texMap.alphaMode) << std::endl;
            std::cout << "UV Offset: " << texMap.offset.x << ", " << texMap.offset.y << std::endl;
            std::cout << "UV Tiling: " << texMap.tiling.x << ", " << texMap.tiling.y << std::endl;

            // Verify texture parameters in OpenGL
            GLint wrap_s, wrap_t, min_filter, mag_filter;
            glBindTexture(GL_TEXTURE_2D, texMap.textureId);
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrap_s);
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &wrap_t);
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min_filter);
            glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &mag_filter);

            std::cout << "GL Parameters:" << std::endl;
            std::cout << "  Wrap S: " << wrap_s << std::endl;
            std::cout << "  Wrap T: " << wrap_t << std::endl;
            std::cout << "  Min Filter: " << min_filter << std::endl;
            std::cout << "  Mag Filter: " << mag_filter << std::endl;
        }


#endif

        return newMaterial;
    }

    std::shared_ptr<Mesh> processMesh(aiMesh* mesh, const aiScene* scene) {
        std::cout << "\n --- Calling processMesh --- \n";

        auto newMesh = std::make_shared<Mesh>(false);

        // Check if the scene has animations
        if (scene->HasAnimations()) {
            std::cout << "\n --- Scene hasAnimations = true \n";
            auto animatedMesh = std::make_shared<AnimatedMesh>();

            // Process bone data if present
            if (mesh->HasBones()) {
                std::map<std::string, int> boneMapping;
                animatedMesh->boneData.resize(mesh->mNumVertices);

                for (unsigned int i = 0; i < mesh->mNumBones; i++) {
                    const aiBone* bone = mesh->mBones[i];
                    std::string boneName = bone->mName.data;
                    int boneIndex;

                    if (boneMapping.find(boneName) == boneMapping.end()) {
                        boneIndex = animatedMesh->armature.bones.size();
                        boneMapping[boneName] = boneIndex;

                        glm::mat4 offsetMatrix = aiToGlmMatrix(bone->mOffsetMatrix);
                        animatedMesh->armature.addBone(boneName, offsetMatrix);
                    }
                    else {
                        boneIndex = boneMapping[boneName];
                    }

                    for (unsigned int j = 0; j < bone->mNumWeights; j++) {
                        const aiVertexWeight& weight = bone->mWeights[j];
                        animatedMesh->boneData[weight.mVertexId].addBoneInfluence(boneIndex, weight.mWeight);
                    }
                }
            }

            newMesh = animatedMesh;
        }
        else {
            newMesh = std::make_shared<Mesh>(false);
        }

        // Track unique vertex-UV combinations
        struct VertexData {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 uv;

            bool operator==(const VertexData& other) const {
                // Define what makes vertices unique - include UV coordinates
                return position == other.position &&
                    normal == other.normal &&
                    uv == other.uv;
            }
        };
        std::vector<VertexData> vertices;
        std::vector<unsigned int> indices;

        // Process each face first
        for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
            const aiFace& face = mesh->mFaces[i];
            for (unsigned int j = 0; j < face.mNumIndices; j++) {
                unsigned int vertexIndex = face.mIndices[j];

                VertexData vertex;
                // Position
                const aiVector3D& pos = mesh->mVertices[vertexIndex];
                vertex.position = glm::vec3(pos.x, pos.y, pos.z);

                // Normal
                if (mesh->HasNormals()) {
                    const aiVector3D& normal = mesh->mNormals[vertexIndex];
                    vertex.normal = glm::vec3(normal.x, normal.y, normal.z);
                }

                // UV coordinates
                if (mesh->HasTextureCoords(0)) {
                    const aiVector3D& uv = mesh->mTextureCoords[0][vertexIndex];
                    vertex.uv = glm::vec2(uv.x, uv.y);
                }

                // Find if this exact vertex (position + normal + UV) exists
                auto it = std::find(vertices.begin(), vertices.end(), vertex);
                if (it == vertices.end()) {
                    // New unique vertex
                    indices.push_back(vertices.size());
                    vertices.push_back(vertex);
                }
                else {
                    // Reuse existing vertex
                    indices.push_back(std::distance(vertices.begin(), it));
                }
            }
        }

        // Now populate the mesh data
        newMesh->positions.reserve(vertices.size());
        newMesh->normals.reserve(vertices.size());
        newMesh->uvSets["map1"].reserve(vertices.size());

        for (const auto& vertex : vertices) {
            newMesh->positions.push_back(vertex.position);
            newMesh->normals.push_back(vertex.normal);
            newMesh->uvSets["map1"].push_back(vertex.uv);
            newMesh->colors.push_back(glm::vec4(1.0f)); // Default white color
        }

        newMesh->indices = indices;

        // Process material
        if (mesh->mMaterialIndex >= 0) {
            newMesh->materials.push_back(processMaterial(scene->mMaterials[mesh->mMaterialIndex], scene));
        }

        newMesh->setupBuffers();
        //debugMesh(mesh, scene, newMesh);

        return newMesh;
    }

    void processNode(aiNode* node, const aiScene* scene, std::shared_ptr<Node> engineNode) {
        // Process transforms
        std::cout << "\n --- Calling processNode --- \n";
        glm::mat4 transform = aiToGlmMatrix(node->mTransformation);
        engineNode->localTranslation = glm::vec3(transform[3]);
        engineNode->localRotation = glm::quat(transform);
        engineNode->localScale = glm::vec3(1.0f);

        // Process meshes
        if (node->mNumMeshes > 0) {
            engineNode->mesh = processMesh(scene->mMeshes[node->mMeshes[0]], scene);
        }

        // If this node has animations, process them
        if (scene->HasAnimations()) {
            std::cout << "scene has animations" << std::endl;
            auto animatedMesh = std::dynamic_pointer_cast<AnimatedMesh>(engineNode->mesh);
            if (animatedMesh) {
                // Process all animations in the scene
                for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
                    const aiAnimation* anim = scene->mAnimations[i];
                    Action action;
                    action.name = anim->mName.C_Str();
                    action.duration = static_cast<float>(anim->mDuration / anim->mTicksPerSecond);

                    // Find animation channels targeting this node
                    for (unsigned int j = 0; j < anim->mNumChannels; j++) {
                        const aiNodeAnim* nodeAnim = anim->mChannels[j];
                        if (std::string(nodeAnim->mNodeName.C_Str()) == engineNode->name) {
                            AnimationChannel channel;
                            channel.targetProperty = engineNode->name;

                            // Process position keyframes
                            for (unsigned int k = 0; k < nodeAnim->mNumPositionKeys; k++) {
                                const auto& key = nodeAnim->mPositionKeys[k];
                                Keyframe keyframe;
                                keyframe.time = static_cast<float>(key.mTime / anim->mTicksPerSecond);
                                keyframe.position = glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z);
                                channel.addKeyframe(keyframe);
                            }

                            // Process rotation keyframes
                            for (unsigned int k = 0; k < nodeAnim->mNumRotationKeys; k++) {
                                const auto& key = nodeAnim->mRotationKeys[k];
                                Keyframe keyframe;
                                keyframe.time = static_cast<float>(key.mTime / anim->mTicksPerSecond);
                                keyframe.rotation = glm::quat(key.mValue.w, key.mValue.x, key.mValue.y, key.mValue.z);
                                channel.addKeyframe(keyframe);
                            }

                            // Process scaling keyframes
                            for (unsigned int k = 0; k < nodeAnim->mNumScalingKeys; k++) {
                                const auto& key = nodeAnim->mScalingKeys[k];
                                Keyframe keyframe;
                                keyframe.time = static_cast<float>(key.mTime / anim->mTicksPerSecond);
                                keyframe.scale = glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z);
                                channel.addKeyframe(keyframe);
                            }

                            action.channels.push_back(channel);
                        }
                    }

                    if (!action.channels.empty()) {
                        animatedMesh->actions.push_back(action);
                    }
                }

                // Initialize armature if we have bone animations
                if (!animatedMesh->armature.bones.empty()) {
                    animatedMesh->armature.initialize(scene->mRootNode);
                }
            }
            engineNode->mesh = animatedMesh;
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
            aiProcess_SortByPType |
            aiProcess_PreTransformVertices;

        const aiScene* scene = importer.ReadFile(path, flags);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            lastError = importer.GetErrorString();
            return nullptr;
        }

        auto rootNode = std::make_shared<Node>();
        rootNode->name = "GLB_Root: " + std::string(scene->mRootNode->mName.C_Str());
        processNode(scene->mRootNode, scene, rootNode);
        rootNode->updateWorldTransform();
        return rootNode;
    }

    const std::string& getLastError() const { return lastError; }
};
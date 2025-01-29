#pragma once
#include "GameEngine.h"
#include "misc_funcs.h"

// Forward declarations
class Node;
class Mesh;
class Material;


// Material definition (matches FBX material)
class Material {
public:

    std::string name;

    // Base Properties for Principled BSDF
    glm::vec3 baseColor;
    float subsurface;
    glm::vec3 subsurfaceRadius;
    glm::vec3 subsurfaceColor;
    float subsurfaceIOR;
    float subsurfaceAnisotropy;
    float metallic;
    float specular;
    float specularTint;
    float roughness;
    float anisotropic;
    float anisotropicRotation;
    float sheen;
    float sheenTint;
    float clearcoat;
    float clearcoatRoughness;
    float ior;
    float transmission;
    float transmissionRoughness;
    glm::vec3 emission;
    float emissionStrength;
    float alpha;

    // Texture maps (like FBX's texture properties)
    struct TextureMap {
        GLuint textureId;
        std::string uvSet;  // Which UV set to use
        glm::vec2 offset;
        glm::vec2 tiling;
        float strength;

        enum class Interpolation {
            Linear,
            Closest,
            Cubic
        } interpolation = Interpolation::Linear;

        enum class Projection {
            Flat,
            Box,
            Sphere,
            Tube
        } projection = Projection::Flat;

        enum class Extension {
            Repeat,
            Extend,
            Clip
        } extension = Extension::Repeat;

        enum class ColorSpace {
            sRGB,
            Linear,
            NonColor
        } colorSpace = ColorSpace::sRGB;

        enum class AlphaMode {
            Straight,
            Premultiplied
        } alphaMode = AlphaMode::Straight;
    };

    std::map<std::string, TextureMap> textureMaps; // "baseColor", "normal", "metallic", etc.

    // Extended properties (matches FBX material properties)
    std::map<std::string, float> numericalProperties;
    std::map<std::string, std::string> stringProperties;

    Material() :
        baseColor(1.0f),
        subsurface(0.0f),
        subsurfaceRadius(1.0f),
        subsurfaceColor(1.0f),
        subsurfaceIOR(1.4f),
        subsurfaceAnisotropy(0.0f),
        metallic(0.0f),
        specular(0.5f),
        specularTint(0.0f),
        roughness(0.5f),
        anisotropic(0.0f),
        anisotropicRotation(0.0f),
        sheen(0.0f),
        sheenTint(0.0f),
        clearcoat(0.0f),
        clearcoatRoughness(0.0f),
        ior(1.45f),
        transmission(0.0f),
        transmissionRoughness(0.0f),
        emission(0.0f),
        emissionStrength(0.0f),
        alpha(1.0f) {}

    void bind(GLuint shaderProgram) {
        // Bind PBR base properties
        glUniform3fv(glGetUniformLocation(shaderProgram, "material.baseColor"), 1, glm::value_ptr(baseColor));
        glUniform1f(glGetUniformLocation(shaderProgram, "material.subsurface"), subsurface);
        glUniform3fv(glGetUniformLocation(shaderProgram, "material.subsurfaceRadius"), 1, glm::value_ptr(subsurfaceRadius));
        glUniform3fv(glGetUniformLocation(shaderProgram, "material.subsurfaceColor"), 1, glm::value_ptr(subsurfaceColor));
        glUniform1f(glGetUniformLocation(shaderProgram, "material.subsurfaceIOR"), subsurfaceIOR);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.subsurfaceAnisotropy"), subsurfaceAnisotropy);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.metallic"), metallic);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.specular"), specular);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.specularTint"), specularTint);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.roughness"), roughness);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.anisotropic"), anisotropic);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.anisotropicRotation"), anisotropicRotation);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.sheen"), sheen);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.sheenTint"), sheenTint);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.clearcoat"), clearcoat);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.clearcoatRoughness"), clearcoatRoughness);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.ior"), ior);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.transmission"), transmission);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.transmissionRoughness"), transmissionRoughness);
        glUniform3fv(glGetUniformLocation(shaderProgram, "material.emission"), 1, glm::value_ptr(emission));
        glUniform1f(glGetUniformLocation(shaderProgram, "material.emissionStrength"), emissionStrength);
        glUniform1f(glGetUniformLocation(shaderProgram, "material.alpha"), alpha);

        // Define explicit texture units and their properties
        const struct TextureUnitMapping {
            std::string name;
            int unit;
            std::string uniformName;
            std::string projectionUniform;
            std::string offsetUniform;
            std::string tilingUniform;
        } textureUnits[] = {
            {"baseColor", 0, "material.baseColorMap", "material.baseColorProjection", "material.baseColorOffset", "material.baseColorTiling"},
            {"normal", 1, "material.normalMap", "material.normalProjection", "material.normalOffset", "material.normalTiling"},
            {"metallic", 2, "material.metallicMap", "material.metallicProjection", "material.metallicOffset", "material.metallicTiling"},
            {"roughness", 3, "material.roughnessMap", "material.roughnessProjection", "material.roughnessOffset", "material.roughnessTiling"},
            {"emission", 4, "material.emissionMap", "material.emissionProjection", "material.emissionOffset", "material.emissionTiling"},
            {"occlusion", 5, "material.occlusionMap", "material.occlusionProjection", "material.occlusionOffset", "material.occlusionTiling"},
            {"specular", 6, "material.specularMap", "material.specularProjection", "material.specularOffset", "material.specularTiling"},
            {"transmission", 7, "material.transmissionMap", "material.transmissionProjection", "material.transmissionOffset", "material.transmissionTiling"}
        };

        // Track bound textures for presence flags
        std::unordered_map<std::string, bool> boundTextures;

        // Save current OpenGL state
        GLint lastActiveTexture;
        glGetIntegerv(GL_ACTIVE_TEXTURE, &lastActiveTexture);

        // Bind textures and their properties
        for (const auto& mapping : textureUnits) {
            auto it = textureMaps.find(mapping.name);
            if (it != textureMaps.end()) {
                // Activate texture unit and bind texture
                glActiveTexture(GL_TEXTURE0 + mapping.unit);
                glBindTexture(GL_TEXTURE_2D, it->second.textureId);
                std::cout << "binding texture unit " << mapping.unit << " to id " << it->second.textureId << std::endl;

                // Set texture uniform
                glUniform1i(glGetUniformLocation(shaderProgram, mapping.uniformName.c_str()), mapping.unit);

                // Set projection mode
                glUniform1i(glGetUniformLocation(shaderProgram, mapping.projectionUniform.c_str()),
                    static_cast<int>(it->second.projection));

                // Set UV transform uniforms
                glUniform2fv(glGetUniformLocation(shaderProgram, mapping.offsetUniform.c_str()), 1,
                    glm::value_ptr(it->second.offset));
                glUniform2fv(glGetUniformLocation(shaderProgram, mapping.tilingUniform.c_str()), 1,
                    glm::value_ptr(it->second.tiling));

                // Track that we bound this texture
                boundTextures[mapping.name] = true;

#ifdef _DEBUG
                // Verify texture binding
                GLint currentTexture;
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
                if (currentTexture != it->second.textureId) {
                    std::cout << "Warning: Texture binding mismatch for " << mapping.name << std::endl;
                    std::cout << "Expected: " << it->second.textureId << ", Got: " << currentTexture << std::endl;
                }

                // Check for OpenGL errors
                GLenum err = glGetError();
                if (err != GL_NO_ERROR) {
                    std::cout << "GL Error binding " << mapping.name << " texture: " << err << std::endl;
                }
#endif
            }
            else {
                boundTextures[mapping.name] = false;
            }
        }

        // Set texture presence flags
        glUniform1i(glGetUniformLocation(shaderProgram, "material.hasBaseColorMap"), boundTextures["baseColor"]);
        glUniform1i(glGetUniformLocation(shaderProgram, "material.hasNormalMap"), boundTextures["normal"]);
        glUniform1i(glGetUniformLocation(shaderProgram, "material.hasMetallicMap"), boundTextures["metallic"]);
        glUniform1i(glGetUniformLocation(shaderProgram, "material.hasRoughnessMap"), boundTextures["roughness"]);
        glUniform1i(glGetUniformLocation(shaderProgram, "material.hasEmissionMap"), boundTextures["emission"]);
        glUniform1i(glGetUniformLocation(shaderProgram, "material.hasOcclusionMap"), boundTextures["occlusion"]);
        glUniform1i(glGetUniformLocation(shaderProgram, "material.hasSpecularMap"), boundTextures["specular"]);
        glUniform1i(glGetUniformLocation(shaderProgram, "material.hasTransmissionMap"), boundTextures["transmission"]);

        // Restore previous active texture
        glActiveTexture(lastActiveTexture);

#ifdef _DEBUG
        // Debug verification of final state
        for (const auto& mapping : textureUnits) {
            if (boundTextures[mapping.name]) {
                glActiveTexture(GL_TEXTURE0 + mapping.unit);
                GLint currentTexture;
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTexture);
                if (currentTexture != textureMaps[mapping.name].textureId) {
                    std::cout << "Final state mismatch for " << mapping.name << std::endl;
                }

                // Verify uniform values
                GLint location = glGetUniformLocation(shaderProgram, mapping.uniformName.c_str());
                GLint value;
                glGetUniformiv(shaderProgram, location, &value);
                if (value != mapping.unit) {
                    std::cout << "Uniform value mismatch for " << mapping.name << std::endl;
                }
            }
        }
#endif

    }

    void debug() {
        std::cout << "-- Material Debug --" << std::endl;
        std::cout << "Name: " << name << std::endl;
        std::cout << "baseColor = " << vec3_to_string(baseColor) << std::endl;
        std::cout << "subsurface = " << subsurface << std::endl;
        std::cout << "subsurfaceRadius = " << vec3_to_string(subsurfaceRadius) << std::endl;
        std::cout << "subsurfaceColor = " << vec3_to_string(subsurfaceColor) << std::endl;
        std::cout << "subsurfaceIOR = " << subsurfaceIOR << std::endl;
        std::cout << "subsurfaceAnisotropy = " << subsurfaceAnisotropy << std::endl;
        std::cout << "metallic = " << metallic << std::endl;
        std::cout << "specular = " << specular << std::endl;
        std::cout << "specularTint = " << specularTint << std::endl;
        std::cout << "roughness = " << roughness << std::endl;
        std::cout << "anisotropic = " << anisotropic << std::endl;
        std::cout << "anisotropicRotation = " << anisotropicRotation << std::endl;
        std::cout << "sheen = " << sheen << std::endl;
        std::cout << "sheenTint = " << sheenTint << std::endl;
        std::cout << "clearcoat = " << clearcoat << std::endl;
        std::cout << "clearcoatRoughness = " << clearcoatRoughness << std::endl;
        std::cout << "ior = " << ior << std::endl;
        std::cout << "transmission = " << transmission << std::endl;
        std::cout << "transmissionRoughness = " << transmissionRoughness << std::endl;
        std::cout << "emission = " << vec3_to_string(emission) << std::endl;
        std::cout << "emissionStrength = " << emissionStrength << std::endl;
        std::cout << "alpha = " << alpha << std::endl;;
    }
};

// Geometry data (matches FBX mesh attribute)
class Mesh {
public:
    // Vertex data
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> tangents;

    std::vector<glm::vec4> colors;  // Vertex colors (RGBA)
    std::map<std::string, std::vector<glm::vec2>> uvSets;  // Named UV sets
    std::vector<unsigned int> indices;

    // Material assignments
    std::vector<int> materialIds;  // Per-polygon material assignment
    std::vector<std::shared_ptr<Material>> materials;  // Material slots

    // OpenGL buffers
    GLuint VAO, VBO, EBO;

    Mesh(bool useDefaultMaterial=true) : VAO(0), VBO(0), EBO(0) {
        // Default UV set
        uvSets["map1"] = std::vector<glm::vec2>();

        // use default material
        if (useDefaultMaterial) {
            materials.push_back(std::make_shared<Material>());
        }
    }

    Mesh(std::shared_ptr<Material> material) : VAO(0), VBO(0), EBO(0) {
        // Default UV set
        uvSets["map1"] = std::vector<glm::vec2>();
        materials.push_back(material);
    }

    Mesh(std::vector<std::shared_ptr<Material>> vecMaterials) : VAO(0), VBO(0), EBO(0) {
        // Default UV set
        uvSets["map1"] = std::vector<glm::vec2>();
        materials = vecMaterials;
    }

    void setupBuffers() {
        std::cout << "Setting up mesh buffers..." << std::endl;
        std::cout << "Positions: " << positions.size() << std::endl;
        std::cout << "Normals: " << normals.size() << std::endl;
        std::cout << "UV coords: " << uvSets["map1"].size() << std::endl;
        std::cout << "Indices: " << indices.size() << std::endl;

        // Calculate tangents if we have UV coordinates
        if (!uvSets["map1"].empty()) {
            calculateTangents();
        }

        // Generate buffers
        if (VAO == 0) glGenVertexArrays(1, &VAO);
        if (VBO == 0) glGenBuffers(1, &VBO);
        if (EBO == 0) glGenBuffers(1, &EBO);

        glBindVertexArray(VAO);

        // Calculate stride and offsets
        size_t stride = sizeof(glm::vec3);  // Position
        if (!normals.empty()) stride += sizeof(glm::vec3);
        if (!colors.empty()) stride += sizeof(glm::vec4);
        if (!uvSets["map1"].empty()) stride += sizeof(glm::vec2);
        if (!tangents.empty()) stride += sizeof(glm::vec3);

        // Allocate buffer
        size_t totalSize = positions.size() * stride;
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, totalSize, nullptr, GL_STATIC_DRAW);

        // Upload interleaved data
        size_t offset = 0;
        size_t currentOffset = 0;

        // Positions
        for (size_t i = 0; i < positions.size(); i++) {
            glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sizeof(glm::vec3), &positions[i]);
            currentOffset += stride;
        }
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
        glEnableVertexAttribArray(0);
        offset += sizeof(glm::vec3);

        // Normals
        if (!normals.empty()) {
            currentOffset = offset;
            for (size_t i = 0; i < normals.size(); i++) {
                glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sizeof(glm::vec3), &normals[i]);
                currentOffset += stride;
            }
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
            glEnableVertexAttribArray(1);
            offset += sizeof(glm::vec3);
        }

        // Colors
        if (!colors.empty()) {
            currentOffset = offset;
            for (size_t i = 0; i < colors.size(); i++) {
                glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sizeof(glm::vec4), &colors[i]);
                currentOffset += stride;
            }
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)offset);
            glEnableVertexAttribArray(2);
            offset += sizeof(glm::vec4);
        }

        // UVs
        if (!uvSets["map1"].empty()) {
            currentOffset = offset;
            for (size_t i = 0; i < uvSets["map1"].size(); i++) {
                glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sizeof(glm::vec2), &uvSets["map1"][i]);
                currentOffset += stride;
            }
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (void*)offset);
            glEnableVertexAttribArray(3);
            offset += sizeof(glm::vec2);
        }

        // Tangents
        if (!tangents.empty()) {
            currentOffset = offset;
            for (size_t i = 0; i < tangents.size(); i++) {
                glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sizeof(glm::vec3), &tangents[i]);
                currentOffset += stride;
            }
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset);
            glEnableVertexAttribArray(4);
        }

        // Set up index buffer
        if (!indices.empty()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                indices.data(), GL_STATIC_DRAW);
        }

        glBindVertexArray(0);
    }

    void calculateTangents() {
        tangents.resize(positions.size(), glm::vec3(0.0f));
        std::vector<glm::vec3> bitangents(positions.size(), glm::vec3(0.0f));

        // Process each triangle
        for (size_t i = 0; i < indices.size(); i += 3) {
            unsigned int i0 = indices[i];
            unsigned int i1 = indices[i + 1];
            unsigned int i2 = indices[i + 2];

            const glm::vec3& v0 = positions[i0];
            const glm::vec3& v1 = positions[i1];
            const glm::vec3& v2 = positions[i2];

            const glm::vec2& uv0 = uvSets["map1"][i0];
            const glm::vec2& uv1 = uvSets["map1"][i1];
            const glm::vec2& uv2 = uvSets["map1"][i2];

            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
            f = std::isfinite(f) ? f : 0.0f;  // Handle degenerate UV case

            glm::vec3 tangent;
            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

            // Keep tangent consistent with vertex winding order
            if (glm::dot(glm::cross(edge1, edge2), tangent) < 0.0f) {
                tangent = -tangent;
            }

            // Accumulate tangents (will normalize after)
            tangents[i0] += tangent;
            tangents[i1] += tangent;
            tangents[i2] += tangent;
        }

        // Normalize all tangents
        for (auto& tangent : tangents) {
            tangent = glm::length(tangent) > 0.0f ? glm::normalize(tangent) : glm::vec3(1.0f, 0.0f, 0.0f);
        }
    }

    void draw(GLuint shaderProgram) {
        // Debug vertex buffer state before drawing
        // std::cout << "Drawing mesh with:" << std::endl;
        // std::cout << "Positions: " << positions.size() << std::endl;
        // std::cout << "Normals: " << normals.size() << std::endl;
        // std::cout << "Colors: " << colors.size() << std::endl;
        // std::cout << "UVs: " << uvSets["map1"].size() << std::endl;
        // std::cout << "Indices: " << indices.size() << std::endl;
        // std::cout << "VAO: " << VAO << std::endl;

        if (VAO == 0) {
            std::cout << "Error: VAO not initialized. Calling setupBuffers()..." << std::endl;
            setupBuffers();
        }

        glBindVertexArray(VAO);

        // Check VAO binding
        GLint currentVAO;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &currentVAO);
        //std::cout << "Current VAO binding: " << currentVAO << std::endl;

        // Enable vertex attributes
        if (!positions.empty()) {
            glEnableVertexAttribArray(0);
            // Verify attribute state
            GLint enabled;
            glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
            //std::cout << "Position attribute enabled: " << enabled << std::endl;
        }

        if (!normals.empty()) {
            glEnableVertexAttribArray(1);
            GLint enabled;
            glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
            //std::cout << "Normal attribute enabled: " << enabled << std::endl;
        }

        if (!colors.empty()) {
            glEnableVertexAttribArray(2);
            GLint enabled;
            glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
            //std::cout << "Color attribute enabled: " << enabled << std::endl;
        }

        if (!uvSets["map1"].empty()) {
            glEnableVertexAttribArray(3);
            GLint enabled;
            glGetVertexAttribiv(3, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &enabled);
            std::cout << "UV attribute enabled: " << enabled << std::endl;
        }

        // Bind materials if available
        if (!materials.empty() && materials[0]) {
            materials[0]->bind(shaderProgram);
        }

        // Draw with material assignments if available
        if (!materialIds.empty() && !materials.empty()) {
            // Draw each material group separately
            int currentMaterial = -1;
            int startIndex = 0;

            for (size_t i = 0; i < materialIds.size(); i++) {
                if (materialIds[i] != currentMaterial) {
                    // Draw previous group
                    if (currentMaterial != -1) {
                        int count = i * 3 - startIndex;  // Assuming triangles
                        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT,
                            (void*)(startIndex * sizeof(unsigned int)));
                    }

                    // Start new group
                    currentMaterial = materialIds[i];
                    startIndex = i * 3;  // Assuming triangles
                    if (currentMaterial < materials.size()) {
                        materials[currentMaterial]->bind(shaderProgram);
                    }
                }
            }

            // Draw final group
            int count = indices.size() - startIndex;
            glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT,
                (void*)(startIndex * sizeof(unsigned int)));
        }
        else {
            // Draw everything with one material
            if (!indices.empty()) {
                //std::cout << "Drawing " << indices.size() << " indices" << std::endl;
                glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
            }
            else {
                //std::cout << "Drawing " << positions.size() << " vertices" << std::endl;
                glDrawArrays(GL_TRIANGLES, 0, positions.size());
            }
        }

        // Check for OpenGL errors
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cout << "OpenGL error during draw: " << err << std::endl;
        }

        // Disable vertex attributes
        if (!positions.empty()) glDisableVertexAttribArray(0);
        if (!normals.empty()) glDisableVertexAttribArray(1);
        if (!colors.empty()) glDisableVertexAttribArray(2);
        if (!uvSets["map1"].empty()) glDisableVertexAttribArray(3);

        glBindVertexArray(0);
    }

    void drawShadow(GLuint depthShaderProgram) {
        glBindVertexArray(VAO);

        // Enable position attribute (location 0) since that's all we need
        glEnableVertexAttribArray(0);

        // For shadow mapping, we only need positions
        // No need to bind materials or textures since we're only writing depth
        if (!materialIds.empty() && !materials.empty()) {
            // Draw each material group separately to maintain consistent triangle order
            int currentMaterial = -1;
            int startIndex = 0;

            for (size_t i = 0; i < materialIds.size(); i++) {
                if (materialIds[i] != currentMaterial) {
                    // Draw previous group
                    if (currentMaterial != -1) {
                        int count = i * 3 - startIndex;  // Assuming triangles
                        glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (void*)(startIndex * sizeof(unsigned int)));
                    }

                    // Start new group
                    currentMaterial = materialIds[i];
                    startIndex = i * 3;  // Assuming triangles
                }
            }

            // Draw final group
            int count = indices.size() - startIndex;
            glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, (void*)(startIndex * sizeof(unsigned int)));
        }
        else {
            // Draw everything at once
            glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
        }

        glDisableVertexAttribArray(0); 
        glBindVertexArray(0);
    }

    void drawWireframe() {
        glLineWidth(1.0f);  // Set line width
        glColor3f(0.0f, 1.0f, 0.0f);  // Green wireframe
        // need glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glBegin(GL_TRIANGLES);
        for (size_t i = 0; i < indices.size(); i++) {
            const glm::vec3& pos = positions[indices[i]];
            glVertex3f(pos.x, pos.y, pos.z);
        }
        glEnd();

    }

    void flipNormals() {
        // Flip all stored normals
        for (auto& normal : normals) {
            normal = -normal;
        }

        // Flip winding order of triangles while preserving UV mapping
        for (size_t i = 0; i < indices.size(); i += 3) {
            // Cache UV indices before swapping
            unsigned int i1 = indices[i + 1];
            unsigned int i2 = indices[i + 2];

            // Swap indices
            indices[i + 1] = i2;
            indices[i + 2] = i1;
        }

        // Flip tangents if they exist
        for (auto& tangent : tangents) {
            tangent = -tangent;
        }

        // If buffers are already set up, update them
        if (VAO != 0) {
            glBindVertexArray(VAO);

            // Update normal data
            size_t stride = sizeof(glm::vec3);  // Position
            if (!normals.empty()) stride += sizeof(glm::vec3);
            if (!colors.empty()) stride += sizeof(glm::vec4);
            if (!uvSets["map1"].empty()) stride += sizeof(glm::vec2);
            if (!tangents.empty()) stride += sizeof(glm::vec3);

            glBindBuffer(GL_ARRAY_BUFFER, VBO);

            // Update normals
            size_t normalOffset = sizeof(glm::vec3); // After positions
            size_t currentOffset = normalOffset;
            for (size_t i = 0; i < normals.size(); i++) {
                glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sizeof(glm::vec3), &normals[i]);
                currentOffset += stride;
            }

            // Update tangents if they exist
            if (!tangents.empty()) {
                size_t tangentOffset = normalOffset +
                    ((!colors.empty()) ? sizeof(glm::vec4) : 0) +
                    ((!uvSets["map1"].empty()) ? sizeof(glm::vec2) : 0);
                currentOffset = tangentOffset;
                for (size_t i = 0; i < tangents.size(); i++) {
                    glBufferSubData(GL_ARRAY_BUFFER, currentOffset, sizeof(glm::vec3), &tangents[i]);
                    currentOffset += stride;
                }
            }

            // Update index buffer
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                indices.data(), GL_STATIC_DRAW);

            glBindVertexArray(0);
        }

        // Recalculate tangents since normals changed
        if (!uvSets["map1"].empty()) {
            calculateTangents();
        }
    }

    void debug() {
        std::cout << "\n--- Mesh Debug ---" << std::endl;
        std::cout << "Materials vector size = " << materials.size() << std::endl;
    }
};

// maybe this should be MeshType
enum class NodeType {
    Default,
    Sphere,
    Box,
    Cylinder,
    SpotLight,
    PointLight,
    SunLight
    // Add other types as needed
};

// Base class for all scene objects (matches FBX node concept)
class Node {
public:
    std::string name;

    //Node could have parent
    Node* parent;

    // Node could have children
    std::vector<std::shared_ptr<Node>> children;

    //node geometry type, default means could or could not have geometry, but the mesh is not of a special type
    NodeType type = NodeType::Default;

    // Transform data 
    glm::vec3 localTranslation;
    glm::quat localRotation;
    glm::vec3 localScale;
    glm::mat4 worldTransform;

    // Visibility and properties 
    bool visible;
    bool castsShadows;
    bool receivesShadows;



    std::map<std::string, std::string> properties; // Custom properties

    // Optional components 
    std::shared_ptr<Mesh> mesh; // mesh has materials
    // std::shared_ptr<Light> light;


    Node() :
        parent(nullptr),
        localTranslation(0.0f),
        localRotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
        localScale(1.0f),
        worldTransform(1.0f),
        visible(true),
        castsShadows(true),
        receivesShadows(true) {
    }

    // Add virtual destructor since we have inheritance
    virtual ~Node() = default;

    void addChild(std::shared_ptr<Node> child) {
        child->parent = this;
        children.push_back(child);
        child->updateWorldTransform();
    }

    virtual void updateWorldTransform() {
        // Build local transform
        glm::mat4 localTransform = glm::translate(glm::mat4(1.0f), localTranslation);
        localTransform = localTransform * glm::mat4_cast(localRotation);
        localTransform = glm::scale(localTransform, localScale);

        // Combine with parent transform
        if (parent) {
            worldTransform = parent->worldTransform * localTransform;
        }
        else {
            worldTransform = localTransform;
        }

        // Update children
        for (auto& child : children) {
            child->updateWorldTransform();
        }
    }

    // Set world position directly
    virtual void setWorldPosition(const glm::vec3& worldPos) {
        if (parent) {
            // Convert world position to local space
            glm::vec3 parentWorldPos = glm::vec3(parent->worldTransform[3]);
            glm::mat3 parentRotation = glm::mat3(parent->worldTransform);
            glm::vec3 parentScale = glm::vec3(
                glm::length(parentRotation[0]),
                glm::length(parentRotation[1]),
                glm::length(parentRotation[2])
            );

            // Normalize rotation matrix
            parentRotation[0] /= parentScale.x;
            parentRotation[1] /= parentScale.y;
            parentRotation[2] /= parentScale.z;

            // Transform to local space
            glm::vec3 localPos = glm::inverse(parentRotation) * ((worldPos - parentWorldPos) / parentScale);
            localTranslation = localPos;
        }
        else {
            // No parent, just set local translation directly
            localTranslation = worldPos;
        }

        // Update transforms
        updateWorldTransform();
    }

    glm::vec3 getWorldPosition() const {
        return glm::vec3(worldTransform[3]);
    }
    


};


// Debug function to check texture parameters
inline void debugTextureParameters(GLuint textureId) {
    GLint currentTextureBinding;
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &currentTextureBinding);

    glBindTexture(GL_TEXTURE_2D, textureId);

    GLint wrapS, wrapT, minFilter, magFilter;
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrapS);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &wrapT);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &minFilter);
    glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &magFilter);

    std::cout << "\n=== Texture Parameters for ID " << textureId << " ===\n";
    std::cout << "Wrap S: " << wrapS << " (GL_REPEAT=" << GL_REPEAT << ")\n";
    std::cout << "Wrap T: " << wrapT << " (GL_REPEAT=" << GL_REPEAT << ")\n";
    std::cout << "Min Filter: " << minFilter << " (GL_LINEAR=" << GL_LINEAR << ")\n";
    std::cout << "Mag Filter: " << magFilter << " (GL_LINEAR=" << GL_LINEAR << ")\n";

    glBindTexture(GL_TEXTURE_2D, currentTextureBinding);
}

// Debug function to check UV coordinates
inline void debugUVCoordinates(const std::vector<glm::vec2>& uvs, const std::string& uvSetName) {
    if (uvs.empty()) {
        std::cout << "UV set '" << uvSetName << "' is empty!\n";
        return;
    }

    glm::vec2 minUV(std::numeric_limits<float>::max());
    glm::vec2 maxUV(std::numeric_limits<float>::lowest());

    for (const auto& uv : uvs) {
        minUV.x = std::min(minUV.x, uv.x);
        minUV.y = std::min(minUV.y, uv.y);
        maxUV.x = std::max(maxUV.x, uv.x);
        maxUV.y = std::max(maxUV.y, uv.y);
    }

    std::cout << "\n=== UV Coordinates for " << uvSetName << " ===\n";
    std::cout << "Number of UV coordinates: " << uvs.size() << "\n";
    std::cout << "UV range: [" << minUV.x << ", " << minUV.y << "] to ["
        << maxUV.x << ", " << maxUV.y << "]\n";

    // Print first few UVs
    std::cout << "First 5 UVs:\n";
    for (size_t i = 0; i < std::min(size_t(5), uvs.size()); ++i) {
        std::cout << "UV[" << i << "]: (" << uvs[i].x << ", " << uvs[i].y << ")\n";
    }
}

// Debug function to check texture binding in shader
inline void debugTextureBindings(GLuint shaderProgram) {
    GLint numActiveUniforms;
    glGetProgramiv(shaderProgram, GL_ACTIVE_UNIFORMS, &numActiveUniforms);

    std::cout << "\n=== Active Texture Uniforms ===\n";

    const GLsizei bufSize = 256;
    GLchar name[bufSize];
    GLsizei length;
    GLint size;
    GLenum type;

    for (GLint i = 0; i < numActiveUniforms; i++) {
        glGetActiveUniform(shaderProgram, i, bufSize, &length, &size, &type, name);

        // Check if this uniform is a sampler
        if (type == GL_SAMPLER_2D) {
            GLint location = glGetUniformLocation(shaderProgram, name);
            GLint textureUnit;
            glGetUniformiv(shaderProgram, location, &textureUnit);

            std::cout << "Texture uniform '" << name << "' at location " << location
                << " bound to texture unit " << textureUnit << "\n";
        }
    }
}

// In ModelImporter::processMaterial():
inline void debugMaterialTextures(const std::shared_ptr<Material>& material) {
    std::cout << "\n=== Material Texture Debug ===\n";
    for (const auto& [mapType, texMap] : material->textureMaps) {
        std::cout << "Texture type: " << mapType << "\n";
        std::cout << "Texture ID: " << texMap.textureId << "\n";
        std::cout << "UV Set: " << texMap.uvSet << "\n";
        std::cout << "Offset: (" << texMap.offset.x << ", " << texMap.offset.y << ")\n";
        std::cout << "Tiling: (" << texMap.tiling.x << ", " << texMap.tiling.y << ")\n";
        debugTextureParameters(texMap.textureId);
    }
}



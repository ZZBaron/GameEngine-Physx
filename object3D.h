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

        // Get location of uniform
        GLint baseColorLoc = glGetUniformLocation(shaderProgram, "material.baseColor");

        // Bind the base color
        glUniform3fv(baseColorLoc, 1, glm::value_ptr(baseColor));

        // Check for GL errors
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cout << "GL Error after binding base color: " << err << std::endl;
        }

        // Bind material properties (Principled BSDF)
        //glUniform3fv(glGetUniformLocation(shaderProgram, "material.baseColor"), 1, glm::value_ptr(baseColor));
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

        // Bind textures
        int textureUnit = 2; 
        for (const auto& [mapType, textureMap] : textureMaps) {
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, textureMap.textureId);
            glUniform1i(glGetUniformLocation(shaderProgram, ("material." + mapType + "Map").c_str()), textureUnit);
            glUniform2fv(glGetUniformLocation(shaderProgram, ("material." + mapType + "Offset").c_str()), 1, glm::value_ptr(textureMap.offset));
            glUniform2fv(glGetUniformLocation(shaderProgram, ("material." + mapType + "Tiling").c_str()), 1, glm::value_ptr(textureMap.tiling));
            glUniform1f(glGetUniformLocation(shaderProgram, ("material." + mapType + "Strength").c_str()), textureMap.strength);
            textureUnit++;
        }

        // Set texture presence flags
        glUniform1i(glGetUniformLocation(shaderProgram, "material.hasBaseColorMap"), textureMaps.count("baseColor") > 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "material.hasNormalMap"), textureMaps.count("normal") > 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "material.hasMetallicMap"), textureMaps.count("metallic") > 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "material.hasRoughnessMap"), textureMaps.count("roughness") > 0);
        glUniform1i(glGetUniformLocation(shaderProgram, "material.hasEmissionMap"), textureMaps.count("emission") > 0);
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

        // Generate buffers if they don't exist
        if (VAO == 0) {
            glGenVertexArrays(1, &VAO);
        }
        if (VBO == 0) {
            glGenBuffers(1, &VBO);
        }
        if (EBO == 0) {
            glGenBuffers(1, &EBO);
        }

        // Bind and set up VAO
        glBindVertexArray(VAO);


        // Calculate total buffer size
        size_t totalSize = positions.size() * sizeof(glm::vec3) +
            normals.size() * sizeof(glm::vec3) +
            colors.size() * sizeof(glm::vec4) +
            uvSets["map1"].size() * sizeof(glm::vec2) +
            tangents.size() * sizeof(glm::vec3);

        //upload vertex data
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, totalSize, nullptr, GL_STATIC_DRAW);

        // Upload data in portions
        size_t offset = 0;

        // Positions - attribute 0
        glBufferSubData(GL_ARRAY_BUFFER, offset, positions.size() * sizeof(glm::vec3), positions.data());
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset);
        glEnableVertexAttribArray(0);
        offset += positions.size() * sizeof(glm::vec3);

        // Normals - attribute 1
        if (!normals.empty()) {
            glBufferSubData(GL_ARRAY_BUFFER, offset, normals.size() * sizeof(glm::vec3), normals.data());
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset);
            glEnableVertexAttribArray(1);
            offset += normals.size() * sizeof(glm::vec3);
        }

        // Colors - attribute 2
        if (!colors.empty()) {
            glBufferSubData(GL_ARRAY_BUFFER, offset, colors.size() * sizeof(glm::vec4), colors.data());
            glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 0, (void*)offset);
            glEnableVertexAttribArray(2);
            offset += colors.size() * sizeof(glm::vec4);
        }

        // UV coordinates - attribute 3
        if (!uvSets["map1"].empty()) {
            glBufferSubData(GL_ARRAY_BUFFER, offset, uvSets["map1"].size() * sizeof(glm::vec2), uvSets["map1"].data());
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, (void*)offset);
            glEnableVertexAttribArray(3);
        }

        // Upload tangents (attribute 4)
        if (!tangents.empty()) {
            glBufferSubData(GL_ARRAY_BUFFER, offset, tangents.size() * sizeof(glm::vec3), tangents.data());
            glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, (void*)offset);
            glEnableVertexAttribArray(4);
        }


        // Set up index buffer if we have indices
        if (!indices.empty()) {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        }

        // Unbind VAO last
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

            glm::vec3 v0 = positions[i0];
            glm::vec3 v1 = positions[i1];
            glm::vec3 v2 = positions[i2];

            glm::vec2 uv0 = uvSets["map1"][i0];
            glm::vec2 uv1 = uvSets["map1"][i1];
            glm::vec2 uv2 = uvSets["map1"][i2];

            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            float f = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

            glm::vec3 tangent;
            tangent.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
            tangent.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
            tangent.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

            // Add to all vertices of the triangle
            tangents[i0] += tangent;
            tangents[i1] += tangent;
            tangents[i2] += tangent;
        }

        // Normalize all tangents
        for (auto& tangent : tangents) {
            tangent = glm::normalize(tangent);
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
            //std::cout << "UV attribute enabled: " << enabled << std::endl;
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

        glBindVertexArray(0);
    }

    void drawWireframe() {
        glColor3f(0.0f, 1.0f, 0.0f);  // Green wireframe
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        glBegin(GL_TRIANGLES);
        for (size_t i = 0; i < indices.size(); i++) {
            const glm::vec3& pos = positions[indices[i]];
            glVertex3f(pos.x, pos.y, pos.z);
        }
        glEnd();

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
    Cylinder
    // Add other types as needed
};

// Base class for all scene objects (matches FBX node concept)
class Node {
public:
    std::string name;
    Node* parent;
    std::vector<std::shared_ptr<Node>> children;

    //node geometry type, default means could or could not have geometry, but the mesh is not of a special type
    NodeType type = NodeType::Default;

    // Transform data (matches FBX transform inheritance)
    glm::vec3 localTranslation;
    glm::quat localRotation;
    glm::vec3 localScale;
    glm::mat4 worldTransform;

    // Visibility and properties (matches FBX node attributes)
    bool visible;
    bool castsShadows;
    bool receivesShadows;
    std::map<std::string, std::string> properties; // Custom properties

    // Optional components (like FBX node attributes)
    std::shared_ptr<Mesh> mesh;
    std::shared_ptr<Material> material = std::make_shared<Material>();

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

    void updateWorldTransform() {
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
    void setWorldPosition(const glm::vec3& worldPos) {
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


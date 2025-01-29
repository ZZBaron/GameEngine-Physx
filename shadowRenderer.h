// ShadowRenderer.h
#pragma once
#include "shadowMap.h"
#include "GameEngine.h"
#include "object3D.h"
#include "shader.h"
#include "misc_funcs.h"
#include "light.h"
#include "paths.h"


class ShadowRenderer {
private:
    static const int MAX_SPOT_LIGHTS = 4;
    std::vector<ShadowMap> shadowMaps;
    std::vector<glm::mat4> lightSpaceMatrices;
    std::vector<std::shared_ptr<SpotLight>> activeLights;

    static const GLuint SHADOW_MAP_TEXTURE_UNIT = 8;  // Start after material textures

    GLuint depthShaderProgram;
    GLuint mainShaderProgram;

    float nearPlane = 0.1f;
    float farPlane = 50.0f;

public:
    bool shadowsEnabled = true;

    ShadowRenderer() {
        

    }

    void initialize() {
        // Initialize depth shader for shadow mapping
        Shader depthShader(Paths::Shaders::depthVertexShader.c_str(), Paths::Shaders::depthFragmentShader.c_str());
        depthShaderProgram = depthShader.getShaderProgram();

        // Initialize shader for rendering with shadows
        Shader mainShader(Paths::Shaders::vertexShader.c_str(), Paths::Shaders::fragmentShader.c_str());
        mainShaderProgram = mainShader.getShaderProgram();


        // Initialize shadow maps for maximum number of lights
        shadowMaps.resize(MAX_SPOT_LIGHTS);
        for (size_t i = 0; i < shadowMaps.size(); i++) {
            shadowMaps[i].initialize();
        }
    }

    void addSpotLight(std::shared_ptr<SpotLight> spotlight) {
        activeLights.push_back(spotlight);
        lightSpaceMatrices.push_back(glm::mat4(0.0f));
    }

    void setShadowProperties(float near, float far) {
        nearPlane = near;
        farPlane = far;
    }

    void toggleShadows(bool enabled) {
        shadowsEnabled = enabled;
    }

    void renderShadowPass(const std::vector<std::shared_ptr<Node>>& sceneNodes) {
        if (!shadowsEnabled) return;
        std::cout << "\n --- Calling renderShadowPass --- \n";

        // Update for each active light
        for (size_t i = 0; i < activeLights.size() && i < MAX_SPOT_LIGHTS; ++i) {
            auto& light = activeLights[i];
            std::cout << i << std::endl;

            // Calculate light space matrix for this light
            glm::mat4 lightProjection = glm::perspective(
                glm::radians(90.0f), 1.0f, nearPlane, farPlane
            );
            glm::mat4 lightView = createViewMatrix(
                light->getWorldPosition(),
                light->getWorldPosition() + light->direction,
                glm::vec3(0.0f, 1.0f, 0.0f)
            );

            lightSpaceMatrices[i] = lightProjection * lightView;


            // Render shadow map for this light
            shadowMaps[i].bindForWriting();
            glClear(GL_DEPTH_BUFFER_BIT);

            glUseProgram(depthShaderProgram);
            glUniformMatrix4fv(
                glGetUniformLocation(depthShaderProgram, "lightSpaceMatrix"),
                1, GL_FALSE, glm::value_ptr(lightSpaceMatrices[i])
            );

            for (const auto& node : sceneNodes) {
                if (node->mesh && node->castsShadows) {
                    glUniformMatrix4fv(
                        glGetUniformLocation(depthShaderProgram, "model"),
                        1, GL_FALSE, glm::value_ptr(node->worldTransform)
                    );
                    node->mesh->drawShadow(depthShaderProgram);
                }
            }
        }

        
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

    }

    void prepareMainPass(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {

        glUseProgram(mainShaderProgram);

        //debug texture units
        GLint boundTextures[16];
        for (int i = 0; i < 16; i++) {
            glActiveTexture(GL_TEXTURE0 + i);
            glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTextures[i]);
            std::cout << "Texture unit " << i << " bound to: " << boundTextures[i] << std::endl;
        }

        // Set common uniforms
        glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "projection"),
            1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "view"),
            1, GL_FALSE, glm::value_ptr(view));

        glUniform3fv(glGetUniformLocation(mainShaderProgram, "viewPos"),
            1, glm::value_ptr(cameraPos));

        // Set number of active lights
        glUniform1i(glGetUniformLocation(mainShaderProgram, "numActiveSpotLights"),
            std::min((int)activeLights.size(), MAX_SPOT_LIGHTS));

        // Set light properties for all active lights
        for (size_t i = 0; i < activeLights.size() && i < MAX_SPOT_LIGHTS; ++i) {
            std::string base = "spotLights[" + std::to_string(i) + "].";
            std::string baseForMatrices = "spotLightSpaceMatrix[" + std::to_string(i) + "]";
            auto& light = activeLights[i];

            glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, baseForMatrices.c_str()),
                1, GL_FALSE, glm::value_ptr(lightSpaceMatrices[i]));
            

            glUniform3fv(glGetUniformLocation(mainShaderProgram, (base + "position").c_str()),
                1, glm::value_ptr(light->getWorldPosition()));
            glUniform3fv(glGetUniformLocation(mainShaderProgram, (base + "direction").c_str()),
                1, glm::value_ptr(light->direction));
            glUniform3fv(glGetUniformLocation(mainShaderProgram, (base + "color").c_str()),
                1, glm::value_ptr(light->color));
            glUniform1f(glGetUniformLocation(mainShaderProgram, (base + "intensity").c_str()),
                light->intensity);
            glUniform1f(glGetUniformLocation(mainShaderProgram, (base + "constant").c_str()),
                light->constant);
            glUniform1f(glGetUniformLocation(mainShaderProgram, (base + "linear").c_str()),
                light->linear);
            glUniform1f(glGetUniformLocation(mainShaderProgram, (base + "quadratic").c_str()),
                light->quadratic);
            glUniform1f(glGetUniformLocation(mainShaderProgram, (base + "innerCutoff").c_str()),
                light->innerCutoff);
            glUniform1f(glGetUniformLocation(mainShaderProgram, (base + "outerCutoff").c_str()),
                light->outerCutoff);

            // Bind shadow map for this light
            if (shadowsEnabled) {
                shadowMaps[i].bindForReading(GL_TEXTURE0 + SHADOW_MAP_TEXTURE_UNIT + i);
                glUniform1i(glGetUniformLocation(mainShaderProgram,
                    ("shadowMaps[" + std::to_string(i) + "]").c_str()),
                    SHADOW_MAP_TEXTURE_UNIT + i);
            }

            //  after binding shadow maps
            for (size_t i = 0; i < activeLights.size() && i < MAX_SPOT_LIGHTS; ++i) {
                GLint shadowTexture;
                glActiveTexture(GL_TEXTURE0 + SHADOW_MAP_TEXTURE_UNIT + i);
                glGetIntegerv(GL_TEXTURE_BINDING_2D, &shadowTexture);
                std::cout << "Shadow map " << i << " bound to texture unit " << (SHADOW_MAP_TEXTURE_UNIT + i)
                    << " with texture ID: " << shadowTexture << std::endl;

                // Verify uniform location and value
                GLint location = glGetUniformLocation(mainShaderProgram,
                    ("shadowMaps[" + std::to_string(i) + "]").c_str());
                GLint value;
                glGetUniformiv(mainShaderProgram, location, &value);
                std::cout << "shadowMaps[" << i << "] uniform location: " << location
                    << " value: " << value << std::endl;
            }
        }


    }

    void renderMainPass(const std::vector<std::shared_ptr<Node>>& sceneNodes, const glm::mat4& view, const glm::mat4& projection) {
        glUseProgram(mainShaderProgram);

        GLint boundTex;
        glGetIntegerv(GL_TEXTURE_BINDING_2D, &boundTex);
        std::cout << "Texture bound at start of renderMainPass: " << boundTex << std::endl;

        // Draw nodes in scene
        for (const auto& node : sceneNodes) {
            if (node->mesh && node->visible) {

                glUniformMatrix4fv(
                    glGetUniformLocation(mainShaderProgram, "model"),
                    1, GL_FALSE,
                    glm::value_ptr(node->worldTransform)
                );

                //set material properties
                // Bind the material for this mesh
                if (!node->mesh->materials.empty() && node->mesh->materials[0]) {;
                    //node->mesh->materials[0]->debug();
                    node->mesh->materials[0]->bind(mainShaderProgram);
                }

                // Draw the mesh
                node->mesh->draw(mainShaderProgram);
            }
        }

    }

    // Getter methods
    GLuint getDepthShaderProgram() const { return depthShaderProgram; }
    GLuint getMainShaderProgram() const { return mainShaderProgram; }
    ShadowMap getShadowMap(int index) { return shadowMaps[index]; }
    glm::mat4 getLightSpaceMatrix(int index) { return lightSpaceMatrices[index]; }
    float getNearPlane() { return nearPlane; }
    float getFarPlane() { return farPlane; }
    bool areShadowsEnabled() const { return shadowsEnabled; }
};
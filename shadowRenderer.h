// ShadowRenderer.h
#pragma once
#include "shadowMap.h"
#include "GameEngine.h"
#include "object3D.h"
#include "shader.h"
#include "misc_funcs.h"


class ShadowRenderer {
private:
    ShadowMap shadowMap;
    static const GLuint SHADOW_MAP_TEXTURE_UNIT = 1;

    GLuint depthShaderProgram;
    GLuint mainShaderProgram;
    glm::mat4 lightSpaceMatrix;

    float nearPlane = 1.0f;
    float farPlane = 50.0f;

    glm::vec3 lightPos;
    glm::vec3 lightColor;
    float luminousPower;

public:
    bool shadowsEnabled = true;

    ShadowRenderer() :
        lightPos(0.0f, 15.0f, 0.0f),
        lightColor(1.0f, 1.0f, 1.0f),
        luminousPower(1.0f) {}

    void initialize(const char* depthVertexPath, const char* depthFragmentPath,
        const char* mainShaderVertexPath, const char* mainShaderFragmentPath) {
        // Initialize depth shader for shadow mapping
        Shader depthShader(depthVertexPath, depthFragmentPath);
        depthShaderProgram = depthShader.getShaderProgram();

        // Initialize shader for rendering with shadows
        Shader mainShader(mainShaderVertexPath, mainShaderFragmentPath);
        mainShaderProgram = mainShader.getShaderProgram();

        // Initialize shadow map
        shadowMap.initialize();
    }

    void setLightProperties(const glm::vec3& pos, const glm::vec3& color, float power) {
        lightPos = pos;
        lightColor = color;
        luminousPower = power;
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

        // Calculate light space matrix
        glm::mat4 lightProjection = glm::ortho(-20.0f, 20.0f, -20.0f, 20.0f, nearPlane, farPlane);
        glm::mat4 lightView = createViewMatrix(lightPos, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        lightSpaceMatrix = lightProjection * lightView;

        // Bind shadow framebuffer and shader
        shadowMap.bindForWriting();
        glUseProgram(depthShaderProgram);
        glUniformMatrix4fv(glGetUniformLocation(depthShaderProgram, "lightSpaceMatrix"),
            1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        // Render sceneNodes to shadow map
        for (const auto& node : sceneNodes) {
            if (node->mesh) {
                glUniformMatrix4fv(
                    glGetUniformLocation(depthShaderProgram, "model"),
                    1, GL_FALSE,
                    glm::value_ptr(node->worldTransform)
                );

                node->mesh->drawShadow(depthShaderProgram);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void prepareMainPass(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {


        glUseProgram(mainShaderProgram);

        // Set common uniforms
        glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "projection"),
            1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "view"),
            1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(mainShaderProgram, "lightSpaceMatrix"),
            1, GL_FALSE, glm::value_ptr(lightSpaceMatrix));

        glUniform3fv(glGetUniformLocation(mainShaderProgram, "viewPos"),
            1, glm::value_ptr(cameraPos));
        glUniform3fv(glGetUniformLocation(mainShaderProgram, "lightPos"),
            1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(mainShaderProgram, "lightColor"),
            1, glm::value_ptr(lightColor));
        glUniform1f(glGetUniformLocation(mainShaderProgram, "luminousPower"),
            luminousPower);

        if (shadowsEnabled) {
            // Explicitly use texture unit 1 for shadow map
            shadowMap.bindForReading(GL_TEXTURE0 + SHADOW_MAP_TEXTURE_UNIT);
            glUniform1i(glGetUniformLocation(mainShaderProgram, "shadowMap"), SHADOW_MAP_TEXTURE_UNIT);
        }
    }

    void renderMainPass(const std::vector<std::shared_ptr<Node>>& sceneNodes, const glm::mat4& view, const glm::mat4& projection) {

        // Draw nodes in scene
        for (const auto& node : sceneNodes) {
            if (node->mesh && node->visible) {

                glUseProgram(mainShaderProgram);
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
    GLuint getShadowMap() const { return shadowMap.depthMap; }
    const glm::mat4& getLightSpaceMatrix() const { return lightSpaceMatrix; }
    const glm::vec3& getLightPosition() const { return lightPos; }
    bool areShadowsEnabled() const { return shadowsEnabled; }
};
// background.h
#pragma once
#include "GameEngine.h"
#include "shader.h"
#include "misc_funcs.h"
#include "textureManager.h"

enum class BackgroundType {
    Color,
    ImageTexture,
    EnvironmentTexture,
    SkyTexture
};

class Background {
private:
    GLuint shaderProgram;
    GLuint VAO, VBO;
    BackgroundType type;
    glm::vec3 backgroundColor;
    float strength;
    GLuint textureID;

    void setupQuad() {
        // Create a quad that fills the screen in NDC
        float quadVertices[] = {
            // positions        // texture coords
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,

            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f
        };

        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

        // position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        // texture coord attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }

    void initializeShader() {
        // Load and compile background shader
        std::string vertexShaderPath = getProjectRoot() + "/shaders/background_vertex.glsl";
        std::string fragmentShaderPath = getProjectRoot() + "/shaders/background_fragment.glsl";

        Shader shader(vertexShaderPath.c_str(), fragmentShaderPath.c_str());
        shaderProgram = shader.getShaderProgram();
    }

public:
    Background() :
        type(BackgroundType::Color),
        backgroundColor(0.0f),
        strength(1.0f),
        textureID(0) {
        initializeShader();
        setupQuad();
    }

    void setColor(const glm::vec3& color) {
        type = BackgroundType::Color;
        backgroundColor = color;
    }

    void setImageTexture(const std::string& path) {
        type = BackgroundType::ImageTexture;
        textureID = TextureManager::getInstance().LoadTexture(path, "background");
    }

    void setEnvironmentTexture(const std::string& path) {
        type = BackgroundType::EnvironmentTexture;
        textureID = TextureManager::getInstance().LoadTexture(path, "environment");
    }

    void setSkyTexture(const std::string& path) {
        type = BackgroundType::SkyTexture;
        textureID = TextureManager::getInstance().LoadTexture(path, "sky");
    }

    void setStrength(float value) {
        strength = glm::max(0.0f, value);
    }

    void render(const glm::mat4& view, const glm::mat4& projection) {
        glDepthFunc(GL_LEQUAL);
        glUseProgram(shaderProgram);

        // Set common uniforms
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(shaderProgram, "strength"), strength);
        glUniform1i(glGetUniformLocation(shaderProgram, "backgroundType"), static_cast<int>(type));

        if (type == BackgroundType::Color) {
            glUniform3fv(glGetUniformLocation(shaderProgram, "backgroundColor"), 1, glm::value_ptr(backgroundColor));
        }
        else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureID);
            glUniform1i(glGetUniformLocation(shaderProgram, "backgroundTexture"), 0);
        }

        // Render background quad
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glDepthFunc(GL_LESS); // Reset depth function
    }

    ~Background() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
    }
};
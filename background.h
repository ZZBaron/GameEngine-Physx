// background.h
#pragma once
#include "GameEngine.h"
#include "shader.h"
#include "misc_funcs.h"
#include "textureManager.h"
#include "paths.h"

class Skybox {
private:
    unsigned int skyboxVAO, skyboxVBO;
    unsigned int cubemapTexture;
    std::shared_ptr<Shader> skyboxShader;

    // Skybox vertices - a cube centered at origin
    float skyboxVertices[108] = {
        -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f, -1.0f,  1.0f,
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f,
         1.0f,  1.0f, -1.0f,
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f,
         1.0f, -1.0f,  1.0f
    };

public:
    void setup() {
        // Create and compile shaders
        skyboxShader = std::make_shared<Shader>(Paths::Shaders::skyboxVertexShader.c_str(), Paths::Shaders::skyboxFragmentShader.c_str());

        // Create VAO and VBO
        glGenVertexArrays(1, &skyboxVAO);
        glGenBuffers(1, &skyboxVBO);

        glBindVertexArray(skyboxVAO);
        glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }

    unsigned int loadCubemap(const std::vector<std::string>& faces) {
        unsigned int textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        int width, height, nrChannels;
        for (unsigned int i = 0; i < faces.size(); i++) {
            unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
            if (data) {
                GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                    0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }
            else {
                std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
                stbi_image_free(data);
            }
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        cubemapTexture = textureID;
        return textureID;
    }

    void render(const glm::mat4& view, const glm::mat4& projection) {
        // Change depth function and disable depth writing
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);

        skyboxShader->use();

        // Remove translation from view matrix
        glm::mat4 skyboxView = glm::mat4(glm::mat3(view));

        skyboxShader->setMat4("view", skyboxView);
        skyboxShader->setMat4("projection", projection);

        // Bind cubemap texture
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);

        // Reset depth settings
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }

    ~Skybox() {
        glDeleteVertexArrays(1, &skyboxVAO);
        glDeleteBuffers(1, &skyboxVBO);
    }
};

enum class BackgroundType {
    Color,
    ImageTexture,
    EnvironmentTexture,
    SkyTexture
};

class Background {
private:
    GLuint shaderProgram;
    BackgroundType type;
    glm::vec3 backgroundColor;
    float strength;
    GLuint textureID;

   

    void initializeShader() {
        // Load and compile background shader
        std::string vertexShaderPath = Paths::Shaders::backgroundVertexShader;
        std::string fragmentShaderPath = Paths::Shaders::backgroundFragmentShader;


        Shader shader(vertexShaderPath.c_str(), fragmentShaderPath.c_str());
        shaderProgram = shader.getShaderProgram();
    }

public:
    GLuint VAO, VBO; //public so they can be used for rendering in subclasses like SkyBackground

    Background() :
        type(BackgroundType::Color),
        backgroundColor(0.0f),
        strength(1.0f),
        textureID(0) {
    }

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

    virtual void setup() {
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

    void setType(BackgroundType backgroundType) {
		type = backgroundType;
	}

    void setStrength(float value) {
        strength = glm::max(0.0f, value);
    }

    virtual void render(const glm::mat4& view, const glm::mat4& projection) {
        std::cout << "\n --- Rendering background --- \n";

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
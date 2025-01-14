#pragma once
#include "GameEngine.h"
#include "object3D.h"

class UVViewer {
private:
    GLuint shaderProgram;
    GLuint vao, vbo, ebo;
    std::vector<glm::vec2> uvPoints;
    std::vector<unsigned int> uvIndices;

    const char* vertexShader = R"(
        #version 330 core
        layout (location = 0) in vec2 aPos;
        void main() {
            gl_Position = vec4(aPos, 0.0, 1.0);
        }
    )";

    const char* fragmentShader = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 1.0, 1.0, 1.0);  // White lines
        }
    )";

public:
    void initialize() {
        GLuint vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vertexShader, NULL);
        glCompileShader(vertex);

        GLuint fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fragmentShader, NULL);
        glCompileShader(fragment);

        shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertex);
        glAttachShader(shaderProgram, fragment);
        glLinkProgram(shaderProgram);

        glDeleteShader(vertex);
        glDeleteShader(fragment);

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);
    }

    void setupMeshUVs(const std::shared_ptr<Mesh>& mesh) {
        if (!mesh || mesh->uvSets["map1"].empty()) return;

        uvPoints.clear();
        uvIndices.clear();

        const auto& uvs = mesh->uvSets["map1"];
        for (const auto& uv : uvs) {
            glm::vec2 screenPos;
            screenPos.x = (uv.x * 2.0f - 1.0f) * 0.8f;
            screenPos.y = (uv.y * 2.0f - 1.0f) * 0.8f;
            uvPoints.push_back(screenPos);
        }
        uvIndices = mesh->indices;

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, uvPoints.size() * sizeof(glm::vec2), uvPoints.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, uvIndices.size() * sizeof(unsigned int), uvIndices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(glm::vec2), (void*)0);
        glEnableVertexAttribArray(0);
    }

    void render(GLuint) {
        if (uvPoints.empty() || uvIndices.empty()) return;

        GLint previousViewport[4];
        glGetIntegerv(GL_VIEWPORT, previousViewport);

        glViewport(previousViewport[2] - 256, 0, 256, 256);
        glUseProgram(shaderProgram);
        glBindVertexArray(vao);

        glLineWidth(1.0f);
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawElements(GL_TRIANGLES, uvIndices.size(), GL_UNSIGNED_INT, 0);
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
    }

    ~UVViewer() {
        glDeleteProgram(shaderProgram);
        glDeleteVertexArrays(1, &vao);
        glDeleteBuffers(1, &vbo);
        glDeleteBuffers(1, &ebo);
    }
};
// Renderer2D.h
#pragma once
#include "GameEngine.h"
#include "shader.h"

struct Vertex2D {
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec4 color;
};

class Renderer2D {
private:
    GLuint VAO, VBO, EBO;
    std::unique_ptr<Shader> spriteShader;

    static const size_t MAX_SPRITES = 1000;
    static const size_t MAX_VERTICES = MAX_SPRITES * 4;
    static const size_t MAX_INDICES = MAX_SPRITES * 6;

    std::vector<Vertex2D> vertices;
    std::vector<GLuint> indices;

public:
    void initialize() {
        // Create and compile shaders
        spriteShader = std::make_unique<Shader>(
            "sprite_vertex_shader.glsl",
            "sprite_fragment_shader.glsl"
        );

        // Generate vertex array and buffers
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);

        // Bind VAO
        glBindVertexArray(VAO);

        // Setup vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, MAX_VERTICES * sizeof(Vertex2D), nullptr, GL_DYNAMIC_DRAW);

        // Setup index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_INDICES * sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);

        // Setup vertex attributes
        // Position attribute
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, position));

        // Texture coordinate attribute
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, texCoord));

        // Color attribute
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, color));

        // Unbind VAO
        glBindVertexArray(0);

        // Setup blending
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void beginBatch() {
        vertices.clear();
        indices.clear();
    }

    void drawSprite(const Sprite& sprite, const glm::mat4& transform) {
        if (vertices.size() >= MAX_VERTICES) {
            flushBatch();
        }

        // Calculate sprite vertices
        float width = sprite.size.x;
        float height = sprite.size.y;

        // Calculate texture coordinates based on source rectangle
        glm::vec2 texCoords[4] = {
            {sprite.sourceRect.x, sprite.sourceRect.y},
            {sprite.sourceRect.x + sprite.sourceRect.z, sprite.sourceRect.y},
            {sprite.sourceRect.x + sprite.sourceRect.z, sprite.sourceRect.y + sprite.sourceRect.w},
            {sprite.sourceRect.x, sprite.sourceRect.y + sprite.sourceRect.w}
        };

        // Add vertices
        size_t vertexOffset = vertices.size();

        vertices.push_back({ {-width * sprite.origin.x, -height * sprite.origin.y, 0.0f},
                           texCoords[0], sprite.color });
        vertices.push_back({ {width * (1.0f - sprite.origin.x), -height * sprite.origin.y, 0.0f},
                           texCoords[1], sprite.color });
        vertices.push_back({ {width * (1.0f - sprite.origin.x), height * (1.0f - sprite.origin.y), 0.0f},
                           texCoords[2], sprite.color });
        vertices.push_back({ {-width * sprite.origin.x, height * (1.0f - sprite.origin.y), 0.0f},
                           texCoords[3], sprite.color });

        // Add indices
        indices.push_back(vertexOffset);
        indices.push_back(vertexOffset + 1);
        indices.push_back(vertexOffset + 2);
        indices.push_back(vertexOffset + 2);
        indices.push_back(vertexOffset + 3);
        indices.push_back(vertexOffset);
    }

    void flushBatch() {
        if (vertices.empty()) {
            return;
        }

        // Bind shader and VAO
        spriteShader->use();
        glBindVertexArray(VAO);

        // Update vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0,
            vertices.size() * sizeof(Vertex2D), vertices.data());

        // Update index buffer
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
            indices.size() * sizeof(GLuint), indices.data());

        // Draw sprites
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        // Reset batch
        vertices.clear();
        indices.clear();
    }

    void setViewProjection(const glm::mat4& view, const glm::mat4& projection) {
        spriteShader->use();
        spriteShader->setMat4("view", view);
        spriteShader->setMat4("projection", projection);
    }

    ~Renderer2D() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteBuffers(1, &EBO);
    }
};
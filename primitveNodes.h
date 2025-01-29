//primitiveNodes.h
#pragma once
#include "object3D.h"

class SphereNode : public Node {
public:
    float radius;
    int slices;
    int stacks;

    SphereNode(float r, int sl = 20, int st = 20) :
        radius(r), slices(sl), stacks(st) {
        type = NodeType::Sphere;
        generateMesh();
    }

    //default constructer
    SphereNode() {
        radius = 0.1f;
        slices = 20;
        stacks = 20;
        type = NodeType::Sphere;
        generateMesh();
    }


private:
    void generateMesh() {
        mesh = std::make_shared<Mesh>();

        // Generate vertices
        for (int i = 0; i <= stacks; ++i) {
            float stackAngle = i * (glm::pi<float>() / stacks);
            float r = radius * sin(stackAngle);
            float y = radius * cos(stackAngle);

            // UV vertical coordinate
            float t = 1.0f - (float)i / stacks;

            for (int j = 0; j <= slices; ++j) {
                float sliceAngle = j * (2.0f * glm::pi<float>() / slices);
                float x = r * cos(sliceAngle);
                float z = r * sin(sliceAngle);

                // UV horizontal coordinate
                float s = (float)j / slices;

                // Position
                mesh->positions.push_back(glm::vec3(x, y, z));

                // Normal (normalized position for sphere)
                mesh->normals.push_back(glm::normalize(glm::vec3(x, y, z)));

                // UV coordinates
                mesh->uvSets["map1"].push_back(glm::vec2(s, t));

                // Default color (white)
                mesh->colors.push_back(glm::vec4(1.0f));
            }
        }

        // Generate indices
        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < slices; ++j) {
                unsigned int first = (unsigned int)(i * (slices + 1) + j);
                unsigned int second = first + (unsigned int)(slices + 1);

                mesh->indices.push_back(first);
                mesh->indices.push_back(second);
                mesh->indices.push_back((unsigned int)(first + 1));

                mesh->indices.push_back(second);
                mesh->indices.push_back((unsigned int)(second + 1));
                mesh->indices.push_back((unsigned int)(first + 1));
            }
        }

        mesh->setupBuffers();
    }
};

class BoxNode : public Node {
public:
    float width;
    float height;
    float depth;

    BoxNode(float w, float h, float d) :
        width(w), height(h), depth(d) {
        type = NodeType::Box;
        generateMesh();
    }

private:
    void generateMesh() {
        mesh = std::make_shared<Mesh>();

        float hw = width * 0.5f;   // half width
        float hh = height * 0.5f;  // half height
        float hd = depth * 0.5f;   // half depth

        // Define the 8 vertices
        glm::vec3 vertices[8] = {
            glm::vec3(-hw, -hh, -hd),  // 0: left-bottom-back
            glm::vec3(hw, -hh, -hd),   // 1: right-bottom-back
            glm::vec3(hw, hh, -hd),    // 2: right-top-back
            glm::vec3(-hw, hh, -hd),   // 3: left-top-back
            glm::vec3(-hw, -hh, hd),   // 4: left-bottom-front
            glm::vec3(hw, -hh, hd),    // 5: right-bottom-front
            glm::vec3(hw, hh, hd),     // 6: right-top-front
            glm::vec3(-hw, hh, hd)     // 7: left-top-front
        };

        // Front face (-Z)
        mesh->positions.insert(mesh->positions.end(), { vertices[0], vertices[1], vertices[2], vertices[3] });
        for (int i = 0; i < 4; i++) {
            mesh->normals.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
            mesh->colors.push_back(glm::vec4(1.0f));
        }
        mesh->uvSets["map1"].insert(mesh->uvSets["map1"].end(), {
            glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(1, 1), glm::vec2(0, 1)
            });

        // Back face (+Z)
        mesh->positions.insert(mesh->positions.end(), { vertices[4], vertices[5], vertices[6], vertices[7] });
        for (int i = 0; i < 4; i++) {
            mesh->normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
            mesh->colors.push_back(glm::vec4(1.0f));
        }
        mesh->uvSets["map1"].insert(mesh->uvSets["map1"].end(), {
            glm::vec2(1, 0), glm::vec2(0, 0), glm::vec2(0, 1), glm::vec2(1, 1)
            });

        // Left face (-X)
        mesh->positions.insert(mesh->positions.end(), { vertices[0], vertices[3], vertices[7], vertices[4] });
        for (int i = 0; i < 4; i++) {
            mesh->normals.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
            mesh->colors.push_back(glm::vec4(1.0f));
        }
        mesh->uvSets["map1"].insert(mesh->uvSets["map1"].end(), {
            glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(1, 1), glm::vec2(0, 1)
            });

        // Right face (+X)
        mesh->positions.insert(mesh->positions.end(), { vertices[1], vertices[5], vertices[6], vertices[2] });
        for (int i = 0; i < 4; i++) {
            mesh->normals.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
            mesh->colors.push_back(glm::vec4(1.0f));
        }
        mesh->uvSets["map1"].insert(mesh->uvSets["map1"].end(), {
            glm::vec2(1, 0), glm::vec2(0, 0), glm::vec2(0, 1), glm::vec2(1, 1)
            });

        // Bottom face (-Y)
        mesh->positions.insert(mesh->positions.end(), { vertices[0], vertices[1], vertices[5], vertices[4] });
        for (int i = 0; i < 4; i++) {
            mesh->normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
            mesh->colors.push_back(glm::vec4(1.0f));
        }
        mesh->uvSets["map1"].insert(mesh->uvSets["map1"].end(), {
            glm::vec2(0, 1), glm::vec2(1, 1), glm::vec2(1, 0), glm::vec2(0, 0)
            });

        // Top face (+Y)
        mesh->positions.insert(mesh->positions.end(), { vertices[3], vertices[2], vertices[6], vertices[7] });
        for (int i = 0; i < 4; i++) {
            mesh->normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
            mesh->colors.push_back(glm::vec4(1.0f));
        }
        mesh->uvSets["map1"].insert(mesh->uvSets["map1"].end(), {
            glm::vec2(0, 0), glm::vec2(1, 0), glm::vec2(1, 1), glm::vec2(0, 1)
            });

        // Generate indices for all faces (6 faces, 2 triangles each)
        for (unsigned int face = 0; face < 6; face++) {
            unsigned int base = face * 4;
            mesh->indices.insert(mesh->indices.end(), {
                (unsigned int)(base),
                (unsigned int)(base + 1),
                (unsigned int)(base + 2),
                (unsigned int)(base + 2),
                (unsigned int)(base + 3),
                (unsigned int)(base)
                });
        }

        mesh->setupBuffers();
    }
};

class CylinderNode : public Node {
public:
    float radius;
    float height;
    int slices;
    int stacks;

    CylinderNode(float r, float h, int sl = 20, int st = 1) :
        radius(r), height(h), slices(sl), stacks(st) {
        type = NodeType::Cylinder;
        generateMesh();
    }

private:
    void generateMesh() {
        mesh = std::make_shared<Mesh>();

        // Generate vertices for the cylinder body
        for (int i = 0; i <= stacks; ++i) {
            float stackHeight = height * ((float)i / stacks) - height / 2.0f;

            for (int j = 0; j <= slices; ++j) {
                float angle = j * (2.0f * glm::pi<float>() / slices);
                float x = radius * cos(angle);
                float z = radius * sin(angle);

                // Position
                mesh->positions.push_back(glm::vec3(x, stackHeight, z));

                // Normal (pointing outward for the body)
                mesh->normals.push_back(glm::normalize(glm::vec3(x, 0.0f, z)));

                // UV coordinates
                float u = (float)j / slices;
                float v = (float)i / stacks;
                mesh->uvSets["map1"].push_back(glm::vec2(u, v));

                // Default color (white)
                mesh->colors.push_back(glm::vec4(1.0f));
            }
        }

        // Generate indices for the cylinder body
        for (int i = 0; i < stacks; ++i) {
            for (int j = 0; j < slices; ++j) {
                unsigned int first = i * (slices + 1) + j;
                unsigned int second = first + slices + 1;

                mesh->indices.push_back(first);
                mesh->indices.push_back(second);
                mesh->indices.push_back(first + 1);

                mesh->indices.push_back(second);
                mesh->indices.push_back(second + 1);
                mesh->indices.push_back(first + 1);
            }
        }

        // Generate vertices for top and bottom caps
        for (int cap = 0; cap < 2; ++cap) {
            float y = (cap == 0) ? -height / 2.0f : height / 2.0f;
            float normalY = (cap == 0) ? -1.0f : 1.0f;

            // Center vertex
            unsigned int centerIndex = mesh->positions.size();
            mesh->positions.push_back(glm::vec3(0.0f, y, 0.0f));
            mesh->normals.push_back(glm::vec3(0.0f, normalY, 0.0f));
            mesh->uvSets["map1"].push_back(glm::vec2(0.5f, 0.5f));
            mesh->colors.push_back(glm::vec4(1.0f));

            // Generate vertices around the cap
            for (int i = 0; i <= slices; ++i) {
                float angle = i * (2.0f * glm::pi<float>() / slices);
                float x = radius * cos(angle);
                float z = radius * sin(angle);

                mesh->positions.push_back(glm::vec3(x, y, z));
                mesh->normals.push_back(glm::vec3(0.0f, normalY, 0.0f));

                // UV coordinates for the cap (circular mapping)
                float u = cos(angle) * 0.5f + 0.5f;
                float v = sin(angle) * 0.5f + 0.5f;
                mesh->uvSets["map1"].push_back(glm::vec2(u, v));

                mesh->colors.push_back(glm::vec4(1.0f));

                // Generate indices for the cap triangles
                if (i < slices) {
                    if (cap == 0) {  // Bottom cap
                        mesh->indices.push_back(centerIndex);
                        mesh->indices.push_back(centerIndex + i + 1);
                        mesh->indices.push_back(centerIndex + i + 2);
                    }
                    else {  // Top cap
                        mesh->indices.push_back(centerIndex);
                        mesh->indices.push_back(centerIndex + i + 2);
                        mesh->indices.push_back(centerIndex + i + 1);
                    }
                }
            }
        }

        mesh->setupBuffers();
    }
};

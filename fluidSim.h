// FluidSimulation.h
#pragma once
#include "GameEngine.h"
#include "object3D.h"

class FluidSimulation {
private:
    struct Cell {
        glm::vec3 velocity;
        float density;
        float pressure;
        float temperature;
    };

    int gridSize;
    float cellSize;
    float timeStep;
    float viscosity;
    float diffusionRate;

    std::vector<Cell> grid;
    std::vector<Cell> prevGrid;

    // Mesh data for rendering
    std::shared_ptr<Mesh> fluidMesh;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<unsigned int> indices;

public:
    FluidSimulation(int size = 32, float cellSize = 0.1f)
        : gridSize(size), cellSize(cellSize), timeStep(0.016f),
        viscosity(0.1f), diffusionRate(0.1f) {

        // Initialize grids
        grid.resize(size * size * size);
        prevGrid = grid;

        // Initialize rendering mesh
        setupRenderingMesh();
    }

    void setupRenderingMesh() {
        fluidMesh = std::make_shared<Mesh>();

        // Create initial mesh (can be updated based on fluid surface)
        updateMeshGeometry();

        // Setup OpenGL buffers
        fluidMesh->setupBuffers();
    }

    void simulate() {
        // Save current state
        prevGrid = grid;

        // Solve Navier-Stokes equations
        advect();
        diffuse();
        projectPressure();

        // Update rendering mesh
        updateMeshGeometry();
    }

private:
    void advect() {
        // Semi-Lagrangian advection
        for (int i = 1; i < gridSize - 1; i++) {
            for (int j = 1; j < gridSize - 1; j++) {
                for (int k = 1; k < gridSize - 1; k++) {
                    int idx = getIndex(i, j, k);

                    // Backtrace particle
                    glm::vec3 pos(i * cellSize, j * cellSize, k * cellSize);
                    glm::vec3 velocity = grid[idx].velocity;
                    glm::vec3 backPos = pos - velocity * timeStep;

                    // Interpolate from previous grid
                    grid[idx].density = interpolateDensity(backPos);
                    grid[idx].velocity = interpolateVelocity(backPos);
                }
            }
        }
    }

    void diffuse() {
        // Gauss-Seidel relaxation for diffusion
        float a = timeStep * viscosity * gridSize * gridSize;

        for (int iter = 0; iter < 20; iter++) {
            for (int i = 1; i < gridSize - 1; i++) {
                for (int j = 1; j < gridSize - 1; j++) {
                    for (int k = 1; k < gridSize - 1; k++) {
                        int idx = getIndex(i, j, k);
                        int idx_x1 = getIndex(i - 1, j, k);
                        int idx_x2 = getIndex(i + 1, j, k);
                        int idx_y1 = getIndex(i, j - 1, k);
                        int idx_y2 = getIndex(i, j + 1, k);
                        int idx_z1 = getIndex(i, j, k - 1);
                        int idx_z2 = getIndex(i, j, k + 1);

                        grid[idx].velocity = (prevGrid[idx].velocity +
                            a * (grid[idx_x1].velocity + grid[idx_x2].velocity +
                                grid[idx_y1].velocity + grid[idx_y2].velocity +
                                grid[idx_z1].velocity + grid[idx_z2].velocity)) / (1 + 6 * a);
                    }
                }
            }
        }
    }

    void projectPressure() {
        // Pressure projection to enforce incompressibility
        std::vector<float> divergence(gridSize * gridSize * gridSize);
        std::vector<float> pressure(gridSize * gridSize * gridSize);

        // Calculate divergence
        for (int i = 1; i < gridSize - 1; i++) {
            for (int j = 1; j < gridSize - 1; j++) {
                for (int k = 1; k < gridSize - 1; k++) {
                    int idx = getIndex(i, j, k);
                    int idx_x1 = getIndex(i + 1, j, k);
                    int idx_y1 = getIndex(i, j + 1, k);
                    int idx_z1 = getIndex(i, j, k + 1);

                    float dx = (grid[idx_x1].velocity.x - grid[idx].velocity.x +
                        grid[idx_y1].velocity.y - grid[idx].velocity.y +
                        grid[idx_z1].velocity.z - grid[idx].velocity.z) / cellSize;

                    divergence[idx] = -dx / 2.0f;
                    pressure[idx] = 0;
                }
            }
        }

        // Solve pressure
        for (int iter = 0; iter < 20; iter++) {
            for (int i = 1; i < gridSize - 1; i++) {
                for (int j = 1; j < gridSize - 1; j++) {
                    for (int k = 1; k < gridSize - 1; k++) {
                        int idx = getIndex(i, j, k);
                        int idx_x1 = getIndex(i - 1, j, k);
                        int idx_x2 = getIndex(i + 1, j, k);
                        int idx_y1 = getIndex(i, j - 1, k);
                        int idx_y2 = getIndex(i, j + 1, k);
                        int idx_z1 = getIndex(i, j, k - 1);
                        int idx_z2 = getIndex(i, j, k + 1);

                        pressure[idx] = (divergence[idx] +
                            pressure[idx_x1] + pressure[idx_x2] +
                            pressure[idx_y1] + pressure[idx_y2] +
                            pressure[idx_z1] + pressure[idx_z2]) / 6.0f;
                    }
                }
            }
        }

        // Apply pressure gradient to velocity
        for (int i = 1; i < gridSize - 1; i++) {
            for (int j = 1; j < gridSize - 1; j++) {
                for (int k = 1; k < gridSize - 1; k++) {
                    int idx = getIndex(i, j, k);
                    int idx_x1 = getIndex(i + 1, j, k);
                    int idx_y1 = getIndex(i, j + 1, k);
                    int idx_z1 = getIndex(i, j, k + 1);

                    grid[idx].velocity -= glm::vec3(
                        (pressure[idx_x1] - pressure[idx]) / cellSize,
                        (pressure[idx_y1] - pressure[idx]) / cellSize,
                        (pressure[idx_z1] - pressure[idx]) / cellSize
                    ) * 0.5f;
                }
            }
        }
    }

    void updateMeshGeometry() {
        // Clear previous geometry
        vertices.clear();
        normals.clear();
        indices.clear();

        // Marching cubes or level set method would go here
        // For now, we'll use a simple isosurface approach
        float threshold = 0.5f;

        for (int i = 1; i < gridSize - 1; i++) {
            for (int j = 1; j < gridSize - 1; j++) {
                for (int k = 1; k < gridSize - 1; k++) {
                    if (grid[getIndex(i, j, k)].density > threshold) {
                        // Add cube for this cell
                        addCubeToMesh(i, j, k);
                    }
                }
            }
        }

        // Update mesh data
        fluidMesh->positions = vertices;
        fluidMesh->normals = normals;
        fluidMesh->indices = indices;

        // Update GPU buffers
        fluidMesh->setupBuffers();
    }

    void addCubeToMesh(int i, int j, int k) {
        // Add vertices for a cube at grid position (i,j,k)
        glm::vec3 pos(i * cellSize, j * cellSize, k * cellSize);
        size_t baseIndex = vertices.size();

        // Cube vertices
        vertices.push_back(pos + glm::vec3(0, 0, 0));
        vertices.push_back(pos + glm::vec3(cellSize, 0, 0));
        vertices.push_back(pos + glm::vec3(cellSize, cellSize, 0));
        vertices.push_back(pos + glm::vec3(0, cellSize, 0));
        vertices.push_back(pos + glm::vec3(0, 0, cellSize));
        vertices.push_back(pos + glm::vec3(cellSize, 0, cellSize));
        vertices.push_back(pos + glm::vec3(cellSize, cellSize, cellSize));
        vertices.push_back(pos + glm::vec3(0, cellSize, cellSize));

        // Simple normal for each vertex (can be improved)
        for (int n = 0; n < 8; n++) {
            normals.push_back(glm::normalize(vertices[baseIndex + n] - pos));
        }

        // Indices for cube faces
        std::vector<unsigned int> cubeIndices = {
            0, 1, 2, 2, 3, 0,  // front
            1, 5, 6, 6, 2, 1,  // right
            5, 4, 7, 7, 6, 5,  // back
            4, 0, 3, 3, 7, 4,  // left
            3, 2, 6, 6, 7, 3,  // top
            4, 5, 1, 1, 0, 4   // bottom
        };

        for (unsigned int idx : cubeIndices) {
            indices.push_back(baseIndex + idx);
        }
    }

    int getIndex(int i, int j, int k) const {
        return i + j * gridSize + k * gridSize * gridSize;
    }

    float interpolateDensity(const glm::vec3& pos) {
        // Trilinear interpolation
        int i = static_cast<int>(pos.x / cellSize);
        int j = static_cast<int>(pos.y / cellSize);
        int k = static_cast<int>(pos.z / cellSize);

        float fx = (pos.x / cellSize) - i;
        float fy = (pos.y / cellSize) - j;
        float fz = (pos.z / cellSize) - k;

        i = glm::clamp(i, 0, gridSize - 2);
        j = glm::clamp(j, 0, gridSize - 2);
        k = glm::clamp(k, 0, gridSize - 2);

        float c000 = prevGrid[getIndex(i, j, k)].density;
        float c100 = prevGrid[getIndex(i + 1, j, k)].density;
        float c010 = prevGrid[getIndex(i, j + 1, k)].density;
        float c110 = prevGrid[getIndex(i + 1, j + 1, k)].density;
        float c001 = prevGrid[getIndex(i, j, k + 1)].density;
        float c101 = prevGrid[getIndex(i + 1, j, k + 1)].density;
        float c011 = prevGrid[getIndex(i, j + 1, k + 1)].density;
        float c111 = prevGrid[getIndex(i + 1, j + 1, k + 1)].density;

        return glm::mix(
            glm::mix(
                glm::mix(c000, c100, fx),
                glm::mix(c010, c110, fx),
                fy),
            glm::mix(
                glm::mix(c001, c101, fx),
                glm::mix(c011, c111, fx),
                fy),
            fz);
    }

    glm::vec3 interpolateVelocity(const glm::vec3& pos) {
        // Similar to interpolateDensity but for velocity vector
        int i = static_cast<int>(pos.x / cellSize);
        int j = static_cast<int>(pos.y / cellSize);
        int k = static_cast<int>(pos.z / cellSize);

        i = glm::clamp(i, 0, gridSize - 2);
        j = glm::clamp(j, 0, gridSize - 2);
        k = glm::clamp(k, 0, gridSize - 2);

        return prevGrid[getIndex(i, j, k)].velocity;  // Simplified for now
    }

public:
    std::shared_ptr<Mesh> getMesh() const { return fluidMesh; }

    void addForce(const glm::vec3& position, const glm::vec3& force) {
        int i = static_cast<int>(position.x / cellSize);
        int j = static_cast<int>(position.y / cellSize);
        int k = static_cast<int>(position.z / cellSize);

        if (i >= 0 && i < gridSize - 1 &&
            j >= 0 && j < gridSize - 1 &&
            k >= 0 && k < gridSize - 1) {
            grid[getIndex(i, j, k)].velocity += force * timeStep;
        }
    }

    void addDensity(const glm::vec3& position, float amount) {
        int i = static_cast<int>(position.x / cellSize);
        int j = static_cast<int>(position.y / cellSize);
        int k = static_cast<int>(position.z / cellSize);

        if (i >= 0 && i < gridSize - 1 &&
            j >= 0 && j < gridSize - 1 &&
            k >= 0 && k < gridSize - 1) {
            grid[getIndex(i, j, k)].density += amount;
        }
    }
};
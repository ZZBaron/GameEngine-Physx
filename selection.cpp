// selection.cpp
#include "selection.h"
#include "scene.h"

SelectionSystem* SelectionSystem::instance = nullptr;

SelectionSystem& SelectionSystem::getInstance() {
    if (!instance) {
        instance = new SelectionSystem();
    }
    return *instance;
}

Ray SelectionSystem::screenToWorldRay(double mouseX, double mouseY, int screenWidth, int screenHeight,
    Scene& scene) {

    // First check if we have a valid camera
    if (!camera) {
        throw std::runtime_error("Camera not set in SelectionSystem");
    }
    glm::mat4 projection = scene.activeCamera->getProjectionMatrix();
    glm::mat4 view = scene.activeCamera->getViewMatrix();

    // Convert to normalized device coordinates
    float x = (2.0f * mouseX) / screenWidth - 1.0f;
    float y = 1.0f - (2.0f * mouseY) / screenHeight;  // Flip Y coordinate

    // Create ray in clip space
    glm::vec4 rayClip = glm::vec4(x, y, -1.0f, 1.0f);


    // Convert to eye space
    glm::vec4 rayEye = glm::inverse(projection) * rayClip;
    rayEye = glm::vec4(rayEye.x, rayEye.y, -1.0f, 0.0f);

    // Convert to world space
    glm::vec4 rayWorld = glm::inverse(view) * rayEye;
    glm::vec3 rayDirection = glm::normalize(glm::vec3(rayWorld));

    //debug dray ray
    Ray ray = Ray(camera->cameraPos, rayDirection);
    drawRay(ray, 10.0f, scene);

    return ray;
}

bool SelectionSystem::triangleIntersection(const Ray& ray, const glm::vec3& v0, const glm::vec3& v1,
    const glm::vec3& v2, float& t) {
    const float EPSILON = 0.0000001f;

    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 h = glm::cross(ray.direction, edge2);
    float a = glm::dot(edge1, h);

    if (a > -EPSILON && a < EPSILON) return false;

    float f = 1.0f / a;
    glm::vec3 s = ray.origin - v0;
    float u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f) return false;

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(ray.direction, q);

    if (v < 0.0f || u + v > 1.0f) return false;

    t = f * glm::dot(edge2, q);
    return t > EPSILON;
}

SelectionSystem::IntersectionResult SelectionSystem::rayMeshIntersection(
    const Ray& localRay, const std::shared_ptr<Mesh>& mesh) {

    IntersectionResult result;
    if (!mesh || mesh->indices.empty()) return result;

    for (size_t i = 0; i < mesh->indices.size(); i += 3) {
        const glm::vec3& v0 = mesh->positions[mesh->indices[i]];
        const glm::vec3& v1 = mesh->positions[mesh->indices[i + 1]];
        const glm::vec3& v2 = mesh->positions[mesh->indices[i + 2]];

        float t;
        if (triangleIntersection(localRay, v0, v1, v2, t)) {
            if (t < result.distance) {
                result.hit = true;
                result.distance = t;
            }
        }
    }

    return result;
}

SelectionSystem::IntersectionResult SelectionSystem::rayIntersectMesh(
    const Ray& ray, const std::shared_ptr<Node>& node) {

    IntersectionResult result;
    if (!node || !node->mesh) return result;

    // Transform ray to local space
    glm::mat4 invWorld = glm::inverse(node->worldTransform);
    glm::vec3 localOrigin = glm::vec3(invWorld * glm::vec4(ray.origin, 1.0f));
    glm::vec3 localDir = glm::normalize(glm::vec3(invWorld * glm::vec4(ray.direction, 0.0f)));
    Ray localRay(localOrigin, localDir);

    return rayMeshIntersection(localRay, node->mesh);
}

bool SelectionSystem::handleSelection(double mouseX, double mouseY, int screenWidth, int screenHeight, Scene& scene, bool additive) {

    Ray ray = screenToWorldRay(mouseX, mouseY, screenWidth, screenHeight, scene);
    float closestDistance = std::numeric_limits<float>::max();
    std::shared_ptr<Node> closestNode = nullptr;

    for (const auto& node : scene.sceneNodes) {
        auto result = rayIntersectMesh(ray, node);
        if (result.hit && result.distance < closestDistance) {
            closestDistance = result.distance;
            closestNode = node;
        }
    }

    // Handle selection based on additive flag
    if (!additive) {
        scene.clearSelection();
    }

    if (closestNode) {
        // If clicking an already selected node in additive mode, deselect it
        if (additive && scene.isNodeSelected(closestNode)) {
            scene.removeSelectedNode(closestNode);
        }
        else {
            scene.addSelectedNode(closestNode);
        }
    }

    return !scene.selectedNodes.empty();
}

void SelectionSystem::drawRay(Ray ray, float length, Scene& scene) {
    std::cout << "\n --- Drawing Ray --- \n";
    // Save current shader program
    GLint currentProgram;
    glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);

    // Disable the main shader and switch to a basic shader for lines
    glUseProgram(0);

    // Save current polygon mode
    GLint previousPolygonMode[2];
    glGetIntegerv(GL_POLYGON_MODE, previousPolygonMode);

    // Enable wireframe mode to call each drawWireframe
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // Set up matrices in fixed-function pipeline
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(glm::value_ptr(scene.activeCamera->getProjectionMatrix()));
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf(glm::value_ptr(scene.activeCamera->getViewMatrix()));

    glLineWidth(1.0f);  // Set line width

    glm::vec3 start = ray.origin;
    glm::vec3 end = start + ray.direction * length;

    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);  // Red ray
    glVertex3f(start.x, start.y, start.z);
    glVertex3f(end.x, end.y, end.z);
    glEnd();

    // Restore original states
    glPolygonMode(GL_FRONT, previousPolygonMode[0]);
    glPolygonMode(GL_BACK, previousPolygonMode[1]);

    // Restore previous shader program
    glUseProgram(currentProgram);
}

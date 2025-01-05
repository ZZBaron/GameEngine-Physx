#pragma once
#include "object3D.h"
#include "lineParameterization.h"


class TubeNode : public Node {
public:
    struct TubeParameters {
        int radialSegments = 16;    // Number of segments around the circumference
        int lengthSegments = 64;    // Number of segments along the tube length
        bool capEnds = true;        // Whether to cap the ends of the tube
    };

private:
    float radius;
    TubeParameters params;

    // Compute parallel transport frames along the curve
    std::vector<Frame> computeFrames(const std::vector<glm::vec3>& points) {
        std::vector<Frame> frames(points.size());

        if (points.size() < 2) return frames;

        // Initialize first frame
        Frame frame;
        glm::vec3 tangent = glm::normalize(points[1] - points[0]);

        // Choose initial normal perpendicular to tangent
        if (glm::abs(glm::dot(tangent, glm::vec3(0, 1, 0))) < 0.999f)
            frame.normal = glm::normalize(glm::cross(tangent, glm::vec3(0, 1, 0)));
        else
            frame.normal = glm::normalize(glm::cross(tangent, glm::vec3(1, 0, 0)));

        frame.binormal = glm::cross(tangent, frame.normal);
        frame.tangent = tangent;
        frames[0] = frame;

        // Propagate frame along curve using parallel transport
        for (size_t i = 1; i < points.size(); i++) {
            frames[i] = frames[i - 1];
            glm::vec3 newTangent;

            if (i < points.size() - 1)
                newTangent = glm::normalize(points[i + 1] - points[i]);
            else
                newTangent = glm::normalize(points[i] - points[i - 1]);

            frames[i].transport(newTangent);
        }

        return frames;
    }

    // Generate circle points in the normal-binormal plane
    std::vector<glm::vec3> generateCirclePoints(const Frame& frame, const glm::vec3& center) {
        std::vector<glm::vec3> circlePoints;
        float angleStep = 2.0f * glm::pi<float>() / params.radialSegments;

        for (int i = 0; i < params.radialSegments; ++i) {
            float angle = i * angleStep;
            float x = radius * cos(angle);
            float y = radius * sin(angle);
            glm::vec3 point = center + frame.normal * x + frame.binormal * y;
            circlePoints.push_back(point);
        }

        return circlePoints;
    }

    // Generate the tube mesh using parallel transport frames
    void generateTubeMesh(const std::vector<glm::vec3>& points) {
        mesh = std::make_shared<Mesh>();

        if (points.size() < 2) return;

        // Compute frames along the curve
        std::vector<Frame> frames = computeFrames(points);

        // Generate vertices for each segment
        for (size_t i = 0; i < points.size() - 1; ++i) {
            auto circle1 = generateCirclePoints(frames[i], points[i]);
            auto circle2 = generateCirclePoints(frames[i + 1], points[i + 1]);

            // Add vertices and attributes
            size_t baseIndex = mesh->positions.size();

            for (const auto& pos : circle1) {
                mesh->positions.push_back(pos);
                mesh->normals.push_back(glm::normalize(pos - points[i]));
                mesh->colors.push_back(glm::vec4(1.0f));
                mesh->uvSets["map1"].push_back(glm::vec2(
                    float(i) / (points.size() - 1),
                    0.0f
                ));
            }

            for (const auto& pos : circle2) {
                mesh->positions.push_back(pos);
                mesh->normals.push_back(glm::normalize(pos - points[i + 1]));
                mesh->colors.push_back(glm::vec4(1.0f));
                mesh->uvSets["map1"].push_back(glm::vec2(
                    float(i + 1) / (points.size() - 1),
                    1.0f
                ));
            }

            // Generate triangles
            for (int j = 0; j < params.radialSegments; ++j) {
                int next = (j + 1) % params.radialSegments;

                // First triangle
                mesh->indices.push_back(baseIndex + j);
                mesh->indices.push_back(baseIndex + params.radialSegments + j);
                mesh->indices.push_back(baseIndex + params.radialSegments + next);

                // Second triangle
                mesh->indices.push_back(baseIndex + j);
                mesh->indices.push_back(baseIndex + params.radialSegments + next);
                mesh->indices.push_back(baseIndex + next);
            }
        }

        if (params.capEnds) {
            generateEndCap(points.front(), -frames.front().tangent, frames.front(), true);
            generateEndCap(points.back(), frames.back().tangent, frames.back(), false);
        }

        mesh->setupBuffers();
    }

    // Generate end caps using the frame orientation
    void generateEndCap(const glm::vec3& center, const glm::vec3& normal, const Frame& frame, bool isStart) {
        auto circlePoints = generateCirclePoints(frame, center);
        size_t centerIndex = mesh->positions.size();

        // Add center vertex
        mesh->positions.push_back(center);
        mesh->normals.push_back(normal);
        mesh->colors.push_back(glm::vec4(1.0f));
        mesh->uvSets["map1"].push_back(glm::vec2(0.5f, 0.5f));

        // Add circle vertices
        size_t startIndex = mesh->positions.size();
        for (const auto& pos : circlePoints) {
            mesh->positions.push_back(pos);
            mesh->normals.push_back(normal);
            mesh->colors.push_back(glm::vec4(1.0f));

            glm::vec2 uv(
                0.5f + 0.5f * glm::normalize(pos - center).x,
                0.5f + 0.5f * glm::normalize(pos - center).y
            );
            mesh->uvSets["map1"].push_back(uv);
        }

        // Generate triangles
        for (int i = 0; i < params.radialSegments; ++i) {
            int next = (i + 1) % params.radialSegments;
            if (isStart) {
                mesh->indices.push_back(centerIndex);
                mesh->indices.push_back(startIndex + next);
                mesh->indices.push_back(startIndex + i);
            }
            else {
                mesh->indices.push_back(centerIndex);
                mesh->indices.push_back(startIndex + i);
                mesh->indices.push_back(startIndex + next);
            }
        }
    }

public:
    // Constructor using LineParameterization
    TubeNode(const LineParameterization& param, float tubeRadius, const TubeParameters& tubeParams = TubeParameters())
        : radius(tubeRadius), params(tubeParams) {
        std::vector<glm::vec3> points;
        float step = (param.getEnd() - param.getStart()) / params.lengthSegments;

        for (int i = 0; i <= params.lengthSegments; ++i) {
            float t = param.getStart() + i * step;
            points.push_back(param.evaluate(t));
        }

        generateTubeMesh(points);
    }

    // Constructor using vector of points
    TubeNode(const std::vector<glm::vec3>& points, float tubeRadius, const TubeParameters& tubeParams = TubeParameters())
        : radius(tubeRadius), params(tubeParams) {
        generateTubeMesh(points);
    }
};
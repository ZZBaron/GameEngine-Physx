// paths.h
#pragma once
#include <string>
#include "misc_funcs.h"

namespace Paths {
    namespace Shaders {
        const std::string vertexShader = getProjectRoot() + "/shaders/main_vertex_shader.glsl";
        const std::string fragmentShader = getProjectRoot() + "/shaders/main_fragment_shader.glsl";
        const std::string depthVertexShader = getProjectRoot() + "/shaders/depth_vertex_shader.glsl";
        const std::string depthFragmentShader = getProjectRoot() + "/shaders/depth_fragment_shader.glsl";
        const std::string backgroundVertexShader = getProjectRoot() + "/shaders/background_vertex.glsl";
        const std::string backgroundFragmentShader = getProjectRoot() + "/shaders/background_fragment.glsl";
        const std::string skyboxVertexShader = getProjectRoot() + "/shaders/skybox_vertex.glsl";
        const std::string skyboxFragmentShader = getProjectRoot() + "/shaders/skybox_fragment.glsl";
    }

    namespace Textures {
        const std::string skybox = getProjectRoot() + "/textures/skybox.png";
        const std::string defaultTexture = getProjectRoot() + "/textures/default.png";
    }

    namespace Models {
        const std::string cube = getProjectRoot() + "/models/cube.obj";
        const std::string sphere = getProjectRoot() + "/models/sphere.obj";
    }
}
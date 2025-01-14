// background_vertex.glsl
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;
out vec3 WorldPos;

uniform mat4 projection;
uniform mat4 view;

void main() {
    TexCoords = aTexCoords;

    // Remove translation from view matrix to make background static
    mat4 rotView = mat4(mat3(view));
    vec4 clipPos = projection * rotView * vec4(aPos, 1.0);

    // Ensure background is always at maximum depth
    gl_Position = clipPos.xyww;

    // Calculate world position for environment mapping
    WorldPos = aPos;
}
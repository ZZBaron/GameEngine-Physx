#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 5) in ivec4 aBoneIds;
layout(location = 6) in vec4 aBoneWeights;

uniform mat4 model;
uniform bool isAnimated;

const int MAX_BONES = 100;
uniform mat4 boneTransforms[MAX_BONES];

out vec4 FragPos; // Need world position for point light depth calculation

void main() {
    vec4 totalPosition = vec4(0.0);

    if (isAnimated) {
        for (int i = 0; i < 4; i++) {
            if (aBoneIds[i] != -1) {
                vec4 localPosition = boneTransforms[aBoneIds[i]] * vec4(aPos, 1.0);
                totalPosition += localPosition * aBoneWeights[i];
            }
        }
    }
    else {
        totalPosition = vec4(aPos, 1.0);
    }

    FragPos = model * totalPosition;
    gl_Position = FragPos;
}
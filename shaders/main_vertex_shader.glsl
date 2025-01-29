#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec2 aTexCoord;
layout(location = 4) in vec3 aTangent;

const int MAX_SPOT_LIGHTS = 4;

out vec3 FragPos;
out vec3 Normal;
out vec4 Color;
out vec2 TexCoord;
out vec4 FragPosLightSpace[MAX_SPOT_LIGHTS];
out vec3 Tangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


uniform mat4 spotLightSpaceMatrix[MAX_SPOT_LIGHTS];
uniform int numActiveSpotLights;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalMatrix * aNormal;
    Tangent = normalMatrix * aTangent;
    Color = aColor;
    TexCoord = aTexCoord;

    // Calculate light space positions for all active spot lights
    for (int i = 0; i < numActiveSpotLights && i < MAX_SPOT_LIGHTS; ++i) {
        FragPosLightSpace[i] = spotLightSpaceMatrix[i] * vec4(FragPos, 1.0);
    }

    gl_Position = projection * view * vec4(FragPos, 1.0);
}
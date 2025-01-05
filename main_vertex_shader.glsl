#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in vec2 aTexCoord;
layout(location = 4) in vec3 aTangent;

out vec3 FragPos;
out vec3 Normal;
out vec4 Color;
out vec2 TexCoord;
out vec4 FragPosLightSpace;
out vec3 Tangent;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix;

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    Normal = normalMatrix * aNormal;
    Tangent = normalMatrix * aTangent;
    Color = aColor;
    TexCoord = aTexCoord;
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0);
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
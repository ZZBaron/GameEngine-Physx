// background_fragment.glsl
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;

uniform int backgroundType;
uniform vec3 backgroundColor;
uniform float strength;
uniform sampler2D backgroundTexture;

void main() {
    vec3 color;

    if (backgroundType == 0) { // Color
        color = backgroundColor;
    }
    else if (backgroundType == 1) { // Image Texture
        color = texture(backgroundTexture, TexCoords).rgb;
    }
    else if (backgroundType == 2) { // Environment Texture
        // Convert world position to spherical coordinates for environment mapping
        vec3 normal = normalize(WorldPos);
        vec2 envCoord = vec2(
            0.5 + atan(normal.z, normal.x) / (2.0 * 3.1415926535897932384626433832795),
            0.5 - asin(normal.y) / 3.1415926535897932384626433832795
        );
        color = texture(backgroundTexture, envCoord).rgb;
    }
    else { // Sky Texture
        // Implement sky texture calculations here
        // This could include time of day, sun position, etc.
        vec3 normal = normalize(WorldPos);
        float height = normal.y;

        // Simple gradient from bottom to top
        vec3 zenithColor = texture(backgroundTexture, vec2(0.5, 1.0)).rgb;
        vec3 horizonColor = texture(backgroundTexture, vec2(0.5, 0.5)).rgb;
        vec3 groundColor = texture(backgroundTexture, vec2(0.5, 0.0)).rgb;

        if (height > 0.0) {
            color = mix(horizonColor, zenithColor, height);
        }
        else {
            color = mix(horizonColor, groundColor, -height);
        }
    }

    // Apply strength
    FragColor = vec4(color * strength, 1.0);

    //debug override
    FragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
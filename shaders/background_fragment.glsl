// background_fragment.glsl
#version 330 core
out vec4 FragColor;

in vec2 TexCoords;

uniform int backgroundType;
uniform vec3 backgroundColor;
uniform float strength;
uniform sampler2D backgroundTexture;

void main() {
    vec3 color;

    if (backgroundType == 0) { // Color
        color = backgroundColor;
    }
    else { // Texture
        color = texture(backgroundTexture, TexCoords).rgb;
    }

    // Apply strength
    FragColor = vec4(color * strength, 1.0);
}
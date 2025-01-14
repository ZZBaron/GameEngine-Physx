#version 330 core
out vec4 FragColor;
in vec3 localPos;

uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);

vec2 SampleSphericalMap(vec3 v) {
    // Convert direction vector to spherical coordinates
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));

    // Map spherical coordinates to UV coordinates
    uv *= invAtan;
    uv += 0.5;

    return uv;
}

void main() {
    // Get normalized sampling direction from local position
    vec3 direction = normalize(localPos);

    // Sample equirectangular texture using spherical mapping
    vec2 uv = SampleSphericalMap(direction);
    vec3 color = texture(equirectangularMap, uv).rgb;

    // Store the HDR color without any tonemapping
    FragColor = vec4(color, 1.0);
}
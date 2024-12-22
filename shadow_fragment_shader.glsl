#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;
in float FragTransparency;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float luminousPower;
uniform vec3 viewPos;
uniform vec3 objectColor;
uniform bool isEmissive;
uniform bool hasTexture;
uniform sampler2D textureSampler;
uniform sampler2D shadowMap;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Get closest depth value from light's perspective
    float closestDepth = texture(shadowMap, projCoords.xy).r;

    // Get current depth value
    float currentDepth = projCoords.z;

    // Calculate bias based on depth map resolution and slope
    vec3 normal = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float bias = max(0.05 * (1.0 - dot(normal, lightDir)), 0.005);

    // Check whether current frag pos is in shadow
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);

    // PCF filtering
    for (int x = -1; x <= 1; ++x)
    {
        for (int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    // Keep shadow at 0.0 when outside the far_plane region
    if (projCoords.z > 1.0)
        shadow = 0.0;

    return shadow;
}

void main()
{
    if (isEmissive) {
        FragColor = vec4(objectColor, FragTransparency);
        return;
    }

    // Get base color from texture or object color
    vec4 baseColor;
    if (hasTexture) {
        baseColor = texture(textureSampler, TexCoord) * vec4(objectColor, 1.0);
    }
    else {
        baseColor = vec4(objectColor, 1.0);
    }

    vec3 normal = normalize(Normal);
    vec3 adjustedLightColor = lightColor * luminousPower;

    // Ambient
    vec3 ambient = 0.15 * baseColor.rgb;

    // Diffuse
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * adjustedLightColor;

    // Specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 64.0);
    vec3 specular = spec * adjustedLightColor;

    // Shadow
    float shadow = ShadowCalculation(FragPosLightSpace);
    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * baseColor.rgb;

    FragColor = vec4(lighting, baseColor.a * FragTransparency);
}
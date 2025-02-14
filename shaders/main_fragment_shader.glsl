#version 330 core
out vec4 FragColor;

const int MAX_SPOT_LIGHTS = 4;
const float ambientFactor = 0.3;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace[MAX_SPOT_LIGHTS];
in vec3 Tangent;

// Texture projection modes
#define PROJECTION_FLAT 0
#define PROJECTION_BOX 1
#define PROJECTION_SPHERE 2
#define PROJECTION_TUBE 3

struct Material {
    // Base Properties
    vec3 baseColor;
    float subsurface;
    vec3 subsurfaceRadius;
    vec3 subsurfaceColor;
    float subsurfaceIOR;

    // Metallic Workflow
    float metallic;
    float specular;
    float specularTint;
    float roughness;

    // Anisotropic
    float anisotropic;
    float anisotropicRotation;

    // Sheen
    float sheen;
    float sheenTint;

    // Clearcoat
    float clearcoat;
    float clearcoatRoughness;
    float clearcoatIOR;

    // Transmission
    float transmission;
    float transmissionRoughness;
    float ior;

    // Emission
    vec3 emission;
    float emissionStrength;
    float alpha;

    // Textures
    sampler2D baseColorMap;
    sampler2D normalMap;
    sampler2D metallicMap;
    sampler2D roughnessMap;
    sampler2D emissionMap;
    sampler2D occlusionMap;
    sampler2D transmissionMap;

    // UV Transforms
    vec2 baseColorOffset;
    vec2 baseColorTiling;
    vec2 normalOffset;
    vec2 normalTiling;
    vec2 metallicOffset;
    vec2 metallicTiling;
    vec2 roughnessOffset;
    vec2 roughnessTiling;
    vec2 emissionOffset;
    vec2 emissionTiling;
    vec2 occlusionOffset;
    vec2 occlusionTiling;
    vec2 transmissionOffset;
    vec2 transmissionTiling;

    // Texture presence flags
    bool hasBaseColorMap;
    bool hasNormalMap;
    bool hasMetallicMap;
    bool hasRoughnessMap;
    bool hasEmissionMap;
    bool hasOcclusionMap;
    bool hasTransmissionMap;

    int baseColorProjection;
    int normalProjection;
    int metallicProjection;
    int roughnessProjection;
    int emissionProjection;
    int occlusionProjection;
    int transmissionProjection;

};

// Lights properties
struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float radius;
};

struct SpotLight {
    vec3 position;
    vec3 direction;
    vec3 color;
    float intensity;
    float constant;
    float linear;
    float quadratic;
    float innerCutoff;
    float outerCutoff;
};

// Light properties
uniform SpotLight spotLights[MAX_SPOT_LIGHTS];
uniform int numActiveSpotLights;

uniform vec3 viewPos;
uniform sampler2D shadowMaps[MAX_SPOT_LIGHTS];
uniform Material material;

const float PI = 3.14159265359;
const float EPSILON = 0.00001;

// GGX/Trowbridge-Reitz normal distribution function
float D_GGX(float NoH, float roughness) {
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float NoH2 = NoH * NoH;
    float denom = NoH2 * (alpha2 - 1.0) + 1.0;
    return alpha2 / (PI * denom * denom);
}

// Schlick's approximation for Fresnel
vec3 F_Schlick(float cosTheta, vec3 F0, float roughness) {
    float roughnessFactor = 1.0 - roughness;
    return F0 + (max(vec3(roughnessFactor), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// Smith's method for geometric shadowing/masking
float G_Smith(float NoV, float NoL, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;
    float ggx1 = NoV / (NoV * (1.0 - k) + k);
    float ggx2 = NoL / (NoL * (1.0 - k) + k);
    return ggx1 * ggx2;
}

// Disney diffuse term
vec3 DisneyDiffuse(vec3 baseColor, float roughness, float NoV, float NoL, float VoH, float metallic) {
    float FD90 = 0.5 + 2.0 * VoH * VoH * roughness;
    float FdV = 1.0 + (FD90 - 1.0) * pow(1.0 - NoV, 5.0);
    float FdL = 1.0 + (FD90 - 1.0) * pow(1.0 - NoL, 5.0);
    return baseColor * ((1.0 / PI) * FdV * FdL) * (1.0 - metallic);
}

// Anisotropic GGX
float D_GGX_Anisotropic(float NoH, float HoX, float HoY, float ax, float ay) {
    float d = HoX * HoX / (ax * ax) + HoY * HoY / (ay * ay) + NoH * NoH;
    return 1.0 / (PI * ax * ay * d * d);
}

// Subsurface scattering approximation
vec3 SubsurfaceScattering(vec3 L, vec3 V, vec3 N, vec3 baseColor, float subsurface, vec3 subsurfaceColor, vec3 subsurfaceRadius) {
    vec3 scatterDir = normalize(L + N * subsurfaceRadius);
    float scatterVoL = max(0.0, dot(V, -scatterDir));
    float forwardScatter = exp(-subsurface * scatterVoL);
    float backScatter = exp(-subsurface * (1.0 - scatterVoL));
    return subsurfaceColor * baseColor * (forwardScatter + backScatter);
}

// Sheen distribution
float D_Charlie(float roughness, float NoH) {
    float alpha = roughness * roughness;
    float invAlpha = 1.0 / alpha;
    float cos2h = NoH * NoH;
    float sin2h = max(1.0 - cos2h, 0.0078125);
    return (2.0 + invAlpha) * pow(sin2h, invAlpha * 0.5) / (2.0 * PI);
}

vec2 getProjectedUV(vec2 baseUV, int projectionType, vec3 worldPos, vec3 normal) {
    vec2 projectedUV = baseUV;
    vec3 nrm;  // Single declaration here

    switch (projectionType) {
    case PROJECTION_BOX:
        vec3 absNormal = abs(normal);
        if (absNormal.x > absNormal.y && absNormal.x > absNormal.z) {
            projectedUV = worldPos.zy;
        }
        else if (absNormal.y > absNormal.z) {
            projectedUV = worldPos.xz;
        }
        else {
            projectedUV = worldPos.xy;
        }
        break;

    case PROJECTION_SPHERE:
        nrm = normalize(normal);  // Use the single declaration
        projectedUV = vec2(
            0.5 + atan(nrm.z, nrm.x) / (2.0 * PI),
            0.5 - asin(nrm.y) / PI
        );
        break;

    case PROJECTION_TUBE:
        nrm = normalize(normal);  // Use the same nrm variable
        projectedUV = vec2(
            0.5 + atan(nrm.z, nrm.x) / (2.0 * PI),
            worldPos.y
        );
        break;
    }

    return projectedUV;
}

float calcSpotLightShadow(sampler2D shadowMap, vec4 fragPosLightSpace) {
    // Perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // Check if fragment is in bounds
    if (projCoords.z > 1.0) return 0.0;

    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // PCF Sampling for softer shadows
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - 0.005 > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;
    return shadow;
}

vec3 calcSpotLight(SpotLight light, vec3 normal, vec3 fragPos, vec3 viewDir, int lightIndex, vec3 albedo) {
    vec3 lightDir = normalize(light.position - fragPos);

    // Spot light cone calculation
    float theta = dot(lightDir, normalize(-light.direction));
    float epsilon = light.innerCutoff - light.outerCutoff;
    float intensity = clamp((theta - light.outerCutoff) / epsilon, 0.0, 1.0);

    // If outside the cone, return no light
    if (theta < light.outerCutoff) return vec3(0.0);



    // Calculate attenuation
    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * distance * distance);

    // Calculate shadow
    float shadow = calcSpotLightShadow(shadowMaps[lightIndex], FragPosLightSpace[lightIndex]);

    // Calculate standard vectors and dot products
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float NoV = max(dot(normal, viewDir), EPSILON);
    float NoL = max(dot(normal, lightDir), EPSILON);
    float NoH = max(dot(normal, halfwayDir), EPSILON);
    float VoH = max(dot(viewDir, halfwayDir), EPSILON);
    float LoH = max(dot(lightDir, halfwayDir), EPSILON);

    // Base material properties
    vec3 F0 = mix(vec3(0.04), albedo, material.metallic);

    // Standard PBR terms
    float D = D_GGX(NoH, material.roughness);
    vec3 F = F_Schlick(VoH, F0, material.roughness);
    float G = G_Smith(NoV, NoL, material.roughness);
    vec3 specular = (D * F * G) / max(4.0 * NoV * NoL, EPSILON);

    // Diffuse term with subsurface scattering
    vec3 diffuse = DisneyDiffuse(albedo, material.roughness, NoV, NoL, VoH, material.metallic);
    if (material.subsurface > 0.0) {
        vec3 subsurfaceColor = SubsurfaceScattering(lightDir, viewDir, normal,
            albedo, material.subsurface,
            material.subsurfaceColor, material.subsurfaceRadius);
        diffuse = mix(diffuse, subsurfaceColor, material.subsurface);
    }

    // Sheen term
    vec3 sheenColor = mix(vec3(1.0), albedo, material.sheenTint);
    float sheenDistribution = D_Charlie(material.roughness, NoH);
    vec3 sheenSpecular = material.sheen * sheenColor * sheenDistribution;

    // Clearcoat term
    float Dr = D_GGX(NoH, material.clearcoatRoughness);
    float Fr = F_Schlick(VoH, vec3(0.04), 0.0).r;
    float Gr = G_Smith(NoV, NoL, material.clearcoatRoughness);
    float clearcoatSpecular = material.clearcoat * (Dr * Fr * Gr) / max(4.0 * NoV * NoL, EPSILON);

    // Transmission term
    vec3 transmission = vec3(0.0);
    if (material.transmission > 0.0) {
        vec3 transmissionRay = refract(-viewDir, normal, 1.0 / material.ior);
        // Simple approximation of transmission without ray tracing
        transmission = albedo * (1.0 - F) * material.transmission;
    }

    // Combine all lighting components
    vec3 radiance = light.color * light.intensity * attenuation * intensity;
    vec3 directLighting = (1.0 - shadow) * radiance * NoL * (
        diffuse +
        specular +
        sheenSpecular +
        vec3(clearcoatSpecular)
        );

    return directLighting + transmission;
}


void main() {
    // Sample all textures with their transforms
    vec3 albedo = vec3(1.0);
    if (material.hasBaseColorMap) {
        vec2 baseColorUV = getProjectedUV(TexCoord, material.baseColorProjection, FragPos, Normal);
        baseColorUV = (baseColorUV * material.baseColorTiling) + material.baseColorOffset;
        albedo *= texture(material.baseColorMap, baseColorUV).rgb;
    }
    else {
        albedo = material.baseColor;
    }

    // Normal mapping
    vec3 N = normalize(Normal);
    vec3 T = normalize(Tangent);
    vec3 B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);
    vec3 V = normalize(viewPos - FragPos);

    // Sample normal map with projection
    if (material.hasNormalMap) {
        vec2 normalUV = getProjectedUV(TexCoord, material.normalProjection, FragPos, Normal);
        normalUV = (normalUV * material.normalTiling) + material.normalOffset;
        vec3 tangentNormal = texture(material.normalMap, normalUV).xyz * 2.0 - 1.0;
        N = normalize(TBN * tangentNormal);
    }

    // Sample metallic with projection
    float metallic = material.metallic;
    if (material.hasMetallicMap) {
        vec2 metallicUV = getProjectedUV(TexCoord, material.metallicProjection, FragPos, Normal);
        metallicUV = (metallicUV * material.metallicTiling) + material.metallicOffset;
        metallic *= texture(material.metallicMap, metallicUV).r;
    }

    // Sample roughness with projection
    float roughness = material.roughness;
    if (material.hasRoughnessMap) {
        vec2 roughnessUV = getProjectedUV(TexCoord, material.roughnessProjection, FragPos, Normal);
        roughnessUV = (roughnessUV * material.roughnessTiling) + material.roughnessOffset;
        roughness *= texture(material.roughnessMap, roughnessUV).r;
    }

    // Sample emission with projection
    vec3 emission = material.emission;
    if (material.hasEmissionMap) {
        vec2 emissionUV = getProjectedUV(TexCoord, material.emissionProjection, FragPos, Normal);
        emissionUV = (emissionUV * material.emissionTiling) + material.emissionOffset;
        emission *= texture(material.emissionMap, emissionUV).rgb;
    }

    // Sample occlusion with projection
    float occlusion = 1.0;
    if (material.hasOcclusionMap) {
        vec2 occlusionUV = getProjectedUV(TexCoord, material.occlusionProjection, FragPos, Normal);
        occlusionUV = (occlusionUV * material.occlusionTiling) + material.occlusionOffset;
        occlusion = texture(material.occlusionMap, occlusionUV).r;
    }

    // Sample transmission with projection
    float transmission = material.transmission;
    if (material.hasTransmissionMap) {
        vec2 transmissionUV = getProjectedUV(TexCoord, material.transmissionProjection, FragPos, Normal);
        transmissionUV = (transmissionUV * material.transmissionTiling) + material.transmissionOffset;
        transmission *= texture(material.transmissionMap, transmissionUV).r;
    }


    // Accumulate light from all spot lights
    vec3 lighting = vec3(0.0);
    for (int i = 0; i < numActiveSpotLights && i < MAX_SPOT_LIGHTS; ++i) {
        lighting += calcSpotLight(spotLights[i], N, FragPos, V, i, albedo);
    }

    // Combine all lighting components
    vec3 ambient = albedo * ambientFactor;
    vec3 color = ambient + lighting;



    // Add emission
    if (material.hasEmissionMap) {
        vec2 emissionUV = (TexCoord * material.emissionTiling) + material.emissionOffset;
        color += material.emission * material.emissionStrength * texture(material.emissionMap, emissionUV).rgb;
    }
    else {
        color += material.emission * material.emissionStrength;
    }


    // Tone mapping (ACES approximation)
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    color = clamp((color * (a * color + b)) / (color * (c * color + d) + e), 0.0, 1.0);

    // Gamma correction
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, material.alpha);


    //FragColor = vec4(fract(TexCoord), 0.0, 1.0); // This will show tiling patterns
    // debug override
    //FragColor = vec4(material.baseColor, 1.0);
    
}
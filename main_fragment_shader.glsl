#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;
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

// Light properties
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform float luminousPower;
uniform vec3 viewPos;
uniform sampler2D shadowMap;
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


void main() {
    // Sample all textures with their transforms
    vec3 albedo = material.baseColor;
    if (material.hasBaseColorMap) {
        vec2 baseColorUV = getProjectedUV(TexCoord, material.baseColorProjection, FragPos, Normal);
        baseColorUV = (baseColorUV * material.baseColorTiling) + material.baseColorOffset;
        albedo *= texture(material.baseColorMap, baseColorUV).rgb;
    }

    // Normal mapping
    vec3 N = normalize(Normal);
    vec3 T = normalize(Tangent);
    vec3 B = normalize(cross(N, T));
    mat3 TBN = mat3(T, B, N);

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



    // Prepare vectors and dot products
    vec3 V = normalize(viewPos - FragPos);
    vec3 L = normalize(lightPos - FragPos);
    vec3 H = normalize(V + L);
    vec3 R = reflect(-L, N);

    float NoV = max(dot(N, V), EPSILON);
    float NoL = max(dot(N, L), EPSILON);
    float NoH = max(dot(N, H), EPSILON);
    float VoH = max(dot(V, H), EPSILON);
    float LoH = max(dot(L, H), EPSILON);

    // Anisotropic calculations
    vec3 X = T;
    vec3 Y = B;
    float HoX = dot(H, X);
    float HoY = dot(H, Y);
    float ax = max(0.001, roughness * (1.0 + material.anisotropic));
    float ay = max(0.001, roughness * (1.0 - material.anisotropic));

    // Calculate base F0 with metallic workflow
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // Specular BRDF
    float D = material.anisotropic > 0.0 ?
        D_GGX_Anisotropic(NoH, HoX, HoY, ax, ay) :
        D_GGX(NoH, roughness);
    vec3 F = F_Schlick(VoH, F0, roughness);
    float G = G_Smith(NoV, NoL, roughness);
    vec3 specular = (D * F * G) / max(4.0 * NoV * NoL, EPSILON);

    // Diffuse BRDF
    vec3 diffuse = DisneyDiffuse(albedo, roughness, NoV, NoL, VoH, metallic);

    // Subsurface scattering
    vec3 subsurface = SubsurfaceScattering(L, V, N, albedo, material.subsurface,
        material.subsurfaceColor, material.subsurfaceRadius);

    // Sheen
    vec3 sheenColor = mix(vec3(1.0), albedo, material.sheenTint);
    float sheenDistribution = D_Charlie(roughness, NoH);
    vec3 sheenSpecular = material.sheen * sheenColor * sheenDistribution;

    // Clearcoat
    float Dr = D_GGX(NoH, material.clearcoatRoughness);
    float Fr = F_Schlick(VoH, vec3(0.04), 0.0).r;
    float Gr = G_Smith(NoV, NoL, material.clearcoatRoughness);
    float clearcoatSpecular = material.clearcoat * (Dr * Fr * Gr) / max(4.0 * NoV * NoL, EPSILON);

    // Calculate shadow
    float shadow = 0.0;
    vec3 projCoords = FragPosLightSpace.xyz / FragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z <= 1.0) {
        float currentDepth = projCoords.z;
        float bias = max(0.05 * (1.0 - NoL), 0.005);

        vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
        for (int x = -2; x <= 2; ++x) {
            for (int y = -2; y <= 2; ++y) {
                float pcfDepth = texture(shadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
                shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
            }
        }
        shadow /= 25.0;
    }

    // Calculate light attenuation and radiance
    float distance = length(lightPos - FragPos);
    float attenuation = luminousPower / (distance * distance);
    vec3 radiance = lightColor * attenuation;

    // Combine all lighting components
    vec3 ambient = albedo * 0.03 * occlusion;
    vec3 directLight = (1.0 - shadow) * radiance * NoL;

    vec3 color = ambient + directLight * (
        diffuse +
        specular +
        subsurface +
        sheenSpecular +
        vec3(clearcoatSpecular)
        );

    // Add emission
    if (material.hasEmissionMap) {
        vec2 emissionUV = (TexCoord * material.emissionTiling) + material.emissionOffset;
        color += material.emission * material.emissionStrength * texture(material.emissionMap, emissionUV).rgb;
    }
    else {
        color += material.emission * material.emissionStrength;
    }

    // Handle transmission
    if (transmission > 0.0) {
        vec3 transmissionColor = albedo;
        float transmissionRoughness = material.transmissionRoughness;
        vec3 transmissionRay = refract(-V, N, 1.0 / material.ior);
        // Simple approximation of transmission without ray tracing
        vec3 transmissionContribution = transmissionColor * (1.0 - F) * transmission;
        color = mix(color, transmissionContribution, transmission);
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
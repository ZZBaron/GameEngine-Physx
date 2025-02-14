// sky.h
#pragma once
#include "GameEngine.h"
#include "shader.h"
#include "background.h"

class SkyBackground : public Background {
private:
    // Nishita model parameters
    struct SkyParams {
        float turbidity = 3.0f;     // Range: 2-10
        float groundAlbedo = 0.1f;  // Range: 0-1
        float sunSize = 3.0f;       // Angular diameter in degrees
        float sunIntensity = 1.0f;  // Multiplier for sun disc
        float sunElevation = 45.0f; // Degrees from horizon
        float sunRotation = 0.0f;   // Degrees around zenith
        float altitude = 0.0f;      // km from sea level
        float airDensity = 1.0f;    // Range: 0-2
        float dustDensity = 1.0f;   // Range: 0-10
        float ozoneDensity = 1.0f;  // Range: 0-2
        bool enableSunDisc = true;
    };

    SkyParams params;
    GLuint skyShaderProgram;
    GLuint environmentMapFBO;
    GLuint environmentMap;

    void initializeSkyShader() {
        // Load and compile sky shader
        const char* skyVertexShader = R"(
            #version 330 core
            layout (location = 0) in vec3 aPos;
            layout (location = 1) in vec2 aTexCoords;
            
            out vec3 worldPos;
            out vec3 viewDir;
            
            uniform mat4 projection;
            uniform mat4 view;
            
            void main() {
                gl_Position = vec4(aPos, 1.0);
                mat4 viewRotation = mat4(mat3(view)); // Remove translation
                vec4 clipPos = inverse(projection * viewRotation) * vec4(aPos, 1.0);
                worldPos = clipPos.xyz / clipPos.w;
                viewDir = normalize(worldPos);
            }
        )";

        const char* skyFragmentShader = R"(
            #version 330 core
            out vec4 FragColor;
            
            in vec3 worldPos;
            in vec3 viewDir;
            
            uniform vec3 sunDirection;
            uniform float turbidity;
            uniform float groundAlbedo;
            uniform float sunSize;
            uniform float sunIntensity;
            uniform float altitude;
            uniform float airDensity;
            uniform float dustDensity;
            uniform float ozoneDensity;
            uniform bool enableSunDisc;

            const float PI = 3.14159265359;
            
            // Rayleigh scattering coefficients
            const vec3 betaR = vec3(5.8e-6, 13.5e-6, 33.1e-6);
            
            // Mie scattering coefficients
            const vec3 betaM = vec3(2.1e-5);
            
            float rayleighPhase(float cosTheta) {
                return 3.0 / (16.0 * PI) * (1.0 + cosTheta * cosTheta);
            }
            
            float miePhase(float cosTheta, float g) {
                float g2 = g * g;
                return 3.0 / (8.0 * PI) * ((1.0 - g2) * (1.0 + cosTheta * cosTheta)) / 
                       (pow(1.0 + g2 - 2.0 * g * cosTheta, 1.5) * (2.0 + g2));
            }
            
            vec3 calculateScattering(vec3 viewDir) {
                float cosTheta = dot(viewDir, sunDirection);
                float sunAngularDiameter = sunSize * PI / 180.0;
                
                // Calculate Rayleigh and Mie scattering
                vec3 rayleigh = betaR * rayleighPhase(cosTheta) * airDensity;
                vec3 mie = betaM * miePhase(cosTheta, 0.76) * dustDensity;
                
                // Apply ozone absorption
                vec3 ozone = vec3(0.650, 1.881, 0.085) * ozoneDensity;
                
                // Calculate optical depth based on view angle
                float zenithAngle = acos(max(0.0, dot(vec3(0, 1, 0), viewDir)));
                float opticalDepth = 1.0 / cos(zenithAngle);
                
                // Apply turbidity
                vec3 extinction = exp(-(rayleigh + mie + ozone) * opticalDepth * turbidity);
                
                // Calculate sun disc
                float sunDisc = 0.0;
                if (enableSunDisc && cosTheta > cos(sunAngularDiameter)) {
                    sunDisc = sunIntensity;
                }
                
                // Final color
                vec3 skyColor = (rayleigh + mie) * (1.0 - extinction) + vec3(sunDisc);
                
                // Apply ground albedo influence
                float horizonAngle = max(0.0, dot(viewDir, vec3(0, 1, 0)));
                skyColor += groundAlbedo * (1.0 - horizonAngle) * 0.2;
                
                // Apply altitude effect
                float altitudeFactor = exp(-altitude / 8.0);
                skyColor *= mix(1.0, 0.2, altitudeFactor);
                
                return skyColor;
            }
            
            void main() {
                vec3 color = calculateScattering(normalize(viewDir));
                
                // Tone mapping and gamma correction
                color = color / (1.0 + color); // Simple Reinhard tone mapping
                color = pow(color, vec3(1.0 / 2.2)); // Gamma correction
                
                FragColor = vec4(color, 1.0);
            }
        )";

        Shader skyShader(skyVertexShader, skyFragmentShader, true);
        skyShaderProgram = skyShader.getShaderProgram();
    }

    void generateEnvironmentMap() {
        glGenFramebuffers(1, &environmentMapFBO);
        glBindFramebuffer(GL_FRAMEBUFFER, environmentMapFBO);

        glGenTextures(1, &environmentMap);
        glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMap);
        for (unsigned int i = 0; i < 6; ++i) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

public:
    SkyBackground() {
        setType(BackgroundType::SkyTexture);
    }

    void setup() override {
        initializeSkyShader();
        setupQuad();
        generateEnvironmentMap();
    }

    void setSunDirection(float elevation, float rotation) {
        params.sunElevation = elevation;
        params.sunRotation = rotation;
    }

    void setAtmosphereParams(float turbidity, float airDensity, float dustDensity, float ozoneDensity) {
        params.turbidity = turbidity;
        params.airDensity = airDensity;
        params.dustDensity = dustDensity;
        params.ozoneDensity = ozoneDensity;
    }

    void setSunParams(float size, float intensity, bool enableDisc) {
        params.sunSize = size;
        params.sunIntensity = intensity;
        params.enableSunDisc = enableDisc;
    }

    void setGroundAlbedo(float albedo) {
        params.groundAlbedo = albedo;
    }

    void setAltitude(float km) {
        params.altitude = glm::clamp(km, 0.0f, 60.0f);
    }

    void render(const glm::mat4& view, const glm::mat4& projection) override {
        glDepthFunc(GL_LEQUAL);
        glUseProgram(skyShaderProgram);

        // Calculate sun direction from elevation and rotation
        float elevRad = glm::radians(params.sunElevation);
        float rotRad = glm::radians(params.sunRotation);
        glm::vec3 sunDir(
            cos(elevRad) * sin(rotRad),
            sin(elevRad),
            cos(elevRad) * cos(rotRad)
        );

        // Set uniforms
        glUniformMatrix4fv(glGetUniformLocation(skyShaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(skyShaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(skyShaderProgram, "sunDirection"), 1, glm::value_ptr(sunDir));
        glUniform1f(glGetUniformLocation(skyShaderProgram, "turbidity"), params.turbidity);
        glUniform1f(glGetUniformLocation(skyShaderProgram, "groundAlbedo"), params.groundAlbedo);
        glUniform1f(glGetUniformLocation(skyShaderProgram, "sunSize"), params.sunSize);
        glUniform1f(glGetUniformLocation(skyShaderProgram, "sunIntensity"), params.sunIntensity);
        glUniform1f(glGetUniformLocation(skyShaderProgram, "altitude"), params.altitude);
        glUniform1f(glGetUniformLocation(skyShaderProgram, "airDensity"), params.airDensity);
        glUniform1f(glGetUniformLocation(skyShaderProgram, "dustDensity"), params.dustDensity);
        glUniform1f(glGetUniformLocation(skyShaderProgram, "ozoneDensity"), params.ozoneDensity);
        glUniform1i(glGetUniformLocation(skyShaderProgram, "enableSunDisc"), params.enableSunDisc);

        // Render sky quad
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        glDepthFunc(GL_LESS);
    }
};
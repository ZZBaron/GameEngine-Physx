// ShadowMap.h
#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>

class ShadowMap {
public:
    GLuint depthMapFBO;
    GLuint depthMap;
    unsigned int shadowWidth, shadowHeight;

    ShadowMap(unsigned int width = 2048, unsigned int height = 2048)
        : shadowWidth(width), shadowHeight(height) {
    }

    void initialize() {

        // Verify OpenGL context/state
        if (!glGetString(GL_VERSION)) {
            std::cerr << "Error: Attempting to initialize ShadowMap without OpenGL context\n";
            return;
        }

        //depth map setup
        glGenFramebuffers(1, &depthMapFBO);
        glGenTextures(1, &depthMap);

        glBindTexture(GL_TEXTURE_2D, depthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
            shadowWidth, shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        // Configure texture parameters
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void bindForWriting() {
        glViewport(0, 0, shadowWidth, shadowHeight);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
    }

    void bindForReading(GLuint textureUnit = GL_TEXTURE1) {
        glActiveTexture(textureUnit);
        glBindTexture(GL_TEXTURE_2D, depthMap);
    }
};
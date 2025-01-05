// textureManager.h
#pragma once
#include "GameEngine.h"
#include <stb_image.h>
#include <unordered_map>
#include <filesystem>

struct TextureInfo {
    GLuint id;
    int width;
    int height;
    int channels;
    std::string type;
};

class TextureManager {
private:
    static TextureManager* instance;
    std::unordered_map<std::string, TextureInfo> textureCache;
    std::string lastError;

    TextureManager() {
        CreateDefaultTextures();
    }

    void CreateDefaultTextures() {
        // White texture (default diffuse)
        GLuint whiteTexture;
        glGenTextures(1, &whiteTexture);
        glBindTexture(GL_TEXTURE_2D, whiteTexture);
        unsigned char white[] = { 255, 255, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
        SetDefaultTextureParams();
        textureCache["default_white"] = { whiteTexture, 1, 1, 4, "diffuse" };

        // Normal map (flat surface)
        GLuint normalTexture;
        glGenTextures(1, &normalTexture);
        glBindTexture(GL_TEXTURE_2D, normalTexture);
        unsigned char normal[] = { 128, 128, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, normal);
        SetDefaultTextureParams();
        textureCache["default_normal"] = { normalTexture, 1, 1, 4, "normal" };

        // Black texture (default specular)
        GLuint blackTexture;
        glGenTextures(1, &blackTexture);
        glBindTexture(GL_TEXTURE_2D, blackTexture);
        unsigned char black[] = { 0, 0, 0, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
        SetDefaultTextureParams();
        textureCache["default_black"] = { blackTexture, 1, 1, 4, "specular" };
    }

    void SetDefaultTextureParams() {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

public:
    static TextureManager& getInstance() {
        if (instance == nullptr) {
            instance = new TextureManager();
        }
        return *instance;
    }

    GLuint LoadFromMemory(unsigned char* data, size_t size, const char* formatHint) {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        int width, height, channels;
        unsigned char* imageData = nullptr;

        stbi_set_flip_vertically_on_load(true);
        imageData = stbi_load_from_memory(data, static_cast<int>(size),
            &width, &height, &channels, 0);

        if (!imageData) {
            lastError = "Failed to load embedded texture";
            return GetDefaultTexture();
        }

        GLenum format;
        GLenum internalFormat;
        switch (channels) {
        case 1: format = GL_RED;  internalFormat = GL_RED; break;
        case 3: format = GL_RGB;  internalFormat = GL_SRGB; break;
        case 4: format = GL_RGBA; internalFormat = GL_SRGB_ALPHA; break;
        default: format = GL_RGB; internalFormat = GL_SRGB; break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);
        glGenerateMipmap(GL_TEXTURE_2D);

        SetDefaultTextureParams();
        stbi_image_free(imageData);

        // Cache with unique identifier
        std::string cacheKey = "embedded_" + std::to_string(size);
        textureCache[cacheKey] = { textureID, width, height, channels, "embedded" };

        return textureID;
    }

    GLuint LoadTexture(const std::string& path, const std::string& type = "diffuse") {
        // Return cached if exists
        auto it = textureCache.find(path);
        if (it != textureCache.end()) {
            return it->second.id;
        }

        // Check file exists
        if (!std::filesystem::exists(path)) {
            std::cout << "Texture not found: " << path << std::endl;
            return GetDefaultTexture(type);
        }

        // Load texture data
        int width, height, channels;
        stbi_set_flip_vertically_on_load(true);
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);

        if (!data) {
            lastError = "Failed to load texture: " + path;
            return GetDefaultTexture(type);
        }

        // Create OpenGL texture
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Set format based on channels
        GLenum format;
        GLenum internalFormat;
        switch (channels) {
        case 1: format = GL_RED;  internalFormat = GL_RED; break;
        case 3: format = GL_RGB;  internalFormat = (type == "normal") ? GL_RGB : GL_SRGB; break;
        case 4: format = GL_RGBA; internalFormat = (type == "normal") ? GL_RGBA : GL_SRGB_ALPHA; break;
        default: format = GL_RGB; internalFormat = GL_SRGB; break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        // Set parameters
        SetDefaultTextureParams();

        // Cache texture
        textureCache[path] = { textureID, width, height, channels, type };

        stbi_image_free(data);
        return textureID;
    }

    GLuint GetDefaultTexture(const std::string& type = "diffuse") {
        if (type == "normal") return textureCache["default_normal"].id;
        if (type == "specular") return textureCache["default_black"].id;
        return textureCache["default_white"].id;
    }

    const TextureInfo& GetTextureInfo(const std::string& path) {
        auto it = textureCache.find(path);
        if (it != textureCache.end()) {
            return it->second;
        }
        return textureCache["default_white"];
    }

    void UnloadTexture(const std::string& path) {
        auto it = textureCache.find(path);
        if (it != textureCache.end() && !path.starts_with("default_")) {
            glDeleteTextures(1, &it->second.id);
            textureCache.erase(it);
        }
    }

    void UnloadAll() {
        for (const auto& [path, info] : textureCache) {
            if (!path.starts_with("default_")) {
                glDeleteTextures(1, &info.id);
            }
        }
        auto defaults = {
            textureCache["default_white"],
            textureCache["default_normal"],
            textureCache["default_black"]
        };
        textureCache.clear();
        textureCache["default_white"] = defaults.begin()[0];
        textureCache["default_normal"] = defaults.begin()[1];
        textureCache["default_black"] = defaults.begin()[2];
    }

    const std::string& GetLastError() const { return lastError; }

    ~TextureManager() {
        UnloadAll();
        for (const auto& [path, info] : textureCache) {
            glDeleteTextures(1, &info.id);
        }
    }
};

// Initialize static member
TextureManager* TextureManager::instance = nullptr;
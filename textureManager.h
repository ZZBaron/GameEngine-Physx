#pragma once
#include "GameEngine.h"
#include "object3D.h"
#include <stb_image.h>
#include <unordered_map>
#include <filesystem>

struct TextureInfo {
    GLuint id;
    int width;
    int height;
    int channels;
    std::string type;
    Material::TextureMap settings;  // Store texture settings with the texture
};

class TextureManager {
private:
    inline static TextureManager* instance;
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
        SetDefaultTextureParams(Material::TextureMap());
        textureCache["default_white"] = { whiteTexture, 1, 1, 4, "diffuse" };

        // Normal map (flat surface)
        GLuint normalTexture;
        glGenTextures(1, &normalTexture);
        glBindTexture(GL_TEXTURE_2D, normalTexture);
        unsigned char normal[] = { 128, 128, 255, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, normal);
        SetDefaultTextureParams(Material::TextureMap());
        textureCache["default_normal"] = { normalTexture, 1, 1, 4, "normal" };

        // Black texture (default specular)
        GLuint blackTexture;
        glGenTextures(1, &blackTexture);
        glBindTexture(GL_TEXTURE_2D, blackTexture);
        unsigned char black[] = { 0, 0, 0, 255 };
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
        SetDefaultTextureParams(Material::TextureMap());
        textureCache["default_black"] = { blackTexture, 1, 1, 4, "specular" };
    }

    void SetDefaultTextureParams(const Material::TextureMap& settings) {
        // Handle interpolation
        GLint minFilter, magFilter;
        switch (settings.interpolation) {
        case Material::TextureMap::Interpolation::Closest:
            minFilter = GL_NEAREST_MIPMAP_NEAREST;
            magFilter = GL_NEAREST;
            break;
        case Material::TextureMap::Interpolation::Cubic:
            minFilter = GL_LINEAR_MIPMAP_LINEAR;
            magFilter = GL_LINEAR;
            break;
        case Material::TextureMap::Interpolation::Linear:
        default:
            minFilter = GL_LINEAR_MIPMAP_LINEAR;
            magFilter = GL_LINEAR;
            break;
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

        // Handle extension (wrapping)
        GLint wrap;
        switch (settings.extension) {
        case Material::TextureMap::Extension::Extend:
            wrap = GL_CLAMP_TO_EDGE;
            break;
        case Material::TextureMap::Extension::Clip:
            wrap = GL_CLAMP_TO_BORDER;
            break;
        case Material::TextureMap::Extension::Repeat:
        default:
            wrap = GL_REPEAT;
            break;
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

        // Set anisotropic filtering if available
        float maxAniso;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
    }

    void ProcessImageData(unsigned char* data, int width, int height, int channels,
        const Material::TextureMap& settings) {
        if (!data) return;

        // Handle alpha mode for RGBA textures
        if (channels == 4 && settings.alphaMode == Material::TextureMap::AlphaMode::Premultiplied) {
            for (int i = 0; i < width * height; i++) {
                float alpha = data[i * 4 + 3] / 255.0f;
                data[i * 4 + 0] = static_cast<unsigned char>(data[i * 4 + 0] * alpha);
                data[i * 4 + 1] = static_cast<unsigned char>(data[i * 4 + 1] * alpha);
                data[i * 4 + 2] = static_cast<unsigned char>(data[i * 4 + 2] * alpha);
            }
        }
    }

public:
    static TextureManager& getInstance() {
        if (instance == nullptr) {
            instance = new TextureManager();
        }
        return *instance;
    }

    GLuint LoadFromMemory(unsigned char* data, size_t size, const char* formatHint,
        const Material::TextureMap& settings = Material::TextureMap()) {
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        int width, height, channels;
        unsigned char* imageData = nullptr;

        stbi_set_flip_vertically_on_load(true);
        imageData = stbi_load_from_memory(data, static_cast<int>(size), &width, &height, &channels, 0);

        if (!imageData) {
            lastError = "Failed to load embedded texture";
            return GetDefaultTexture();
        }

        ProcessImageData(imageData, width, height, channels, settings);

        // Determine format based on color space and channels
        GLenum format, internalFormat;
        switch (channels) {
        case 1:
            format = GL_RED;
            internalFormat = GL_RED;
            break;
        case 3:
            format = GL_RGB;
            internalFormat = settings.colorSpace == Material::TextureMap::ColorSpace::sRGB ?
                GL_SRGB8 : GL_RGB8;
            break;
        case 4:
            format = GL_RGBA;
            internalFormat = settings.colorSpace == Material::TextureMap::ColorSpace::sRGB ?
                GL_SRGB8_ALPHA8 : GL_RGBA8;
            break;
        default:
            format = GL_RGB;
            internalFormat = GL_SRGB8;
            break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, imageData);
        glGenerateMipmap(GL_TEXTURE_2D);

        SetDefaultTextureParams(settings);
        stbi_image_free(imageData);

        // Cache texture info
        std::string cacheKey = "embedded_" + std::to_string(size);
        textureCache[cacheKey] = { textureID, width, height, channels, "embedded", settings };

        return textureID;
    }

    GLuint LoadTexture(const std::string& path, const std::string& type = "diffuse",
        const Material::TextureMap& settings = Material::TextureMap()) {
        // Return cached if exists
        auto it = textureCache.find(path);
        if (it != textureCache.end()) {
            // Update settings if they've changed
            if (memcmp(&it->second.settings, &settings, sizeof(Material::TextureMap)) != 0) {
                glBindTexture(GL_TEXTURE_2D, it->second.id);
                SetDefaultTextureParams(settings);
                it->second.settings = settings;
            }
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

        ProcessImageData(data, width, height, channels, settings);

        // Create OpenGL texture
        GLuint textureID;
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_2D, textureID);

        // Determine format based on color space and channels
        GLenum format, internalFormat;
        switch (channels) {
        case 1:
            format = GL_RED;
            internalFormat = GL_RED;
            break;
        case 3:
            format = GL_RGB;
            if (type == "normal" || type == "roughness" || type == "metallic") {
                internalFormat = GL_RGB8;
            }
            else {
                internalFormat = settings.colorSpace == Material::TextureMap::ColorSpace::sRGB ?
                    GL_SRGB8 : GL_RGB8;
            }
            break;
        case 4:
            format = GL_RGBA;
            if (type == "normal" || type == "roughness" || type == "metallic") {
                internalFormat = GL_RGBA8;
            }
            else {
                internalFormat = settings.colorSpace == Material::TextureMap::ColorSpace::sRGB ?
                    GL_SRGB8_ALPHA8 : GL_RGBA8;
            }
            break;
        default:
            format = GL_RGB;
            internalFormat = GL_SRGB8;
            break;
        }

        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        SetDefaultTextureParams(settings);

        // Cache texture
        textureCache[path] = { textureID, width, height, channels, type, settings };

        stbi_image_free(data);

        //DebugTextureState(textureID);

        return textureID;
    }

    void DebugTextureState(GLuint textureID) {
        glBindTexture(GL_TEXTURE_2D, textureID);

        GLint wrap_s, wrap_t, min_filter, mag_filter;
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrap_s);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, &wrap_t);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, &min_filter);
        glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &mag_filter);

        std::cout << "=== Texture State Debug ===" << std::endl;
        std::cout << "Texture ID: " << textureID << std::endl;
        std::cout << "Wrap S: " << wrap_s << " (GL_REPEAT=" << GL_REPEAT << ")" << std::endl;
        std::cout << "Wrap T: " << wrap_t << " (GL_REPEAT=" << GL_REPEAT << ")" << std::endl;
        std::cout << "Min Filter: " << min_filter << " (GL_LINEAR_MIPMAP_LINEAR=" << GL_LINEAR_MIPMAP_LINEAR << ")" << std::endl;
        std::cout << "Mag Filter: " << mag_filter << " (GL_LINEAR=" << GL_LINEAR << ")" << std::endl;

        // Check for any OpenGL errors
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            std::cout << "OpenGL error: " << err << std::endl;
        }
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
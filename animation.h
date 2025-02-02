#pragma once
#include "GameEngine.h"
#include "object3D.h"
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <map>
#include <array>

// Maximum number of bones allowed to influence a vertex
const int MAX_BONES = 100;


// A keyframe represents a specific point in time for an animation channel
class Keyframe {
public:
    float time;                  // Time in seconds
    glm::vec3 position;         // Translation
    glm::quat rotation;         // Rotation as quaternion
    glm::vec3 scale;           // Scale

    // Blender-style interpolation types
    enum class Interpolation {
        CONSTANT,
        LINEAR,
        BEZIER
    } interpolationType;

    // Bezier handle data (for Blender-style curve editing)
    struct Handle {
        glm::vec2 position;    // Position relative to keyframe
        bool connected;        // Whether handle is connected to its pair
        bool auto_smooth;      // Whether handle position is auto-calculated
    };
    Handle leftHandle, rightHandle;

    Keyframe() :
        time(0.0f),
        position(0.0f),
        rotation(glm::quat(1.0f, 0.0f, 0.0f, 0.0f)),
        scale(1.0f),
        interpolationType(Interpolation::LINEAR) {}
};

// An animation channel controls a specific property over time (like Blender's F-Curves)
class AnimationChannel {
public:
    std::string targetProperty;  // What this channel animates (e.g., "position", "rotation")
    std::vector<Keyframe> keyframes;

    void addKeyframe(const Keyframe& keyframe) {
        // Insert keeping sorted by time
        auto it = std::lower_bound(keyframes.begin(), keyframes.end(), keyframe,
            [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
        keyframes.insert(it, keyframe);
    }

    // Evaluate channel at a specific time
    void evaluate(float time, glm::vec3& position, glm::quat& rotation, glm::vec3& scale) const {
        if (keyframes.empty()) return;
        if (keyframes.size() == 1) {
            position = keyframes[0].position;
            rotation = keyframes[0].rotation;
            scale = keyframes[0].scale;
            return;
        }

        // Find surrounding keyframes
        size_t nextIdx = 0;
        for (; nextIdx < keyframes.size(); nextIdx++) {
            if (keyframes[nextIdx].time > time) break;
        }

        if (nextIdx == 0) {
            // Before first keyframe
            position = keyframes[0].position;
            rotation = keyframes[0].rotation;
            scale = keyframes[0].scale;
            return;
        }

        if (nextIdx == keyframes.size()) {
            // After last keyframe
            position = keyframes.back().position;
            rotation = keyframes.back().rotation;
            scale = keyframes.back().scale;
            return;
        }

        const Keyframe& k1 = keyframes[nextIdx - 1];
        const Keyframe& k2 = keyframes[nextIdx];

        float t = (time - k1.time) / (k2.time - k1.time);

        switch (k1.interpolationType) {
        case Keyframe::Interpolation::CONSTANT:
            position = k1.position;
            rotation = k1.rotation;
            scale = k1.scale;
            break;

        case Keyframe::Interpolation::LINEAR:
            position = glm::mix(k1.position, k2.position, t);
            rotation = glm::slerp(k1.rotation, k2.rotation, t);
            scale = glm::mix(k1.scale, k2.scale, t);
            break;

        case Keyframe::Interpolation::BEZIER:
            // TODO: Implement Bezier interpolation using handle data
            position = glm::mix(k1.position, k2.position, t);
            rotation = glm::slerp(k1.rotation, k2.rotation, t);
            scale = glm::mix(k1.scale, k2.scale, t);
            break;
        }
    }
};

// An Action is a named set of animation channels (like Blender's Actions)
class Action {
public:
    std::string name;
    float duration;
    std::vector<AnimationChannel> channels;

    // Load from Assimp animation
    void loadFromAssimp(const aiAnimation* anim) {
        name = anim->mName.data;
        duration = static_cast<float>(anim->mDuration / anim->mTicksPerSecond);

        for (unsigned int i = 0; i < anim->mNumChannels; i++) {
            const aiNodeAnim* channel = anim->mChannels[i];
            AnimationChannel newChannel;
            newChannel.targetProperty = channel->mNodeName.data;

            // Convert position keyframes
            for (unsigned int j = 0; j < channel->mNumPositionKeys; j++) {
                const auto& key = channel->mPositionKeys[j];
                Keyframe keyframe;
                keyframe.time = static_cast<float>(key.mTime / anim->mTicksPerSecond);
                keyframe.position = glm::vec3(key.mValue.x, key.mValue.y, key.mValue.z);
                newChannel.addKeyframe(keyframe);
            }

            channels.push_back(newChannel);
        }
    }
};

// A Bone represents a single transformable element in the armature
class Bone {
public:
    std::string name;
    glm::mat4 offsetMatrix;     // Bind pose inverse
    glm::mat4 localTransform;   // Current local transform
    glm::mat4 worldTransform;   // Current world transform
    int parentIndex;            // Index of parent bone (-1 for root)
    std::vector<int> childIndices;  // Indices of child bones

    Bone() : offsetMatrix(1.0f), localTransform(1.0f),
        worldTransform(1.0f), parentIndex(-1) {}
};

// Vertex weight data for skinning
struct VertexBoneData {
    std::array<int, 4> boneIds;
    std::array<float, 4> weights;

    VertexBoneData() {
        boneIds.fill(-1);
        weights.fill(0.0f);
    }

    void addBoneInfluence(int boneId, float weight) {
        // Find slot with lowest weight
        int minIndex = 0;
        float minWeight = weights[0];

        for (int i = 1; i < 4; i++) {
            if (weights[i] < minWeight) {
                minWeight = weights[i];
                minIndex = i;
            }
        }

        // Only replace if new weight is higher
        if (weight > minWeight) {
            boneIds[minIndex] = boneId;
            weights[minIndex] = weight;
        }

        // Normalize weights
        float sum = 0.0f;
        for (float w : weights) sum += w;
        if (sum > 0.0f) {
            for (float& w : weights) w /= sum;
        }
    }
};

// The Armature manages a complete bone hierarchy
class Armature {
public:
    std::vector<Bone> bones;
    std::map<std::string, int> boneNameToIndex;
    glm::mat4 globalInverseTransform;

    void initialize(const aiNode* rootNode) {
        globalInverseTransform = assimpToGlm(rootNode->mTransformation);
        globalInverseTransform = glm::inverse(globalInverseTransform);
    }

    void addBone(const std::string& name, const glm::mat4& offset, int parent = -1) {
        int index = bones.size();
        boneNameToIndex[name] = index;

        Bone bone;
        bone.name = name;
        bone.offsetMatrix = offset;
        bone.parentIndex = parent;

        if (parent >= 0) {
            bones[parent].childIndices.push_back(index);
        }

        bones.push_back(bone);
    }

    // Update bone transforms for the current frame
    void updateBoneTransforms() {
        for (size_t i = 0; i < bones.size(); i++) {
            if (bones[i].parentIndex == -1) {
                updateBoneTransformRecursive(i, glm::mat4(1.0f));
            }
        }
    }

    const std::vector<glm::mat4>& getBoneTransforms() const {
        // Convert bone world transforms to array format expected by shader
        finalTransforms.clear();
        finalTransforms.reserve(bones.size());

        for (const auto& bone : bones) {
            finalTransforms.push_back(bone.worldTransform);
        }

        return finalTransforms;
    }
    
private:
    // Cache for final transforms to avoid reallocating each frame
    mutable std::vector<glm::mat4> finalTransforms;

    void updateBoneTransformRecursive(int boneIndex, const glm::mat4& parentTransform) {
        Bone& bone = bones[boneIndex];
        glm::mat4 globalTransform = parentTransform * bone.localTransform;

        bone.worldTransform = globalInverseTransform * globalTransform * bone.offsetMatrix;

        for (int childIndex : bone.childIndices) {
            updateBoneTransformRecursive(childIndex, globalTransform);
        }
    }

    static glm::mat4 assimpToGlm(const aiMatrix4x4& m) {
        return glm::mat4(
            m.a1, m.b1, m.c1, m.d1,
            m.a2, m.b2, m.c2, m.d2,
            m.a3, m.b3, m.c3, m.d3,
            m.a4, m.b4, m.c4, m.d4
        );
    }
};

// AnimatedMesh extends the basic Mesh to support skeletal animation
class AnimatedMesh : public Mesh {
public:
    Armature armature;
    std::vector<VertexBoneData> boneData;
    std::vector<Action> actions;
    const aiScene* assimpScene;  // Keep reference to source data

    AnimatedMesh() : Mesh(false) {
        isAnimated = true;
    }

    void setupBuffers() override {
        Mesh::setupBuffers();

        if (!boneData.empty()) {
            GLuint boneVBO;
            glGenBuffers(1, &boneVBO);
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, boneVBO);
            glBufferData(GL_ARRAY_BUFFER, boneData.size() * sizeof(VertexBoneData),
                boneData.data(), GL_STATIC_DRAW);

            // Bone IDs (layout location = 5)
            glEnableVertexAttribArray(5);
            glVertexAttribIPointer(5, 4, GL_INT, sizeof(VertexBoneData),
                (const GLvoid*)offsetof(VertexBoneData, boneIds));

            // Bone weights (layout location = 6)
            glEnableVertexAttribArray(6);
            glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, sizeof(VertexBoneData),
                (const GLvoid*)offsetof(VertexBoneData, weights));
        }
    }
};

// Animation player for controlling playback
class AnimationPlayer {
public:
    enum class PlayMode {
        ONCE,
        LOOP,
        PING_PONG
    };

    void play(const std::string& actionName, PlayMode mode = PlayMode::LOOP) {
        // Find action by name
        for (const auto& action : mesh->actions) {
            if (action.name == actionName) {
                currentAction = &action;
                playMode = mode;
                currentTime = 0.0f;
                isPlaying = true;
                return;
            }
        }
    }

    void update(float deltaTime) {
        if (!isPlaying || !currentAction) return;

        currentTime += deltaTime * speed;

        // Handle play modes
        if (currentTime >= currentAction->duration) {
            switch (playMode) {
            case PlayMode::ONCE:
                currentTime = currentAction->duration;
                isPlaying = false;
                break;
            case PlayMode::LOOP:
                currentTime = fmod(currentTime, currentAction->duration);
                break;
            case PlayMode::PING_PONG:
                // TODO: Implement ping-pong playback
                break;
            }
        }

        // Update bone transforms
        updateBoneTransforms();
    }

    void stop() { isPlaying = false; currentTime = 0.0f; }
    void pause() { isPlaying = false; }
    void resume() { isPlaying = true; }
    void setSpeed(float newSpeed) { speed = newSpeed; }

    std::shared_ptr<AnimatedMesh> mesh;
    const Action* currentAction = nullptr;
    float currentTime = 0.0f;
    float speed = 1.0f;
    bool isPlaying = false;
    PlayMode playMode = PlayMode::LOOP;

private:
    void updateBoneTransforms() {
        if (!currentAction) return;

        // Update bone transforms based on current time
        for (auto& channel : currentAction->channels) {
            if (auto it = mesh->armature.boneNameToIndex.find(channel.targetProperty);
                it != mesh->armature.boneNameToIndex.end()) {
                int boneIndex = it->second;
                Bone& bone = mesh->armature.bones[boneIndex];

                glm::vec3 position;
                glm::quat rotation;
                glm::vec3 scale;
                channel.evaluate(currentTime, position, rotation, scale);

                bone.localTransform = glm::translate(glm::mat4(1.0f), position) *
                    glm::mat4_cast(rotation) *
                    glm::scale(glm::mat4(1.0f), scale);
            }
        }

        mesh->armature.updateBoneTransforms();
    }
};

// Animation system that handles all animations in the scene
class AnimationSystem {
public:
    enum class PlaybackMode {
        PLAY,       // Normal forward playback
        REVERSE,    // Reverse playback
        PING_PONG,  // Play forward then backward
        LOOP,       // Loop forward
        LOOP_PING_PONG  // Loop forward and backward
    };

    struct ActiveAction {
        std::shared_ptr<Action> action;
        std::shared_ptr<Node> targetNode;
        float startTime;
        float weight;
        float speed;
        PlaybackMode mode;
        bool isPlaying;
        bool shouldRemove;
    };

    void update(float deltaTime) {
        currentTime += deltaTime;

        for (auto& [name, activeAction] : activeActions) {
            if (!activeAction.isPlaying) continue;

            float localTime = (currentTime - activeAction.startTime) * activeAction.speed;

            // Handle different playback modes
            float actionTime = 0.0f;
            switch (activeAction.mode) {
            case PlaybackMode::PLAY:
                actionTime = localTime;
                if (localTime > activeAction.action->duration) {
                    activeAction.isPlaying = false;
                    continue;
                }
                break;

            case PlaybackMode::REVERSE:
                actionTime = activeAction.action->duration - localTime;
                if (actionTime < 0) {
                    activeAction.isPlaying = false;
                    continue;
                }
                break;

            case PlaybackMode::LOOP:
                actionTime = fmod(localTime, activeAction.action->duration);
                break;

            case PlaybackMode::PING_PONG:
            {
                float cycle = localTime / activeAction.action->duration;
                if (static_cast<int>(cycle) % 2 == 0) {
                    actionTime = fmod(localTime, activeAction.action->duration);
                }
                else {
                    actionTime = activeAction.action->duration - fmod(localTime, activeAction.action->duration);
                }
            }
            break;
            }

            // Evaluate each channel in the action
            for (const auto& channel : activeAction.action->channels) {
                glm::vec3 position;
                glm::quat rotation;
                glm::vec3 scale;

                channel.evaluate(actionTime, position, rotation, scale);

                // Apply transforms to target node
                if (activeAction.targetNode) {
                    // Blend with current transform based on weight
                    if (activeAction.weight < 1.0f) {
                        position = glm::mix(activeAction.targetNode->localTranslation, position, activeAction.weight);
                        rotation = glm::slerp(activeAction.targetNode->localRotation, rotation, activeAction.weight);
                        scale = glm::mix(activeAction.targetNode->localScale, scale, activeAction.weight);
                    }

                    activeAction.targetNode->localTranslation = position;
                    activeAction.targetNode->localRotation = rotation;
                    activeAction.targetNode->localScale = scale;
                }
            }
        }

        // Clean up finished non-looping animations
        auto it = activeActions.begin();
        while (it != activeActions.end()) {
            if (!it->second.isPlaying || it->second.shouldRemove) {
                it = activeActions.erase(it);
            }
            else {
                ++it;
            }
        }
    }

    void playAction(const std::string& name, std::shared_ptr<Action> action, std::shared_ptr<Node> target,
        PlaybackMode mode = PlaybackMode::LOOP, float weight = 1.0f, float speed = 1.0f) {
        ActiveAction activeAction;
        activeAction.action = action;
        activeAction.targetNode = target;
        activeAction.startTime = currentTime;
        activeAction.weight = weight;
        activeAction.speed = speed;
        activeAction.mode = mode;
        activeAction.isPlaying = true;
        activeAction.shouldRemove = false;

        activeActions[name] = activeAction;
    }

    void stopAction(const std::string& name) {
        auto it = activeActions.find(name);
        if (it != activeActions.end()) {
            it->second.shouldRemove = true;
        }
    }

    void stopAllActions() {
        for (auto& [name, action] : activeActions) {
            action.shouldRemove = true;
        }
    }

    void pauseAction(const std::string& name) {
        auto it = activeActions.find(name);
        if (it != activeActions.end()) {
            it->second.isPlaying = false;
        }
    }

    void resumeAction(const std::string& name) {
        auto it = activeActions.find(name);
        if (it != activeActions.end()) {
            it->second.isPlaying = true;
        }
    }

    void setActionWeight(const std::string& name, float weight) {
        auto it = activeActions.find(name);
        if (it != activeActions.end()) {
            it->second.weight = std::clamp(weight, 0.0f, 1.0f);
        }
    }

    void setActionSpeed(const std::string& name, float speed) {
        auto it = activeActions.find(name);
        if (it != activeActions.end()) {
            it->second.speed = speed;
        }
    }

    bool isActionPlaying(const std::string& name) const {
        auto it = activeActions.find(name);
        return it != activeActions.end() && it->second.isPlaying;
    }

    const std::map<std::string, ActiveAction>& getActiveActions() const {
        return activeActions;
    }

private:
    float currentTime = 0.0f;
    std::map<std::string, ActiveAction> activeActions;
};
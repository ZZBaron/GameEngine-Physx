// camera.h
#pragma once
#include "GameEngine.h"

// Define initial camera parameters
class Camera {
public:
	std::string name;

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 5.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	//screen parameters for camera
	int screenWidth = 1792;
	int screenHeight = 1008;
	float near = 0.1f;
	float far = 100.0f;

	glm::mat4 projectionMatrix = glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, near, far);

	float cameraSpeed = 0.1f; // Speed of camera movement
	float yaw = -90.0f;// Yaw angle
	float pitch = 0.0f; // Pitch angle
	float sensitivity = 0.1f; // Mouse sensitivity
	bool camstate = false; // camera can be moved with controls in input.cpp when true

	Camera (std::string name) :
		name(name) {
	}


	void setYaw(float newYaw) {
		yaw = newYaw;

		// Calculate new front vector based on updated yaw and current pitch
		glm::vec3 direction;
		direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = sin(glm::radians(pitch));
		direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		// Normalize the direction vector to maintain unit length
		cameraFront = glm::normalize(direction);
	}

	void setPitch(float newPitch) {
		// Constrain pitch to avoid camera flipping
		pitch = std::clamp(newPitch, -89.0f, 89.0f);

		// Calculate new front vector based on current yaw and updated pitch
		glm::vec3 direction;
		direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		direction.y = sin(glm::radians(pitch));
		direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

		// Normalize the direction vector to maintain unit length
		cameraFront = glm::normalize(direction);
	}

	void setCameraPos(const glm::vec3& newPos) {
		cameraPos = newPos;
	}

	void setCameraUp(const glm::vec3& newUp) {
		// Normalize the up vector to ensure unit length
		cameraUp = glm::normalize(newUp);
	}

	void setCameraFront(const glm::vec3& newFront) {
		// Normalize the front vector to ensure unit length
		cameraFront = glm::normalize(newFront);

		// Calculate new yaw and pitch based on the front vector
		pitch = glm::degrees(asin(newFront.y));
		yaw = glm::degrees(atan2(newFront.z, newFront.x));
	}
	
	glm::mat4 getViewMatrix() {
		return glm::lookAt(
			cameraPos,           // Camera position
			cameraPos + cameraFront,  // Look at position
			cameraUp                                                      // Up vector
		);
	}

	glm::mat4 getProjectionMatrix() {
		return glm::perspective(glm::radians(45.0f), (float)screenWidth / (float)screenHeight, 0.1f, 100.0f);
	}
	
	void toggleCam(GLFWwindow* window) {
		camstate = !camstate;
		if (camstate) {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			// Update cameraFront based on current yaw and pitch
			glm::vec3 front;
			front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
			front.y = sin(glm::radians(pitch));
			front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
			cameraFront = glm::normalize(front);
		}
		else {
			glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			// Optionally, save current yaw and pitch if you want to restore them later
		}
	}

};


bool isInViewFrustum(const glm::vec3& point, const glm::mat4& viewProjection);
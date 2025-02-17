// input.cpp
#pragma once
#include "GameEngine.h"
#include "ui.h"
#include "camera.h"
#include "console.h"
#include "selection.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

extern Scene scene;

bool consoleInputOnly = false;
std::map<int, bool> keyStates; // Map to track key states
std::map<int, bool> mouseButtonStates; // Map to track mouse button states
extern bool genSpheres;


// A function to execute another function once when the key is pressed and held down
void press_once(GLFWwindow* window, int key, void(*func)(GLFWwindow* window)) {
    bool isPressed = glfwGetKey(window, key) == GLFW_PRESS;

    // Initialize key state if not already present
    if (keyStates.find(key) == keyStates.end()) {
        keyStates[key] = false;
    }

    // Check if the key was not pressed previously but is pressed now
    if (isPressed && !keyStates[key]) {
        func(window);
    }

    // Update the previous key state
    keyStates[key] = isPressed;
}

void press_once_noargs(GLFWwindow* window, int key, void(*func)()) {
    bool isPressed = glfwGetKey(window, key) == GLFW_PRESS;

    // Initialize key state if not already present
    if (keyStates.find(key) == keyStates.end()) {
        keyStates[key] = false;
    }

    // Check if the key was not pressed previously but is pressed now
    if (isPressed && !keyStates[key]) {
        func();
    }

    // Update the previous key state
    keyStates[key] = isPressed;
}

// A function to execute another function once when the mouse button is pressed and held down
void mouse_press_once(GLFWwindow* window, int button, void(*func)(GLFWwindow* window)) {
    bool isPressed = glfwGetMouseButton(window, button) == GLFW_PRESS;

    // Initialize mouse button state if not already present
    if (mouseButtonStates.find(button) == mouseButtonStates.end()) {
        mouseButtonStates[button] = false;
    }

    // Check if the button was not pressed previously but is pressed now
    if (isPressed && !mouseButtonStates[button]) {
        func(window);
    }

    // Update the previous button state
    mouseButtonStates[button] = isPressed;
}

// Mouse callback function
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (!scene.activeCamera) return;

    static double lastX = 400.0;
    static double lastY = 300.0;
    static bool firstMouse = true;

    // Process mouse movement only if camstate is true
    if (scene.activeCamera->camstate) {
        if (firstMouse) {
            lastX = xpos;
            lastY = ypos;
            firstMouse = false;
        }

        float xoffset = xpos - lastX;
        float yoffset = lastY - ypos; // Reversed Y axis
        lastX = xpos;
        lastY = ypos;

        xoffset *= scene.activeCamera->sensitivity;
        yoffset *= scene.activeCamera->sensitivity;

        scene.activeCamera->setYaw(scene.activeCamera->yaw + xoffset);
        scene.activeCamera->setPitch(scene.activeCamera->pitch + yoffset);
    }
    else {
        // If camstate is false, reset lastX and lastY to the current cursor position
        lastX = xpos;
        lastY = ypos;
    }


}


 void static togglePlay() {
     scene.play = !scene.play;
 }

void static toggleGenSpheres() {
	genSpheres = !genSpheres;
}

// Add these functions
void toggleConsole() {
    consoleInputOnly = !consoleInputOnly;
    Console::getInstance().toggleVisibility();
}

void toggleWireFrames() {
    scene.drawWireframes = !scene.drawWireframes;
    scene.drawObjects = !scene.drawObjects;
}

void handleConsoleInput(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (!Console::getInstance().isVisible()) return;



    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (key == GLFW_KEY_BACKSPACE) {
            Console::getInstance().removeLastChar();
        }
		else if (key == GLFW_KEY_ESCAPE) {
			toggleConsole();
		}
        else if (key == GLFW_KEY_ENTER) {
            Console::getInstance().executeCommand();
        }
        else if (key >= 32 && key <= 126) {
            char character = static_cast<char>(key);
            // Check if Shift key is pressed
            if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
                // Convert to uppercase if Shift is pressed
                if (character >= 'a' && character <= 'z') {
                    character = character - 'a' + 'A';
                }
            }
            else {
                // Convert to lowercase if Shift is not pressed
                if (character >= 'A' && character <= 'Z') {
                    character = character - 'A' + 'a';
                }
            }
            Console::getInstance().addInput(character);
        }
    }
}

void processInput(GLFWwindow* window) {

    if (Console::getInstance().isVisible()) {
        press_once_noargs(window, GLFW_KEY_GRAVE_ACCENT, toggleConsole);
        return;
    }

    /*if (MenuSystem::getInstance().isMenuOpen()) {
        std::cout << "working" << std::endl;
        // Handle menu input
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        MenuSystem::getInstance().handleMouseMove(xpos, ypos);

        if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
            MenuSystem::getInstance().handleClick(xpos, ypos);
        }

        press_once_noargs(window, GLFW_KEY_P, toggleMenu);
        return;
    }*/

   
    // Handle selection with mouse click (using lambda function)
    mouse_press_once(window, GLFW_MOUSE_BUTTON_LEFT, [](GLFWwindow* w) {
        // Get window size for selection system
        int width, height;
        glfwGetWindowSize(w, &width, &height);

        double xpos, ypos;
        glfwGetCursorPos(w, &xpos, &ypos);

        // Check if CTRL is pressed for additive selection
        bool isControlPressed = glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;

        if (scene.activeCamera) {
            // Set the camera in SelectionSystem
            SelectionSystem::getInstance().setCamera(scene.activeCamera);

            try {
                SelectionSystem::getInstance().handleSelection(
                    xpos, ypos,
                    width, height,
                    scene,
                    isControlPressed
                );
            }
            catch (const std::runtime_error& e) {
                std::cerr << "Selection error: " << e.what() << std::endl;
            }
        }
        });


    // Call press_once for the spacebar key
    // Use a simple lambda to wrap the camera.toggleCam call
    press_once(window, GLFW_KEY_SPACE, [](GLFWwindow* w) {
        if (scene.activeCamera) scene.activeCamera->toggleCam(w);
        });

    press_once(window, GLFW_KEY_P, [](GLFWwindow* w) {
        scene.togglePlayer();
        });

    press_once_noargs(window, GLFW_KEY_ESCAPE, toggleMenu);
    press_once_noargs(window, GLFW_KEY_L, togglePlay);
    press_once_noargs(window, GLFW_KEY_GRAVE_ACCENT, toggleConsole);
	press_once_noargs(window, GLFW_KEY_G, toggleGenSpheres);
    press_once_noargs(window, GLFW_KEY_O, toggleWireFrames);

    // Handle movement controls only if camstate is true
    if (scene.activeCamera->camstate) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            scene.activeCamera->setCameraPos(scene.activeCamera->cameraPos + scene.activeCamera->cameraSpeed * scene.activeCamera->cameraFront);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            scene.activeCamera->setCameraPos(scene.activeCamera->cameraPos - scene.activeCamera->cameraSpeed * scene.activeCamera->cameraFront);
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            scene.activeCamera->setCameraPos(scene.activeCamera->cameraPos - scene.activeCamera->cameraSpeed * glm::normalize(glm::cross(scene.activeCamera->cameraFront, scene.activeCamera->cameraUp)));
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            scene.activeCamera->setCameraPos(scene.activeCamera->cameraPos + scene.activeCamera->cameraSpeed * glm::normalize(glm::cross(scene.activeCamera->cameraFront, scene.activeCamera->cameraUp)));
        }

        // Additional processing for mouse
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        mouse_callback(window, xpos, ypos);

    }




}

// After the processInput function, add:
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (Console::getInstance().isVisible()) {
        handleConsoleInput(window, key, scancode, action, mods);
    }

}

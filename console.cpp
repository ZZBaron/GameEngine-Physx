// console.cpp
#pragma once
#include "GameEngine.h"
#include "console.h"
#include "font.h"
#include "shape.h"

extern GLuint textShaderProgram;
extern std::vector<std::shared_ptr<Shape>> shapes;

Console& Console::getInstance() {
    static Console instance;
    return instance;
}

void Console::toggleVisibility() {
    visible = !visible;
}

void Console::addCommandHistory(const std::string& entry) {
    commandHistory.push_back(entry);
}

bool Console::isVisible() const {
    return visible;
}

void Console::render() {
    if (!visible) return;

    // Render console background
    // (Add code to render a semi-transparent background)

    // Render input buffer
    renderText(textShaderProgram, "> " + inputBuffer, 10.0f, 30.0f, 0.75f, glm::vec3(1.0f, 1.0f, 1.0f));

    // Render command history
    float y = 60.0f;
    for (const auto& cmd : commandHistory) {
        renderText(textShaderProgram, cmd, 10.0f, y, 0.75f, glm::vec3(0.8f, 0.8f, 0.8f));
        y += 30.0f;
    }
}

void Console::addInput(char c) {
    inputBuffer += c;
}

void Console::removeLastChar() {
    if (!inputBuffer.empty()) {
        inputBuffer.pop_back();
    }
}

void Console::executeCommand() {
    if (inputBuffer.empty()) return;

    commandHistory.push_back("> " + inputBuffer);
    std::vector<std::string> tokens = tokenizeInput(inputBuffer);

    if (!tokens.empty()) {
        std::string cmdName = tokens[0];
        tokens.erase(tokens.begin());

        auto it = commands.find(cmdName);
        if (it != commands.end()) {
            it->second(tokens);
        }
        else {
            commandHistory.push_back("Unknown command: " + cmdName);
        }
    }

    inputBuffer.clear();
}

void Console::registerCommand(const std::string& name, std::function<void(const std::vector<std::string>&)> func) {
    commands[name] = func;
}

std::vector<std::string> Console::tokenizeInput(const std::string& input) {
    std::vector<std::string> tokens;
    std::istringstream iss(input);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// Add these functions to your existing code (e.g., in GameEngine.cpp)
void initializeConsole() {
    Console& console = Console::getInstance();

    // Add more commands here as needed
}
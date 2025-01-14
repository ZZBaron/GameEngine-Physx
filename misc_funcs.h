#pragma once 
#include <filesystem>
#include "GameEngine.h"

namespace fs = std::filesystem;
inline std::string PROJECT_ROOT_NAME = "GameEngine Physx";


// Cross - platform solution to getting project root using std::filesystem
inline std::string getProjectRoot() {
	std::filesystem::path executablePath = std::filesystem::current_path();
	while (executablePath.has_parent_path() && executablePath.filename() != PROJECT_ROOT_NAME) {
		executablePath = executablePath.parent_path();
	}

	if (executablePath.filename() != PROJECT_ROOT_NAME) {
		throw std::runtime_error("Could not find " + PROJECT_ROOT_NAME + " directory in path");
	}

	// Convert the path to a string with platform-appropriate directory separators
	return executablePath.string();
}


// Misclanious functions
inline std::string vec3_to_string(glm::vec3 v, int decimal_places = 2) { // assuming vec3 is floats
	std::stringstream x_comp_stream;
	x_comp_stream << std::fixed << std::setprecision(decimal_places) << v.x;
	std::string x_str = x_comp_stream.str();

	std::stringstream y_comp_stream;
	y_comp_stream << std::fixed << std::setprecision(decimal_places) << v.y;
	std::string y_str = y_comp_stream.str();

	std::stringstream z_comp_stream;
	z_comp_stream << std::fixed << std::setprecision(decimal_places) << v.z;
	std::string z_str = z_comp_stream.str();

	std::string s = std::string("(") + x_str + std::string(", ") + y_str + std::string(", ") + z_str + std::string(")");

	return s;

}

inline std::string vec2_to_string(glm::vec2 v, int decimal_places = 2) { // assuming vec3 is floats
	std::stringstream x_comp_stream;
	x_comp_stream << std::fixed << std::setprecision(decimal_places) << v.x;
	std::string x_str = x_comp_stream.str();

	std::stringstream y_comp_stream;
	y_comp_stream << std::fixed << std::setprecision(decimal_places) << v.y;
	std::string y_str = y_comp_stream.str();


	std::string s = std::string("(") + x_str + std::string(", ") + y_str + std::string(")");

	return s;

}

class debugLog {
public:
	std::vector<std::string> log;

	void add_to_debugLog(std::string s, bool print = true) {
		if (print) {
			std::cout << s << std::endl;
		}

		log.push_back(s);
	}

	void save_to_file() { //  saves each string in log to a new line in a txt file
		// Create the Debug Logs directory if it doesn't exist
		fs::path debugLogsPath = fs::path(getProjectRoot()) / "Debug Logs";
		if (!fs::exists(debugLogsPath)) {
			fs::create_directory(debugLogsPath);
		}

		// Open file for writing (will overwrite existing file)
		fs::path logFile = debugLogsPath / "log.txt";
		std::ofstream file(logFile, std::ios::trunc);

		if (!file.is_open()) {
			throw std::runtime_error("Failed to open log file for writing: " + logFile.string());
		}

		// Write each log entry to the file
		for (const auto& entry : log) {
			file << entry << std::endl;
		}

		file.close();
	}

};

// Singularites in lightView matrix made creating this function necessary
inline glm::mat4 createViewMatrix(const glm::vec3& pos, const glm::vec3& targetPos, const glm::vec3& upVector) {
	// Calculate the direction vector from light to target
	glm::vec3 direction = targetPos - pos;

	// Check if direction is parallel to up vector (their cross product is near zero)
	glm::vec3 right = glm::cross(direction, upVector);
	if (glm::length(right) < 0.0001f) {
		// If they're parallel, use a different up vector
		// If light is above target (common case), use world Z as temp up
		// If light is below target, use negative world Z as temp up
		glm::vec3 tempUp = (direction.y >= 0.0f) ?
			glm::vec3(0.0f, 0.0f, 1.0f) :
			glm::vec3(0.0f, 0.0f, -1.0f);

		return glm::lookAt(pos, targetPos, tempUp);
	}

	// Normal case - use the provided up vector
	return glm::lookAt(pos, targetPos, upVector);
}


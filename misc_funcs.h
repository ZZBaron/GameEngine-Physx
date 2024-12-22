#pragma once 
#include <filesystem>
#include "GameEngine.h"

namespace fs = std::filesystem;
std::string PROJECT_ROOT_NAME = "GameEngine Physx";


// Cross - platform solution to getting project root using std::filesystem
std::string getProjectRoot() {
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
std::string vec3_to_string(glm::vec3 v, int decimal_places = 2) { // assuming vec3 is floats
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


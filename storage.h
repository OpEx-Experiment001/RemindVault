#pragma once
// ============================================================
// storage.h — Helper functions to read and write data to files
// We save all users and tasks as JSON files in a "data" folder
// ============================================================

#include "json.hpp"
#include <fstream>
#include <string>

using json = nlohmann::json;

// Read a JSON file and return its contents.
// If the file doesn't exist yet, return an empty list.
inline json readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return json::array(); // Return empty array if no file yet
    json data;
    try {
        file >> data;
    } catch (...) {
        return json::array(); // Return empty if file is corrupted
    }
    return data;
}

// Write data to a JSON file (pretty-printed so you can read it easily)
inline void writeFile(const std::string& filename, const json& data) {
    std::ofstream file(filename);
    file << data.dump(4); // 4 spaces of indentation for readability
}

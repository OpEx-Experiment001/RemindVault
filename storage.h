#pragma once
// ============================================================
// storage.h — Read and write JSON files using only std::string
// No external libraries — pure standard C++
// ============================================================

#include <string>
#include <fstream>
#include <sstream>

// Read entire file contents as a string
inline std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return "[]";
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    if (content.empty()) return "[]";
    return content;
}

// Write string content to a file
inline void writeFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    file << content;
}
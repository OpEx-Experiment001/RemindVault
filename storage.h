#pragma once

#include <string>
#include <fstream>
#include <sstream>


inline std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return "[]";
    std::ostringstream ss;
    ss << file.rdbuf();
    std::string content = ss.str();
    if (content.empty()) return "[]";
    return content;
}


inline void writeFile(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    file << content;
}

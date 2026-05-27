#pragma once

#include <string>

// Custom SHA-256 implementation
std::string sha256(const std::string& input);

// Password hashing using timestamp salting
std::string hashPassword(const std::string& password, const std::string& timestampSalt);
bool verifyPassword(const std::string& password, const std::string& storedHash);

// Data Encryption (XOR cipher for PBL simplicity)
std::string encryptData(const std::string& data, const std::string& key);
std::string decryptData(const std::string& data, const std::string& key);

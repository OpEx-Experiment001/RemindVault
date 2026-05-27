#pragma once

#include <string>
using namespace std;

// Custom SHA-256 implementation
string sha256(const string& input);

// Password hashing using timestamp salting
string hashPassword(const string& password, const string& timestampSalt);
bool verifyPassword(const string& password, const string& storedHash);

// Data Encryption (XOR cipher for PBL simplicity)
string encryptData(const string& data, const string& key);
string decryptData(const string& data, const string& key);

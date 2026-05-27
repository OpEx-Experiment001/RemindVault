#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║  storage.h — Encrypted File I/O (XOR cipher, symmetric)     ║
// ╚══════════════════════════════════════════════════════════════╝
#include "crypto.h"
#include "platform.h"
#include <string>
#include <fstream>
#include <sstream>

// The master key for XOR encryption. Simple but effective obfuscation.
static const std::string STORAGE_KEY = "RemindVault2026!";

extern PlatMutex storageMutex;

inline std::string readFile(const std::string& path) {
    PlatLock lock(storageMutex);
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) return "[]";
    std::ostringstream ss; ss << f.rdbuf();
    std::string raw = ss.str();
    if (raw.empty()) return "[]";
    return decryptData(raw, STORAGE_KEY); // XOR is its own inverse
}

inline void writeFile(const std::string& path, const std::string& content) {
    PlatLock lock(storageMutex);
    std::ofstream f(path, std::ios::binary);
    f << encryptData(content, STORAGE_KEY);
}

#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║  storage.h — Encrypted File I/O (XOR cipher, symmetric)     ║
// ╚══════════════════════════════════════════════════════════════╝
#include "crypto.h"
#include "platform.h"
#include <string>
#include <fstream>
#include <sstream>
using namespace std;

// The master key for XOR encryption. Simple but effective obfuscation.
static const string STORAGE_KEY = "RemindVault2026!";

extern PlatMutex storageMutex;

inline string readFile(const string& path) {
    PlatLock lock(storageMutex);
    ifstream f(path, ios::binary);
    if (!f.is_open()) return "[]";
    ostringstream ss; ss << f.rdbuf();
    string raw = ss.str();
    if (raw.empty()) return "[]";
    return decryptData(raw, STORAGE_KEY); // XOR is its own inverse
}

inline void writeFile(const string& path, const string& content) {
    PlatLock lock(storageMutex);
    ofstream f(path, ios::binary);
    f << encryptData(content, STORAGE_KEY);
}

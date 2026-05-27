#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║  pipeutil.h — Cross-platform popen/pclose wrapper           ║
// ╚══════════════════════════════════════════════════════════════╝
#include <cstdio>
#include <string>
using namespace std;

// MinGW on Windows names them _popen/_pclose but may not declare them
// even with <cstdio>. We forward-declare them here when needed.
#ifdef _WIN32
extern "C" {
    FILE* __cdecl _popen(const char* command, const char* type);
    int   __cdecl _pclose(FILE* stream);
}
#endif

// Unified run-command-capture-output helper
inline string pipeCapture(const string& cmd) {
    string out;
#ifdef _WIN32
    FILE* f = _popen(cmd.c_str(), "r");
#else
    FILE* f = popen(cmd.c_str(), "r");
#endif
    if (!f) return out;
    char buf[512];
    while (fgets(buf, sizeof(buf), f)) out += buf;
#ifdef _WIN32
    _pclose(f);
#else
    pclose(f);
#endif
    // Strip trailing whitespace/newlines
    while (!out.empty() &&
           (out.back() == '\n' || out.back() == '\r' || out.back() == ' '))
        out.pop_back();
    return out;
}

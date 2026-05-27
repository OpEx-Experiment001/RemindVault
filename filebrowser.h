#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║  filebrowser.h — Native OS File Picker                      ║
// ╚══════════════════════════════════════════════════════════════╝
#include <string>
using namespace std;

// Opens a native OS "Open File" dialog and returns the absolute
// path chosen by the user, or an empty string on cancel/error.
string browseForFile();

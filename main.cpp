// ============================================================
// main.cpp — HTTP server using Winsock2 (Windows native sockets)
// No external libraries — pure standard C++ + Winsock2
//
// HOW THIS WORKS:
//   1. We initialise Winsock2 with WSAStartup()
//   2. We open a TCP socket and bind it to port 8080
//   3. For each incoming connection we read the HTTP request
//   4. We parse the method, path, headers, and body manually
//   5. We call our auth/task functions and send back a response
//
// COMPILE (in VS Code terminal):
//   g++ -std=c++17 main.cpp auth.cpp tasks.cpp -o server.exe -lws2_32
//
// RUN:
//   server.exe
//   → http://localhost:8080
// ============================================================

#include "auth.h"
#include "tasks.h"
#include "json_utils.h"

#include <iostream>
#include <string>
#include <sstream>
#include <map>

#include <algorithm>
#include <cstring>
// NOTE: No <filesystem> needed — we use CreateDirectoryA() from windows.h
// which is pulled in automatically by <winsock2.h> below.

// ── Winsock2 — Windows socket API ───────────────────────────
// Must be included BEFORE any other Windows headers
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")   // Auto-link Winsock library
                                      // (no need to add -lws2_32 manually
                                      //  when using MSVC; still needed for g++)

// ════════════════════════════════════════════════════════════
// SOCKET TYPE ALIAS
// On POSIX:   sockets are plain int file descriptors
// On Windows: sockets are SOCKET (an unsigned pointer-sized int)
// ════════════════════════════════════════════════════════════
using SocketFd = SOCKET;
static const SocketFd INVALID_SOCK = INVALID_SOCKET;

// ── Winsock uses closesocket(), not close() ──────────────────
inline void closeSocket(SocketFd fd) { closesocket(fd); }

// ════════════════════════════════════════════════════════════
// HTTP REQUEST — parsed from raw socket bytes
// ════════════════════════════════════════════════════════════
struct HttpRequest {
    std::string method;   // GET, POST, OPTIONS
    std::string path;     // /register, /tasks, etc.
    std::string query;    // everything after '?' e.g. userId=abc
    std::map<std::string, std::string> headers;
    std::string body;
};

// ════════════════════════════════════════════════════════════
// HTTP RESPONSE — we build and send this
// ════════════════════════════════════════════════════════════
struct HttpResponse {
    int statusCode = 200;
    std::string statusText = "OK";
    std::map<std::string, std::string> headers;
    std::string body;

    void setStatus(int code) {
        statusCode = code;
        switch(code) {
            case 200: statusText="OK";          break;
            case 201: statusText="Created";     break;
            case 204: statusText="No Content";  break;
            case 400: statusText="Bad Request"; break;
            case 401: statusText="Unauthorized";break;
            case 409: statusText="Conflict";    break;
            case 500: statusText="Server Error";break;
            default:  statusText="OK";
        }
    }

    void setJSON(const std::string& json) {
        body = json;
        headers["Content-Type"]   = "application/json";
        headers["Content-Length"] = std::to_string(json.size());
    }

    std::string serialize() const {
        std::ostringstream out;
        out << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
        for (auto& h : headers)
            out << h.first << ": " << h.second << "\r\n";
        out << "\r\n" << body;
        return out.str();
    }
};

// ════════════════════════════════════════════════════════════
// PARSE — Read raw HTTP bytes into HttpRequest
// ════════════════════════════════════════════════════════════
static HttpRequest parseRequest(const std::string& raw) {
    HttpRequest req;
    std::istringstream stream(raw);
    std::string line;

    // ── Request line: METHOD /path?query HTTP/1.1 ──
    if (!std::getline(stream, line)) return req;
    if (!line.empty() && line.back()=='\r') line.pop_back();
    {
        std::istringstream ls(line);
        std::string fullPath, ver;
        ls >> req.method >> fullPath >> ver;

        // Split path and query string
        auto qpos = fullPath.find('?');
        if (qpos != std::string::npos) {
            req.path  = fullPath.substr(0, qpos);
            req.query = fullPath.substr(qpos + 1);
        } else {
            req.path = fullPath;
        }
    }

    // ── Headers ──
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (line.empty()) break; // blank line = end of headers
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string key = line.substr(0, colon);
            std::string val = line.substr(colon+1);
            // Trim leading space from value
            while (!val.empty() && (val[0]==' '||val[0]=='\t')) val.erase(0,1);
            req.headers[key] = val;
        }
    }

    // ── Body (remaining stream) ──
    std::ostringstream bodyStream;
    bodyStream << stream.rdbuf();
    req.body = bodyStream.str();

    return req;
}

// ════════════════════════════════════════════════════════════
// QUERY STRING PARSER — "userId=abc&foo=bar" → map
// ════════════════════════════════════════════════════════════
static std::map<std::string, std::string> parseQuery(const std::string& qs) {
    std::map<std::string, std::string> params;
    std::istringstream ss(qs);
    std::string token;
    while (std::getline(ss, token, '&')) {
        auto eq = token.find('=');
        if (eq != std::string::npos) {
            params[token.substr(0, eq)] = token.substr(eq + 1);
        }
    }
    return params;
}

// ════════════════════════════════════════════════════════════
// CORS — Add headers so browser can talk to localhost:8080
// ════════════════════════════════════════════════════════════
static void addCORS(HttpResponse& res) {
    res.headers["Access-Control-Allow-Origin"]  = "*";
    res.headers["Access-Control-Allow-Methods"] = "GET, POST, OPTIONS";
    res.headers["Access-Control-Allow-Headers"] = "Content-Type";
}

// ════════════════════════════════════════════════════════════
// ROUTE HANDLERS
// ════════════════════════════════════════════════════════════

// ── POST /register ───────────────────────────────────────────
static HttpResponse handleRegister(const HttpRequest& req) {
    HttpResponse res;
    addCORS(res);
    try {
        size_t pos = 0;
        JsonObj body = parseObject(req.body, pos);

        std::string name       = getStr(body, "name");
        int         age        = getInt(body, "age");
        std::string email      = getStr(body, "email");
        std::string gender     = getStr(body, "gender");
        std::string profession = getStr(body, "profession");
        std::string password   = getStr(body, "password");

        if (name.empty() || password.empty()) {
            res.setStatus(400);
            res.setJSON(JsonObject().addBool("success",false).addStr("message","Please fill all fields correctly.").build());
            return res;
        }

        bool ok = registerUser(name, age, email, gender, profession, password);
        if (ok) {
            res.setStatus(201);
            res.setJSON(JsonObject().addBool("success",true).addStr("message","Account created successfully! Please log in.").build());
        } else {
            res.setStatus(409);
            res.setJSON(JsonObject().addBool("success",false).addStr("message","This name is already taken. Please choose a different name.").build());
        }
    } catch (...) {
        res.setStatus(400);
        res.setJSON(JsonObject().addBool("success",false).addStr("message","Something went wrong. Please fill all fields correctly.").build());
    }
    return res;
}

// ── POST /login ──────────────────────────────────────────────
static HttpResponse handleLogin(const HttpRequest& req) {
    HttpResponse res;
    addCORS(res);
    try {
        size_t pos = 0;
        JsonObj body = parseObject(req.body, pos);

        std::string name     = getStr(body, "name");
        std::string password = getStr(body, "password");

        std::string userId = loginUser(name, password);
        if (!userId.empty()) {
            res.setStatus(200);
            res.setJSON(JsonObject()
                .addBool("success", true)
                .addStr("message", "Welcome back, " + name + "!")
                .addStr("userId", userId)
                .addStr("name", name)
                .build());
        } else {
            res.setStatus(401);
            res.setJSON(JsonObject().addBool("success",false).addStr("message","Incorrect name or password. Please try again.").build());
        }
    } catch (...) {
        res.setStatus(400);
        res.setJSON(JsonObject().addBool("success",false).addStr("message","Something went wrong. Please try again.").build());
    }
    return res;
}

// ── POST /tasks/add ──────────────────────────────────────────
static HttpResponse handleAddTask(const HttpRequest& req) {
    HttpResponse res;
    addCORS(res);
    try {
        size_t pos = 0;
        JsonObj body = parseObject(req.body, pos);

        std::string userId      = getStr(body, "userId");
        std::string title       = getStr(body, "title");
        std::string description = getStr(body, "description");
        std::string image       = getStr(body, "image");
        std::string startDate   = getStr(body, "startDate");
        std::string endDate     = getStr(body, "endDate");

        if (userId.empty() || title.empty() || startDate.empty() || endDate.empty()) {
            res.setStatus(400);
            res.setJSON(JsonObject().addBool("success",false).addStr("message","Please fill all required fields (title, dates, userId).").build());
            return res;
        }

        bool ok = addTask(userId, title, description, image, startDate, endDate);
        if (ok) {
            res.setStatus(201);
            res.setJSON(JsonObject().addBool("success",true).addStr("message","Task added successfully!").build());
        } else {
            res.setStatus(500);
            res.setJSON(JsonObject().addBool("success",false).addStr("message","Failed to save the task. Please try again.").build());
        }
    } catch (...) {
        res.setStatus(400);
        res.setJSON(JsonObject().addBool("success",false).addStr("message","Please fill all required fields.").build());
    }
    return res;
}

// ── GET /tasks ───────────────────────────────────────────────
static HttpResponse handleGetTasks(const HttpRequest& req) {
    HttpResponse res;
    addCORS(res);
    auto params = parseQuery(req.query);
    std::string userId = params["userId"];
    if (userId.empty()) {
        res.setStatus(400);
        res.setJSON(JsonObject().addBool("success",false).addStr("message","Please provide a userId in the URL.").build());
        return res;
    }
    std::string tasks = getTasksByUser(userId);
    res.setStatus(200);
    res.setJSON("{\"success\":true,\"tasks\":" + tasks + "}");
    return res;
}

// ── GET /tasks/calendar ──────────────────────────────────────
static HttpResponse handleGetCalendar(const HttpRequest& req) {
    HttpResponse res;
    addCORS(res);
    auto params = parseQuery(req.query);
    std::string userId = params["userId"];
    if (userId.empty()) {
        res.setStatus(400);
        res.setJSON(JsonObject().addBool("success",false).addStr("message","Please provide a userId in the URL.").build());
        return res;
    }
    std::string cal = getTasksForCalendar(userId);
    res.setStatus(200);
    res.setJSON("{\"success\":true,\"calendar\":" + cal + "}");
    return res;
}

// ════════════════════════════════════════════════════════════
// WINDOWS THREAD CALLBACK
// CreateThread() requires a specific function signature:
//   DWORD WINAPI functionName(LPVOID param)
// We pass the clientFd as a heap-allocated pointer so it
// survives across the thread boundary safely.
// ════════════════════════════════════════════════════════════
static void handleClient(SocketFd clientFd); // forward declaration

static DWORD WINAPI clientThreadProc(LPVOID param) {
    // Recover the socket fd from the heap pointer, then free it
    SocketFd clientFd = *(SocketFd*)param;
    delete (SocketFd*)param;
    handleClient(clientFd);
    return 0;
}

// ════════════════════════════════════════════════════════════
// HANDLE ONE CLIENT CONNECTION — runs in its own thread
// ════════════════════════════════════════════════════════════
static void handleClient(SocketFd clientFd) {
    std::string rawRequest;
    char buf[4096];

    // Keep reading until we have full headers + full body
    while (true) {
        int bytesRead = recv(clientFd, buf, sizeof(buf) - 1, 0);
        if (bytesRead <= 0) break;          // connection closed or error
        buf[bytesRead] = '\0';
        rawRequest += std::string(buf, bytesRead);

        // Wait until we've received the blank line ending the headers
        auto headerEnd = rawRequest.find("\r\n\r\n");
        if (headerEnd == std::string::npos) continue;

        // Now check Content-Length so we know if the body is complete
        size_t bodyStart    = headerEnd + 4;
        size_t bodyReceived = rawRequest.size() - bodyStart;

        size_t clPos = rawRequest.find("Content-Length:");
        if (clPos == std::string::npos) clPos = rawRequest.find("content-length:");
        size_t contentLen = 0;
        if (clPos != std::string::npos && clPos < headerEnd) {
            size_t valStart = clPos + 15; // skip "Content-Length:"
            while (valStart < rawRequest.size() && rawRequest[valStart] == ' ') valStart++;
            try { contentLen = std::stoul(rawRequest.substr(valStart)); } catch (...) {}
        }

        if (bodyReceived >= contentLen) break;
    }

    if (rawRequest.empty()) { closeSocket(clientFd); return; }

    HttpRequest  req = parseRequest(rawRequest);
    HttpResponse res;

    // ── Router ──────────────────────────────────────────────
    if (req.method == "OPTIONS") {
        addCORS(res);
        res.setStatus(204);
        res.headers["Content-Length"] = "0";
    }
    else if (req.method == "POST" && req.path == "/register") {
        res = handleRegister(req);
    }
    else if (req.method == "POST" && req.path == "/login") {
        res = handleLogin(req);
    }
    else if (req.method == "POST" && req.path == "/tasks/add") {
        res = handleAddTask(req);
    }
    else if (req.method == "GET" && req.path == "/tasks/calendar") {
        res = handleGetCalendar(req);
    }
    else if (req.method == "GET" && req.path == "/tasks") {
        res = handleGetTasks(req);
    }
    else {
        addCORS(res);
        res.setStatus(404);
        res.setJSON(JsonObject().addBool("success", false).addStr("message", "Route not found.").build());
    }

    // ── Send response ────────────────────────────────────────
    std::string responseStr = res.serialize();
    send(clientFd, responseStr.c_str(), (int)responseStr.size(), 0);
    closeSocket(clientFd);
}

// ════════════════════════════════════════════════════════════
// MAIN — Initialise Winsock, create socket, bind, listen, loop
// ════════════════════════════════════════════════════════════
int main() {
    // ── Step 1: Initialise Winsock2 ──────────────────────────
    // REQUIRED on Windows before ANY socket call.
    // This is the main difference from POSIX — Linux/macOS don't need this.
    WSADATA wsaData;
    int wsaResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaResult != 0) {
        std::cerr << "[ERROR] WSAStartup failed: " << wsaResult << "\n";
        return 1;
    }

    // ── Step 2: Create the data directory ────────────────────
    // CreateDirectoryA() is from windows.h (pulled in by winsock2.h above).
    // Does nothing if the folder already exists.
    CreateDirectoryA("data", NULL);

    // ── Step 3: Create TCP socket ────────────────────────────
    SocketFd serverFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverFd == INVALID_SOCK) {
        std::cerr << "[ERROR] Could not create socket. WSA error: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }

    // Allow port reuse — avoids "address already in use" on restart
    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    //                                               ^^^^^^^^^^^^^^
    // Winsock needs (const char*) cast here, unlike POSIX which uses (void*)

    // ── Step 4: Bind to 0.0.0.0:8080 ────────────────────────
    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(8080);

    if (bind(serverFd, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "[ERROR] Bind failed. Is port 8080 already in use? WSA error: "
                << WSAGetLastError() << "\n";
        closeSocket(serverFd);
        WSACleanup();
        return 1;
    }

    // ── Step 5: Start listening ──────────────────────────────
    if (listen(serverFd, 32) == SOCKET_ERROR) {
        std::cerr << "[ERROR] Listen failed. WSA error: " << WSAGetLastError() << "\n";
        closeSocket(serverFd);
        WSACleanup();
        return 1;
    }

    std::cout << "\n========================================\n";
    std::cout << "  Backend Server Started!  (Windows)\n";
    std::cout << "  Running at: http://localhost:8080\n";
    std::cout << "  No external libraries used.\n";
    std::cout << "  Waiting for requests...\n";
    std::cout << "========================================\n\n";

    // ── Step 6: Accept loop — one thread per connection ──────
    while (true) {
        sockaddr_in clientAddr{};
        int clientLen = sizeof(clientAddr);

        SocketFd clientFd = accept(serverFd, (sockaddr*)&clientAddr, &clientLen);
        if (clientFd == INVALID_SOCK) {
            std::cerr << "[WARN] accept() failed: " << WSAGetLastError() << "\n";
            continue;
        }

        // Heap-allocate the fd so the thread can safely read it
        // after this loop iteration moves on.
        SocketFd* fdPtr = new SocketFd(clientFd);
        HANDLE hThread = CreateThread(
            NULL,             // default security attributes
            0,                // default stack size
            clientThreadProc, // our thread function above
            fdPtr,            // argument passed to it
            0,                // start immediately
            NULL              // we don't need the thread ID
        );
        if (hThread) {
            // We don't need to track the thread — close the handle
            // immediately. The thread keeps running until it returns.
            CloseHandle(hThread);
        } else {
            // Thread creation failed — clean up and skip this client
            std::cerr << "[WARN] CreateThread failed: " << GetLastError() << "\n";
            delete fdPtr;
            closeSocket(clientFd);
        }
    }

    // ── Cleanup (reached only if the loop above exits) ───────
    closeSocket(serverFd);
    WSACleanup();   // Always call this to release Winsock resources
    return 0;
}
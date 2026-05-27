#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║  sse.h — Server-Sent Events Infrastructure                  ║
// ╚══════════════════════════════════════════════════════════════╝
#include "platform.h"
#include <string>
#include <vector>

// Registered long-lived SSE client sockets
extern std::vector<SocketFd> sseClients;
extern PlatMutex             sseMutex;

// Initialize the mutex — call once from main()
void sseInit();

// Push a JSON data line to all connected SSE clients
void broadcastEvent(const std::string& jsonData);

// Register / unregister SSE client sockets
void sseAddClient(SocketFd fd);
void sseRemoveClient(SocketFd fd);

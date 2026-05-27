// ╔══════════════════════════════════════════════════════════════╗
// ║  sse.cpp — Server-Sent Events Implementation                ║
// ╚══════════════════════════════════════════════════════════════╝
#include "sse.h"
#include <algorithm>

std::vector<SocketFd> sseClients;
PlatMutex             sseMutex;

void sseInit() {
    mutexInit(sseMutex);
}

void broadcastEvent(const std::string& jsonData) {
    PlatLock lock(sseMutex);
    std::string payload = "data: " + jsonData + "\n\n";
    for (auto it = sseClients.begin(); it != sseClients.end(); ) {
        int sent = send(*it, payload.c_str(), (int)payload.size(), 0);
        if (sent == PLAT_SOCK_ERR) {
            closeSocket(*it);
            it = sseClients.erase(it);
        } else {
            ++it;
        }
    }
}

void sseAddClient(SocketFd fd) {
    PlatLock lock(sseMutex);
    sseClients.push_back(fd);
}

void sseRemoveClient(SocketFd fd) {
    PlatLock lock(sseMutex);
    sseClients.erase(
        std::remove(sseClients.begin(), sseClients.end(), fd),
        sseClients.end()
    );
}

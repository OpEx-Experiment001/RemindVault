// ╔══════════════════════════════════════════════════════════════╗
// ║  sse.cpp — Server-Sent Events Implementation                ║
// ╚══════════════════════════════════════════════════════════════╝
#include "sse.h"
#include <algorithm>
using namespace std;

vector<SocketFd> sseClients;
PlatMutex             sseMutex;

void sseInit() {
    mutexInit(sseMutex);
}

void broadcastEvent(const string& jsonData) {
    PlatLock lock(sseMutex);
    string payload = "data: " + jsonData + "\n\n";
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
        remove(sseClients.begin(), sseClients.end(), fd),
        sseClients.end()
    );
}

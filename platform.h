using namespace std;
#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║  platform.h — Cross-Platform OS Abstractions                ║
// ║  Detects Windows / macOS / Linux and exposes a unified API  ║
// ╚══════════════════════════════════════════════════════════════╝

// ─── Sockets ──────────────────────────────────────────────────
#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
  #pragma comment(lib, "ws2_32.lib")

  using SocketFd = SOCKET;
  static const SocketFd INVALID_SOCK = INVALID_SOCKET;
  inline void   closeSocket(SocketFd fd) { closesocket(fd); }
  inline int    getLastSockErr()         { return (int)WSAGetLastError(); }
  inline void   sleepMs(int ms)          { Sleep(ms); }
  inline bool   initNetwork()            { WSADATA w; return WSAStartup(MAKEWORD(2,2),&w)==0; }
  inline void   cleanupNetwork()         { WSACleanup(); }
  inline void   ensureDir(const char* p) { CreateDirectoryA(p, NULL); }
  static const int PLAT_SOCK_ERR        = SOCKET_ERROR;
  static const int PLAT_WOULDBLOCK      = WSAEWOULDBLOCK;

#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  #include <cerrno>
  #include <sys/stat.h>
  #include <pthread.h>

  using SocketFd = int;
  static const SocketFd INVALID_SOCK = -1;
  inline void   closeSocket(SocketFd fd) { ::close(fd); }
  inline int    getLastSockErr()         { return errno; }
  inline void   sleepMs(int ms)          { usleep(ms * 1000); }
  inline bool   initNetwork()            { return true; }
  inline void   cleanupNetwork()         {}
  inline void   ensureDir(const char* p) { mkdir(p, 0755); }
  static const int PLAT_SOCK_ERR        = -1;
  static const int PLAT_WOULDBLOCK      = EWOULDBLOCK;
#endif

// ─── Mutex ────────────────────────────────────────────────────
struct PlatMutex {
#ifdef _WIN32
    CRITICAL_SECTION cs;
#else
    pthread_mutex_t m;
#endif
};

inline void mutexInit(PlatMutex& mx) {
#ifdef _WIN32
    InitializeCriticalSection(&mx.cs);
#else
    pthread_mutex_init(&mx.m, nullptr);
#endif
}
inline void mutexLock(PlatMutex& mx) {
#ifdef _WIN32
    EnterCriticalSection(&mx.cs);
#else
    pthread_mutex_lock(&mx.m);
#endif
}
inline void mutexUnlock(PlatMutex& mx) {
#ifdef _WIN32
    LeaveCriticalSection(&mx.cs);
#else
    pthread_mutex_unlock(&mx.m);
#endif
}

// RAII lock guard
struct PlatLock {
    PlatMutex& _m;
    PlatLock(PlatMutex& m) : _m(m) { mutexLock(_m); }
    ~PlatLock()                     { mutexUnlock(_m); }
};

// ─── Threads ──────────────────────────────────────────────────
#ifdef _WIN32
  #define THREAD_FN(name) DWORD WINAPI name(LPVOID _arg)
  #define THREAD_RETURN   return 0

  typedef DWORD (WINAPI *ThreadFn)(LPVOID);
  inline void spawnThread(ThreadFn fn, void* arg) {
      HANDLE h = CreateThread(NULL, 0, fn, arg, 0, NULL);
      if (h) CloseHandle(h);
  }
#else
  #define THREAD_FN(name) void* name(void* _arg)
  #define THREAD_RETURN   return nullptr

  typedef void* (*ThreadFn)(void*);
  inline void spawnThread(ThreadFn fn, void* arg) {
      pthread_t t;
      pthread_create(&t, nullptr, fn, arg);
      pthread_detach(t);
  }
#endif

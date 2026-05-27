// ╔══════════════════════════════════════════════════════════════╗
// ║  main.cpp — HTTP Server Entry Point & Request Router        ║
// ║  All heavy lifting is in the modules below.                 ║
// ╚══════════════════════════════════════════════════════════════╝
#include "platform.h"
#include "auth.h"
#include "tasks.h"
#include "json_utils.h"
#include "storage.h"
#include "sse.h"
#include "alarm.h"
#include "filebrowser.h"
#include "admin.h"

#include <iostream>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
using namespace std;

// ═══════════════════════════════════════════════════════════════
//  HTTP Request / Response
// ═══════════════════════════════════════════════════════════════
struct HttpRequest {
    string method, path, query, body;
    map<string,string> headers;
};

struct HttpResponse {
    int statusCode = 200;
    string statusText = "OK";
    map<string,string> headers;
    string body;

    void setStatus(int c) {
        statusCode = c;
        const char* t = "OK";
        switch(c) {
            case 201: t="Created";      break;
            case 204: t="No Content";   break;
            case 400: t="Bad Request";  break;
            case 401: t="Unauthorized"; break;
            case 404: t="Not Found";    break;
            case 409: t="Conflict";     break;
            case 500: t="Server Error"; break;
        }
        statusText = t;
    }

    void setJSON(const string& j) {
        body = j;
        headers["Content-Type"]   = "application/json";
        headers["Content-Length"] = to_string(j.size());
    }

    string serialize() const {
        ostringstream o;
        o << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
        for (auto& h : headers) o << h.first << ": " << h.second << "\r\n";
        o << "\r\n" << body;
        return o.str();
    }
};

// ═══════════════════════════════════════════════════════════════
//  Parsing Helpers
// ═══════════════════════════════════════════════════════════════
static HttpRequest parseRequest(const string& raw) {
    HttpRequest req;
    istringstream s(raw);
    string line;
    if (!getline(s, line)) return req;
    if (!line.empty() && line.back()=='\r') line.pop_back();
    {
        istringstream ls(line);
        string full, ver;
        ls >> req.method >> full >> ver;
        auto q = full.find('?');
        if (q != string::npos) { req.path = full.substr(0,q); req.query = full.substr(q+1); }
        else req.path = full;
    }
    while (getline(s, line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (line.empty()) break;
        auto c = line.find(':');
        if (c != string::npos) {
            string v = line.substr(c+1);
            while (!v.empty() && (v[0]==' '||v[0]=='\t')) v.erase(0,1);
            req.headers[line.substr(0,c)] = v;
        }
    }
    ostringstream b; b << s.rdbuf(); req.body = b.str();
    return req;
}

static map<string,string> parseQuery(const string& qs) {
    map<string,string> p;
    istringstream ss(qs); string tok;
    while (getline(ss, tok, '&')) {
        auto eq = tok.find('=');
        if (eq != string::npos) p[tok.substr(0,eq)] = tok.substr(eq+1);
    }
    return p;
}

static void addCORS(HttpResponse& res) {
    res.headers["Access-Control-Allow-Origin"]  = "*";
    res.headers["Access-Control-Allow-Methods"] = "GET, POST, DELETE, PUT, OPTIONS";
    res.headers["Access-Control-Allow-Headers"] = "Content-Type, X-Admin-Token";
}

// ─── Admin token helper ─────────────────────────────────────────
static string getAdminToken(const HttpRequest& req) {
    for (const auto& h : req.headers) {
        string k = h.first;
        transform(k.begin(), k.end(), k.begin(), ::tolower);
        if (k == "x-admin-token") return h.second;
    }
    return "";
}
static bool checkAdmin(const HttpRequest& req, HttpResponse& res) {
    if (!isAdminToken(getAdminToken(req))) {
        addCORS(res);
        res.setStatus(401);
        res.setJSON("{\"success\":false,\"message\":\"Unauthorized. Invalid admin token.\"}");
        return false;
    }
    return true;
}

// ═══════════════════════════════════════════════════════════════
//  Route Handlers
// ═══════════════════════════════════════════════════════════════
static HttpResponse onRegister(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    try {
        size_t pos = 0; JsonObj b = parseObject(req.body, pos);
        string name=getStr(b,"name"), email=getStr(b,"email"),
            gender=getStr(b,"gender"), prof=getStr(b,"profession"),
            pass=getStr(b,"password");
        int age=getInt(b,"age");
        if (name.empty()||pass.empty()) {
            res.setStatus(400);
            res.setJSON(JsonObject().addBool("success",false).addStr("message","Name and password required.").build());
            return res;
        }
        if (registerUser(name,age,email,gender,prof,pass)) {
            res.setStatus(201);
            res.setJSON(JsonObject().addBool("success",true).addStr("message","Account created!").build());
        } else {
            res.setStatus(409);
            res.setJSON(JsonObject().addBool("success",false).addStr("message","Username already taken.").build());
        }
    } catch(...) {
        res.setStatus(400);
        res.setJSON(JsonObject().addBool("success",false).addStr("message","Bad request.").build());
    }
    return res;
}

static HttpResponse onLogin(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    try {
        size_t pos=0; JsonObj b=parseObject(req.body,pos);
        string name=getStr(b,"name"), pass=getStr(b,"password");
        string uid=loginUser(name,pass);
        if (!uid.empty()) {
            res.setStatus(200);
            res.setJSON(JsonObject().addBool("success",true)
                .addStr("message","Welcome back, "+name+"!")
                .addStr("userId",uid).addStr("name",name).build());
        } else {
            res.setStatus(401);
            res.setJSON(JsonObject().addBool("success",false).addStr("message","Incorrect credentials.").build());
        }
    } catch(...) {
        res.setStatus(400);
        res.setJSON(JsonObject().addBool("success",false).addStr("message","Bad request.").build());
    }
    return res;
}

static HttpResponse onAddTask(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    try {
        size_t pos=0; JsonObj b=parseObject(req.body,pos);
        string uid=getStr(b,"userId"), title=getStr(b,"title"),
            desc=getStr(b,"description"), img=getStr(b,"image"),
            sd=getStr(b,"startDate"), ed=getStr(b,"endDate"),
            freq=getStr(b,"frequency"), atype=getStr(b,"attachmentType"),
            apath=getStr(b,"attachmentPath"), atime=getStr(b,"alarmTime");
        if (uid.empty()||title.empty()||sd.empty()||ed.empty()) {
            res.setStatus(400);
            res.setJSON(JsonObject().addBool("success",false).addStr("message","Missing required fields.").build());
            return res;
        }
        bool ok=addTask(uid,title,desc,img,sd,ed,freq,atype,apath,atime);
        if (ok) {
            res.setStatus(201);
            res.setJSON(JsonObject().addBool("success",true).addStr("message","Task added!").build());
        } else {
            res.setStatus(500);
            res.setJSON(JsonObject().addBool("success",false).addStr("message","Save failed.").build());
        }
    } catch(...) {
        res.setStatus(400);
        res.setJSON(JsonObject().addBool("success",false).addStr("message","Bad request.").build());
    }
    return res;
}

static HttpResponse onTaskAction(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    auto p=parseQuery(req.query);
    string id=p["id"], action=p["action"];
    if (id.empty()||action.empty()) {
        res.setStatus(400);
        res.setJSON(JsonObject().addBool("success",false).addStr("message","Missing id or action.").build());
        return res;
    }
    bool ok=handleTaskAction(id,action);
    res.setStatus(ok?200:404);
    res.setJSON(JsonObject().addBool("success",ok).build());
    return res;
}

static HttpResponse onGetTasks(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    string uid=parseQuery(req.query)["userId"];
    if (uid.empty()) { res.setStatus(400); res.setJSON("{\"success\":false}"); return res; }
    res.setJSON("{\"success\":true,\"tasks\":"+getTasksByUser(uid)+"}");
    return res;
}

static HttpResponse onGetCalendar(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    string uid=parseQuery(req.query)["userId"];
    if (uid.empty()) { res.setStatus(400); res.setJSON("{\"success\":false}"); return res; }
    res.setJSON("{\"success\":true,\"calendar\":"+getTasksForCalendar(uid)+"}");
    return res;
}

static HttpResponse onBrowse(const HttpRequest&) {
    HttpResponse res; addCORS(res);
    string path = browseForFile();
    if (path.empty()) {
        res.setStatus(204); // No content = user cancelled
        res.setJSON(JsonObject().addBool("success",false).addStr("path","").build());
    } else {
        res.setJSON(JsonObject().addBool("success",true).addStr("path",path).build());
    }
    return res;
}

// ═══════════════════════════════════════════════════════════════
//  Admin Route Handlers
// ═══════════════════════════════════════════════════════════════
static HttpResponse onAdminGetUsers(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    if (!checkAdmin(req, res)) return res;
    res.setJSON("{\"success\":true,\"users\":" + adminGetUsers() + "}");
    return res;
}
static HttpResponse onAdminDeleteUser(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    if (!checkAdmin(req, res)) return res;
    string id = parseQuery(req.query)["id"];
    if (id.empty()) { res.setStatus(400); res.setJSON("{\"success\":false,\"message\":\"Missing id\"}"); return res; }
    bool ok = adminDeleteUser(id);
    res.setStatus(ok ? 200 : 404);
    res.setJSON(JsonObject().addBool("success",ok).build());
    return res;
}
static HttpResponse onAdminEditUser(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    if (!checkAdmin(req, res)) return res;
    size_t pos=0; JsonObj b=parseObject(req.body,pos);
    bool ok = adminEditUser(getStr(b,"id"),getStr(b,"name"),getStr(b,"email"),getStr(b,"profession"));
    res.setStatus(ok ? 200 : 404);
    res.setJSON(JsonObject().addBool("success",ok).build());
    return res;
}
static HttpResponse onAdminResetPassword(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    if (!checkAdmin(req, res)) return res;
    size_t pos=0; JsonObj b=parseObject(req.body,pos);
    string id=getStr(b,"id"), pass=getStr(b,"password");
    if (id.empty()||pass.empty()) { res.setStatus(400); res.setJSON("{\"success\":false,\"message\":\"Missing id or password\"}"); return res; }
    bool ok = adminResetPassword(id, pass);
    res.setStatus(ok ? 200 : 404);
    res.setJSON(JsonObject().addBool("success",ok).build());
    return res;
}
static HttpResponse onAdminGetTasks(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    if (!checkAdmin(req, res)) return res;
    res.setJSON("{\"success\":true,\"tasks\":" + adminGetAllTasks() + "}");
    return res;
}
static HttpResponse onAdminDeleteTask(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    if (!checkAdmin(req, res)) return res;
    string id = parseQuery(req.query)["id"];
    if (id.empty()) { res.setStatus(400); res.setJSON("{\"success\":false,\"message\":\"Missing id\"}"); return res; }
    bool ok = adminDeleteTask(id);
    res.setStatus(ok ? 200 : 404);
    res.setJSON(JsonObject().addBool("success",ok).build());
    return res;
}
static HttpResponse onAdminEditTask(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    if (!checkAdmin(req, res)) return res;
    size_t pos=0; JsonObj b=parseObject(req.body,pos);
    bool ok = adminEditTask(getStr(b,"id"),getStr(b,"title"),getStr(b,"description"),
                            getStr(b,"status"),getStr(b,"endDate"),getStr(b,"alarmTime"));
    res.setStatus(ok ? 200 : 404);
    res.setJSON(JsonObject().addBool("success",ok).build());
    return res;
}
static HttpResponse onAdminNuke(const HttpRequest& req) {
    HttpResponse res; addCORS(res);
    if (!checkAdmin(req, res)) return res;
    adminNukeAll();
    res.setJSON(JsonObject().addBool("success",true).addStr("message","All data wiped.").build());
    return res;
}

// ═══════════════════════════════════════════════════════════════
//  Client Thread
// ═══════════════════════════════════════════════════════════════
static void handleClient(SocketFd fd) {
    ofstream flog("client_state.txt", ios::app);
    flog << "handleClient started\n" << flush;

    string raw; char buf[4096];
    while (true) {
        int n=recv(fd,buf,sizeof(buf)-1,0);
        flog << "recv returned n=" << n << "\n" << flush;
        if (n<=0) break;
        buf[n]='\0'; raw+=string(buf,n);
        auto he=raw.find("\r\n\r\n");
        if (he==string::npos) continue;
        size_t cl=0;
        auto cp=raw.find("Content-Length:");
        if (cp==string::npos) cp=raw.find("content-length:");
        if (cp!=string::npos && cp<he) {
            size_t vs=cp+15;
            while(vs<raw.size()&&raw[vs]==' ')vs++;
            try{cl=stoul(raw.substr(vs));}catch(...){}
        }
        flog << "cl=" << cl << ", raw.size()=" << raw.size() << ", he=" << he << "\n" << flush;
        if (raw.size()-he-4>=cl) {
            flog << "Breaking loop\n" << flush;
            break;
        }
    }
    if (raw.empty()) { 
        flog << "raw is empty, returning\n" << flush;
        closeSocket(fd); return; 
    }

    flog << "Parsing request...\n" << flush;
    HttpRequest  req = parseRequest(raw);
    HttpResponse res;

    if (req.method=="OPTIONS") {
        addCORS(res); res.setStatus(204); res.headers["Content-Length"]="0";
    }
    else if (req.method=="GET" && req.path=="/events") {
        // SSE: keep connection open
        addCORS(res);
        res.setStatus(200);
        res.headers["Content-Type"]  = "text/event-stream";
        res.headers["Cache-Control"] = "no-cache";
        res.headers["Connection"]    = "keep-alive";
        string hdr = res.serialize();
        send(fd, hdr.c_str(), (int)hdr.size(), 0);
        sseAddClient(fd);
        // Block until client disconnects
        char peek[1];
        while (recv(fd,peek,1,MSG_PEEK)>0) sleepMs(1000);
        sseRemoveClient(fd);
        closeSocket(fd);
        return;
    }
    // ── Global OPTIONS handler for CORS preflight ──
    else if (req.method=="OPTIONS") {
        addCORS(res);
        res.setStatus(204);
        string out=res.serialize();
        send(fd,out.c_str(),(int)out.size(),0);
        closeSocket(fd);
        return;
    }
    else if (req.method=="POST" && req.path=="/register")  res=onRegister(req);
    else if (req.method=="POST" && req.path=="/login")     res=onLogin(req);
    else if (req.method=="POST" && req.path=="/tasks/add") res=onAddTask(req);
    else if (req.method=="GET"  && req.path=="/tasks")         res=onGetTasks(req);
    else if (req.method=="GET"  && req.path=="/tasks/calendar") res=onGetCalendar(req);
    else if (req.method=="GET"  && req.path=="/tasks/action")   res=onTaskAction(req);
    else if (req.method=="GET"  && req.path=="/tasks/browse")    res=onBrowse(req);
    // ── Admin routes (require X-Admin-Token header) ──
    else if (req.path=="/admin/users"    && req.method=="GET")    res=onAdminGetUsers(req);
    else if (req.path=="/admin/users"    && req.method=="DELETE") res=onAdminDeleteUser(req);
    else if (req.path=="/admin/users"    && req.method=="PUT")    res=onAdminEditUser(req);
    else if (req.path=="/admin/password" && req.method=="POST")   res=onAdminResetPassword(req);
    else if (req.path=="/admin/tasks"    && req.method=="GET")    res=onAdminGetTasks(req);
    else if (req.path=="/admin/tasks"    && req.method=="DELETE") res=onAdminDeleteTask(req);
    else if (req.path=="/admin/tasks"    && req.method=="PUT")    res=onAdminEditTask(req);
    else if (req.path=="/admin/nuke"     && req.method=="POST")   res=onAdminNuke(req);
    // ── Static HTML pages served from the backend (solves file:// CORS) ──
    else if (req.method=="GET" && (req.path=="/" || req.path=="/app")) {
        ifstream f("RemindVault_Integrated.html");
        if (f) {
            ostringstream ss; ss << f.rdbuf();
            addCORS(res); res.setStatus(200);
            res.headers["Content-Type"]   = "text/html; charset=utf-8";
            res.body = ss.str();
            res.headers["Content-Length"] = to_string(res.body.size());
        } else { addCORS(res); res.setStatus(404); res.body="<h2>RemindVault_Integrated.html not found</h2>"; }
    }
    else if (req.method=="GET" && (req.path=="/admin" || req.path=="/admin-panel")) {
        ifstream f("admin.html");
        if (f) {
            ostringstream ss; ss << f.rdbuf();
            addCORS(res); res.setStatus(200);
            res.headers["Content-Type"]   = "text/html; charset=utf-8";
            res.body = ss.str();
            res.headers["Content-Length"] = to_string(res.body.size());
        } else { addCORS(res); res.setStatus(404); res.body="<h2>admin.html not found</h2>"; }
    }
    else {
        addCORS(res); res.setStatus(404);
        res.setJSON(JsonObject().addBool("success",false).addStr("message","Route not found.").build());
    }

    string out=res.serialize();
    send(fd,out.c_str(),(int)out.size(),0);
    closeSocket(fd);
}

static THREAD_FN(clientThreadProc) {
    SocketFd fd=*(SocketFd*)_arg; delete (SocketFd*)_arg;
    handleClient(fd);
    THREAD_RETURN;
}

// ═══════════════════════════════════════════════════════════════
//  main()
// ═══════════════════════════════════════════════════════════════
PlatMutex storageMutex;

int main() {
    mutexInit(storageMutex);
    if (!initNetwork()) { cerr << "[ERROR] Network init failed.\n"; return 1; }
    ensureDir("data");
    sseInit();
    startAlarmThread();

    SocketFd srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (srv == INVALID_SOCK) {
        cerr << "[ERROR] socket() failed: " << getLastSockErr() << "\n";
        cleanupNetwork(); return 1;
    }

#if defined(_WIN32)
    // CRITICAL FIX: Prevent child processes (like alarms or file browsers) from inheriting the socket.
    // This stops ghost processes from locking the port if the server is killed.
    SetHandleInformation((HANDLE)srv, HANDLE_FLAG_INHERIT, 0);
#endif

    int opt=1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(8081);

    if (bind(srv,(sockaddr*)&addr,sizeof(addr))==PLAT_SOCK_ERR) {
        cerr << "[ERROR] bind() failed: " << getLastSockErr() << "\n";
        closeSocket(srv); cleanupNetwork(); return 1;
    }
    if (listen(srv,32)==PLAT_SOCK_ERR) {
        cerr << "[ERROR] listen() failed\n";
        closeSocket(srv); cleanupNetwork(); return 1;
    }

    cout
        << "\n================================================\n"
        << "  RemindVault Backend  —  http://localhost:8081\n"
        << "  Cross-platform | No external dependencies\n"
        << "================================================\n\n" << flush;
        
    {
        ofstream f("server_state.txt");
        f << "Server reached main loop!";
    }

    while (true) {
        sockaddr_in ca{}; int cl=sizeof(ca);
        SocketFd cfd=accept(srv,(sockaddr*)&ca,&cl);
        {
            ofstream f("server_state.txt", ios::app);
            f << "\naccept returned! cfd=" << cfd;
        }
        if (cfd==INVALID_SOCK) continue;
        SocketFd* p=new SocketFd(cfd);
        cout << "Accepted connection! Spawning thread...\n" << flush;
        spawnThread((ThreadFn)clientThreadProc, p);
    }

    closeSocket(srv);
    cleanupNetwork();
    return 0;
}

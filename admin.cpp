// ╔══════════════════════════════════════════════════════════════╗
// ║  admin.cpp — Admin CRUD Operations (Users + Tasks)          ║
// ╚══════════════════════════════════════════════════════════════╝
#include "admin.h"
#include "storage.h"
#include "json_utils.h"
#include "crypto.h"
#include "sse.h"

#include <vector>
#include <string>
#include <chrono>
#include <iostream>
using namespace std;

// ─── Master Token ──────────────────────────────────────────────
// Change this string to your own secret before sharing the project.
const string ADMIN_TOKEN = "rv@dmin#2026!";

bool isAdminToken(const string& token) {
    return token == ADMIN_TOKEN;
}

static const string USERS_FILE = "data/users.json";
static const string TASKS_FILE = "data/tasks.json";

// ═══════════════════════════════════════════════════════════════
//  USERS
// ═══════════════════════════════════════════════════════════════
string adminGetUsers() {
    return readFile(USERS_FILE);
}

bool adminDeleteUser(const string& userId) {
    // Delete user record
    string uraw = readFile(USERS_FILE);
    vector<JsonObj> users = parseObjArray(uraw);
    bool found = false;
    vector<JsonObj> newUsers;
    for (auto& u : users) {
        if (getStr(u, "id") == userId) { found = true; continue; }
        newUsers.push_back(u);
    }
    if (!found) return false;
    writeFile(USERS_FILE, objArrayToString(newUsers));

    // Also delete all their tasks
    string traw = readFile(TASKS_FILE);
    vector<JsonObj> tasks = parseObjArray(traw);
    vector<JsonObj> newTasks;
    for (auto& t : tasks)
        if (getStr(t, "userId") != userId) newTasks.push_back(t);
    writeFile(TASKS_FILE, objArrayToString(newTasks));

    cout << "[Admin] Deleted user: " << userId << "\n";
    broadcastEvent("{\"type\":\"update\"}");
    return true;
}

bool adminResetPassword(const string& userId,
                        const string& newPassword) {
    string raw = readFile(USERS_FILE);
    vector<JsonObj> users = parseObjArray(raw);
    bool found = false;
    for (auto& u : users) {
        if (getStr(u, "id") == userId) {
            found = true;
            string salt = to_string(
                chrono::system_clock::to_time_t(
                    chrono::system_clock::now()));
            u["password"] = jStr(hashPassword(newPassword, salt));
            break;
        }
    }
    if (!found) return false;
    writeFile(USERS_FILE, objArrayToString(users));
    cout << "[Admin] Password reset for user: " << userId << "\n";
    return true;
}

bool adminEditUser(const string& userId,
                   const string& name,
                   const string& email,
                   const string& profession) {
    string raw = readFile(USERS_FILE);
    vector<JsonObj> users = parseObjArray(raw);
    bool found = false;
    for (auto& u : users) {
        if (getStr(u, "id") == userId) {
            found = true;
            if (!name.empty())       u["name"]       = jStr(name);
            if (!email.empty())      u["email"]      = jStr(email);
            if (!profession.empty()) u["profession"] = jStr(profession);
            break;
        }
    }
    if (!found) return false;
    writeFile(USERS_FILE, objArrayToString(users));
    return true;
}

// ═══════════════════════════════════════════════════════════════
//  TASKS
// ═══════════════════════════════════════════════════════════════
string adminGetAllTasks() {
    return readFile(TASKS_FILE);
}

bool adminDeleteTask(const string& taskId) {
    string raw = readFile(TASKS_FILE);
    vector<JsonObj> tasks = parseObjArray(raw);
    bool found = false;
    vector<JsonObj> newTasks;
    for (auto& t : tasks) {
        if (getStr(t, "id") == taskId) { found = true; continue; }
        newTasks.push_back(t);
    }
    if (!found) return false;
    writeFile(TASKS_FILE, objArrayToString(newTasks));
    cout << "[Admin] Deleted task: " << taskId << "\n";
    broadcastEvent("{\"type\":\"update\"}");
    return true;
}

bool adminEditTask(const string& taskId,
                   const string& title,
                   const string& description,
                   const string& status,
                   const string& endDate,
                   const string& alarmTime) {
    string raw = readFile(TASKS_FILE);
    vector<JsonObj> tasks = parseObjArray(raw);
    bool found = false;
    for (auto& t : tasks) {
        if (getStr(t, "id") == taskId) {
            found = true;
            if (!title.empty())       t["title"]       = jStr(title);
            if (!description.empty()) t["description"] = jStr(description);
            if (!status.empty())      t["status"]      = jStr(status);
            if (!endDate.empty())     t["endDate"]     = jStr(endDate);
            if (!alarmTime.empty())   t["alarmTime"]   = jStr(alarmTime);
            break;
        }
    }
    if (!found) return false;
    writeFile(TASKS_FILE, objArrayToString(tasks));
    broadcastEvent("{\"type\":\"update\"}");
    return true;
}

bool adminNukeAll() {
    writeFile(USERS_FILE, "[]");
    writeFile(TASKS_FILE, "[]");
    cout << "[Admin] ⚠ ALL DATA WIPED.\n";
    broadcastEvent("{\"type\":\"update\"}");
    return true;
}

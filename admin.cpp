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

// ─── Master Token ──────────────────────────────────────────────
// Change this string to your own secret before sharing the project.
const std::string ADMIN_TOKEN = "rv@dmin#2026!";

bool isAdminToken(const std::string& token) {
    return token == ADMIN_TOKEN;
}

static const std::string USERS_FILE = "data/users.json";
static const std::string TASKS_FILE = "data/tasks.json";

// ═══════════════════════════════════════════════════════════════
//  USERS
// ═══════════════════════════════════════════════════════════════
std::string adminGetUsers() {
    return readFile(USERS_FILE);
}

bool adminDeleteUser(const std::string& userId) {
    // Delete user record
    std::string uraw = readFile(USERS_FILE);
    std::vector<JsonObj> users = parseObjArray(uraw);
    bool found = false;
    std::vector<JsonObj> newUsers;
    for (auto& u : users) {
        if (getStr(u, "id") == userId) { found = true; continue; }
        newUsers.push_back(u);
    }
    if (!found) return false;
    writeFile(USERS_FILE, objArrayToString(newUsers));

    // Also delete all their tasks
    std::string traw = readFile(TASKS_FILE);
    std::vector<JsonObj> tasks = parseObjArray(traw);
    std::vector<JsonObj> newTasks;
    for (auto& t : tasks)
        if (getStr(t, "userId") != userId) newTasks.push_back(t);
    writeFile(TASKS_FILE, objArrayToString(newTasks));

    std::cout << "[Admin] Deleted user: " << userId << "\n";
    broadcastEvent("{\"type\":\"update\"}");
    return true;
}

bool adminResetPassword(const std::string& userId,
                        const std::string& newPassword) {
    std::string raw = readFile(USERS_FILE);
    std::vector<JsonObj> users = parseObjArray(raw);
    bool found = false;
    for (auto& u : users) {
        if (getStr(u, "id") == userId) {
            found = true;
            std::string salt = std::to_string(
                std::chrono::system_clock::to_time_t(
                    std::chrono::system_clock::now()));
            u["password"] = jStr(hashPassword(newPassword, salt));
            break;
        }
    }
    if (!found) return false;
    writeFile(USERS_FILE, objArrayToString(users));
    std::cout << "[Admin] Password reset for user: " << userId << "\n";
    return true;
}

bool adminEditUser(const std::string& userId,
                   const std::string& name,
                   const std::string& email,
                   const std::string& profession) {
    std::string raw = readFile(USERS_FILE);
    std::vector<JsonObj> users = parseObjArray(raw);
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
std::string adminGetAllTasks() {
    return readFile(TASKS_FILE);
}

bool adminDeleteTask(const std::string& taskId) {
    std::string raw = readFile(TASKS_FILE);
    std::vector<JsonObj> tasks = parseObjArray(raw);
    bool found = false;
    std::vector<JsonObj> newTasks;
    for (auto& t : tasks) {
        if (getStr(t, "id") == taskId) { found = true; continue; }
        newTasks.push_back(t);
    }
    if (!found) return false;
    writeFile(TASKS_FILE, objArrayToString(newTasks));
    std::cout << "[Admin] Deleted task: " << taskId << "\n";
    broadcastEvent("{\"type\":\"update\"}");
    return true;
}

bool adminEditTask(const std::string& taskId,
                   const std::string& title,
                   const std::string& description,
                   const std::string& status,
                   const std::string& endDate,
                   const std::string& alarmTime) {
    std::string raw = readFile(TASKS_FILE);
    std::vector<JsonObj> tasks = parseObjArray(raw);
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
    std::cout << "[Admin] ⚠ ALL DATA WIPED.\n";
    broadcastEvent("{\"type\":\"update\"}");
    return true;
}

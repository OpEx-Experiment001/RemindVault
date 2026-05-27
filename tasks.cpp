
#include "tasks.h"
#include "storage.h"
#include "json_utils.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <map>
#include <cstdlib>
#include "sse.h"

const std::string TASKS_FILE = "data/tasks.json";

// ─── Cross-Platform: Open a file or URL with the default app ───
static void openAttachment(const std::string& path) {
    if (path.empty()) return;
    std::string cmd;
#if defined(_WIN32)
    cmd = "start \"\" \"" + path + "\"";
#elif defined(__APPLE__)
    cmd = "open \"" + path + "\"";
#else
    cmd = "xdg-open \"" + path + "\" &";
#endif
    std::system(cmd.c_str());
}

static std::string getCurrentDate() {
    auto now  = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm* tm   = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d");
    return oss.str();
}

bool addTask(const std::string& userId,
             const std::string& title,
             const std::string& description,
             const std::string& imageBase64,
             const std::string& startDate,
             const std::string& endDate,
             const std::string& frequency,
             const std::string& attachmentType,
             const std::string& attachmentPath,
             const std::string& alarmTime) {

    std::string raw = readFile(TASKS_FILE);
    std::vector<JsonObj> tasks = parseObjArray(raw);

    std::string id = "task_" + std::to_string(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()
    );

    JsonObj newTask;
    newTask["id"]             = jStr(id);
    newTask["userId"]         = jStr(userId);
    newTask["title"]          = jStr(title);
    newTask["description"]    = jStr(description);
    newTask["image"]          = jStr(imageBase64);
    newTask["startDate"]      = jStr(startDate);
    newTask["endDate"]        = jStr(endDate);
    newTask["createdAt"]      = jStr(getCurrentDate());
    newTask["status"]         = jStr("pending");
    newTask["frequency"]      = jStr(frequency.empty()       ? "none" : frequency);
    newTask["attachmentType"] = jStr(attachmentType.empty()  ? "none" : attachmentType);
    newTask["attachmentPath"] = jStr(attachmentPath);
    newTask["snoozeCount"]    = jInt(0);
    newTask["snoozeUntil"]    = jInt(0);
    newTask["ignoredLog"]     = "[]";
    newTask["alarmTime"]      = jStr(alarmTime.empty() ? "none" : alarmTime);

    tasks.push_back(newTask);
    writeFile(TASKS_FILE, objArrayToString(tasks));

    std::cout << "[Task] Added: '" << title << "' for user " << userId << "\n";
    broadcastEvent("{\"type\":\"update\"}");
    return true;
}

std::string getTasksByUser(const std::string& userId) {
    std::string raw = readFile(TASKS_FILE);
    std::vector<JsonObj> tasks = parseObjArray(raw);
    std::vector<JsonObj> result;
    for (auto& t : tasks)
        if (getStr(t, "userId") == userId)
            result.push_back(t);
    return objArrayToString(result);
}

std::string getTasksForCalendar(const std::string& userId) {
    std::string raw = readFile(TASKS_FILE);
    std::vector<JsonObj> tasks = parseObjArray(raw);
    std::map<std::string, std::vector<std::string>> calendar;

    for (auto& t : tasks) {
        if (getStr(t, "userId") != userId) continue;
        std::string startDate = getStr(t, "startDate");
        std::string endDate   = getStr(t, "endDate");
        calendar[startDate].push_back(objToString(t));
        if (endDate != startDate) {
            JsonObj copy = t;
            copy["note"] = jStr("Due date");
            calendar[endDate].push_back(objToString(copy));
        }
    }

    std::string out = "{";
    bool firstDate = true;
    for (auto& kv : calendar) {
        if (!firstDate) out += ",";
        out += "\"" + kv.first + "\":[";
        for (size_t i = 0; i < kv.second.size(); i++) {
            if (i > 0) out += ",";
            out += kv.second[i];
        }
        out += "]";
        firstDate = false;
    }
    out += "}";
    return out;
}

bool handleTaskAction(const std::string& taskId, const std::string& action) {
    std::string raw = readFile(TASKS_FILE);
    std::vector<JsonObj> tasks = parseObjArray(raw);
    bool found = false;

    for (auto& t : tasks) {
        if (getStr(t, "id") == taskId) {
            found = true;
            if (action == "open") {
                openAttachment(getStr(t, "attachmentPath"));
            } else if (action == "snooze") {
                int count = getInt(t, "snoozeCount");
                if (count < 3) {
                    t["snoozeCount"] = jInt(count + 1);
                    auto now = std::chrono::system_clock::now();
                    auto snoozeTime = std::chrono::system_clock::to_time_t(
                        now + std::chrono::minutes(10));
                    t["snoozeUntil"] = jInt((int)snoozeTime);
                    t["status"]      = jStr("snoozed");
                } else {
                    t["status"] = jStr("ignored");
                }
            } else if (action == "ignore") {
                t["status"] = jStr("ignored");
            } else if (action == "missed") {
                t["status"] = jStr("missed");
            } else if (action == "completed") {
                t["status"] = jStr("completed");
            } else if (action == "pending") {
                t["status"] = jStr("pending");
            }
            break;
        }
    }

    if (found) {
        writeFile(TASKS_FILE, objArrayToString(tasks));
        broadcastEvent("{\"type\":\"update\"}");
        return true;
    }
    return false;
}


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
using namespace std;

const string TASKS_FILE = "data/tasks.json";

// ─── Cross-Platform: Open a file or URL with the default app ───
static void openAttachment(const string& path) {
    if (path.empty()) return;
    string cmd;
#if defined(_WIN32)
    cmd = "start \"\" \"" + path + "\"";
#elif defined(__APPLE__)
    cmd = "open \"" + path + "\"";
#else
    cmd = "xdg-open \"" + path + "\" &";
#endif
    system(cmd.c_str());
}

static string getCurrentDate() {
    auto now  = chrono::system_clock::now();
    time_t t = chrono::system_clock::to_time_t(now);
    tm* tm   = localtime(&t);
    ostringstream oss;
    oss << put_time(tm, "%Y-%m-%d");
    return oss.str();
}

bool addTask(const string& userId,
             const string& title,
             const string& description,
             const string& imageBase64,
             const string& startDate,
             const string& endDate,
             const string& frequency,
             const string& attachmentType,
             const string& attachmentPath,
             const string& alarmTime) {

    string raw = readFile(TASKS_FILE);
    vector<JsonObj> tasks = parseObjArray(raw);

    string id = "task_" + to_string(
        chrono::high_resolution_clock::now().time_since_epoch().count()
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

    cout << "[Task] Added: '" << title << "' for user " << userId << "\n";
    broadcastEvent("{\"type\":\"update\"}");
    return true;
}

string getTasksByUser(const string& userId) {
    string raw = readFile(TASKS_FILE);
    vector<JsonObj> tasks = parseObjArray(raw);
    vector<JsonObj> result;
    for (auto& t : tasks)
        if (getStr(t, "userId") == userId)
            result.push_back(t);
    return objArrayToString(result);
}

string getTasksForCalendar(const string& userId) {
    string raw = readFile(TASKS_FILE);
    vector<JsonObj> tasks = parseObjArray(raw);
    map<string, vector<string>> calendar;

    for (auto& t : tasks) {
        if (getStr(t, "userId") != userId) continue;
        string startDate = getStr(t, "startDate");
        string endDate   = getStr(t, "endDate");
        calendar[startDate].push_back(objToString(t));
        if (endDate != startDate) {
            JsonObj copy = t;
            copy["note"] = jStr("Due date");
            calendar[endDate].push_back(objToString(copy));
        }
    }

    string out = "{";
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

bool handleTaskAction(const string& taskId, const string& action) {
    string raw = readFile(TASKS_FILE);
    vector<JsonObj> tasks = parseObjArray(raw);
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
                    auto now = chrono::system_clock::now();
                    auto snoozeTime = chrono::system_clock::to_time_t(
                        now + chrono::minutes(10));
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

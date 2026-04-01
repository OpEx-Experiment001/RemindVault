
#include "tasks.h"
#include "storage.h"
#include "json_utils.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <map>

const std::string TASKS_FILE = "data/tasks.json";

static std::string getCurrentDate() {
    auto now   = std::chrono::system_clock::now();
    std::time_t t  = std::chrono::system_clock::to_time_t(now);
    std::tm* tm    = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d");
    return oss.str();
}


bool addTask(const std::string& userId,
            const std::string& title,
            const std::string& description,
            const std::string& imageBase64,
            const std::string& startDate,
            const std::string& endDate) {

    std::string raw = readFile(TASKS_FILE);
    std::vector<JsonObj> tasks = parseObjArray(raw);

    std::string id = "task_" + std::to_string(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()
    );

    JsonObj newTask;
    newTask["id"]          = jStr(id);
    newTask["userId"]      = jStr(userId);
    newTask["title"]       = jStr(title);
    newTask["description"] = jStr(description);
    newTask["image"]       = jStr(imageBase64);
    newTask["startDate"]   = jStr(startDate);
    newTask["endDate"]     = jStr(endDate);
    newTask["createdAt"]   = jStr(getCurrentDate());
    newTask["status"]      = jStr("pending");

    tasks.push_back(newTask);
    writeFile(TASKS_FILE, objArrayToString(tasks));

    std::cout << "[Task] Added: '" << title << "' for user " << userId << "\n";
    return true;
}


std::string getTasksByUser(const std::string& userId) {
    std::string raw = readFile(TASKS_FILE);
    std::vector<JsonObj> tasks = parseObjArray(raw);

    std::vector<JsonObj> result;
    for (auto& t : tasks) {
        if (getStr(t, "userId") == userId)
            result.push_back(t);
    }
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
        std::string taskJson  = objToString(t);

       
        calendar[startDate].push_back(taskJson);

       
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

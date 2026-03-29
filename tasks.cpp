// ============================================================
// tasks.cpp — Implementation of task management functions
// Tasks are saved in: data/tasks.json
// ============================================================

#include "tasks.h"
#include "storage.h"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <iostream>

// The file where all tasks are stored
const std::string TASKS_FILE = "data/tasks.json";

// -------------------------------------------------------
// Helper — Get today's date as a string like "2026-03-29"
// -------------------------------------------------------
std::string getCurrentDate() {
    auto now    = std::chrono::system_clock::now();
    std::time_t t  = std::chrono::system_clock::to_time_t(now);
    std::tm* tm    = std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(tm, "%Y-%m-%d");
    return oss.str();
}

// -------------------------------------------------------
// addTask — Save a new task to tasks.json
// -------------------------------------------------------
bool addTask(const std::string& userId,
             const std::string& title,
             const std::string& description,
             const std::string& imageBase64,
             const std::string& startDate,
             const std::string& endDate) {

    // Load existing tasks
    json tasks = readFile(TASKS_FILE);

    // Generate a unique task ID using current time
    std::string id = "task_" + std::to_string(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()
    );

    // Build the task object
    json newTask = {
        {"id",          id},
        {"userId",      userId},        // Which user this task belongs to
        {"title",       title},         // Short task name
        {"description", description},   // Full task details
        {"image",       imageBase64},   // Base64 image string, or "" if no image
        {"startDate",   startDate},     // When the task starts (YYYY-MM-DD)
        {"endDate",     endDate},       // When the task should be done (YYYY-MM-DD)
        {"createdAt",   getCurrentDate()},
        {"status",      "pending"}      // Can be: "pending", "completed", "missed"
    };

    tasks.push_back(newTask);
    writeFile(TASKS_FILE, tasks);

    std::cout << "[Task] Added: '" << title << "' for user " << userId << "\n";
    return true;
}

// -------------------------------------------------------
// getTasksByUser — Return all tasks for one user
// -------------------------------------------------------
json getTasksByUser(const std::string& userId) {
    json tasks     = readFile(TASKS_FILE);
    json userTasks = json::array();

    // Go through every task and keep only the ones that belong to this user
    for (auto& task : tasks) {
        if (task["userId"] == userId) {
            userTasks.push_back(task);
        }
    }

    return userTasks;
}

// -------------------------------------------------------
// getTasksForCalendar — Return tasks grouped by date
// This is what Anmol/Shivang will use to show tasks on the calendar
// -------------------------------------------------------
json getTasksForCalendar(const std::string& userId) {
    json tasks        = readFile(TASKS_FILE);
    json calendarData = json::object(); // A map of date -> list of tasks

    for (auto& task : tasks) {
        if (task["userId"] != userId) continue; // Skip other users' tasks

        std::string startDate = task["startDate"];
        std::string endDate   = task["endDate"];

        // Show the task on its START date
        if (!calendarData.contains(startDate)) {
            calendarData[startDate] = json::array();
        }
        calendarData[startDate].push_back(task);

        // Also show it on its END date (if different from start)
        if (endDate != startDate) {
            if (!calendarData.contains(endDate)) {
                calendarData[endDate] = json::array();
            }
            // Add with a note that this is the due/end date
            json taskCopy   = task;
            taskCopy["note"] = "Due date";
            calendarData[endDate].push_back(taskCopy);
        }
    }

    return calendarData;
}

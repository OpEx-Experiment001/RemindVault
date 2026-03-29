#pragma once
// ============================================================
// tasks.h — Everything related to tasks (add & retrieve)
// ============================================================

#include "json.hpp"
#include <string>

using json = nlohmann::json;

// Add a new task for a user.
// image can be a base64 string or just "" if no image.
// Dates must be in YYYY-MM-DD format (e.g. "2026-03-29")
bool addTask(const std::string& userId,
             const std::string& title,
             const std::string& description,
             const std::string& imageBase64,
             const std::string& startDate,
             const std::string& endDate);

// Get all tasks belonging to a specific user
json getTasksByUser(const std::string& userId);

// Get tasks organized by date — used for the calendar view
// Returns something like: { "2026-03-29": [...tasks], "2026-04-05": [...tasks] }
json getTasksForCalendar(const std::string& userId);

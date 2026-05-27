#pragma once

#include <string>

bool addTask(const std::string& userId,
             const std::string& title,
             const std::string& description,
             const std::string& imageBase64,
             const std::string& startDate,
             const std::string& endDate,
             const std::string& frequency,
             const std::string& attachmentType,
             const std::string& attachmentPath,
             const std::string& alarmTime);

std::string getTasksByUser(const std::string& userId);
std::string getTasksForCalendar(const std::string& userId);
bool handleTaskAction(const std::string& taskId, const std::string& action);

#pragma once

#include <string>

bool addTask(const std::string& userId,
            const std::string& title,
            const std::string& description,
            const std::string& imageBase64,
            const std::string& startDate,
            const std::string& endDate);

std::string getTasksByUser(const std::string& userId);
std::string getTasksForCalendar(const std::string& userId);

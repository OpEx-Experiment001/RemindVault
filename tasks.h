#pragma once

#include <string>
using namespace std;

bool addTask(const string& userId,
             const string& title,
             const string& description,
             const string& imageBase64,
             const string& startDate,
             const string& endDate,
             const string& frequency,
             const string& attachmentType,
             const string& attachmentPath,
             const string& alarmTime);

string getTasksByUser(const string& userId);
string getTasksForCalendar(const string& userId);
bool handleTaskAction(const string& taskId, const string& action);

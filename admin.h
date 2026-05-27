#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║  admin.h — Admin API Declarations                           ║
// ║  All endpoints require X-Admin-Token header.                ║
// ╚══════════════════════════════════════════════════════════════╝
#include <string>
using namespace std;

// The master token — change this before deploying!
extern const string ADMIN_TOKEN;

// Validate the token from a request header value
bool isAdminToken(const string& token);

// ─── Users ────────────────────────────────────────────────────
string adminGetUsers();
bool        adminDeleteUser(const string& userId);
bool        adminResetPassword(const string& userId,
                               const string& newPassword);
bool        adminEditUser(const string& userId,
                          const string& name,
                          const string& email,
                          const string& profession);

// ─── Tasks ────────────────────────────────────────────────────
string adminGetAllTasks();
bool        adminDeleteTask(const string& taskId);
bool        adminEditTask(const string& taskId,
                          const string& title,
                          const string& description,
                          const string& status,
                          const string& endDate,
                          const string& alarmTime);
bool        adminNukeAll();   // delete ALL data (use with caution)

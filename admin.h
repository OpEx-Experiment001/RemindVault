#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║  admin.h — Admin API Declarations                           ║
// ║  All endpoints require X-Admin-Token header.                ║
// ╚══════════════════════════════════════════════════════════════╝
#include <string>

// The master token — change this before deploying!
extern const std::string ADMIN_TOKEN;

// Validate the token from a request header value
bool isAdminToken(const std::string& token);

// ─── Users ────────────────────────────────────────────────────
std::string adminGetUsers();
bool        adminDeleteUser(const std::string& userId);
bool        adminResetPassword(const std::string& userId,
                               const std::string& newPassword);
bool        adminEditUser(const std::string& userId,
                          const std::string& name,
                          const std::string& email,
                          const std::string& profession);

// ─── Tasks ────────────────────────────────────────────────────
std::string adminGetAllTasks();
bool        adminDeleteTask(const std::string& taskId);
bool        adminEditTask(const std::string& taskId,
                          const std::string& title,
                          const std::string& description,
                          const std::string& status,
                          const std::string& endDate,
                          const std::string& alarmTime);
bool        adminNukeAll();   // delete ALL data (use with caution)

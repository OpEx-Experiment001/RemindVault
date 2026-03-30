#pragma once
// ============================================================
// auth.h — Register & login function declarations
// ============================================================
#include <string>

bool registerUser(const std::string& name, int age, const std::string& email,
                const std::string& gender, const std::string& profession,
                const std::string& password);

std::string loginUser(const std::string& name, const std::string& password);
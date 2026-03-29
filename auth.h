#pragma once
// ============================================================
// auth.h — Everything related to users (register & login)
// ============================================================

#include <string>

// Register a new user.
// Returns true if registration worked, false if the name is already taken.
bool registerUser(const std::string& name, int age, const std::string& email,
                  const std::string& gender, const std::string& profession,
                  const std::string& password);

// Login a user with their name and password.
// Returns the user's unique ID if correct, or empty string "" if wrong.
std::string loginUser(const std::string& name, const std::string& password);

// ============================================================
// auth.cpp — Implementation of register and login functions
// Users are saved in: data/users.json
// ============================================================

#include "auth.h"
#include "storage.h"
#include <chrono>
#include <iostream>

// The file where all users are stored
const std::string USERS_FILE = "data/users.json";

// -------------------------------------------------------
// registerUser — Save a new user to users.json
// -------------------------------------------------------
bool registerUser(const std::string& name, int age, const std::string& email,
                  const std::string& gender, const std::string& profession,
                  const std::string& password) {

    // Load the existing users list from the file
    json users = readFile(USERS_FILE);

    // Check if someone already has this name — names must be unique
    for (auto& user : users) {
        if (user["name"] == name) {
            std::cout << "[Register] FAILED — Name '" << name << "' already exists.\n";
            return false;
        }
    }

    // Generate a unique ID using the current time in nanoseconds
    std::string id = "user_" + std::to_string(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()
    );

    // Build the new user object
    json newUser = {
        {"id",         id},
        {"name",       name},
        {"age",        age},
        {"email",      email},
        {"gender",     gender},
        {"profession", profession}, // "student", "teacher", or "worker"
        {"password",   password}
        // NOTE FOR PRODUCTION: Never store plain passwords!
        // Use a hashing library like bcrypt before saving.
        // For this project/demo, plain text is okay.
    };

    // Add the new user and save back to file
    users.push_back(newUser);
    writeFile(USERS_FILE, users);

    std::cout << "[Register] SUCCESS — New user: " << name << " (ID: " << id << ")\n";
    return true;
}

// -------------------------------------------------------
// loginUser — Check name + password and return user ID
// -------------------------------------------------------
std::string loginUser(const std::string& name, const std::string& password) {

    // Load users from file
    json users = readFile(USERS_FILE);

    // Look for a user with matching name AND password
    for (auto& user : users) {
        if (user["name"] == name && user["password"] == password) {
            std::string userId = user["id"];
            std::cout << "[Login] SUCCESS — " << name << " logged in.\n";
            return userId; // Return the user's ID (frontend will store this)
        }
    }

    std::cout << "[Login] FAILED — Wrong name or password for: " << name << "\n";
    return ""; // Empty string means login failed
}

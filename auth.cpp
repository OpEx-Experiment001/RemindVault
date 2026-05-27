#include "auth.h"
#include "storage.h"
#include "json_utils.h"
#include "crypto.h"
#include <chrono>
#include <iostream>

const std::string USERS_FILE = "data/users.json";

bool registerUser(const std::string& name, int age, const std::string& email,
                const std::string& gender, const std::string& profession,
                const std::string& password) {

    std::string raw = readFile(USERS_FILE);
    std::vector<JsonObj> users = parseObjArray(raw);

    for (auto& u : users) {
        if (getStr(u, "name") == name) {
            std::cout << "[Register] FAILED — name '" << name << "' already taken.\n";
            return false;
        }
    }

    std::string id = "user_" + std::to_string(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()
    );

    JsonObj newUser;
    newUser["id"]         = jStr(id);
    newUser["name"]       = jStr(name);
    newUser["age"]        = jInt(age);
    newUser["email"]      = jStr(email);
    newUser["gender"]     = jStr(gender);
    newUser["profession"] = jStr(profession);
    std::string timestamp = std::to_string(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    newUser["password"]   = jStr(hashPassword(password, timestamp));

    users.push_back(newUser);
    writeFile(USERS_FILE, objArrayToString(users));

    std::cout << "[Register] SUCCESS — " << name << " (ID: " << id << ")\n";
    return true;
}

std::string loginUser(const std::string& name, const std::string& password) {
    std::string raw = readFile(USERS_FILE);
    std::vector<JsonObj> users = parseObjArray(raw);

    for (auto& u : users) {
        if (getStr(u, "name") == name && verifyPassword(password, getStr(u, "password"))) {
            std::string userId = getStr(u, "id");
            std::cout << "[Login] SUCCESS — " << name << "\n";
            return userId;
        }
    }

    std::cout << "[Login] FAILED — wrong name or password for: " << name << "\n";
    return ""; // Empty = failed
}

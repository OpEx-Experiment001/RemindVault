#include "auth.h"
#include "storage.h"
#include "json_utils.h"
#include "crypto.h"
#include <chrono>
#include <iostream>
using namespace std;

const string USERS_FILE = "data/users.json";

bool registerUser(const string& name, int age, const string& email,
                const string& gender, const string& profession,
                const string& password) {

    string raw = readFile(USERS_FILE);
    vector<JsonObj> users = parseObjArray(raw);

    for (auto& u : users) {
        if (getStr(u, "name") == name) {
            cout << "[Register] FAILED — name '" << name << "' already taken.\n";
            return false;
        }
    }

    string id = "user_" + to_string(
        chrono::high_resolution_clock::now().time_since_epoch().count()
    );

    JsonObj newUser;
    newUser["id"]         = jStr(id);
    newUser["name"]       = jStr(name);
    newUser["age"]        = jInt(age);
    newUser["email"]      = jStr(email);
    newUser["gender"]     = jStr(gender);
    newUser["profession"] = jStr(profession);
    string timestamp = to_string(chrono::system_clock::to_time_t(chrono::system_clock::now()));
    newUser["password"]   = jStr(hashPassword(password, timestamp));

    users.push_back(newUser);
    writeFile(USERS_FILE, objArrayToString(users));

    cout << "[Register] SUCCESS — " << name << " (ID: " << id << ")\n";
    return true;
}

string loginUser(const string& name, const string& password) {
    string raw = readFile(USERS_FILE);
    vector<JsonObj> users = parseObjArray(raw);

    for (auto& u : users) {
        if (getStr(u, "name") == name && verifyPassword(password, getStr(u, "password"))) {
            string userId = getStr(u, "id");
            cout << "[Login] SUCCESS — " << name << "\n";
            return userId;
        }
    }

    cout << "[Login] FAILED — wrong name or password for: " << name << "\n";
    return ""; // Empty = failed
}

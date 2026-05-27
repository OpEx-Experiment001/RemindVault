#pragma once
#include <string>
using namespace std;

bool registerUser(const string& name, int age, const string& email,
                const string& gender, const string& profession,
                const string& password);

string loginUser(const string& name, const string& password);

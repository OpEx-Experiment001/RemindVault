// ============================================================
// main.cpp — The main server file. This is where everything starts.
//
// HOW THIS WORKS (simple explanation):
// - This program runs a web server on your computer at port 8080
// - The frontend (Anmol & Shivang) sends requests to this server
// - We process the request, save/read data, and send back a response
// - All data is stored in JSON files inside a "data/" folder
//
// TO COMPILE AND RUN:
//   g++ -std=c++17 main.cpp auth.cpp tasks.cpp -o server -lpthread
//   ./server
//
// Then the server runs at: http://localhost:8080
// ============================================================

#include "httplib.h"   // HTTP server library (single header file)
#include "json.hpp"    // JSON library (single header file)
#include "auth.h"      // Our register & login functions
#include "tasks.h"     // Our task functions
#include <iostream>
#include <filesystem>

using json = nlohmann::json;

// -------------------------------------------------------
// Helper — Send a JSON response back to the frontend
// -------------------------------------------------------
void sendJSON(httplib::Response& res, int statusCode, const json& data) {
    res.status = statusCode;
    res.set_content(data.dump(), "application/json");
}

// -------------------------------------------------------
// Helper — Add CORS headers so the frontend can talk to backend
// (This is required when frontend and backend run on different ports)
// -------------------------------------------------------
void addCORS(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// -------------------------------------------------------
// MAIN — Start the server and define all routes
// -------------------------------------------------------
int main() {

    // Create the "data" folder where user and task files will be saved
    std::filesystem::create_directory("data");

    httplib::Server server;

    // Handle browser preflight requests (needed for CORS)
    server.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        addCORS(res);
        res.status = 204;
    });

    // ===========================================================
    // ROUTE 1: Register (Sign Up)
    // ----------------------------------------------------------
    // URL:    POST http://localhost:8080/register
    // Send:   { "name", "age", "email", "gender", "profession", "password" }
    // Get:    { "success": true/false, "message": "..." }
    // ===========================================================
    server.Post("/register", [](const httplib::Request& req, httplib::Response& res) {
        addCORS(res);

        try {
            // Parse the JSON body sent by the frontend
            json body = json::parse(req.body);

            // Extract each field
            std::string name       = body["name"];
            int         age        = body["age"];
            std::string email      = body["email"];
            std::string gender     = body["gender"];
            std::string profession = body["profession"]; // "student" / "teacher" / "worker"
            std::string password   = body["password"];

            // Try to register the user
            bool success = registerUser(name, age, email, gender, profession, password);

            if (success) {
                sendJSON(res, 201, {
                    {"success", true},
                    {"message", "Account created successfully! Please log in."}
                });
            } else {
                sendJSON(res, 409, {
                    {"success", false},
                    {"message", "This name is already taken. Please choose a different name."}
                });
            }

        } catch (const std::exception& e) {
            sendJSON(res, 400, {
                {"success", false},
                {"message", "Something went wrong. Please fill all fields correctly."}
            });
        }
    });

    // ===========================================================
    // ROUTE 2: Login
    // ----------------------------------------------------------
    // URL:    POST http://localhost:8080/login
    // Send:   { "name", "password" }
    // Get:    { "success": true/false, "userId": "...", "name": "..." }
    // ===========================================================
    server.Post("/login", [](const httplib::Request& req, httplib::Response& res) {
        addCORS(res);

        try {
            json body = json::parse(req.body);

            std::string name     = body["name"];
            std::string password = body["password"];

            // Try to log in — returns the user's ID if correct
            std::string userId = loginUser(name, password);

            if (!userId.empty()) {
                // Login successful — send back the userId
                // Frontend should save this userId (e.g. in localStorage)
                sendJSON(res, 200, {
                    {"success", true},
                    {"message", "Welcome back, " + name + "!"},
                    {"userId",  userId}, // <-- IMPORTANT: Frontend saves this!
                    {"name",    name}
                });
            } else {
                sendJSON(res, 401, {
                    {"success", false},
                    {"message", "Incorrect name or password. Please try again."}
                });
            }

        } catch (const std::exception& e) {
            sendJSON(res, 400, {
                {"success", false},
                {"message", "Something went wrong. Please try again."}
            });
        }
    });

    // ===========================================================
    // ROUTE 3: Add a Task
    // ----------------------------------------------------------
    // URL:    POST http://localhost:8080/tasks/add
    // Send:   { "userId", "title", "description", "image", "startDate", "endDate" }
    //         image = base64 string or "" if no image
    //         dates = "YYYY-MM-DD" format, e.g. "2026-03-29"
    // Get:    { "success": true/false, "message": "..." }
    // ===========================================================
    server.Post("/tasks/add", [](const httplib::Request& req, httplib::Response& res) {
        addCORS(res);

        try {
            json body = json::parse(req.body);

            std::string userId      = body["userId"];
            std::string title       = body["title"];
            std::string description = body["description"];
            std::string image       = body.value("image", ""); // Optional — default empty
            std::string startDate   = body["startDate"];
            std::string endDate     = body["endDate"];

            bool success = addTask(userId, title, description, image, startDate, endDate);

            if (success) {
                sendJSON(res, 201, {
                    {"success", true},
                    {"message", "Task added successfully!"}
                });
            } else {
                sendJSON(res, 500, {
                    {"success", false},
                    {"message", "Failed to save the task. Please try again."}
                });
            }

        } catch (const std::exception& e) {
            sendJSON(res, 400, {
                {"success", false},
                {"message", "Please fill all required fields (title, dates, userId)."}
            });
        }
    });

    // ===========================================================
    // ROUTE 4: Get All Tasks for a User
    // ----------------------------------------------------------
    // URL:    GET http://localhost:8080/tasks?userId=user_123
    // Get:    { "success": true, "tasks": [ {...}, {...} ] }
    // ===========================================================
    server.Get("/tasks", [](const httplib::Request& req, httplib::Response& res) {
        addCORS(res);

        // Read the userId from the URL query parameter
        std::string userId = req.get_param_value("userId");

        if (userId.empty()) {
            sendJSON(res, 400, {
                {"success", false},
                {"message", "Please provide a userId in the URL."}
            });
            return;
        }

        json tasks = getTasksByUser(userId);
        sendJSON(res, 200, {
            {"success", true},
            {"tasks",   tasks}
        });
    });

    // ===========================================================
    // ROUTE 5: Get Tasks for Calendar View
    // ----------------------------------------------------------
    // URL:    GET http://localhost:8080/tasks/calendar?userId=user_123
    // Get:    {
    //           "success": true,
    //           "calendar": {
    //             "2026-03-29": [ {...task...} ],
    //             "2026-04-05": [ {...task...} ]
    //           }
    //         }
    // NOTE for Anmol/Shivang: Use this endpoint to fill the calendar.
    // Each key is a date, and the value is a list of tasks on that date.
    // ===========================================================
    server.Get("/tasks/calendar", [](const httplib::Request& req, httplib::Response& res) {
        addCORS(res);

        std::string userId = req.get_param_value("userId");

        if (userId.empty()) {
            sendJSON(res, 400, {
                {"success", false},
                {"message", "Please provide a userId in the URL."}
            });
            return;
        }

        json calendarData = getTasksForCalendar(userId);
        sendJSON(res, 200, {
            {"success",  true},
            {"calendar", calendarData}
        });
    });

    // -------------------------------------------------------
    // Start the server — this line keeps the program running
    // -------------------------------------------------------
    std::cout << "\n========================================\n";
    std::cout << "  Backend Server Started!\n";
    std::cout << "  Running at: http://localhost:8080\n";
    std::cout << "  Waiting for requests...\n";
    std::cout << "========================================\n\n";

    server.listen("0.0.0.0", 8080);
    return 0;
}

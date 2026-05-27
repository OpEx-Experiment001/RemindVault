# DAA PBL — Backend (C++ REST API)
# RemindVault — Backend API & C++ Server
### Written by: Piyush & Aadrash | For frontend use by: Anmol & Shivang

---

## 🚀 Overview
RemindVault is a lightweight, custom-built C++ REST API and Task Management backend. 
It operates completely independently without using bulky external frameworks (like `httplib` or `json.hpp`), relying purely on native Windows sockets and a custom JSON parser.

### Core Features
- **Real-Time Sync**: Server-Sent Events (SSE) instantly push updates to the UI.
- **Native OS Integration**: C++ backend triggers native Windows File Browsers and system Alarm Popups.
- **Encrypted Storage**: All JSON files are symmetrically encrypted using an XOR cipher for security.
- **Admin Panel**: A dedicated `admin.html` dashboard secured by a Master Token for managing users and nuking databases.
- **Concurrency**: Multi-threaded request handling with thread-safe file operations (Mutex locks).

---

## 🛠 Setup & Run

### Step 1 — Compile the Server
Open your terminal (PowerShell or Command Prompt) and run:
```bat
g++ -std=c++17 main.cpp auth.cpp tasks.cpp crypto.cpp sse.cpp filebrowser.cpp alarm.cpp admin.cpp -o server.exe -lws2_32
```
*Note: Requires MinGW/g++ installed on Windows.*

### Step 2 — Run the Server
```bat
.\server.exe
```
You should see:
```
========================================
  Backend Server Started!
  Running at: http://localhost:8081
  No external libraries used.
  Waiting for requests...
========================================
```
The server will bind to `http://localhost:8081` to avoid Windows background socket inheritance bugs.

### Data files
All user and task data is automatically saved in a `data/` folder as encrypted JSON files:
- `data/users.json` — all registered users
- `data/tasks.json` — all tasks

---

### Step 3 — Open the App
You can access the apps natively via the backend's static file server:
- **Main Dashboard**: [http://localhost:8081/app](http://localhost:8081/app)
- **Admin Panel**: [http://localhost:8081/admin](http://localhost:8081/admin)

---

## 📁 File Structure
```text
daa-pbl-4th-sem/
├── main.cpp                 ← Core HTTP server, Socket connections, Routing
├── auth.h / auth.cpp        ← User Registration & Login
├── tasks.h / tasks.cpp      ← CRUD logic for tasks
├── alarm.h / alarm.cpp      ← Background thread for triggering OS-level Alarms
├── filebrowser.h / cpp      ← Spawns native Windows file picker dialogs
├── sse.h / sse.cpp          ← Server-Sent Events broadcaster
├── crypto.h / crypto.cpp    ← XOR Encryption utilities
├── storage.h                ← Thread-safe file read/write (Mutex Protected)
├── admin.h / admin.cpp      ← Admin routes & master token validation
├── Json_utils.h             ← Custom JSON parser & builder
├── platform.h               ← Cross-platform OS abstractions & Threads
├── RemindVault_Integrated.html ← Main frontend UI (served by backend)
├── admin.html               ← Admin dashboard UI (served by backend)
└── data/                    ← Auto-generated directory
    ├── users.json           ← Encrypted user database
    ├── tasks.json           ← Encrypted tasks database
```

---

## API Reference (For Anmol & Shivang)


This is a quick reference of all available routes on `http://localhost:8081`. It is easy to understand for anyone reading the codebase.

### 👤 Authentication
- **POST `/register`** - Create a new user.
- **POST `/login`** - Authenticate and retrieve `userId`.

### 📋 Tasks
- **GET `/tasks?userId=...`** - Fetch all tasks for a specific user.
- **GET `/tasks/calendar?userId=...`** - Fetch tasks mapped by date for calendar rendering.
- **POST `/tasks/add`** - Create a new task (includes alarm time, attachments, etc).
- **GET `/tasks/action?id=...&action=...`** - Perform an action on a task (`snooze`, `completed`, `missed`, `open`).
- **GET `/tasks/browse`** - Opens the native OS file picker and returns the selected file path.

### 📡 Real-Time Events
- **GET `/events`** - Establish an SSE connection to listen for `"update"` or `"alarm"` broadcasts.

### ⚙️ Admin Routes
*Requires header: `X-Admin-Token: rv@dmin#2026!`*
- **GET, PUT, DELETE `/admin/users`** - Manage users.
- **POST `/admin/password`** - Force reset a user's password.
- **GET, PUT, DELETE `/admin/tasks`** - Manage tasks across the entire database.
- **POST `/admin/nuke`** - Wipes all users and tasks permanently.

---

## 💻 API Integration Guide (For Anmol & Shivang)


The backend runs at **http://localhost:8081**. Below are detailed copy-pasteable JavaScript examples for the core frontend routes you will need to build the UI.

### 1. Sign Up / Register
**POST** `/register`

```js
fetch("http://localhost:8081/register", {
  method: "POST",
  headers: { "Content-Type": "application/json" },
  body: JSON.stringify({
    name: "Piyush",
    age: 20,
    email: "piyush@email.com",
    gender: "Male",
    profession: "student",   // "student" / "teacher" / "worker"
    password: "mypass123"
  })
})
```

**Response on success:**
```json
{ "success": true, "message": "Account created successfully! Please log in." }
```

**Response if name already taken:**
```json
{ "success": false, "message": "This name is already taken." }
```

---

### 2. Login
**POST** `/login`

```js
fetch("http://localhost:8081/login", {
  method: "POST",
  headers: { "Content-Type": "application/json" },
  body: JSON.stringify({ name: "Piyush", password: "mypass123" })
})
```

**Response on success:**
```json
{
  "success": true,
  "message": "Welcome back, Piyush!",
  "userId": "user_123456789",
  "name": "Piyush"
}
```

> ⚠️ **IMPORTANT:** Save the `userId` after login!
> `localStorage.setItem("userId", response.userId)`

---

### 3. Add a Task
**POST** `/tasks/add`

```js
fetch("http://localhost:8081/tasks/add", {
  method: "POST",
  headers: { "Content-Type": "application/json" },
  body: JSON.stringify({
    userId: "user_123456789",
    title: "Complete DAA Report",
    description: "Write and submit the project report",
    image: "",                // "" or base64 string
    startDate: "2026-03-29", // YYYY-MM-DD
    endDate: "2026-04-05"
  })
})
```

---

### 4. Get All Tasks
**GET** `/tasks?userId=user_123456789`

```js
fetch("http://localhost:8081/tasks?userId=user_123456789")
```

---

### 5. Get Tasks for Calendar
**GET** `/tasks/calendar?userId=user_123456789`

```js
fetch("http://localhost:8081/tasks/calendar?userId=user_123456789")
```

**Response:**
```json
{
  "success": true,
  "calendar": {
    "2026-03-29": [ { "title": "Complete DAA Report", "status": "pending", ... } ],
    "2026-04-05": [ { "title": "Complete DAA Report", "note": "Due date", ... } ]
  }
}
```



## 🌐 API Reference (Port 8081)

All requests should be made to `http://localhost:8081`.

### 👤 Authentication
- **POST `/register`** - Create a new user.
- **POST `/login`** - Authenticate and retrieve `userId`.

### 📋 Tasks
- **GET `/tasks?userId=...`** - Fetch all tasks for a specific user.
- **GET `/tasks/calendar?userId=...`** - Fetch tasks mapped by date for calendar rendering.
- **POST `/tasks/add`** - Create a new task (includes alarm time, attachments, etc).
- **GET `/tasks/action?id=...&action=...`** - Perform an action on a task (`snooze`, `completed`, `missed`, `open`).
- **GET `/tasks/browse`** - Opens the native OS file picker and returns the selected file path.

### 📡 Real-Time Events
- **GET `/events`** - Establish an SSE connection to listen for `"update"` or `"alarm"` broadcasts.

### ⚙️ Admin Routes
*Requires header: `X-Admin-Token: rv@dmin#2026!`*
- **GET, PUT, DELETE `/admin/users`** - Manage users.
- **POST `/admin/password`** - Force reset a user's password.
- **GET, PUT, DELETE `/admin/tasks`** - Manage tasks across the entire database.
- **POST `/admin/nuke`** - Wipes all users and tasks permanently.

---
*Developed for DAA PBL (4th Semester).*

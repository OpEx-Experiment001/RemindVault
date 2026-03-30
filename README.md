# DAA PBL — Backend (C++ REST API)
### Written by: Piyush & Aadrash | For frontend use by: Anmol & Shivang

---

## What is this?
This is the backend server for our project. It runs on your computer and handles:
- User registration & login
- Saving and retrieving tasks
- Providing task data for the calendar view

---

## How to Set Up & Run

### Step 1 — Compile
```
 g++ -std=c++17 main.cpp auth.cpp tasks.cpp -o server.exe -lws2_32
```
That's it. Nothing to download.

### Step 2 — Run
```
 server.exe
```
You should see:
```
========================================
  Backend Server Started!
  Running at: http://localhost:8080
  No external libraries used.
  Waiting for requests...
========================================
```

### Data files
All user and task data is automatically saved in a `data/` folder as JSON files:
- `data/users.json` — all registered users
- `data/tasks.json` — all tasks

---

## File Structure
```
daa_pbl_backend/
├── main.cpp        ← HTTP server (raw POSIX sockets, no httplib)
├── auth.h
├── auth.cpp        ← Register & login logic
├── tasks.h
├── tasks.cpp       ← Task logic
├── storage.h       ← File read/write helpers (std::fstream only)
├── json_utils.h    ← Manual JSON parser & builder (replaces json.hpp)
└── data/
    ├── users.json  ← Auto-created on first register
    └── tasks.json  ← Auto-created on first task
```

---

## API Reference (For Anmol & Shivang)

The backend runs at **http://localhost:8080** — same API as before, nothing changes on the frontend.

---

### 1. Sign Up / Register
**POST** `/register`

```js
fetch("http://localhost:8080/register", {
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
fetch("http://localhost:8080/login", {
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
fetch("http://localhost:8080/tasks/add", {
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

---

### 5. Get Tasks for Calendar
**GET** `/tasks/calendar?userId=user_123456789`

Response:
```json
{
  "success": true,
  "calendar": {
    "2026-03-29": [ { "title": "Complete DAA Report", "status": "pending", ... } ],
    "2026-04-05": [ { "title": "Complete DAA Report", "note": "Due date", ... } ]
  }
}
```

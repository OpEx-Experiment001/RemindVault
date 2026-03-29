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

### Step 1 — Download these 2 library files (free, single files)
Place both files in the **same folder** as the .cpp files:

| Library | Download Link |
|---------|--------------|
| `httplib.h` | https://github.com/yhirose/cpp-httplib/releases → download `httplib.h` |
| `json.hpp` | https://github.com/nlohmann/json/releases → download `json.hpp` |

### Step 2 — Compile
Open terminal in the project folder and run:
```
g++ -std=c++17 main.cpp auth.cpp tasks.cpp -o server -lpthread
```

### Step 3 — Run
```
./server
```
You should see:
```
========================================
  Backend Server Started!
  Running at: http://localhost:8080
  Waiting for requests...
========================================
```

### Data files
All user and task data is automatically saved in a `data/` folder as JSON files:
- `data/users.json` — all registered users
- `data/tasks.json` — all tasks

---

## API Reference (For Anmol & Shivang)

The backend runs at **http://localhost:8080**

---

### 1. Sign Up / Register
**POST** `/register`

```js
// JavaScript fetch example:
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
  body: JSON.stringify({
    name: "Piyush",
    password: "mypass123"
  })
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
> Store it like this: `localStorage.setItem("userId", response.userId)`
> You'll need it for all task requests.

**Response on wrong password:**
```json
{ "success": false, "message": "Incorrect name or password." }
```

---

### 3. Add a Task
**POST** `/tasks/add`

```js
fetch("http://localhost:8080/tasks/add", {
  method: "POST",
  headers: { "Content-Type": "application/json" },
  body: JSON.stringify({
    userId: "user_123456789",       // From login response
    title: "Complete DAA Report",
    description: "Write and submit the project report",
    image: "",                      // Leave empty OR put base64 image string here
    startDate: "2026-03-29",        // YYYY-MM-DD format
    endDate: "2026-04-05"           // YYYY-MM-DD format
  })
})
```

**For image upload** — convert the image to base64 first:
```js
// Convert image file to base64
const toBase64 = (file) => new Promise((resolve) => {
  const reader = new FileReader();
  reader.onload = () => resolve(reader.result);
  reader.readAsDataURL(file);
});

const base64Image = await toBase64(imageFile);
// Then put base64Image in the "image" field above
```

**Response:**
```json
{ "success": true, "message": "Task added successfully!" }
```

---

### 4. Get All Tasks (for task list page)
**GET** `/tasks?userId=user_123456789`

```js
const userId = localStorage.getItem("userId");
fetch(`http://localhost:8080/tasks?userId=${userId}`)
```

**Response:**
```json
{
  "success": true,
  "tasks": [
    {
      "id": "task_001",
      "userId": "user_123456789",
      "title": "Complete DAA Report",
      "description": "Write and submit the project report",
      "image": "",
      "startDate": "2026-03-29",
      "endDate": "2026-04-05",
      "createdAt": "2026-03-29",
      "status": "pending"
    }
  ]
}
```

---

### 5. Get Tasks for Calendar
**GET** `/tasks/calendar?userId=user_123456789`

```js
const userId = localStorage.getItem("userId");
fetch(`http://localhost:8080/tasks/calendar?userId=${userId}`)
```

**Response:**
```json
{
  "success": true,
  "calendar": {
    "2026-03-29": [
      { "title": "Complete DAA Report", "status": "pending", ... }
    ],
    "2026-04-05": [
      { "title": "Complete DAA Report", "note": "Due date", ... }
    ]
  }
}
```

> 💡 **For the calendar:** Each key is a date. Loop through the `calendar` object and for each date that has tasks, show a dot or task name on that calendar cell.

---

## File Structure
```
daa_pbl_backend/
├── main.cpp        ← Server entry point (routes defined here)
├── auth.h          ← Register & login function declarations
├── auth.cpp        ← Register & login logic
├── tasks.h         ← Task function declarations
├── tasks.cpp       ← Task logic
├── storage.h       ← File read/write helpers
├── httplib.h       ← (Download this) HTTP server library
├── json.hpp        ← (Download this) JSON library
└── data/
    ├── users.json  ← Auto-created when first user registers
    └── tasks.json  ← Auto-created when first task is added
```

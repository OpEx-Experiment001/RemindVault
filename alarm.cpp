// ╔══════════════════════════════════════════════════════════════╗
// ║  alarm.cpp — Native OS Alarm Popups + Recurrence Engine     ║
// ║  Windows: MessageBoxA (Yes=Open / No=Snooze / Cancel=Ignore)║
// ║  macOS:   osascript display alert with 3 buttons            ║
// ║  Linux:   zenity --question with extra Snooze button        ║
// ╚══════════════════════════════════════════════════════════════╝
#include "alarm.h"
#include "platform.h"
#include "tasks.h"
#include "storage.h"
#include "json_utils.h"
#include "sse.h"
#include "pipeutil.h"

#include <stdlib.h>
#include <string.h>
#include <chrono>
#include <ctime>
#include <string>
#include <vector>
#include <set>
#include <iostream>

// ─── Active alarm deduplication ───────────────────────────────
static std::set<std::string> activeAlarms;
static PlatMutex             activeAlarmsMutex;

// ─── Native popup: returns "open" | "snooze" | "ignore" ───────
static std::string showNativePopup(const std::string& title,
                                   const std::string& desc) {
#if defined(_WIN32)
    std::string msg = title + "\n\n" + desc +
        "\n\n[Yes] Open   [No] Snooze 10 min   [Cancel] Ignore";
    int r = MessageBoxA(NULL, msg.c_str(), "RemindVault  Alarm",
                        MB_YESNOCANCEL | MB_ICONINFORMATION | MB_TOPMOST);
    if (r == IDYES) return "open";
    if (r == IDNO)  return "snooze";
    return "ignore";

#elif defined(__APPLE__)
    std::string script =
        "osascript -e 'button returned of (display alert \"" + title +
        "\" message \"" + desc +
        "\" buttons {\"Ignore\", \"Snooze\", \"Open\"} "
        "default button \"Open\" giving up after 120)'";
    std::string btn = pipeCapture(script);
    if (btn == "Open")   return "open";
    if (btn == "Snooze") return "snooze";
    return "ignore";

#else
    // Linux: zenity; extra-button text is printed to stdout, exit 1
    std::string cmd =
        "result=$(zenity --question "
        "--title='RemindVault Alarm' "
        "--text='" + title + "\n" + desc + "' "
        "--ok-label='Open' "
        "--cancel-label='Ignore' "
        "--extra-button='Snooze' 2>/dev/null); "
        "code=$?; "
        "if [ \"$result\" = 'Snooze' ]; then echo snooze; "
        "elif [ $code -eq 0 ]; then echo open; "
        "else echo ignore; fi";
    return pipeCapture(cmd);
#endif
}

// ─── Per-alarm thread payload ──────────────────────────────────
struct AlarmArg {
    std::string taskId, title, desc, attachPath;
};

static THREAD_FN(alarmThreadProc) {
    AlarmArg* a = static_cast<AlarmArg*>(_arg);
    std::string taskId = a->taskId, title = a->title,
                desc   = a->desc,  path  = a->attachPath;
    delete a;

    std::string action = showNativePopup(title, desc);
    handleTaskAction(taskId, action);

    if (action == "open" && !path.empty()) {
#if defined(_WIN32)
        std::string cmd = "start \"\" \"" + path + "\"";
#elif defined(__APPLE__)
        std::string cmd = "open \"" + path + "\"";
#else
        std::string cmd = "xdg-open \"" + path + "\" &";
#endif
        system(cmd.c_str());
    }

    { PlatLock lock(activeAlarmsMutex); activeAlarms.erase(taskId); }
    THREAD_RETURN;
}

static void fireAlarm(const JsonObj& task) {
    std::string id = getStr(task, "id");
    { PlatLock lock(activeAlarmsMutex);
      if (activeAlarms.count(id)) return;
      activeAlarms.insert(id); }

    AlarmArg* a = new AlarmArg{id,
        getStr(task,"title"), getStr(task,"description"),
        getStr(task,"attachmentPath")};
    spawnThread((ThreadFn)alarmThreadProc, a);
    broadcastEvent("{\"type\":\"alarm\",\"id\":\"" + id + "\"}");
    std::cout << "[Alarm] Triggered: " << getStr(task,"title") << "\n";
}

// ─── Check if it's time to fire: date AND alarmTime match ─────
static bool isAlarmDue(const JsonObj& task, const std::string& today,
                        const std::string& nowHHMM) {
    std::string endDate   = getStr(task, "endDate");
    std::string alarmTime = getStr(task, "alarmTime"); // "HH:MM"

    if (endDate != today) return false;
    // If no alarmTime set, fire any time on due date (once per day)
    if (alarmTime.empty() || alarmTime == "none") return true;
    return alarmTime == nowHHMM;
}

// ─── Recurrence engine ─────────────────────────────────────────
static THREAD_FN(recurrenceThreadProc) {
    mutexInit(activeAlarmsMutex);

    while (true) {
        sleepMs(30000); // check every 30 seconds

        std::string raw = readFile("data/tasks.json");
        if (raw == "[]" || raw.empty()) continue;

        std::vector<JsonObj> tasks = parseObjArray(raw);

        auto now  = std::chrono::system_clock::now();
        std::time_t nowT = std::chrono::system_clock::to_time_t(now);
        std::tm* tmNow = std::localtime(&nowT);

        char todayBuf[16], hhmmBuf[8];
        std::strftime(todayBuf, sizeof(todayBuf), "%Y-%m-%d", tmNow);
        std::strftime(hhmmBuf,  sizeof(hhmmBuf),  "%H:%M",    tmNow);
        std::string today(todayBuf), nowHHMM(hhmmBuf);

        for (auto& task : tasks) {
            std::string status = getStr(task, "status");

            // Snooze expired → re-fire
            if (status == "snoozed") {
                int snoozeUntil = getInt(task, "snoozeUntil");
                if (snoozeUntil > 0 && (std::time_t)snoozeUntil <= nowT) {
                    handleTaskAction(getStr(task,"id"), "pending");
                    fireAlarm(task);
                }
            }
            // Pending + due today at the right time → fire
            else if (status == "pending") {
                if (isAlarmDue(task, today, nowHHMM))
                    fireAlarm(task);
            }
        }
    }
    THREAD_RETURN;
}

void startAlarmThread() {
    spawnThread((ThreadFn)recurrenceThreadProc, nullptr);
    std::cout << "[Alarm] Recurrence engine started (checks every 30s).\n";
}

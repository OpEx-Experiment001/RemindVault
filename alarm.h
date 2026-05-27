#pragma once
// ╔══════════════════════════════════════════════════════════════╗
// ║  alarm.h — Native Alarm Popup + Background Recurrence       ║
// ╚══════════════════════════════════════════════════════════════╝
#include <string>

// Starts the background thread that polls tasks and fires alarms.
// Call once from main() after network init.
void startAlarmThread();

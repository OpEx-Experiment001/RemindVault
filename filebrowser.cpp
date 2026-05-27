// ╔══════════════════════════════════════════════════════════════╗
// ║  filebrowser.cpp — Native OS File Picker                    ║
// ╚══════════════════════════════════════════════════════════════╝
#include "filebrowser.h"
#include "pipeutil.h"
#include <string>
using namespace std;

string browseForFile() {
#if defined(_WIN32)
    const string cmd =
        "powershell -NoProfile -Command \""
        "Add-Type -AssemblyName System.Windows.Forms; "
        "$dlg = New-Object System.Windows.Forms.OpenFileDialog; "
        "$dlg.Title = 'Select Attachment for RemindVault'; "
        "$dlg.Filter = 'All Files (*.*)|*.*'; "
        "if ($dlg.ShowDialog() -eq 'OK') { Write-Output $dlg.FileName }\"";
    return pipeCapture(cmd);

#elif defined(__APPLE__)
    return pipeCapture(
        "osascript -e "
        "'POSIX path of (choose file with prompt \"Select Attachment for RemindVault\")'");

#else
    string path = pipeCapture(
        "zenity --file-selection --title='Select Attachment for RemindVault' 2>/dev/null");
    if (path.empty())
        path = pipeCapture(
            "kdialog --getopenfilename ~ '*' --title 'Select Attachment for RemindVault' 2>/dev/null");
    return path;
#endif
}

#include "process_monitor.hpp"

#include <windows.h>
#include <tlhelp32.h>

#include <cwchar>
#include <sstream>

namespace {

constexpr const wchar_t* kGameProcess = L"cs2.exe";
constexpr const wchar_t* kMessagingProcesses[] = {
    L"QQ.exe",
    L"WeChat.exe",
    L"Weixin.exe",
    L"WXWork.exe",
    L"WeCom.exe",
};

bool equalsIgnoreCase(const wchar_t* left, const wchar_t* right) {
    return _wcsicmp(left, right) == 0;
}

bool isMessagingProcess(const wchar_t* name) {
    for (const wchar_t* target : kMessagingProcesses) {
        if (equalsIgnoreCase(name, target)) {
            return true;
        }
    }
    return false;
}

} // namespace

MonitorSnapshot ProcessMonitor::scan() const {
    MonitorSnapshot snapshot;
    HANDLE snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot_handle == INVALID_HANDLE_VALUE) {
        return snapshot;
    }

    PROCESSENTRY32W process{};
    process.dwSize = sizeof(process);
    if (Process32FirstW(snapshot_handle, &process)) {
        do {
            if (equalsIgnoreCase(process.szExeFile, kGameProcess)) {
                snapshot.cs2_running = true;
                ++snapshot.cs2_processes;
            }
            if (isMessagingProcess(process.szExeFile)) {
                ++snapshot.target_processes;
            }
        } while (Process32NextW(snapshot_handle, &process));
    }

    CloseHandle(snapshot_handle);
    return snapshot;
}

CloseReport ProcessMonitor::closeMessagingApps() const {
    CloseReport report;
    HANDLE snapshot_handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot_handle == INVALID_HANDLE_VALUE) {
        report.failed = 1;
        report.detail = "无法读取 Windows 进程列表。";
        return report;
    }

    PROCESSENTRY32W entry{};
    entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot_handle, &entry)) {
        do {
            if (!isMessagingProcess(entry.szExeFile)) {
                continue;
            }

            ++report.found;
            HANDLE process = OpenProcess(PROCESS_TERMINATE, FALSE, entry.th32ProcessID);
            if (process == nullptr) {
                ++report.failed;
                continue;
            }

            if (TerminateProcess(process, 0)) {
                ++report.force_terminated;
            } else {
                ++report.failed;
            }
            CloseHandle(process);
        } while (Process32NextW(snapshot_handle, &entry));
    }
    CloseHandle(snapshot_handle);

    std::ostringstream message;
    if (report.found == 0) {
        message << "未发现正在运行的受控通讯软件。";
    } else {
        message << "已发现 " << report.found << " 个受控进程，已直接强制结束 "
                << report.force_terminated << " 个";
        if (report.failed > 0) {
            message << "，失败 " << report.failed << " 个";
        }
        message << ".";
    }
    report.detail = message.str();
    return report;
}

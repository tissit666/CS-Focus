#pragma once

#include <string>

struct MonitorSnapshot {
    bool cs2_running = false;
    unsigned int cs2_processes = 0;
    unsigned int target_processes = 0;
};

struct CloseReport {
    unsigned int found = 0;
    unsigned int force_terminated = 0;
    unsigned int failed = 0;
    std::string detail;
};

class ProcessMonitor {
public:
    MonitorSnapshot scan() const;
    CloseReport closeMessagingApps() const;
};

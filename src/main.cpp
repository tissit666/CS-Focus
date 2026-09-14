#include "eui_neo.h"

#include "process_monitor.hpp"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <sstream>
#include <string>

namespace app {
namespace {

struct GuardState {
    ProcessMonitor monitor;
    MonitorSnapshot snapshot;
    bool automatic = true;
    float elapsed = 1.0f;
    unsigned int total_closed = 0;
    std::string last_event = "正在等待首次进程扫描...";
    std::string last_scan = "尚未扫描";
};

std::string statusText(const GuardState& state) {
    if (state.snapshot.cs2_running) {
        return "检测到 CS2 正在运行，将关闭通讯软件";
    }
    return "未检测到 CS2，监控处于待命状态";
}

std::string processSummary(const GuardState& state) {
    std::ostringstream out;
    out << "CS2 进程：" << state.snapshot.cs2_processes
        << "   |   受控通讯进程：" << state.snapshot.target_processes;
    return out.str();
}

void scanAndHandle(GuardState& state, bool close_when_detected) {
    state.snapshot = state.monitor.scan();
    const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char timestamp[32]{};
    std::strftime(timestamp, sizeof(timestamp), "%H:%M:%S", std::localtime(&now));
    state.last_scan = std::string("最近扫描：") + timestamp;

    if (state.snapshot.cs2_running && close_when_detected) {
        const CloseReport report = state.monitor.closeMessagingApps();
        state.total_closed += report.found;
        state.last_event = report.detail;
        state.snapshot = state.monitor.scan();
    } else if (!state.snapshot.cs2_running) {
        state.last_event = "未检测到 CS2，未关闭任何应用。";
    } else {
        state.last_event = "已检测到 CS2，但自动处理目前已暂停。";
    }
}

void drawMetric(eui::Ui& ui,
                const std::string& id,
                float x,
                float width,
                const std::string& label,
                const std::string& value,
                const eui::Color& accent,
                const components::theme::ThemeColorTokens& tokens) {
    ui.rect(id + ".background")
        .position(x, 0.0f)
        .size(width, 88.0f)
        .radius(8.0f)
        .color(tokens.surfaceHover)
        .border(1.0f, components::theme::withOpacity(tokens.border, 0.8f))
        .build();
    ui.rect(id + ".accent")
        .position(x, 0.0f)
        .size(4.0f, 88.0f)
        .radius(2.0f)
        .color(accent)
        .build();
    ui.text(id + ".label")
        .position(x + 18.0f, 16.0f)
        .size(width - 32.0f, 18.0f)
        .text(label)
        .fontSize(12.0f)
        .lineHeight(18.0f)
        .color(components::theme::withOpacity(tokens.text, 0.62f))
        .build();
    ui.text(id + ".value")
        .position(x + 18.0f, 40.0f)
        .size(width - 32.0f, 30.0f)
        .text(value)
        .fontSize(22.0f)
        .lineHeight(28.0f)
        .color(tokens.text)
        .build();
}

} // namespace

const DslAppConfig& dslAppConfig() {
    static const DslAppConfig config = DslAppConfig{}
        .title("CS2 专注守卫")
        .pageId("cs2_focus_guard")
        .clearColor({0.055f, 0.065f, 0.085f, 1.0f})
        .windowSize(880, 620)
        .fps(30.0)
        .textFont("assets/JingNanJunJunTi-JinNanJunJunTi-Bold-2.ttf");
    return config;
}

void compose(eui::Ui& ui, const eui::Screen& screen) {
    GuardState& state = ui.state<GuardState>("guard.state");
    const components::theme::ThemeColorTokens tokens = components::theme::dark();
    const float margin = 28.0f;
    const float width = std::max(360.0f, screen.width - margin * 2.0f);
    const float panel_height = 156.0f;
    const float metric_gap = 12.0f;
    const float metric_width = (width - metric_gap * 2.0f) / 3.0f;
    const eui::Color active = {0.26f, 0.82f, 0.52f, 1.0f};
    const eui::Color inactive = {0.52f, 0.60f, 0.70f, 1.0f};
    const eui::Color danger = {0.94f, 0.39f, 0.32f, 1.0f};
    const eui::Color status_color = state.snapshot.cs2_running ? danger : active;

    ui.stack("page")
        .size(screen.width, screen.height)
        .content([&] {
            ui.rect("page.background")
                .size(screen.width, screen.height)
                .color({0.055f, 0.065f, 0.085f, 1.0f})
                .onFrame([&state](float dt) {
                    state.elapsed += dt;
                    if (state.elapsed >= 1.0f) {
                        state.elapsed = 0.0f;
                        scanAndHandle(state, state.automatic);
                    }
                })
                .build();

            ui.column("content")
                .position(margin, 24.0f)
                .size(width, std::max(0.0f, screen.height - 48.0f))
                .gap(14.0f)
                .content([&] {
                    ui.text("eyebrow")
                        .height(18.0f)
                        .text("系统进程监控")
                        .fontSize(12.0f)
                        .lineHeight(18.0f)
                        .color(tokens.primary)
                        .build();
                    ui.text("title")
                        .height(40.0f)
                        .text("CS2 专注守卫")
                        .fontSize(30.0f)
                        .lineHeight(38.0f)
                        .color(tokens.text)
                        .build();
                    ui.text("subtitle")
                        .height(22.0f)
                        .text("检测到 cs2.exe 后，自动关闭 QQ、微信、企业微信等通讯软件。")
                        .fontSize(14.0f)
                        .lineHeight(20.0f)
                        .color(components::theme::withOpacity(tokens.text, 0.66f))
                        .build();

                    ui.stack("status.panel")
                        .size(width, panel_height)
                        .content([&] {
                            ui.rect("status.background")
                                .size(width, panel_height)
                                .radius(8.0f)
                                .color(tokens.surface)
                                .border(1.0f, components::theme::withOpacity(status_color, 0.64f))
                                .build();
                            ui.rect("status.strip")
                                .size(6.0f, panel_height)
                                .radius(3.0f)
                                .color(status_color)
                                .build();
                            ui.text("status.label")
                                .position(24.0f, 22.0f)
                                .size(width - 48.0f, 18.0f)
                                .text(state.snapshot.cs2_running ? "已检测到游戏" : "待命中")
                                .fontSize(12.0f)
                                .lineHeight(18.0f)
                                .color(status_color)
                                .build();
                            ui.text("status.value")
                                .position(24.0f, 48.0f)
                                .size(width - 48.0f, 30.0f)
                                .text(statusText(state))
                                .fontSize(21.0f)
                                .lineHeight(28.0f)
                                .color(tokens.text)
                                .build();
                            ui.text("status.detail")
                                .position(24.0f, 98.0f)
                                .size(width - 48.0f, 22.0f)
                                .text(processSummary(state))
                                .fontSize(13.0f)
                                .lineHeight(20.0f)
                                .color(components::theme::withOpacity(tokens.text, 0.62f))
                                .build();
                        })
                        .build();

                    ui.stack("metrics")
                        .size(width, 88.0f)
                        .content([&] {
                            drawMetric(ui, "metric.cs2", 0.0f, metric_width, "CS2 进程数",
                                       std::to_string(state.snapshot.cs2_processes), status_color, tokens);
                            drawMetric(ui, "metric.targets", metric_width + metric_gap, metric_width,
                                       "通讯软件进程数", std::to_string(state.snapshot.target_processes),
                                       inactive, tokens);
                            drawMetric(ui, "metric.closed", (metric_width + metric_gap) * 2.0f, metric_width,
                                       "已处理进程数", std::to_string(state.total_closed), active, tokens);
                        })
                        .build();

                    ui.row("controls")
                        .width(width)
                        .height(48.0f)
                        .gap(12.0f)
                        .alignItems(eui::Align::CENTER)
                        .content([&] {
                            components::toggleSwitch(ui, "automatic")
                                .size(250.0f, 42.0f)
                                .checked(state.automatic)
                                .text("自动监控")
                                .theme(tokens)
                                .onChange([&state](bool value) {
                                    state.automatic = value;
                                    state.last_event = value ? "已启用自动监控。" : "已暂停自动监控。";
                                })
                                .build();
                            components::button(ui, "scan")
                                .size(132.0f, 42.0f)
                                .text("立即扫描")
                                .theme(tokens, false)
                                .onClick([&state] { scanAndHandle(state, false); })
                                .build();
                            components::button(ui, "handle")
                                .size(174.0f, 42.0f)
                                .text("处理检测到的应用")
                                .theme(tokens, true)
                                .onClick([&state] { scanAndHandle(state, true); })
                                .build();
                        })
                        .build();

                    ui.stack("activity")
                        .size(width, 78.0f)
                        .content([&] {
                            ui.rect("activity.background")
                                .size(width, 78.0f)
                                .radius(8.0f)
                                .color(tokens.surfaceHover)
                                .border(1.0f, components::theme::withOpacity(tokens.border, 0.7f))
                                .build();
                            ui.text("activity.event")
                                .position(16.0f, 14.0f)
                                .size(width - 32.0f, 24.0f)
                                .text(state.last_event)
                                .fontSize(13.0f)
                                .lineHeight(20.0f)
                                .color(tokens.text)
                                .build();
                            ui.text("activity.scan")
                                .position(16.0f, 45.0f)
                                .size(width - 32.0f, 18.0f)
                                .text(state.last_scan + "  |  检测到目标进程后将立即强制结束。")
                                .fontSize(12.0f)
                                .lineHeight(18.0f)
                                .color(components::theme::withOpacity(tokens.text, 0.58f))
                                .build();
                        })
                        .build();
                })
                .build();
        })
        .build();
}

} // namespace app

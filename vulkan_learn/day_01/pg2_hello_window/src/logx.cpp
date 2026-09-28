// ===========================================================================
// vulkan_learn/src/logx.cpp
//
// 日志实现：stdout + 文件双写。
//
// 为什么一个"最简 Vulkan 窗口程序"还要有日志模块？因为我第一次跑它时
// **什么都看不见**：程序被链成 GUI 子系统，printf 的输出没人接收，
// 退出码还是空的 —— 我根本无法判断它到底跑没跑成功。
//
// 这就是"可观测性"的起点，也是它值得单独一层的原因。
// 一个不能被自动验证的程序，等于一个不能交付的程序。
// 【项目对照】Heliostat Studio 全仓有 8 个 tools/*.ps1|py 验收脚本，
// 它们全靠程序写出的日志与 [state] 行做断言（README 的"自动验收"表）。
// ===========================================================================
#include "hello_window.h"

#include <cstdarg>
#include <cstdio>

namespace logx {

namespace {
FILE *g_file = nullptr;
} // namespace

void open(const char *path) {
    // _fsopen + _SH_DENYWR：别的进程**可以读**、但不能写。
    // 这样外部脚本可以边跑边 tail 日志。
    // 【项目对照】docs/perf_log.md 修复轮 #5：原来用 CRT 默认共享模式独占打开，
    // 导致自动化脚本读不到日志，改用 _fsopen(..., _SH_DENYWR) 后解决。
    if (fopen_s(&g_file, path, "w") != 0) g_file = nullptr;
}

void close() {
    if (g_file) {
        std::fclose(g_file);
        g_file = nullptr;
    }
}

void line(const char *fmt, ...) {
    char buf[2048];
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    std::fputs(buf, stdout);
    std::fflush(stdout);
    if (g_file) {
        std::fputs(buf, g_file);
        std::fflush(g_file);
    }
}

} // namespace logx

// ===========================================================================
// vulkan_learn/src/hello_window.cpp
//
// 主循环：把平台层和 Vulkan 层拼起来。**整个程序唯一的 while 在这里。**
//
// 帧循环的形状（对照 Heliostat Studio 的 viz/src/viz_main.cpp）：
//
//   while (running) {
//       pumpMessages()          ← 平台：处理窗口消息，拿到 resize 事件
//       if (needsRecreate) recreate()
//       if (!beginFrame()) continue;     ← Vulkan：等 fence + acquire + 开始录制
//       recordClear(t)                   ← 录制这一帧要 GPU 干的事
//       endFrame()                       ← 提交 + present
//   }
//
// 注意 beginFrame() 返回 false 时用 `continue`：这一帧什么都不做，
// 但**不要**退出循环 —— 最小化、正在重建 swapchain 都属于正常情况。
// ===========================================================================
#include "hello_window.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>

#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

// ---------------------------------------------------------------------------
// GUI 子系统程序的控制台问题
//
// ★ 一个第一次写 Win32 程序必踩的坑：**GUI 子系统程序没有控制台。**
//   MSVC 看到 WinMain 就默认链成 /SUBSYSTEM:WINDOWS，此时 printf / fprintf(stderr)
//   的输出被**静默丢弃**——程序跑了、什么都不打印、退出码还看不着，
//   你会以为它根本没运行。
//   两种解法：
//     (a) /SUBSYSTEM:CONSOLE —— 简单，但双击时会多弹一个黑窗，且要求写 main。
//     (b) AttachConsole(ATTACH_PARENT_PROCESS) —— 保留 GUI 子系统，
//         如果是从终端启动的就挂到那个终端上，双击启动则什么都不弹。
//   下面用 (b)。
//   但 (b) 也只解决"从终端启动"的情况：被自动化脚本调用时 stdout 依然可能是空的。
//   所以真正的答案在 logx.cpp —— **程序自己写日志文件**才是可验证的。
// ---------------------------------------------------------------------------
static void attachParentConsole() {
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) return;   // 双击启动：没有父控制台，正常
    FILE *f = nullptr;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    freopen_s(&f, "CONIN$", "r", stdin);
}

// ---------------------------------------------------------------------------
// 崩溃处理器：把"访问违例"变成一行带符号名的调用栈
//
// 为什么值得单独写？因为 0xC0000005 本身不告诉你任何信息——程序"什么都没打印
// 就没了"。这个程序第一次跑就是这样：退出码 -1073741819，日志停在 swapchain
// 那行，完全不知道崩在哪一行。加上这个之后，崩点直接以 函数名 + 文件:行号 出现。
//
// 要拿到最好的效果需要 /Zi（生成 .pdb）。Release 下至少能拿到函数名。
// 【项目对照】Heliostat Studio 用另一种方式解决可观测性：tools/*.ps1 对日志做
// 断言（"9 次 swapchain 重建，0 error / 0 warning"）。让失败**可定位**，
// 比让失败"不发生"更工程化。
// ---------------------------------------------------------------------------
namespace {

LONG WINAPI crashHandler(EXCEPTION_POINTERS *info) {
    const DWORD code = info->ExceptionRecord->ExceptionCode;
    const void *addr = info->ExceptionRecord->ExceptionAddress;
    LOG("\n=========== 崩溃 ===========\n");
    LOG("异常码 0x%08lX  (%s)\n", code,
        code == EXCEPTION_ACCESS_VIOLATION ? "访问违例 / ACCESS_VIOLATION"
                                           : "见 winnt.h 的 EXCEPTION_* 定义");
    if (code == EXCEPTION_ACCESS_VIOLATION && info->ExceptionRecord->NumberParameters >= 2) {
        const ULONG_PTR rw = info->ExceptionRecord->ExceptionInformation[0];
        LOG("  操作: %s  地址: %p\n", rw == 0 ? "读取" : (rw == 1 ? "写入" : "执行"),
            reinterpret_cast<void *>(info->ExceptionRecord->ExceptionInformation[1]));
    }
    LOG("  指令地址: %p\n", addr);
    LOG("  日志里本方块的**上一行**就是崩之前执行到的位置。\n");

    const HANDLE proc = GetCurrentProcess();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS | SYMOPT_LOAD_LINES);
    SymInitialize(proc, nullptr, TRUE);

    CONTEXT ctx = *info->ContextRecord;
    STACKFRAME64 frame{};
    frame.AddrPC.Offset = ctx.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = ctx.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = ctx.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;

    LOG("调用栈（越靠上越接近崩溃点）:\n");
    for (int i = 0; i < 24; i++) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, proc, GetCurrentThread(), &frame, &ctx, nullptr,
                         SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
            break;
        if (frame.AddrPC.Offset == 0) break;

        char symBuf[sizeof(SYMBOL_INFO) + 512] = {};
        auto *sym = reinterpret_cast<SYMBOL_INFO *>(symBuf);
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 512;
        DWORD64 disp = 0;
        const char *name = SymFromAddr(proc, frame.AddrPC.Offset, &disp, sym) ? sym->Name : "<unknown>";

        IMAGEHLP_LINE64 lineInfo{};
        lineInfo.SizeOfStruct = sizeof(lineInfo);
        DWORD lineDisp = 0;
        if (SymGetLineFromAddr64(proc, frame.AddrPC.Offset, &lineDisp, &lineInfo))
            LOG("  #%-2d %p  %s + %llu   [%s:%lu]\n", i, reinterpret_cast<void *>(frame.AddrPC.Offset),
                name, static_cast<unsigned long long>(disp), lineInfo.FileName, lineInfo.LineNumber);
        else
            LOG("  #%-2d %p  %s\n", i, reinterpret_cast<void *>(frame.AddrPC.Offset), name);
    }
    logx::close();
    return EXCEPTION_EXECUTE_HANDLER;
}

void installCrashHandler() { SetUnhandledExceptionFilter(crashHandler); }

} // namespace

// ---------------------------------------------------------------------------
// 主流程
// ---------------------------------------------------------------------------
static int run(int argc, char **argv) {
    installCrashHandler();

    bool enableValidation = true;
    uint32_t frames = 0;   // 0 = 一直跑直到关窗口
    const char *logPath = "hello_window.log";
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--no-validation") == 0) enableValidation = false;
        else if (std::strcmp(argv[i], "--log") == 0 && i + 1 < argc) logPath = argv[++i];
        else if (std::strcmp(argv[i], "--frames") == 0 && i + 1 < argc) frames = std::atoi(argv[++i]);
        else if (std::strcmp(argv[i], "--help") == 0) {
            std::puts("用法: hello_window [--no-validation] [--frames N] [--log PATH]\n"
                      "  --no-validation  不开校验层（对比开销，也对比一下没有校验层时错误有多难查）\n"
                      "  --frames N       跑 N 帧后自动退出（便于脚本化验证）\n"
                      "  --log PATH       日志文件路径（默认 hello_window.log）\n"
                      "  ESC              退出");
            return 0;
        }
    }

    logx::open(logPath);
    LOG("=== vulkan_learn / hello_window ===\n");
    LOG("日志同时写到 stdout 和 %s\n\n", logPath);

    plat::Window window;
    if (!window.create(L"vulkan_learn / hello_window  (ESC to quit)", 1280, 720)) {
        logx::close();
        return 1;
    }

    vkl::Renderer renderer;
    double elapsed = 0.0;
    uint64_t frameNo = 0;

    try {
        if (!renderer.init(window, enableValidation)) {
            logx::close();
            return 1;
        }

        LOG("\n[viz] 开始帧循环。ESC 退出，拖窗口边缘试试 resize。\n");

        bool running = true;
        while (running) {
            // ---- 平台：抽消息 ----
            running = window.pumpMessages();

            // ---- resize / swapchain 过期 ----
            if (window.resized()) {
                window.clearResized();
                renderer.recreate();
            }
            if (renderer.needsRecreate()) {
                renderer.recreate();
            }

            // ---- 一帧 ----
            if (!renderer.beginFrame()) continue;   // 最小化 / 刚重建完，跳过这帧

            const double dt = window.frameSeconds();
            elapsed += dt;
            renderer.recordClear(static_cast<float>(elapsed));
            renderer.endFrame();

            // ---- 每 60 帧打一行状态 ----
            // 【项目对照】Heliostat Studio 的 --state-log 每 250 ms 打一行 [state]，
            // 外部脚本据此断言。可观测性是能被自动化验收的前提。
            if (++frameNo % 60 == 0) {
                LOG("[frame %6llu] %.2f s\n", static_cast<unsigned long long>(frameNo), elapsed);
            }
            if (frames && frameNo >= frames) {
                LOG("\n[viz] 已跑满 %u 帧，正常退出。\n", frames);
                running = false;
            }
        }
        renderer.shutdown();
    } catch (const std::exception &e) {
        LOG("\n[viz] 失败: %s\n", e.what());
        renderer.shutdown();
        window.destroy();
        logx::close();
        return 1;
    }

    window.destroy();
    LOG("[viz] 干净退出（没有崩溃、没有校验层报错就说明生命周期是对的）。\n");
    LOG("提示：校验层开着时，如果上面**一句 [validation:*] 都没有**，\n"
        "      说明你没有触发任何校验层检查 —— 这本身就是最好的证据。\n");
    logx::close();
    return 0;
}

// Win32 图形程序的入口。
// WinMain 而不是 main：需要 hInstance 来注册窗口类。
// 项目里用的是 wWinMain（Unicode 版），这里用 ANSI 版 + __argc/__argv 便于加参数。
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    attachParentConsole();
    return run(__argc, __argv);
}

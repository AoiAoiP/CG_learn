// ===========================================================================
// vulkan_learn/src/platform_win32.cpp
//
// 平台层：把一个 Win32 窗口包起来。**这里面没有一行 Vulkan 代码。**
//
// 为什么要单独一层？因为"开窗口"和"用 GPU"是两个正交的问题。
//   * OpenGL 时代你几乎感觉不到：GLFW 一个 glfwCreateWindow 两件事都做了
//     （它内部还顺手建了 GL context 和 WGL 那套东西）。
//   * Vulkan 把它们彻底拆开：窗口是**操作系统**的事，GPU 是**驱动**的事，
//     中间的桥是 VkSurfaceKHR。所以你的程序里也应该这么拆。
//
// 对照 Heliostat Studio：viz/src/platform_win32.cpp（238 行）就是这一层，
// 它不知道 Vulkan 的存在；viz/src/vk_context.cpp 才是 Vulkan 那一层。
// ===========================================================================
#include "hello_window.h"

#include <cstdio>

namespace plat {

LRESULT CALLBACK Window::wndProcThunk(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    // 窗口创建时把 this 指针塞进 GWLP_USERDATA，这样静态回调能派发到成员函数。
    // （Win32 的经典套路：C 风格回调 + 用户数据）
    if (msg == WM_NCCREATE) {
        auto *cs = reinterpret_cast<CREATESTRUCTW *>(l);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(cs->lpCreateParams));
        auto *self = reinterpret_cast<Window *>(cs->lpCreateParams);
        self->m_hwnd = hwnd;
    }
    auto *self = reinterpret_cast<Window *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (!self) return DefWindowProcW(hwnd, msg, w, l);
    return self->wndProc(hwnd, msg, w, l);
}

LRESULT Window::wndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_SIZE:
        // 收到 WM_SIZE 不意味着"现在就能重建 swapchain"——此时窗口可能正在被拖动，
        // 尺寸每帧都在变。本课的做法和图省事的做法一样：只记一个标志，
        // 真正的重建放到主循环开头（那里 GPU 已经空闲）。
        m_width = LOWORD(l);
        m_height = HIWORD(l);
        m_minimized = (w == SIZE_MINIMIZED);
        if (!m_minimized) m_resized = true;
        return 0;

    case WM_KEYDOWN:
        // ESC 退出（对比 Heliostat Studio 的按键表：它把每个键做成可发现的提示行）
        if (w == VK_ESCAPE) { PostMessageW(hwnd, WM_CLOSE, 0, 0); return 0; }
        return 0;

    case WM_DESTROY:
    case WM_CLOSE:
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, msg, w, l);
    }
}

bool Window::create(const wchar_t *title, uint32_t width, uint32_t height) {
    HINSTANCE inst = GetModuleHandleW(nullptr);

    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    // CS_OWNDC 对 Vulkan 不是必需的（那是 OpenGL 的老习惯），但无害。
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = &Window::wndProcThunk;
    wc.hInstance = inst;
    wc.hCursor = LoadCursorA(nullptr, IDC_ARROW);
    wc.lpszClassName = L"VulkanLearnWindow";
    if (!RegisterClassExW(&wc)) {
        LOG("[plat] RegisterClassExW failed (%lu)\n", GetLastError());
        return false;
    }

    // 注意：CreateWindow 的宽高是**整个窗口**（含标题栏和边框），
    // 而我们想要的是**客户区**是 width×height。AdjustWindowRect 做这个换算。
    // 这是新手最常见的"我的视口比窗口小一圈"的原因。
    RECT rect{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowExW(0, wc.lpszClassName, title,
                                WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                rect.right - rect.left, rect.bottom - rect.top,
                                nullptr, nullptr, inst, this);
    if (!hwnd) {
        LOG("[plat] CreateWindowExW failed (%lu)\n", GetLastError());
        return false;
    }

    m_width = width;
    m_height = height;
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    m_lastTime = frameSeconds();
    return true;
}

void Window::destroy() {
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    UnregisterClassW(L"VulkanLearnWindow", GetModuleHandleW(nullptr));
}

bool Window::pumpMessages() {
    MSG msg{};
    // PeekMessage 而不是 GetMessage：GetMessage 会**阻塞**直到有消息，
    // 那样没消息的帧就不会渲染了。渲染循环必须用 PeekMessage（非阻塞轮询）。
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) return false;
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return true;
}

double Window::frameSeconds() {
    // QueryPerformanceCounter 是 Windows 上的单调高精度时钟。
    // 对照 Heliostat Studio：它用 std::chrono::steady_clock（底层也是 QPC），
    // 并在 docs/perf_log.md 里明确区分 "wall ms"（含 present 与系统合成器）
    // 和 "GPU ms"（vkCmdWriteTimestamp 的差值）——这两个口径千万别混。
    static LARGE_INTEGER freq{};
    if (freq.QuadPart == 0) QueryPerformanceFrequency(&freq);
    LARGE_INTEGER now{};
    QueryPerformanceCounter(&now);
    const double t = static_cast<double>(now.QuadPart) / static_cast<double>(freq.QuadPart);
    const double dt = (m_lastTime == 0.0) ? 0.0 : (t - m_lastTime);
    m_lastTime = t;
    return dt;
}

} // namespace plat

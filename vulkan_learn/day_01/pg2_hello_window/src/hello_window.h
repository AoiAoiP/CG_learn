// ===========================================================================
// vulkan_learn/src/hello_window.h
//
// 第 1 课：一个最小可用的 Vulkan 窗口程序（只做 clear + present，不画几何）
//
// 四个文件的分工（刻意按"概念边界"切，不写成一个大 main.cpp）：
//
//   hello_window.h        ← 你正在看：数据结构与接口声明
//   platform_win32.cpp    ← 平台层：只负责"一个窗口"和"窗口消息"
//   vk_renderer.cpp       ← Vulkan 层：instance / surface / device / swapchain / 清屏
//   hello_window.cpp      ← 主循环：把上面两个拼起来，唯一有 while 的地方
//
// 这个分层和 Heliostat Studio 的做法一致（viz/src/platform_win32.cpp 与
// viz/src/vk_context.cpp 是分开的），所以从第 1 课起，你学的结构就是项目里的结构。
//
// 编译：见 build.ps1 / build.bat（需要 VS 2022 + Vulkan SDK）
// ===========================================================================
#pragma once

// ★★★ 第一个必须记住的编译期陷阱 ★★★
// VK_USE_PLATFORM_WIN32_KHR 必须在 **#include <vulkan/vulkan.h> 之前** 定义。
// 它控制 vulkan.h 里是否声明 VkWin32SurfaceCreateInfoKHR / vkCreateWin32SurfaceKHR
// 这些平台相关符号。忘了定义，你会得到"标识符未声明"的报错，
// 而且报错位置在 .cpp 里、根因在头文件顺序上，非常难查。
// 【项目对照】src/vk.cpp:5-7 有完全一样的注释和写法（那个文件是唯一碰
//  Win32 surface 入口点的翻译单元，所以宏只在那一个文件里定义）。
#ifndef VK_USE_PLATFORM_WIN32_KHR
#define VK_USE_PLATFORM_WIN32_KHR 1
#endif

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// 平台层：窗口 + 消息
// ---------------------------------------------------------------------------
namespace plat {

// 一个朴素的 Win32 窗口。没有 GLFW 的抽象，你能看到每一个真实调用。
class Window {
public:
    // 返回 false 表示创建失败（已在标准错误打印原因）
    bool create(const wchar_t *title, uint32_t width, uint32_t height);
    void destroy();

    // 取出并清空 Win32 消息队列。
    // 这一句是整个程序里最重要的一行——不抽消息，Windows 会认为程序卡死
    // （转圈 / "无响应"），Vulkan 的 present 也可能被合成器阻塞。
    // 返回 false 表示收到 WM_CLOSE/WM_DESTROY，应该退出。
    bool pumpMessages();

    bool resized() const { return m_resized; }
    void clearResized() { m_resized = false; }
    bool minimized() const { return m_minimized; }

    uint32_t width() const { return m_width; }
    uint32_t height() const { return m_height; }
    HWND hwnd() const { return m_hwnd; }

    // 上一帧到现在经过的秒数（做动画用）
    double frameSeconds();

private:
    static LRESULT CALLBACK wndProcThunk(HWND hwnd, UINT msg, WPARAM w, LPARAM l);
    LRESULT wndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l);

    HWND m_hwnd = nullptr;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    bool m_resized = false;
    bool m_minimized = false;
    double m_lastTime = 0.0;
};

} // namespace plat

// ---------------------------------------------------------------------------
// 日志：同时写到 stdout 和文件
//
// 为什么不只用 printf？因为 GUI 子系统程序的控制台输出**可能根本没人接收**
// （双击启动没有父控制台、被自动化脚本调用时 stdout 也可能为空）。
// 写文件才是"可验证"的做法。
// 【项目对照】Heliostat Studio 的 `--log <path>` 也是这个理由，而且它用
// _fsopen(..., _SH_DENYWR) 打开，好让外部脚本能**边跑边 tail** 日志
// （见 docs/perf_log.md 的"修复轮 #5"）。
// ---------------------------------------------------------------------------
namespace logx {
void open(const char *path);
void close();
void line(const char *fmt, ...);
} // namespace logx

// 统一日志宏：一行日志同时进 stdout 与日志文件
#define LOG(...) ::logx::line(__VA_ARGS__)

// ---------------------------------------------------------------------------
// Vulkan 层
// ---------------------------------------------------------------------------
namespace vkl {

std::string vkResultName(VkResult r);

class Renderer {
public:
    bool init(plat::Window &window, bool enableValidation);
    void shutdown();

    // 每帧三步（对照 Heliostat Studio 的 beginFrame / 录命令 / endFrame）
    // 返回 false 表示本帧要跳过（窗口最小化、或正在重建 swapchain）
    bool beginFrame();
    void recordClear(float timeSeconds);
    void endFrame();

    bool needsRecreate() const { return m_needsRecreate; }
    void recreate();

    uint32_t imageCount() const { return static_cast<uint32_t>(m_images.size()); }
    const std::string &deviceName() const { return m_deviceName; }

private:
    bool createInstance(bool enableValidation);
    bool createSurface();
    bool pickPhysicalDevice();
    bool createDevice();
    bool createSwapchain(bool firstTime);
    void destroySwapchain();
    bool createSyncObjects();
    bool createCommandObjects();

    uint32_t findMemoryType(uint32_t typeBits, VkMemoryPropertyFlags props) const;

    plat::Window *m_window = nullptr;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physical = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_queue = VK_NULL_HANDLE;
    uint32_t m_queueFamily = 0;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_format = VK_FORMAT_UNDEFINED;
    VkColorSpaceKHR m_colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    VkExtent2D m_extent{};
    std::vector<VkImage> m_images;
    std::vector<VkImageView> m_views;

    VkCommandPool m_cmdPool = VK_NULL_HANDLE;
    VkCommandBuffer m_cmd = VK_NULL_HANDLE;
    VkFence m_fence = VK_NULL_HANDLE;                 // CPU 等 GPU
    VkSemaphore m_acquireSem = VK_NULL_HANDLE;        // 图像可用 → 渲染开始
    VkSemaphore m_renderSem = VK_NULL_HANDLE;         // 渲染完成 → 可以 present

    uint32_t m_imageIndex = 0;
    bool m_needsRecreate = false;
    bool m_debugUtilsAvailable = false;
    std::string m_deviceName;
    float m_timestampPeriodNs = 1.0f;

    // 下面这张表是本课最重要的伏笔：记录"每个 swapchain image 上一次是被谁写的"。
    // Heliostat Studio 的 viz/src/vk_context.cpp:224-234 用它解决了
    // "两张图像在飞、但只按帧槽计数会提前复用图像"的问题。
    // 本课只有 1 帧在飞，所以先留空；第 2 课把它用起来。
    std::vector<VkFence> m_imagesInFlight;
};

} // namespace vkl

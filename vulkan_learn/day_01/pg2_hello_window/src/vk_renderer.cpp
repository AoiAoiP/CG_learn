// ===========================================================================
// vulkan_learn/src/vk_renderer.cpp
//
// Vulkan 层：从"什么都没有"到"每帧清屏并上屏"。
//
// 这是本课的主体。请**按函数顺序读**，它就是 Vulkan 初始化的标准顺序：
//
//   init()
//     1. createInstance()        ← 跟 Vulkan 系统建会话（装扩展 / 开校验层）
//     2. createSurface()         ← 把 Win32 窗口变成"可绘制的画布"
//     3. pickPhysicalDevice()    ← 枚举显卡，挑一张（只读地问）
//     4. createDevice()          ← 跟它签合同（选队列族）
//     5. createSwapchain()       ← 申请一叠"待显示图像"
//     6. createCommandObjects()  ← 命令池 + 命令缓冲
//     7. createSyncObjects()     ← fence + 2 个 semaphore
//   beginFrame() / recordClear() / endFrame()   ← 帧循环三步
//
// 几乎每一段都能对应到 Heliostat Studio 的 src/vk.cpp 和 viz/src/vk_context.cpp。
// 对照点在注释里标了【项目对照】。
// ===========================================================================
#include "hello_window.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <vector>

namespace vkl {

namespace {

void check(VkResult r, const char *what) {
    if (r != VK_SUCCESS) {
        throw std::runtime_error(std::string("Vulkan 调用失败: ") + what +
                                 " -> VkResult " + std::to_string(static_cast<int>(r)) + " (" +
                                 vkResultName(r) + ")");
    }
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                            VkDebugUtilsMessageTypeFlagsEXT /*types*/,
                                            const VkDebugUtilsMessengerCallbackDataEXT *data,
                                            void * /*user*/) {
    const char *tag = (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)     ? "ERROR"
                      : (severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) ? "warn"
                                                                                     : "info";
    LOG("[validation:%s] %s\n", tag, data->pMessage);
    return VK_FALSE;
}

PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessenger = nullptr;
PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessenger = nullptr;

// 两段式枚举：先问数量，再取内容。
// 【项目对照】src/vk.cpp:38-56 的 instanceExtensionAvailable() 是同一个套路。
bool instanceExtensionAvailable(const char *name) {
    uint32_t count = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    std::vector<VkExtensionProperties> exts(count);
    vkEnumerateInstanceExtensionProperties(nullptr, &count, exts.data());
    for (const auto &e : exts)
        if (std::strcmp(e.extensionName, name) == 0) return true;
    return false;
}

bool layerAvailable(const char *name) {
    uint32_t count = 0;
    vkEnumerateInstanceLayerProperties(&count, nullptr);
    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    for (const auto &l : layers)
        if (std::strcmp(l.layerName, name) == 0) return true;
    return false;
}

} // namespace

std::string vkResultName(VkResult r) {
    switch (r) {
    case VK_SUCCESS: return "VK_SUCCESS";
    case VK_NOT_READY: return "VK_NOT_READY";
    case VK_TIMEOUT: return "VK_TIMEOUT";
    case VK_INCOMPLETE: return "VK_INCOMPLETE";
    case VK_SUBOPTIMAL_KHR: return "VK_SUBOPTIMAL_KHR";
    case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
    case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
    case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
    case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
    case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
    case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
    case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
    case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
    case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
    case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
    case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
    case VK_ERROR_SURFACE_LOST_KHR: return "VK_ERROR_SURFACE_LOST_KHR";
    case VK_ERROR_OUT_OF_DATE_KHR: return "VK_ERROR_OUT_OF_DATE_KHR";
    case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
    default: return "VkResult(" + std::to_string(static_cast<int>(r)) + ")";
    }
}

// ===========================================================================
// 1. VkInstance —— 跟 Vulkan 系统建立会话
// 【项目对照】src/vk.cpp:94-160 createInstance()
// ===========================================================================
bool Renderer::createInstance(bool enableValidation) {
    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "vulkan_learn / hello_window";
    app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app.pEngineName = "none";
    app.apiVersion = VK_API_VERSION_1_1;   // 本课只用 1.1 的东西，故意要低版本

    std::vector<const char *> layers;
    if (enableValidation) {
        if (layerAvailable("VK_LAYER_KHRONOS_validation")) {
            layers.push_back("VK_LAYER_KHRONOS_validation");
            std::puts("[vk] 校验层已启用");
        } else {
            // 【项目对照】src/vk.cpp:109 —— 项目里也是"探测不到就打印警告继续跑"
            std::puts("[vk] 请求了校验层但系统未安装，继续运行（无校验）");
        }
    }

    std::vector<const char *> exts;
    // 这两个是"画到窗口上"的最低要求：
    //   VK_KHR_surface        —— 提供 VkSurfaceKHR 这个抽象
    //   VK_KHR_win32_surface  —— 提供从 HWND 造 surface 的具体实现
    for (const char *e : {"VK_KHR_surface", "VK_KHR_win32_surface"}) {
        if (!instanceExtensionAvailable(e))
            throw std::runtime_error(std::string("缺少 instance 扩展: ") + e);
        exts.push_back(e);
    }
    if (instanceExtensionAvailable(VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
        exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        m_debugUtilsAvailable = true;
    }

    VkInstanceCreateInfo info{};
    info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    info.pApplicationInfo = &app;
    info.enabledLayerCount = static_cast<uint32_t>(layers.size());
    info.ppEnabledLayerNames = layers.empty() ? nullptr : layers.data();
    info.enabledExtensionCount = static_cast<uint32_t>(exts.size());
    info.ppEnabledExtensionNames = exts.data();
    check(vkCreateInstance(&info, nullptr, &m_instance), "vkCreateInstance");

    // 扩展函数不在 loader 的静态导出表里，必须自己取指针。
    // 【项目对照】src/vk.cpp:87-89
    if (m_debugUtilsAvailable) {
        pfnCreateDebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT"));
        pfnDestroyDebugUtilsMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (pfnCreateDebugUtilsMessenger) {
            VkDebugUtilsMessengerCreateInfoEXT ci{};
            ci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            ci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            ci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            ci.pfnUserCallback = debugCallback;
            check(pfnCreateDebugUtilsMessenger(m_instance, &ci, nullptr, &m_debugMessenger),
                  "vkCreateDebugUtilsMessengerEXT");
        }
    }
    return true;
}

// ===========================================================================
// 2. VkSurfaceKHR —— 窗口 → 可绘制画布
//
// ★ 这是 Vulkan 和 OpenGL 分道扬镳最直观的地方。
//   OpenGL：窗口和 context 是**同一件事**（glfwCreateWindow 一起给了你），
//           而且 context 是**当前线程**的全局状态（wglMakeCurrent）。
//   Vulkan：窗口是 OS 的，GPU 是驱动的，中间必须显式造一座桥 = VkSurfaceKHR。
//           好处立刻可见：同一个 VkInstance 可以对接 Win32 / Xlib / Wayland /
//           Android / 甚至**没有任何窗口**（离屏渲染，你项目的 heliostat_core
//           就是这么跑的，它连 surface 都不创建，见 src/vk.cpp:115）。
// ===========================================================================
bool Renderer::createSurface() {
    VkWin32SurfaceCreateInfoKHR ci{};
    ci.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    ci.hinstance = GetModuleHandleW(nullptr);
    ci.hwnd = m_window->hwnd();
    check(vkCreateWin32SurfaceKHR(m_instance, &ci, nullptr, &m_surface), "vkCreateWin32SurfaceKHR");
    return true;
}

// ===========================================================================
// 3. VkPhysicalDevice —— 枚举显卡，挑一张
//
// ★ 关键认知：PhysicalDevice 是**只读**的。你只能 vkGetPhysicalDevice* 问它能力，
//   不能拿它干活。这一步和你 CUDA 里 cudaGetDeviceProperties 问设备属性是一回事，
//   区别只是 Vulkan 把它做成了一个"可枚举、可选择"的显式步骤。
//
//   为什么非要显式挑？因为一台机器可能有多张卡（核显 + 独显），
//   而且**挑错了就画不出来**——比如笔记本上你可能选到一张不支持 present 的卡。
//   这个选择在 OpenGL 里根本不存在：你只能接受驱动给你的那一张。
//
// 【项目对照】src/vk.cpp 的 pickPhysicalDevice() + deviceUsableForPresent()：
//   那个项目的判断条件是"该 GPU 的某个队列族既支持 compute 又支持 present"，
//   因为 viewer 要在同一个队列上既跑光追又上屏。
// ===========================================================================
bool Renderer::pickPhysicalDevice() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
    if (count == 0) throw std::runtime_error("没有找到任何支持 Vulkan 的 GPU");
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

    LOG("[vk] 发现 %u 张 GPU:\n", count);
    for (uint32_t i = 0; i < count; i++) {
        VkPhysicalDeviceProperties props{};
        vkGetPhysicalDeviceProperties(devices[i], &props);
        LOG("       [%u] %s  (type=%d, api %u.%u.%u)\n", i, props.deviceName,
                    static_cast<int>(props.deviceType), VK_VERSION_MAJOR(props.apiVersion),
                    VK_VERSION_MINOR(props.apiVersion), VK_VERSION_PATCH(props.apiVersion));
    }

    // 挑法：找第一张"有队列族同时支持 graphics + present"的卡。
    // 更讲究的做法是给独显（DISCRETE_GPU）加权，本课从简。
    for (VkPhysicalDevice dev : devices) {
        uint32_t qcount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qcount, nullptr);
        std::vector<VkQueueFamilyProperties> qs(qcount);
        vkGetPhysicalDeviceQueueFamilyProperties(dev, &qcount, qs.data());

        for (uint32_t i = 0; i < qcount; i++) {
            if (!(qs[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)) continue;
            // present 支持是**每队列族、每 surface**单独查询的，
            // 不是设备属性——同一个队列族在不同 surface 上答案可能不同。
            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, m_surface, &presentSupport);
            if (presentSupport) {
                m_physical = dev;
                m_queueFamily = i;
                VkPhysicalDeviceProperties props{};
                vkGetPhysicalDeviceProperties(dev, &props);
                m_deviceName = props.deviceName;
                m_timestampPeriodNs = props.limits.timestampPeriod;
                LOG("[vk] 选中 queue family %u（graphics + present 同族）\n", i);
                return true;
            }
        }
    }
    throw std::runtime_error("没有找到同时支持 graphics 和 present 的队列族");
}

// ===========================================================================
// 4. VkDevice —— 跟选中的卡签合同
//
// ★ PhysicalDevice 是"那台机器"，VkDevice 是"你租下来的使用权"。
//   合同里写三件事：
//     a) 我要用哪些队列族，各要几个队列      → VkDeviceQueueCreateInfo
//     b) 我要开哪些 device 扩展              → 例如 VK_KHR_swapchain
//     c) 我要开哪些**可选特性**（features）   → 例如 geometryShader / wideLines
//
//   b) 尤其容易踩坑：**VK_KHR_swapchain 是 device 级扩展，不是 instance 级**。
//   surface 是 instance 级的，swapchain 是 device 级的——因为 surface 描述
//   "窗口"，而 swapchain 描述"GPU 怎么把图交给这个窗口"，后者是设备的事。
//
//   另一个必答点（华为爱问）：**为什么本项目 graphics/compute/present 共用
//   同一个队列族？** 答案是：共用则不需要跨队列同步（省掉 ownership transfer
//   那些 barrier），代价是没有并行——所有工作串在一条队列上。
//   对 Heliostat Studio 这种"每帧一次 submit"的程序，共用是对的；
//   真正的引擎会把 async compute 放到独立队列族上并行。
// ===========================================================================
bool Renderer::createDevice() {
    const float priority = 1.0f;
    VkDeviceQueueCreateInfo qci{};
    qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    qci.queueFamilyIndex = m_queueFamily;
    qci.queueCount = 1;
    qci.pQueuePriorities = &priority;

    VkPhysicalDeviceFeatures features{};   // 本课不需要任何可选特性（连 shader 都还没有）

    const char *deviceExts[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    ci.queueCreateInfoCount = 1;
    ci.pQueueCreateInfos = &qci;
    ci.enabledExtensionCount = 1;
    ci.ppEnabledExtensionNames = deviceExts;
    ci.pEnabledFeatures = &features;
    check(vkCreateDevice(m_physical, &ci, nullptr, &m_device), "vkCreateDevice");

    // 队列是 device 创建时就存在的，你只是把它取出来。
    // 【项目对照】src/vk.h:63 VkCore::queue()，整个项目就用这**一个**队列。
    vkGetDeviceQueue(m_device, m_queueFamily, 0, &m_queue);
    LOG("[vk] device ready: %s\n", m_deviceName.c_str());
    return true;
}

// ===========================================================================
// 5. Swapchain —— 一叠"待显示图像"
//
// ★ 心智模型（这个比喻一定要记住，面试直接能用）：
//   把 swapchain 想成**一叠便利贴**。你要画画时：
//     ① vkAcquireNextImageKHR → "给我一张空的"（拿到它的下标）
//     ② 往上面画
//     ③ vkQueuePresentKHR     → "画好了，交给显示器"
//   显示器看完把便利贴还回叠里，下次 acquire 可能又给你同一张。
//
//   三个必须会答的点：
//   a) 你**不拥有**这些 image，不能销毁它们；它们随 swapchain 一起生死。
//      （所以它们没有 VkDeviceMemory —— 驱动/合成器管的内存）
//   b) 数量不由你定：capabilities.minImageCount..maxImageCount。
//      你只能在这个区间里挑（本项目 README 说"逐 image fence"的 image 数就是它）。
//   c) 格式和呈现模式也是**能力协商**出来的，不是你想要什么就是什么。
//
// ★ 与 OpenGL 的对比（面试高频）：
//   OpenGL 的双缓冲是**隐式**的——你只管画，SwapBuffers 时驱动自己翻转，
//   你甚至不知道有几张后备缓冲，也不知道 vsync 是怎么实现的。
//   Vulkan 把这一切掀开：几张图、什么格式、什么 present 模式、谁在等谁，
//   全部显式。**代价是代码量，收益是低延迟与可预测性。**
// ===========================================================================
bool Renderer::createSwapchain(bool firstTime) {
    const VkPhysicalDevice pd = m_physical;
    const VkSurfaceKHR surf = m_surface;

    VkSurfaceCapabilitiesKHR caps{};
    check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pd, surf, &caps),
          "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");

    if (caps.currentExtent.width == 0 || caps.currentExtent.height == 0) {
        // 窗口最小化：surface 尺寸为 0，此时**不能**建 swapchain。
        // 【项目对照】viz/src/vk_context.cpp:296 —— 项目遇到 0×0 就"延迟重建"，
        // 而不是崩溃。这是老手和新手很明显的分界。
        std::puts("[vk] 窗口尺寸为 0（最小化？），跳过重建");
        m_needsRecreate = true;
        return false;
    }

    // ---- 格式：能力协商 ----
    uint32_t fmtCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surf, &fmtCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(fmtCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(pd, surf, &fmtCount, formats.data());

    VkSurfaceFormatKHR chosen = formats[0];
    bool found = false;
    for (const auto &f : formats) {
        // 优先选 *_SRGB + SRGB_NONLINEAR。
        // 为什么重要：SRGB 格式意味着**硬件在写入时自动做 linear→sRGB 编码**。
        // 你在 shader 里算的是线性光，显示器要的是 sRGB。选错格式就会出现
        // "画面发灰/发暗"——这是图形程序最经典的坑之一。
        // 【项目对照】Heliostat Studio 的 composite pass 是
        // "ACES tonemap → sRGB 交换链"（README），选的就是这个组合。
        if (f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR &&
            (f.format == VK_FORMAT_B8G8R8A8_SRGB || f.format == VK_FORMAT_R8G8B8A8_SRGB)) {
            chosen = f;
            found = true;
            break;
        }
    }
    if (!found)
        LOG("[vk] 没有 SRGB 交换链格式，退化为 format=%d colorSpace=%d（画面可能发灰）\n",
                    static_cast<int>(chosen.format), static_cast<int>(chosen.colorSpace));

    // ---- 尺寸：currentExtent 为 0xFFFFFFFF 时才是"你自己定" ----
    VkExtent2D extent = caps.currentExtent;
    if (extent.width == UINT32_MAX) {
        extent.width = std::clamp(m_window->width(), caps.minImageExtent.width, caps.maxImageExtent.width);
        extent.height = std::clamp(m_window->height(), caps.minImageExtent.height, caps.maxImageExtent.height);
    }

    // ---- image 数量：最少比 min 多一张，形成双缓冲 ----
    // ★ 为什么"+1"？因为如果只申请 minImageCount 张，acquire 时很可能
    //   无图可用、必须等显示器归还——那就是一次隐式 stall。
    //   多要一张，让你"画下一张"的同时"显示器还在看上一张"。
    //   这是"帧在飞（frames in flight）"概念的第一层直觉。
    uint32_t imageCount = caps.minImageCount + 1;
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) imageCount = caps.maxImageCount;

    VkSwapchainCreateInfoKHR ci{};
    ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    ci.surface = surf;
    ci.minImageCount = imageCount;
    ci.imageFormat = chosen.format;
    ci.imageColorSpace = chosen.colorSpace;
    ci.imageExtent = extent;
    ci.imageArrayLayers = 1;
    // ★ imageUsage 说明"我要拿这些 image 干什么"。
    //   只要 COLOR_ATTACHMENT（当渲染目标）。如果你还想在 shader 里采样它们，
    //   必须加 SAMPLED_BIT —— 少了它就是经典的验证层报错。
    ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // 单队列族时用 EXCLUSIVE：image 只属于一个队列族，不需要 ownership 转移。
    // 若 graphics 和 present 不同族，这里要写 CONCURRENT 并给出两个族下标。
    ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ci.preTransform = caps.currentTransform;
    // compositeAlpha：与 Windows 桌面合成器怎么混合。
    // OPAQUE 表示"我不透明"——绝大多数窗口程序就该用它。
    ci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    ci.presentMode = VK_PRESENT_MODE_FIFO_KHR;   // FIFO 是规范唯一**保证**支持的模式
    ci.clipped = VK_TRUE;
    ci.oldSwapchain = firstTime ? VK_NULL_HANDLE : m_swapchain;  // 重建时传旧的，驱动可复用资源

    VkSwapchainKHR newChain = VK_NULL_HANDLE;
    check(vkCreateSwapchainKHR(m_device, &ci, nullptr, &newChain), "vkCreateSwapchainKHR");

    if (!firstTime) destroySwapchain();
    m_swapchain = newChain;
    m_format = chosen.format;
    m_colorSpace = chosen.colorSpace;
    m_extent = extent;

    uint32_t actual = 0;
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &actual, nullptr);
    m_images.resize(actual);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &actual, m_images.data());

    m_views.assign(actual, VK_NULL_HANDLE);
    for (uint32_t i = 0; i < actual; i++) {
        VkImageViewCreateInfo vi{};
        vi.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        vi.image = m_images[i];
        vi.viewType = VK_IMAGE_VIEW_TYPE_2D;
        vi.format = m_format;
        vi.components = {VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
                         VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY};
        vi.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
        check(vkCreateImageView(m_device, &vi, nullptr, &m_views[i]), "vkCreateImageView(swapchain)");
    }

    // 这个表是第 2 课的主角：哪个 image 还在被 GPU 用。
    m_imagesInFlight.assign(actual, VK_NULL_HANDLE);

    LOG("[vk] swapchain: %u 张 image, %ux%u, format=%d, presentMode=FIFO\n"
                "     （minImageCount=%u, 我申请了 %u —— 多要一张是为了不 stall）\n",
                actual, extent.width, extent.height, static_cast<int>(m_format),
                caps.minImageCount, imageCount);
    return true;
}

void Renderer::destroySwapchain() {
    // 销毁顺序同样倒序：view 先死，image 归 swapchain 管（不能也不该销毁）。
    for (VkImageView v : m_views)
        if (v) vkDestroyImageView(m_device, v, nullptr);
    m_views.clear();
    m_images.clear();
    m_imagesInFlight.clear();
    if (m_swapchain) {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

// ===========================================================================
// 6. 命令池与命令缓冲
//
// ★ 命令缓冲 = "给 GPU 的任务清单"。这是 Vulkan 相对 OpenGL 最大的结构变化之一：
//   OpenGL：你调 glDrawArrays，**立刻**生效（驱动把它塞进它自己的命令队列）。
//           → 你无法预先把一批命令准备好，也无法多线程并行录制。
//   Vulkan：你先 begin 一个 command buffer，把要干的事**录**进去，
//           end，然后 submit。录制期不碰 GPU，提交后才执行。
//           → 可以预先录好一整套；可以在 8 个线程上并行录 8 个；
//             可以录一次提交多次（本项目没这么做，因为它每帧内容都变）。
//
//   ONE_TIME_SUBMIT 标志是给驱动的提示："这个缓冲我只提交一次"，
//   让它可以不做重放优化。
// ===========================================================================
bool Renderer::createCommandObjects() {
    VkCommandPoolCreateInfo cp{};
    cp.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cp.queueFamilyIndex = m_queueFamily;
    // RESET_COMMAND_BUFFER_BIT：允许单个 command buffer 被 reset 重录。
    // 【项目对照】viz/src/vk_context.cpp:167 —— 项目每帧 vkResetCommandBuffer
    // 重录同一个缓冲（零分配），靠的就是这个标志。
    cp.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    check(vkCreateCommandPool(m_device, &cp, nullptr, &m_cmdPool), "vkCreateCommandPool");

    VkCommandBufferAllocateInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    ai.commandPool = m_cmdPool;
    ai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;   // PRIMARY = 可直接提交；SECONDARY = 被别的缓冲调用
    ai.commandBufferCount = 1;
    check(vkAllocateCommandBuffers(m_device, &ai, &m_cmd), "vkAllocateCommandBuffers");
    return true;
}

// ===========================================================================
// 7. 同步对象 —— 本课的重点，也是最容易做错的地方
//
//  ★★ 核心认知：**GPU 是异步的流水线，CPU 提交完就走了。**
//
//    CPU 时间线:  [录命令][submit]→ 立刻去录下一帧 ……（不等 GPU）
//    GPU 时间线:            [画第1帧]....[画第2帧]....
//
//    于是有两个必须回答的问题，而 Vulkan 强迫你自己回答：
//      问题 A：CPU 想重用一块资源（命令缓冲、UBO）时，怎么知道 GPU 已经用完了？
//              → **fence**（CPU 侧等待，vkWaitForFences）
//      问题 B：GPU 内部的先后顺序怎么保证？比如"渲染完才能 present"？
//              → **semaphore**（GPU 侧等待，插在 submit 和 present 之间）
//
//    这就是 fence 和 semaphore 的分工，也是 Day 9 的主题。记住这句：
//      **fence 是 CPU 等 GPU；semaphore 是 GPU 等 GPU。**
//
//    为什么 present 不能等 fence？（面试高频）
//      fence 只能被 **CPU** 等（vkWaitForFences）。present 是队列操作，
//      要等的是"GPU 渲染完成"这个**设备侧**事件，必须用 semaphore。
//      如果你在 present 前 vkWaitForFences，就等于每帧强制 CPU/GPU 串行，
//      流水线彻底没了 —— 那正是 OpenGL 有时被人抱怨的隐式同步行为。
// ===========================================================================
bool Renderer::createSyncObjects() {
    // fence 必须初始化为 **signaled**！
    // 因为 beginFrame() 第一步就是 vkWaitForFences。如果初始是 unsignaled，
    // 第一次等待会永久阻塞 —— 这是新手 100% 会踩的坑。
    VkFenceCreateInfo fi{};
    fi.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fi.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    check(vkCreateFence(m_device, &fi, nullptr, &m_fence), "vkCreateFence");

    VkSemaphoreCreateInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    // 为什么要**两个** semaphore？
    //   m_acquireSem：vkAcquireNextImageKHR 完成 → 图像可写，渲染可以开始
    //   m_renderSem ：渲染完成 → 图像可显示，present 可以开始
    // 用同一个会怎样？会破坏"信号→等待"的严格配对语义：
    // 一个 binary semaphore 在同一时刻只能有一次未消费的信号，
    // 一个 semaphore 同时被 submit 等待、又被 present 等待，行为未定义。
    check(vkCreateSemaphore(m_device, &si, nullptr, &m_acquireSem), "vkCreateSemaphore(acquire)");
    check(vkCreateSemaphore(m_device, &si, nullptr, &m_renderSem), "vkCreateSemaphore(render)");
    return true;
}

// ===========================================================================
// init / shutdown
// ===========================================================================
bool Renderer::init(plat::Window &window, bool enableValidation) {
    m_window = &window;
    createInstance(enableValidation);
    createSurface();
    pickPhysicalDevice();
    createDevice();
    createCommandObjects();
    createSyncObjects();
    createSwapchain(/*firstTime=*/true);
    LOG("[vk] init 完成。timestampPeriod = %.2f ns\n", m_timestampPeriodNs);
    return true;
}

void Renderer::shutdown() {
    if (!m_device) return;
    // 销毁前必须让 GPU 停下来，否则可能正在用你即将释放的东西。
    // 【项目对照】本项目全仓只有两处 device idle：析构（src/vk.cpp:80）
    // 和 swapchain 重建 —— 帧循环里一次都没有（README 明确写了这条纪律）。
    vkDeviceWaitIdle(m_device);

    if (m_renderSem) vkDestroySemaphore(m_device, m_renderSem, nullptr);
    if (m_acquireSem) vkDestroySemaphore(m_device, m_acquireSem, nullptr);
    if (m_fence) vkDestroyFence(m_device, m_fence, nullptr);
    destroySwapchain();
    if (m_cmdPool) vkDestroyCommandPool(m_device, m_cmdPool, nullptr);
    vkDestroyDevice(m_device, nullptr);
    m_device = VK_NULL_HANDLE;

    // ★ 注意：surface 归 **instance** 管，不是 device！传错会直接崩或被校验层抓。
    // 【项目对照】src/vk.cpp:85
    if (m_surface) vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
    if (m_debugMessenger && pfnDestroyDebugUtilsMessenger)
        pfnDestroyDebugUtilsMessenger(m_instance, m_debugMessenger, nullptr);
    if (m_instance) vkDestroyInstance(m_instance, nullptr);
}

// ===========================================================================
// 帧循环三步
// ===========================================================================
bool Renderer::beginFrame() {
    if (m_needsRecreate) return false;
    if (m_window->minimized()) return false;

    // ① 等上一帧的 fence：CPU 在这里等 GPU。
    //    只有 1 帧在飞时，这里**一定**会等 —— 意味着 CPU 和 GPU 是串行的，
    //    帧率 = 1 / (CPU时间 + GPU时间)。这就是"1 帧在飞"的性能代价。
    //    第 2 课改成 2 帧在飞，你会看到它变成 max(CPU, GPU)。
    check(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, UINT64_MAX), "vkWaitForFences");
    check(vkResetFences(m_device, 1, &m_fence), "vkResetFences");

    // ② 取一张可写的 image
    VkResult r = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_acquireSem,
                                       VK_NULL_HANDLE, &m_imageIndex);
    if (r == VK_ERROR_OUT_OF_DATE_KHR) {
        m_needsRecreate = true;   // 窗口尺寸变了
        return false;
    }
    if (r == VK_SUBOPTIMAL_KHR) {
        // SUBOPTIMAL vs OUT_OF_DATE 的区别（面试能问）：
        //   OUT_OF_DATE = 这个 swapchain 已经**不能**再呈现了，必须重建。
        //   SUBOPTIMAL  = 还能用，但已经**不匹配**窗口（比如尺寸对不上、有黑边）。
        //                这一帧照常画完，下一帧再重建 —— 这样不会闪一下。
        // 【项目对照】viz/src/vk_context.cpp:220 就是"画完这帧，下帧重建"。
        m_needsRecreate = true;
    }
    if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) check(r, "vkAcquireNextImageKHR");

    // ③ 开始录制
    check(vkResetCommandBuffer(m_cmd, 0), "vkResetCommandBuffer");
    VkCommandBufferBeginInfo bi{};
    bi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    bi.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(m_cmd, &bi), "vkBeginCommandBuffer");
    return true;
}

void Renderer::recordClear(float t) {
    VkImage img = m_images[m_imageIndex];

    // ★★★ 本课最重要的一段：image layout 转换 ★★★
    //
    // 【为什么需要这一步？—— 面试几乎必问】
    //   GPU 上的 image 不是一块裸内存，它有一个**layout（布局）**状态，
    //   描述"这块内存在物理上怎么排布、供哪个硬件单元用"：
    //     - VK_IMAGE_LAYOUT_UNDEFINED          : 内容无意义 / 由驱动决定
    //     - VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : 渲染目标的最优排布
    //     - VK_IMAGE_LAYOUT_PRESENT_SRC_KHR    : 交给显示引擎的最优排布
    //     - VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : 纹理采样的最优排布
    //   排布不同，硬件访问效率差很多（tiling / compression），
    //   所以**切换 layout 是有真实代价的**，必须显式声明。
    //
    // 【OpenGL 怎么做的？】
    //   驱动在背后猜。你 glBindTexture 到 FBO 再采样，驱动发现冲突就插一次
    //   flush + 隐式 layout 转换 —— 你看不见，也没法优化。这就是"隐式"。
    //
    // 【vkCmdPipelineBarrier 的三个关键参数怎么理解】
    //   vkCmdPipelineBarrier(cmd, srcStageMask, dstStageMask, ...)
    //     srcStage = "**谁**写完了"       → 这里是"谁都没有"（UNDEFINED，内容要丢弃）
    //     dstStage = "**谁**要开始读/写"   → COLOR_ATTACHMENT_OUTPUT（光栅化输出阶段）
    //     barrier 里的 srcAccess/dstAccess = "读写的**可见性**"（内存层面）
    //   一句话：**srcStage/dstStage 管执行顺序，accessMask 管内存可见性。**
    //   两者缺一不可 —— 这是 Vulkan 同步里最核心的一句话，Day 5 会展开。
    //
    // 【为什么 oldLayout 写 UNDEFINED？】
    //   因为我们要 clear 整张图，旧内容完全不要 —— 声明 UNDEFINED 等于告诉驱动
    //   "不用帮我保留内容"，驱动可以跳过解压/回写，是最快的路径。
    //   如果写 COLOR_ATTACHMENT_OPTIMAL（假装内容有效），反而会多干活。

    VkImageMemoryBarrier toColor{};
    toColor.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    toColor.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    toColor.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toColor.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toColor.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    toColor.image = img;
    toColor.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1};
    toColor.srcAccessMask = 0;                                   // 没有前序写者
    toColor.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // 接下来要写颜色
    vkCmdPipelineBarrier(m_cmd,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,          // 从"管线最开头"起
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // 到"颜色输出"止
                         0, 0, nullptr, 0, nullptr, 1, &toColor);

    // ---- 动态渲染：不创建 RenderPass，直接开始渲染 ----
    // 传统路径要 vkCreateRenderPass + vkCreateFramebuffer + vkCmdBeginRenderPass。
    // Vulkan 1.3 起把 dynamic rendering 提升为 core，可以像下面这样直接写附件。
    // 【项目对照】Heliostat Studio 全程走这条路径（README: "动态渲染（无 render pass）"）。
    //   动机很实际：它每个 pass 的输出格式/附件组合都在变（HDR / bloom 各级 /
    //   swapchain），用 RenderPass 要为每种组合建一个对象 + 一个 framebuffer；
    //   动态渲染把这些对象全省了。
    VkRenderingAttachmentInfo color{};
    color.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    color.imageView = m_views[m_imageIndex];
    color.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    color.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;   // 这帧开始时清空附件
    color.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // 保留结果（要显示，当然要保留）
    // 随时间变化的颜色 —— 你**看得见**帧循环在跑。如果它不动，说明卡住了。
    color.clearValue.color.float32[0] = 0.5f + 0.5f * std::sin(t * 1.1f);
    color.clearValue.color.float32[1] = 0.5f + 0.5f * std::sin(t * 1.7f + 2.0f);
    color.clearValue.color.float32[2] = 0.5f + 0.5f * std::sin(t * 2.3f + 4.0f);
    color.clearValue.color.float32[3] = 1.0f;

    VkRenderingInfo ri{};
    ri.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    ri.renderArea = {{0, 0}, m_extent};
    ri.layerCount = 1;
    ri.colorAttachmentCount = 1;
    ri.pColorAttachments = &color;

    vkCmdBeginRendering(m_cmd, &ri);
    // 这里就是将来放 vkCmdBindPipeline / vkCmdDraw 的地方（第 3 课）。
    vkCmdEndRendering(m_cmd);

    // ---- layout 转回 PRESENT ----
    // 注意 dstAccessMask = 0：present 引擎不是一个"渲染阶段"，
    // 它不需要通过 accessMask 声明内存可见性。这是规范规定的写法。
    VkImageMemoryBarrier toPresent = toColor;
    toPresent.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    toPresent.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    toPresent.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    toPresent.dstAccessMask = 0;
    vkCmdPipelineBarrier(m_cmd,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,  // "交给管线末端之后的事情"
                         0, 0, nullptr, 0, nullptr, 1, &toPresent);
}

void Renderer::endFrame() {
    check(vkEndCommandBuffer(m_cmd), "vkEndCommandBuffer");

    // ★ waitDstStageMask 是面试高频点，务必理解：
    //   semaphore 只保证"信号发出去了"，它**不告诉 GPU 在哪个阶段等**。
    //   你必须指定"这一帧的工作从哪个 stage 开始才需要等这张 image 可用"。
    //
    //   为什么是 COLOR_ATTACHMENT_OUTPUT 而不是 TOP_OF_PIPE？
    //   - 这个 semaphore 等的是"swapchain image 可以被写"。
    //     在此之前的工作（顶点取数据、跑 vertex shader）**根本不需要**这张图！
    //     用 TOP_OF_PIPE 等于让整条管线都停下来等，白丢性能。
    //   - 用 COLOR_ATTACHMENT_OUTPUT 则允许"前面的阶段先跑起来"，
    //     只在实际写颜色时才必须等 —— 这是正确且最优的。
    //   【项目对照】viz/src/vk_context.cpp:254 就是这么填的。
    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit{};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.waitSemaphoreCount = 1;
    submit.pWaitSemaphores = &m_acquireSem;
    submit.pWaitDstStageMask = &waitStage;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &m_cmd;
    submit.signalSemaphoreCount = 1;
    submit.pSignalSemaphores = &m_renderSem;
    check(vkQueueSubmit(m_queue, 1, &submit, m_fence), "vkQueueSubmit");

    VkPresentInfoKHR present{};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &m_renderSem;   // present 等 GPU 渲染完 —— 用 semaphore！
    present.swapchainCount = 1;
    present.pSwapchains = &m_swapchain;
    present.pImageIndices = &m_imageIndex;
    const VkResult r = vkQueuePresentKHR(m_queue, &present);
    if (r == VK_ERROR_OUT_OF_DATE_KHR || r == VK_SUBOPTIMAL_KHR) m_needsRecreate = true;
    else if (r != VK_SUCCESS) check(r, "vkQueuePresentKHR");
}

void Renderer::recreate() {
    // 重建 swapchain 是**唯一**允许在稳态之外等 GPU 的地方。
    // 为什么必须先 idle？因为旧 swapchain 的 image 可能还在被 GPU 用，
    // 而你马上要销毁它们的 view。不等就是 use-after-free + 校验层报错。
    m_needsRecreate = false;
    vkDeviceWaitIdle(m_device);
    createSwapchain(/*firstTime=*/false);
    std::puts("[vk] swapchain 已重建");
}

} // namespace vkl

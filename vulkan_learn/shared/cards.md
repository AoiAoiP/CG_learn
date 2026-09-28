# Vulkan 面试问答卡库

> 用法：每天新卡追加在本文件末尾。**正面**是面试官会问的原话，**背面**是四段式答案：
> ① 是什么 → ② 我项目哪用（带文件:行） → ③ 为什么这么选 → ④ 关键数字。
> 背诵要求：④ 里的数字必须能脱口而出，面试官最爱在这儿抓人。
>
> 锚定仓库：`D:\Code\heliostat_optimize\DiffHelio_viz`（Heliostat Studio）

---

## Day 1 — Vulkan 全局图景与 `VkInstance`

### 卡 D1-1
**正面**：Vulkan 和 OpenGL 最本质的区别是什么？为什么 Vulkan 要设计成这么啰嗦？

**背面**
- **是什么**：OpenGL 是「全局状态机 + 驱动在背后替你做决定」——你调 `glBindTexture`，驱动不知道你要干什么，只能保守地做同步、做校验、做内存管理。Vulkan 是「一次性显式描述 + 之后按描述执行」——每个对象怎么用、多大、什么格式、谁和谁能并发，全部在创建时由你声明，运行时驱动不再猜。
  三根支柱（记住这三条，任何对比题都能展开）：
  1. **显式生命周期**：没有隐式创建/销毁，所有对象 `vkCreate*` / `vkDestroy*` 由你管。
  2. **显式同步**：驱动不再替你插 barrier 和 stall，`vkCmdPipelineBarrier` / semaphore / fence 由你排。
  3. **显式内存**：`vkAllocateMemory` + 自己挑 memoryType，分配策略是你的责任。
- **我项目哪用**：`src/vk.cpp:70-76` —— `VkCore` 构造函数 5 行把整条初始化链写死：`createInstance → createSurface → pickPhysicalDevice → createDevice → createPools`。这就是"显式"的字面体现：**没有一行是驱动替你做的**。
- **为什么这么选**：啰嗦是**代价**，换来的是动机一致的三个收益：可预测的性能（驱动不再暗中 stall）、多线程友好（对象独立、无全局状态）、跨平台一致（行为由规范定义而非驱动实现）。
  一句能镇住场的话：**「OpenGL 把复杂度藏在驱动里，Vulkan 把它摊在应用里——因为只有应用知道自己想干什么。」**
- **关键数字**：`VkCore` 的初始化链 **5 步**；对比 OpenGL 初始化通常只有 `glfwCreateWindow` + `glewInit` **2 步**（差距全在显式性上）。

### 卡 D1-2
**正面**：`VkInstance` 到底是干什么的？为什么必须有它，不能直接创建 device？

**背面**
- **是什么**：`VkInstance` 是你进程和 **Vulkan 运行时（loader + 驱动）** 之间的会话句柄。它绑定两样东西：① 你要用的 **instance 级扩展**（如 `VK_KHR_surface`）；② 你要开的 **校验层**（如 `VK_LAYER_KHRONOS_validation`）。它还承载**应用元信息**（名字、`apiVersion`），供驱动做兼容判断和性能分析工具识别。类比：办公司营业执照——先登记（instance），才能跟各个部门（physical device / device）打交道。
- **我项目哪用**：`src/vk.cpp:94-126` `createInstance()`。三个关键动作：
  - `app.apiVersion = m_hwnd ? VK_API_VERSION_1_4 : VK_API_VERSION_1_3`（`vk.cpp:100`）
  - 按需追问扩展：`VK_KHR_surface` + `VK_KHR_win32_surface`（`vk.cpp:117-121`）
  - 校验层只在 `enableValidation` 时开，且**先探测是否存在**（`vk.cpp:103-111`）
- **为什么这么选**：
  - **版本分叉**：无头 CLI（`heliostat_core`）只要 1.3；viewer 要 1.4，因为 **动态渲染**（`vkCmdBeginRendering`）和 **timestamp query** 在 1.4 是核心功能，不再需要 `VK_KHR_dynamic_rendering` 扩展。**用更高的 core 版本换取不带扩展后缀的干净代码。**
  - **无头路径不请求任何 surface 扩展**（`vk.cpp:115` 的 `if (m_hwnd)`）——一个二进制两个运行形态，靠 instance 层分叉。
  - **校验层探测后再开**：`layerAvailable()`（`vk.cpp:28-36`）先 `vkEnumerateInstanceLayerProperties` 问一遍，没装就打印提示继续跑，而不是直接崩。这是"能交付的工程"和"能跑就行的 demo"的区别。
- **关键数字**：instance 级扩展 **3 个**（surface / win32_surface / debug_utils）；`VK_EXT_debug_utils` 由 `vkGetInstanceProcAddr` 取函数指针（`vk.cpp:87-89`），因为它是扩展、不在 loader 的静态导出表里。
- **易被追问的点**：`vk.cpp:122` 写的是 `if ((enableValidation || true) && ...)` ⚠️ —— `|| true` 让条件恒真，也就是**即使不开校验层也会挂 debug messenger**。面试官问"你这行为什么这么写"，诚实答法是"这是让 debug 回调无条件可用的写法，`|| true` 是冗余的、应当删掉；它不影响正确性，但会被代码审查挑出来"。

### 卡 D1-3
**正面**：你怎么知道某个扩展/层在用户机器上存在？不存在怎么办？

**背面**
- **是什么**：Vulkan 里**任何**扩展/能力都不能假定存在，必须主动枚举（`vkEnumerateInstanceExtensionProperties` / `vkEnumerateDeviceExtensionProperties` / `vkEnumerateInstanceLayerProperties`）或查询（`vkGetPhysicalDeviceFeatures` / `vkGetPhysicalDeviceProperties`）。这叫 **capability negotiation（能力协商）**。
- **我项目哪用**：三个探测函数是一套模板，`src/vk.cpp:28-56`：`layerAvailable()` / `instanceExtensionAvailable()` / `deviceExtensionAvailable()`。全是"先问 count 再问内容"的**两段式枚举**。
- **为什么这么选**：
  - **两段式枚举是 Vulkan 的通用惯例**（`vkEnumerate*`、`vkGet*Properties` 都这样）：第一次传 `nullptr` 拿数量，分配，第二次真拿数据。避免 Vulkan 内部替你分配内存——和"显式内存"一脉相承。
  - **失败策略分三级**：致命扩展缺失 → `throw`（`vk.cpp:119`）；可选扩展（debug utils）缺失 → 静默跳过（`vk.cpp:122`）；校验层缺失 → 打印警告继续（`vk.cpp:109`）。
- **关键数字**：`VK_KHR_surface` + `VK_KHR_win32_surface` 缺失时**直接抛异常**（hard requirement）；`VK_EXT_debug_utils` 缺失时降级运行。

### 卡 D1-4
**正面**：你项目里 `apiVersion` 请求 1.4，如果用户驱动只支持 1.2 会怎样？

**背面**
- **是什么**：`VkApplicationInfo::apiVersion` 是**你声明的最高可用版本**。若驱动不支持你要的版本，`vkCreateInstance` 返回 `VK_ERROR_INCOMPATIBLE_DRIVER`。**注意**：`apiVersion` 不是"我要用 1.4 的所有功能"，而是"我的代码按 1.4 的规范写，请给我 ≥1.4 的驱动"。
- **我项目哪用**：`src/vk.cpp:100`。headless 要 1.3，viewer 要 1.4。
- **为什么这么选**：见 D1-2（动态渲染 + timestamp 进 core）。README 明确要求 **Vulkan SDK 1.4+**，等价于"本项目不承诺兼容 1.3 以下驱动"——这是**有意识的取舍**，面试时可以答成："我把驱动门槛写进了 README，而不是在运行时到处 fallback；这是单人项目的合理选择，产品化的话我会降到 1.1/1.2 并把动态渲染作为可选扩展去协商。"
- **关键数字**：`VK_ERROR_INCOMPATIBLE_DRIVER`（=-9）是版本不满足的返回码。

---

## 断点清单（随学习更新）

| # | 断点 | 状态 | 证据 |
|---|---|---|---|
| 1 | 同步原语分工（fence = CPU↔GPU / semaphore = GPU↔GPU / present 必须等 semaphore） | ⏳ 待测（Day 5） | 摸底未答 |
| 2 | 帧槽数量推导（2 帧在飞为什么是 2） | ⏳ 待测（Day 5） | 摸底未答 |
| 3 | `waitDstStageMask` 语义（`COLOR_ATTACHMENT_OUTPUT` 的必然性） | ⏳ 待测（Day 5） | 摸底未答 |
| 4 | 逐 image fence 的存在理由（同 image 复用时序） | 🔴 已确认存在 | `vk_context.cpp:224-234` |
| 5 | descriptor set 的 OpenGL 对照 | ⏳ 待测（Day 7） | 摸底未答 |
| 6 | `computeBarrier` 是 coarse 的（自认） | 🔴 已确认存在 | `src/vk.cpp:548-555` |
| 7 | 无 subpass / 动态渲染路线，移动端问题无法直接回答 | 🔴 已确认存在 | 全仓无 `vkCmdBeginRenderPass` |
| 8 | **Vulkan 基础对象模型（instance/device/queue 从何而来）** | 🔴 已确认存在 | 自述"从 0 小白"（Day 1 正在补） |
| 9 | 无头 / 有窗口**双形态**架构（同一份 `VkCore` 两条路径） | 🔴 已确认存在 | `vk.cpp:100,115` |

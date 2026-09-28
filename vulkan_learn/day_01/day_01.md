# Day 1 — 环境搭建 + 两个入门窗口程序

## 目标
1. 把 Vulkan 开发环境配好（Vulkan SDK / GLFW / GLM / MSVC），并掌握**可复用**的编译与运行命令。
2. 跑通两个程序，形成对照：
   - **pg1**：GLFW 版 —— 用官方那段最简示例验证"环境是通的"。
   - **pg2**：**不用 GLFW** —— 原生 Win32 + Vulkan 自己建窗、清屏、上屏，看清 GLFW 到底替你隐藏了什么。

一句话动机：pg1 能跑只证明"环境没坏"；pg2 才能让人看懂 Vulkan 的初始化到底由哪些对象组成。

---

## 环境事实（本机实测）

| 组件 | 版本 | 位置 |
|---|---|---|
| Vulkan SDK | **1.4.341.1** | `C:\VulkanSDK\1.4.341.1`（`VULKAN_SDK` 已设） |
| Vulkan loader | 1.4.341.1 | `C:\Windows\System32\vulkan-1.dll`（运行时不需要 SDK） |
| 着色器工具 | — | `glslc` / `glslangValidator` / `slangc` / `spirv-dis`（均在 PATH） |
| MSVC | 19.44（VS 2022 Community） | `C:\Program Files\Microsoft Visual Studio\2022\Community` |
| CMake | 4.0.0 | `C:\Program Files\CMake\bin\cmake.exe` |
| **GLFW** | 3.5.1 | 源码编译为静态库 → `third_party/lib/glfw3.lib`（703 KB） |
| **GLM** | 1.0.3 | 头文件 → `third_party/include/glm/` |

**GPU**（实测枚举出两张）：NVIDIA GeForce RTX 4060 Laptop（Vulkan 1.4.329）· Intel Arc Graphics（Vulkan 1.4.325）

---

## 目录结构

```
vulkan_learn/
├─ shared/                       跨项目复用
│   ├─ build_common.bat           通用构建脚本（各程序的 build.bat 调它）
│   ├─ msvc_env.ps1               ★ PowerShell 手动编译前先 dot-source 它
│   ├─ README_environment.md      环境清单
│   └─ cards.md                   面试问答卡库
├─ day_01/                       ← 本日
│   ├─ day_01.md                  本文件
│   ├─ pg1_vulkan_window/         GLFW + GLM + Vulkan 起步程序
│   │   ├─ build.bat
│   │   ├─ src/vulkan_window.cpp
│   │   └─ build/vulkan_window.exe
│   └─ pg2_hello_window/          原生 Win32 + Vulkan 窗口
│       ├─ build.bat
│       ├─ src/{hello_window.h, hello_window.cpp, platform_win32.cpp, vk_renderer.cpp, logx.cpp}
│       └─ build/hello_window.exe
└─ third_party/                  ★ 只读共享依赖，任何项目都不修改
    ├─ include/{GLFW, glm}        头文件（统一 include 根目录）
    └─ lib/glfw3.lib
```

---

## pg1_vulkan_window

**程序**：`src/vulkan_window.cpp` —— GLFW 建窗（`GLFW_CLIENT_API = GLFW_NO_API`，即只开窗口、不建图形上下文）+ 枚举 Vulkan instance 扩展数 + GLM 矩阵/向量乘法。

### 方式 A：脚本编译（快）

```powershell
cd D:\Code\heliostat_optimize\DiffHelio_viz\vulkan_learn\day_01\pg1_vulkan_window
.\build.bat              # Release
.\build.bat debug        # Debug（/Od，栈回溯才有意义）
.\build.bat clean        # 清理 build\
```

### 方式 B：PowerShell 手动编译（推荐用来学）

**先加载 MSVC 环境**（每个 PowerShell 窗口执行一次；注意前面的点，dot-source 才能改当前会话）：

```powershell
. D:\Code\heliostat_optimize\DiffHelio_viz\vulkan_learn\shared\msvc_env.ps1
```

**然后手动编译**（下面是一条完整命令，反引号 `` ` `` 是 PowerShell 的续行符）：

```powershell
$VL = "D:\Code\heliostat_optimize\DiffHelio_viz\vulkan_learn"
cd "$VL\day_01\pg1_vulkan_window"
New-Item -ItemType Directory -Force -Path .\build | Out-Null

cl /nologo /std:c++17 /EHsc /W3 /utf-8 /MD /O2 /DNDEBUG /Zi `
   /I src /I "$VL\third_party\include" /I "$env:VULKAN_SDK\Include" `
   src\vulkan_window.cpp `
   /Fe:build\vulkan_window.exe `
   /link /LIBPATH:"$env:VULKAN_SDK\Lib" `
   "$VL\third_party\lib\glfw3.lib" vulkan-1.lib user32.lib gdi32.lib shell32.lib
```

> ⚠️ **相对路径只对 `third_party` / `shared` 用 `..\..`**：你已经在 `pg1_vulkan_window\` 里，
> 源码在**当前目录**的 `src\` 下 → 写 `src\...`；而 `third_party\` 在 `vulkan_learn\` 下 → 用 `$VL\...`。
> 把源码写成 `..\src\...` 会跑到 `day_01\src\` 去找，报 `c1xx: fatal error C1083: 无法打开源文件`。

**参数逐个解释**（这是手动编译的意义所在）：

| 参数 | 作用 |
|---|---|
| `/std:c++17` | 语言标准 |
| `/EHsc` | 启用 C++ 异常（GLFW 是 C 库，但我们的代码要 `try`） |
| `/W3` | 警告等级 |
| `/utf-8` | 源码按 UTF-8 解读（**否则中文注释会编译报错**） |
| **`/MD`** | **动态 CRT。必须与 GLFW 一致**，见坑 2 |
| `/O2 /DNDEBUG` | Release 优化；`/Zi` 生成 `.pdb`（崩溃时有行号） |
| `/I <dir>` | 头文件搜索路径（include 目录） |
| `/Fe:name.exe` | 输出可执行文件名 |
| `/link ...` | 之后的参数交给链接器 |
| `/LIBPATH:<dir>` | 库搜索路径 |
| `xxx.lib` | 要链接的库（顺序：静态库 → 系统库） |

### 运行

```powershell
cd D:\Code\heliostat_optimize\DiffHelio_viz\vulkan_learn\day_01\pg1_vulkan_window
.\build\vulkan_window.exe
```

> ⚠️ **在 PowerShell 里用 `.\xxx.exe` 启动 GUI 程序时，PowerShell 不会等待它退出**，
> 所以拿不到退出码、看不到实时输出。要等它结束请用：
> ```powershell
> Start-Process .\build\vulkan_window.exe -Wait -PassThru | Select-Object ExitCode
> ```

**预期**：弹出 800×600 窗口，控制台打印 `N extensions supported`，关闭窗口即退出。

### 实测结果
- 编译：**0 warning / 0 error**（脚本与手动两条路径都验证过）
- 运行：**窗口成功弹出、进程持续存活** → `glfwInit` / `glfwCreateWindow` / `vkEnumerateInstanceExtensionProperties` / GLM 全部走通

---

## pg2_hello_window

**程序**：原生 Win32 建窗 + 手写 Vulkan 初始化到每帧清屏并上屏。**不使用 GLFW。**

```
src/
├─ hello_window.h        数据结构与接口声明
├─ platform_win32.cpp    平台层：窗口 + 消息（**无一行 Vulkan 代码**）
├─ vk_renderer.cpp       Vulkan 层：instance/surface/device/swapchain/清屏
├─ logx.cpp              日志：stdout + 文件双写
└─ hello_window.cpp      主循环（全程序唯一的 while）
```

分层方式和 Heliostat Studio 一致（那边 `viz/src/platform_win32.cpp` 与 `viz/src/vk_context.cpp` 也是分开的）。

### 方式 A：脚本编译

```powershell
cd D:\Code\heliostat_optimize\DiffHelio_viz\vulkan_learn\day_01\pg2_hello_window
.\build.bat
```

### 方式 B：PowerShell 手动编译

```powershell
. D:\Code\heliostat_optimize\DiffHelio_viz\vulkan_learn\shared\msvc_env.ps1

$VL = "D:\Code\heliostat_optimize\DiffHelio_viz\vulkan_learn"
cd "$VL\day_01\pg2_hello_window"
New-Item -ItemType Directory -Force -Path .\build | Out-Null

cl /nologo /std:c++17 /EHsc /W3 /utf-8 /MD /O2 /DNDEBUG /Zi `
   /I src /I "$VL\third_party\include" /I "$env:VULKAN_SDK\Include" `
   src\hello_window.cpp src\platform_win32.cpp src\vk_renderer.cpp src\logx.cpp `
   /Fe:build\hello_window.exe `
   /link /LIBPATH:"$env:VULKAN_SDK\Lib" `
   "$VL\third_party\lib\glfw3.lib" vulkan-1.lib user32.lib gdi32.lib shell32.lib
```

> 多文件时把所有 `.cpp` 都列给 `cl` 即可（`cl` 会分别编译再统一链接）。
> 如果不想逐个列，可以用 PowerShell 通配展开：`(Get-ChildItem .\src\*.cpp).FullName`

### 运行

```powershell
cd D:\Code\heliostat_optimize\DiffHelio_viz\vulkan_learn\day_01\pg2_hello_window
.\build\hello_window.exe                          # 一直跑到关窗（ESC 也可）
.\build\hello_window.exe --frames 300             # 跑 300 帧自动退出（脚本化验证用）
.\build\hello_window.exe --no-validation          # 不开校验层（对比开销）
.\build\hello_window.exe --log .\run.log          # 指定日志文件
```

### 实测结果

编译 **0 warning / 0 error**。运行时初始化阶段全部成功，实测日志：

```
[vk] 发现 2 张 GPU:
       [0] NVIDIA GeForce RTX 4060 Laptop GPU  (type=2, api 1.4.329)
       [1] Intel(R) Arc(TM) Graphics  (type=1, api 1.4.325)
[vk] 选中 queue family 0（graphics + present 同族）
[vk] device ready: NVIDIA GeForce RTX 4060 Laptop GPU
[vk] swapchain: 3 张 image, 1280x720, format=50, presentMode=FIFO
     （minImageCount=2, 我申请了 3 —— 多要一张是为了不 stall）
[vk] init 完成。timestampPeriod = 1.00 ns
```

> 有价值的实测点：本机是**双 GPU**，程序按"第一个同时支持 graphics + present 的队列族"选中了独显。
> 这正是"为什么 Vulkan 要显式挑 physical device"的活例子 —— OpenGL 里你没有这个选择权。

### ⚠️ 已知问题：帧循环崩溃（未修）

**现象**：初始化成功后，进入帧循环时收到一次 `WM_SIZE`（`ShowWindow` 之后自动触发）→ 走 `recreate()` 重建 swapchain → 随后崩溃：

```
ExitCode = -1073741819 (0xC0000005，访问违例)
异常码 0xC0000005
  操作: 执行  地址: 0000000000000000     ← 跳到空地址 = 调用了空函数指针
  指令地址: 0000000000000000
调用栈: (空)
```

**已确认的事实**：
- 崩点在**第二次 `createSwapchain()` 成功之后**、帧循环第一次 `beginFrame()` 期间。
- "执行 + 地址 0x0" = 通过**空函数指针**调用，不是访问了非法数据。
- 初始化路径完全正常（首次 swapchain、device、queue、timestamp 全对）。

**当前最可能的根因（待验证，不当结论）**：`recreate()` 只重建了 swapchain，
但 fence / semaphore 是 init 时建的、跨 swapchain 存活 —— 嫌疑在这个生命周期边界上。

**排查计划**（下次继续，按顺序）：
1. 在 `beginFrame()` 每个调用点前后打日志，精确定位到**具体哪个 `vk*` 调用**崩的。
2. 用 Debug 配置重跑（`.\build.bat debug`）—— `/Od` 下栈回溯才有意义（Release 优化把栈帧压掉了，所以回溯是空的）。
3. 检查 `createSwapchain()` 里 `destroySwapchain()` 的调用时机：它销毁旧 image view 是对的，但**不该**动 per-frame 的 fence/semaphore。

---

## 踩到的坑（环境搭建的核心经验）

### 坑 1 — GLFW 无法下载，必须从源码编译
GitHub 直连被重置；注册表里配的 `127.0.0.1:7890` 代理**未运行**；国内镜像也不通（HTTP 全返回 `000`）。
→ 改用你提供的源码编译（只需做一次）：

```powershell
$VL = "D:\Code\heliostat_optimize\DiffHelio_viz\vulkan_learn"
$cmake = "C:\Program Files\CMake\bin\cmake.exe"     # PATH 上的是 devkitPro msys2 版，别用

& $cmake -S "$VL\third_party\glfw-3.5.1" -B "$VL\third_party\build-glfw" `
    -G "Visual Studio 17 2022" -A x64 `
    -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF -DGLFW_INSTALL=OFF

& $cmake --build "$VL\third_party\build-glfw" --config Release --target glfw
```

产物在 `third_party\build-glfw\src\Release\glfw3.lib`，已复制到 `third_party\lib\`。

### 坑 2 — CRT 必须一致：`/MT` vs `/MD` ⭐
首次链接报一堆 `无法解析的外部符号 __imp_strtok / __imp_strncpy / __imp_fminf`。

- **根因**：GLFW 经 CMake 编译用的是**动态 CRT（`/MD`）**，而 `cl` 命令行默认是**静态 CRT（`/MT`）**。
  符号名里的 **`__imp_` 前缀**就是证据 —— 它表示"从 DLL 导入"，静态 CRT 里不存在这种符号。
- **规则**：参与链接的所有目标必须用同一套 CRT。本项目固定带 `/MD`。

### 坑 3 — GLM 的 include 路径要指到内层目录
GLM 解压后是 `glm-1.0.3/glm/glm/glm.hpp`（**三层同名目录**）。代码写的是 `#include <glm/vec4.hpp>`，
所以 include 路径必须指到 `glm-1.0.3/glm`（含 `glm/` 子目录的那一层），而不是 `glm-1.0.3`。
→ 已通过统一整理到 `third_party/include/` 彻底规避。

### 坑 4 — `.bat` 文件必须纯 ASCII
cmd.exe 按 **OEM 代码页（中文 Windows 上是 GBK）** 解析 `.bat`。UTF-8 保存且含中文的 `.bat`
会被**逐字节撕碎**，报出 `'stay' is not recognized as an internal or external command` 这类诡异错误
（中文字节被拆到了行边界上）。PowerShell 脚本有 UTF-8 BOM 所以安全，`.bat` 不安全。

### 坑 5 — 项目下移一层后，`build.bat` 里的共享路径要跟着改
`shared/` 在 `vulkan_learn/` 下，而程序在 `vulkan_learn/day_01/pgX/` 里，
所以调用共享脚本必须写 **`..\..\shared\build_common.bat`**（上两级），写成 `..\shared` 会直接
报 `The system cannot find the path specified`。

### 坑 6 — PowerShell 加载 MSVC 环境不能靠 `vcvars64.bat`
`.bat` 是子进程，它设的环境变量**不会**影响 PowerShell 当前会话。
解法（`shared/msvc_env.ps1`）：让 cmd 跑 `vcvars64.bat` 后用 `set` 导出全部变量，再逐条 `Set-Item Env:` 导进当前会话。
实测导入 **104** 个变量后 `cl.exe` / `link.exe` 均可用。

### 附 — 本机 PATH 上有一堆 msys2/devkitPro 的同名工具
`cmd`、`cmake`、`curl`、`link`、`git` 都会被 `C:\devkitPro\msys2\usr\bin\` 下的版本**抢先**匹配。
表现是 Linux 风格的行为或干脆失败（如 `curl: couldn't create signal pipe`）。
**对策**：脚本里一律用绝对路径调用关键工具（`$env:SystemRoot\System32\cmd.exe`、
`C:\Program Files\CMake\bin\cmake.exe`），或先 dot-source `msvc_env.ps1` 让 MSVC 的 `link.exe` 拿到优先权。

---

## 关键知识点

### 1. `GLFW_CLIENT_API = GLFW_NO_API`
让 GLFW **只开窗口、不建图形上下文**。这是 GLFW 用于 Vulkan 的正确姿势 ——
Vulkan 的上下文概念由 `VkInstance` 承担，不需要 GLFW 那种 WGL/GLX 上下文。

### 2. `#define GLFW_INCLUDE_VULKAN` 必须在 `#include <GLFW/glfw3.h>` **之前**
它保证 `vulkan.h` 在正确位置被引入，避免与 Windows 头文件顺序冲突。

### 3. `glfwGetRequiredInstanceExtensions(&n)`
用 GLFW 时必须向 `VkInstanceCreateInfo` 申请的 instance 扩展（`VK_KHR_surface` + 平台 surface）。
写真正的 Vulkan 初始化时要用到 —— pg2 里是手写这两个扩展名，pg1 里 GLFW 会告诉你。

### 4. `VkSurfaceKHR` —— Vulkan 与 OpenGL 分道扬镳的地方
- **OpenGL**：窗口和 context 是同一件事（`glfwCreateWindow` 一起给你了），且 context 是**当前线程**的全局状态（`wglMakeCurrent`）。
- **Vulkan**：窗口是 OS 的，GPU 是驱动的，中间必须显式造桥 = `VkSurfaceKHR`。
  好处：同一个 `VkInstance` 能对接 Win32 / Xlib / Wayland / Android，**甚至完全没有窗口**。
- ⚠️ `VkSurfaceKHR` 归 **`VkInstance`** 管，销毁时第一个参数传 instance，**不是 device**。

### 5. fence vs semaphore（pg2 最重要的认知）

| | fence | semaphore |
|---|---|---|
| 谁等 | **CPU** 等 GPU（`vkWaitForFences`） | **GPU** 等 GPU（插在 submit / present 之间） |
| 用途 | CPU 要重用资源时，确认 GPU 已用完 | 队列操作之间的先后顺序 |
| present | ❌ 不能用 | ✅ 必须用 |

**present 为什么不能等 fence？** fence 只能被 CPU 等待，而 present 要等的是"GPU 渲染完成"这个
**设备侧**事件。若在 present 前 `vkWaitForFences`，等于每帧强制 CPU/GPU 串行，流水线彻底消失。

### 6. fence 必须初始化为 `VK_FENCE_CREATE_SIGNALED_BIT`
`beginFrame()` 第一步就是 `vkWaitForFences`。初始 unsignaled 会让第一次等待**永久阻塞**。

### 7. `waitDstStageMask` 为什么是 `COLOR_ATTACHMENT_OUTPUT`
semaphore 只保证"信号发出"，**不告诉 GPU 在哪个阶段等**。等的是"swapchain image 可写"，
在此之前的工作（顶点取数、vertex shader）根本不需要这张图。填 `TOP_OF_PIPE` 会让整条管线白等。

### 8. `oldLayout = UNDEFINED` 是最快路径
因为要 clear 整张图、旧内容完全不要。声明 `UNDEFINED` 等于告诉驱动"不用保留内容"，可跳过解压/回写。

### 9. 动态渲染（无 RenderPass）
Vulkan 1.3 起 `vkCmdBeginRendering` 成为 core。Heliostat Studio 全程走这条路 —— 它每个 pass 的
输出格式/附件组合都在变（HDR / bloom 各级 / swapchain），用 RenderPass 要为每种组合建对象 + framebuffer。

### 10. `swapchain` 是"一叠便利贴"
① `vkAcquireNextImageKHR` 拿一张 → ② 画 → ③ `vkQueuePresentKHR` 交回。
你**不拥有**这些 image（不能销毁，它们随 swapchain 生死）。数量由
`caps.minImageCount..maxImageCount` 决定，本机实测 `minImageCount = 2`，程序申请了 **3** 张
（多要一张是为了 acquire 时不 stall）。

---

## 待办 / 下一步

- [ ] **修掉 pg2 的帧循环崩溃**（按上面的排查计划 1 → 2 → 3）
- [ ] 让 pg1 打印出**确切的扩展数量**（示例代码没 `flush`，程序被强杀时 `std::cout` 缓冲会丢）
- [ ] Day 2：把 `vkCmdDraw` 加进 pg2（需要 graphics pipeline + SPIR-V 着色器 + descriptor）
- [ ] 记录本机 `VK_KHR_surface` / `VK_KHR_win32_surface` 之外的可用 instance 扩展清单

# Vulkan 学习环境（已配好，可复用）

本机环境已配置完成。本文档只记录**环境事实**与**编译/运行命令**。

---

## 1. 环境清单

| 组件 | 版本 | 位置 |
|---|---|---|
| Vulkan SDK | **1.4.341.1** | `C:\VulkanSDK\1.4.341.1`（环境变量 `VULKAN_SDK` 已设） |
| Vulkan loader | 1.4.341.1 | `C:\Windows\System32\vulkan-1.dll`（运行时不需要 SDK） |
| 着色器工具 | — | `glslc` / `glslangValidator` / `slangc` / `spirv-dis`（均在 PATH） |
| 编译器 | MSVC 19.44 (VS 2022 Community) | `C:\Program Files\Microsoft Visual Studio\2022\Community` |
| CMake | 4.0.0 | `C:\Program Files\CMake\bin\cmake.exe` |
| **GLFW** | 3.5.1 | 头文件 `third_party/include/GLFW/` · 静态库 `third_party/lib/glfw3.lib` |
| **GLM** | 1.0.3 | 头文件 `third_party/include/glm/`（header-only，无需编译） |

**可用 GPU（本机实测）**：NVIDIA GeForce RTX 4060 Laptop（Vulkan 1.4.329）· Intel Arc Graphics（Vulkan 1.4.325）

---

## 2. 目录结构

```
vulkan_learn/
├─ README.md                      ← 目录结构与规则
├─ shared/                        ← 跨项目复用
│   ├─ build_common.bat           通用构建脚本（各程序的 build.bat 调它）
│   ├─ msvc_env.ps1               ★ PowerShell 手动编译前先 dot-source 它
│   ├─ README_environment.md      本文件
│   └─ cards.md                   面试问答卡库
├─ day_01/                        ← 第 1 天（一天一个文件夹，可含多个程序）
│   ├─ day_01.md                  当日总结（当天所有程序都写在这一份里）
│   ├─ pg1_vulkan_window/         程序 1：GLFW + GLM + Vulkan 起步程序
│   │   ├─ build.bat
│   │   ├─ src/vulkan_window.cpp
│   │   └─ build/                 产物（vulkan_window.exe）
│   └─ pg2_hello_window/          程序 2：原生 Win32 + Vulkan 窗口（不用 GLFW）
│       ├─ build.bat
│       ├─ src/{hello_window.h, hello_window.cpp, platform_win32.cpp, vk_renderer.cpp, logx.cpp}
│       └─ build/                 产物（hello_window.exe）
└─ third_party/                   ← ★ 共享依赖，只读，任何程序都不修改
    ├─ glfw-3.5.1/                ← GLFW 源码（用户提供）
    ├─ glm-1.0.3/                 ← GLM 源码（用户提供）
    ├─ include/                   ← ★ 统一 include 根目录（所有程序都用它）
    │   ├─ GLFW/glfw3.h
    │   └─ glm/glm.hpp
    ├─ lib/glfw3.lib              ← ★ 编译好的 GLFW 静态库（703 KB）
    └─ build-glfw/                ← GLFW 的 CMake 构建目录
```

**关键点**：写新程序时只需要两个 include 路径 + 一个库：

```
/I third_party\include          ← GLFW 与 GLM 都在这里
/I %VULKAN_SDK%\Include
链接: third_party\lib\glfw3.lib  vulkan-1.lib  user32.lib gdi32.lib shell32.lib
```

> ⚠️ 层级关系：程序在 `vulkan_learn/day_NN/pgX/` 里，而 `shared/` 与 `third_party/` 在 `vulkan_learn/` 下。
> 所以从程序目录出发，它们是 **上两级**：
> `..\..\shared\build_common.bat`、`..\..\third_party\include`。
> 写成 `..\shared` 会直接报 `The system cannot find the path specified`。

---

## 3. 编译与运行命令

> 每个程序都有**两套等价命令**：`.bat` 脚本（省事）与**手动 PowerShell 命令**（学习主线，逐参数可见）。
> 完整规则见 `vulkan_learn/README.md`。

### 3.1 编译（脚本方式，在程序目录下）

```powershell
cd D:\Code\heliostat_optimize\DiffHelio_viz\vulkan_learn\day_01\pg1_vulkan_window
.\build.bat
```

### 3.2 运行

```powershell
.\build\vulkan_window.exe
```

程序弹出一个 800×600 的窗口，控制台打印 `N extensions supported`（N 是本机 Vulkan instance 扩展数）。
**关闭窗口即退出。**

> ⚠️ PowerShell 用 `.\xxx.exe` 启动 GUI 程序时**不等待它退出**，拿不到退出码。
> 需要等待并取退出码时用：
> ```powershell
> Start-Process .\build\vulkan_window.exe -Wait -PassThru | Select-Object ExitCode
> ```

### 3.3 手动编译（等效于脚本里做的事，推荐用来理解每一步）

**第一步 — 加载 MSVC 环境（每个 PowerShell 窗口一次）**

```powershell
. D:\Code\heliostat_optimize\DiffHelio_viz\vulkan_learn\shared\msvc_env.ps1
```

注意**前面的点**（dot-source）：这样脚本设置的环境变量才落在当前会话。
`.bat` 做不到 —— 它是子进程，改了环境父进程看不见。实测导入 104 个变量后 `cl.exe` 可用。

**第二步 — 编译**（就在 `pg1_vulkan_window` 目录下，注意 `/MD`，见第 4 节坑 2）

```powershell
$VL = "D:\Code\heliostat_optimize\DiffHelio_viz\vulkan_learn"
New-Item -ItemType Directory -Force -Path .\build | Out-Null

cl /nologo /std:c++17 /EHsc /W3 /utf-8 /MD /O2 /DNDEBUG /Zi `
   /I src /I "$VL\third_party\include" /I "$env:VULKAN_SDK\Include" `
   src\vulkan_window.cpp `
   /Fe:build\vulkan_window.exe `
   /link /LIBPATH:"$env:VULKAN_SDK\Lib" `
   "$VL\third_party\lib\glfw3.lib" vulkan-1.lib user32.lib gdi32.lib shell32.lib
```

> ⚠️ **注意路径写法**：你已经在 `pg1_vulkan_window\` 里，源码就在**当前目录**的 `src\` 下，
> 所以是 `src\vulkan_window.cpp`，**不是 `..\src\vulkan_window.cpp`**。
> `..\src` 会跑到 `day_01\src\` 去找，那里没有 `src` 目录，报
> `c1xx: fatal error C1083: 无法打开源文件`。
> 只有 `third_party\` 和 `shared\` 才需要 `..\..`（它们在 `vulkan_learn\` 下）。
>
> 多文件程序把所有 `.cpp` 都列给 `cl`，或用 `(Get-ChildItem .\src\*.cpp).FullName` 通配展开。

---

## 4. 环境搭建过程中的三个坑（记录备查）

### 坑 1：GLFW 必须自己编译（本机无法访问 GitHub）

GitHub 直连不通、注册表里配的 `127.0.0.1:7890` 代理未运行，所以无法下载 GLFW 官方预编译包。
改用**从源码编译**：把 GLFW 源码放 `third_party/glfw-3.5.1` 后，先配置再编译（只需做一次）：

```bat
cmake -S third_party\glfw-3.5.1 -B third_party\build-glfw -G "Visual Studio 17 2022" -A x64 ^
      -DGLFW_BUILD_EXAMPLES=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_DOCS=OFF -DGLFW_INSTALL=OFF

cmake --build third_party\build-glfw --config Release --target glfw
```

产物在 `third_party\build-glfw\src\Release\glfw3.lib`，已复制到 `third_party\lib\`。

### 坑 2：CRT 必须一致 —— `/MT` 还是 `/MD`

第一次链接报了一堆 `无法解析的外部符号 __imp_strtok / __imp_strncpy / __imp_fminf`。

**原因**：GLFW 通过 CMake 编译时用的是**动态 CRT（`/MD`）**，而 `cl` 命令行默认是**静态 CRT（`/MT`）**。
符号名里的 `__imp_` 前缀就是证据——它是"从 DLL 导入"的标记，静态 CRT 里没有这种符号。

**规则**：**所有参与链接的目标必须用同一套 CRT。** 所以本项目的编译命令固定带 `/MD`。
（若将来自己编 GLFW 并想用 `/MT`，必须给 CMake 传 `-DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded`，两边保持一致。）

### 坑 3：GLM 的 include 路径要指到内层目录

GLM 源码解压后是 `glm-1.0.3/glm/glm/glm.hpp`（**三层同名目录**）。
因为代码里写的是 `#include <glm/vec4.hpp>`，所以 include 路径必须指到 **`glm-1.0.3/glm`**（即包含 `glm/` 子目录的那一层），
而不是 `glm-1.0.3`。指错会报 `无法打开源文件 "glm/vec4.hpp"`。

已统一解决：头文件被整理到 `third_party/include/`，这样 `include` 一个路径就覆盖 GLFW 与 GLM。

---

## 5. 后续写新程序的最小模板

新建 `src/xxx.cpp` 后，复制 `build_vulkan_window.bat` 改两处即可：

1. 源文件名：`"%PRJ%src\vulkan_window.cpp"` → 你的文件名
2. 输出名：`/Fe:vulkan_window.exe` → 你的 exe 名

包含头文件的写法（注意 Vulkan 的顺序要求）：

```cpp
#define GLFW_INCLUDE_VULKAN          // 让 GLFW 帮你引入 <vulkan/vulkan.h>，顺序自动正确
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/mat4x4.hpp>

// 用 GLFW 拿 Vulkan 需要的实例扩展（写真正的 Vulkan 初始化时要用）：
//   uint32_t n; const char** ext = glfwGetRequiredInstanceExtensions(&n);
```

> 提示：`GLFW_INCLUDE_VULKAN` 必须**在** `#include <GLFW/glfw3.h>` **之前**定义。
> 它保证 `vulkan.h` 在正确的位置被引入，否则会出现 Vulkan/Windows 头文件顺序冲突。

# vulkan_learn — 目录结构与规则

Vulkan 学习仓库。按**天**组织：一天一个文件夹，一天内可以有多个学习程序；共享依赖与构建脚本集中放置。

```
vulkan_learn/
├─ README.md                    本文件：结构与规则
├─ shared/                      跨项目复用
│   ├─ build_common.bat          通用构建脚本（各程序的 build.bat 调它）
│   ├─ msvc_env.ps1              ★ PowerShell 手动编译前先 dot-source 它
│   ├─ README_environment.md     环境清单
│   └─ cards.md                  面试问答卡库
├─ day_01/                      ← 第 1 天
│   ├─ day_01.md                 当日总结
│   ├─ pg1_vulkan_window/        程序 1：GLFW + GLM + Vulkan 起步程序
│   │   ├─ build.bat
│   │   ├─ src/
│   │   └─ build/
│   └─ pg2_hello_window/         程序 2：原生 Win32 + Vulkan 窗口
│       ├─ build.bat
│       ├─ src/
│       └─ build/
└─ third_party/                  ★ 只读共享依赖，任何程序都不修改
    ├─ include/{GLFW, glm}        统一 include 根目录
    ├─ lib/glfw3.lib
    ├─ glfw-3.5.1/                GLFW 源码
    ├─ glm-1.0.3/                 GLM 源码
    └─ build-glfw/                GLFW 的 CMake 构建目录
```

---

## 规则

### 规则 1 — 目录层级：`day_NN/` 下面直接放程序

```
day_NN/                        ← 一天
├─ day_NN.md                   ← 当日总结
├─ pg1_主题名/                 ← 程序 1
│   ├─ build.bat
│   ├─ src/
│   └─ build/
└─ pg2_主题名/                 ← 程序 2（同一天可以有多个）
    └─ ...
```

一天只有一个总结文件（`day_NN.md`），当天所有程序都写进这一份，**不按程序拆成多个 md**。

### 规则 2 — 每个程序必须有**两套**命令：`.bat` 脚本 + PowerShell 手动

**这条是硬要求。** 只用 `.bat` 学习是有害的：脚本把编译过程包成了黑盒，
而学习阶段最该看见的恰恰是"到底调了什么、参数是什么意思"。

所以每个程序目录里：

- **`build.bat` 是快捷封装**，只为省事，不作为学习入口；
- **`day_NN.md` 里必须给出等价的手动 PowerShell 命令**，逐参数可读、可改、可单步试。

两套命令必须**等价且都实测过**。

`.bat` 内容固定三件事：设定 `EXE_NAME` / `CFG_FLAGS`，然后调用共享脚本：

```bat
set EXE_NAME=你的程序名
set CFG_NAME=Release
set CFG_FLAGS=/O2 /DNDEBUG /Zi
call "%~dp0..\..\shared\build_common.bat" "%~dp0" %~1
```

> ⚠️ 注意是 **`..\..\shared`**（上两级）。`shared/` 在 `vulkan_learn/` 下，程序在 `day_NN/pgX/` 里。
> 写成 `..\shared` 会直接报 `The system cannot find the path specified`。

支持 `build.bat`（Release）/ `build.bat debug`（Debug）/ `build.bat clean`。

### 规则 3 — 源码放 `src/`，产物放 `build/`
`build_common.bat` 编译 **`src/` 下的所有 `.cpp`**，输出 `build\<EXE_NAME>.exe`。
新增/删除源文件**不需要改构建脚本**。

### 规则 4 — `third_party/` 只读共享
`build_common.bat` 会从程序目录**向上查找**含 `third_party/include/GLFW/glfw3.h` 的目录作为根，
所以程序放在任意嵌套深度都能正确引用共享依赖 —— 这就是 `third_party/` 能一直不动的原因。

### 规则 5 — 总结必须写实测结果，不许写"应该可以"
`day_NN.md` 固定结构：目标 → 环境事实 → 每个程序（两套命令 + 实测结果）→ 踩的坑 → 关键知识点 → 待办。
**没跑过的不写，跑失败的要如实记录**（见 `day_01/day_01.md` 里 pg2 那条未修的崩溃）。

---

## 手动编译（PowerShell）—— 学习主线

### 第一步：加载 MSVC 环境（每个 PowerShell 窗口一次）

```powershell
. D:\Code\heliostat_optimize\DiffHelio_viz\vulkan_learn\shared\msvc_env.ps1
```

**注意前面的点**（dot-source）：这样脚本里设的环境变量才会落在**当前会话**。
`.bat` 做不到这件事 —— 它是子进程，改了环境也不影响父进程。

实测：导入 **104** 个环境变量后 `cl.exe` / `link.exe` 均可用。

### 第二步：编译（以 pg1 为例）

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

> ⚠️ **相对路径只对 `third_party` / `shared` 用 `..\..`**：
> 你已经在 `pgN_xxx\` 里，源码在**当前目录**的 `src\` 下 → 写 `src\...`；
> 而 `third_party\` 与 `shared\` 在 `vulkan_learn\` 下 → 走 `$VL\...`（或 `..\..\`）。
> 把源码写成 `..\src\...` 会跑到 `day_NN\src\` 去找，报
> `c1xx: fatal error C1083: 无法打开源文件`。

**参数含义**：

| 参数 | 作用 |
|---|---|
| `/std:c++17` | 语言标准 |
| `/EHsc` | 启用 C++ 异常 |
| `/W3` | 警告等级 |
| `/utf-8` | 源码按 UTF-8 解读（**否则中文注释会编译报错**） |
| **`/MD`** | **动态 CRT，必须与 GLFW 一致**（见铁律 2） |
| `/O2 /DNDEBUG` | Release 优化 |
| `/Zi` | 生成 `.pdb`，崩溃时能给出文件:行号 |
| `/I <dir>` | 头文件搜索路径 |
| `/Fe:name.exe` | 输出文件名 |
| `/link` | 之后的参数交给链接器 |
| `/LIBPATH:<dir>` | 库搜索路径 |
| `xxx.lib` | 要链接的库 |

多文件时把所有 `.cpp` 都列给 `cl`（也可用 `(Get-ChildItem .\src\*.cpp).FullName` 通配展开）。

### 第三步：运行

```powershell
.\build\vulkan_window.exe
```

> ⚠️ **PowerShell 用 `.\xxx.exe` 启动 GUI 程序时不等待它退出**，所以拿不到退出码。
> 要等它结束并拿退出码：
> ```powershell
> Start-Process .\build\vulkan_window.exe -Wait -PassThru | Select-Object ExitCode
> ```

---

## 铁律（都是踩坑换来的）

1. **`.bat` 文件必须纯 ASCII。**
   cmd.exe 按 OEM 代码页（中文 Windows 上是 GBK）解析 `.bat`，UTF-8 中文会被逐字节撕碎，
   报出 `'stay' is not recognized as an internal or external command` 这类诡异错误。
   PowerShell 脚本有 UTF-8 BOM 所以安全，`.bat` 不安全。

2. **CRT 必须统一为 `/MD`。**
   GLFW 是用 `/MD` 编的；混用 `cl` 默认的 `/MT` 会报一堆 `__imp_xxx` 未解析符号。

3. **不靠 `vcvars64.bat` 改 PowerShell 环境。**
   它是子进程，改了当前会话也看不到。用 `shared/msvc_env.ps1`。

4. **关键工具用绝对路径调用。**
   本机 PATH 上 `cmd` / `cmake` / `curl` / `link` / `git` 都被 `C:\devkitPro\msys2\usr\bin\` 抢先匹配，
   会导致 Linux 风格行为或失败。例：用 `$env:SystemRoot\System32\cmd.exe`、
   `C:\Program Files\CMake\bin\cmake.exe`。

5. **不许在没测的情况下写"更快了" / "应该可以"。**

---

## 新增一天的流程

1. 建目录：`day_NN/pg1_主题名/src/`
2. 拷一个现成的 `build.bat`，改 `EXE_NAME`，并确认共享脚本路径是 `..\..\shared\`
3. 写代码到 `src/`
4. **两套命令都跑一遍**：`.bat` 和手动 PowerShell 编译
5. 写 `day_NN/day_NN.md`（含两套命令 + 实测结果）

---

## 当前进度

| Day | 程序 | 状态 |
|---|---|---|
| **day_01** | `pg1_vulkan_window` — GLFW + GLM + Vulkan 起步程序 | ✅ 编译通过、窗口实测弹出 |
| **day_01** | `pg2_hello_window` — 原生 Win32 + Vulkan 窗口 | ⚠️ 编译通过、init 实测成功；帧循环有**未修崩溃**（见 `day_01/day_01.md`） |

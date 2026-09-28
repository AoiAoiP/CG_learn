# CMakeLists.txt 模板 — 使用说明

## 用途

`vulkan_learn/day_NN/pgN_xxx/` 下每个程序的 CMake 构建文件模板。
复用方式：**复制本文件 → 只改最上面「必须修改」那一节**。

```powershell
Copy-Item D:\...\vulkan_learn\shared\templates\CMakeLists.txt .\CMakeLists.txt
# 然后编辑：PROJ_NAME、以及需要的 USE_* 开关
```

---

## 必须修改的部分

```cmake
set(PROJ_NAME  pg_template)   # 可执行文件名（不加 .exe）
set(PROJ_SRC   src)           # 源码目录

set(USE_GLFW   OFF)           # 需要开窗口 → ON
set(USE_GLM    ON)            # 需要矩阵/向量 → ON
set(USE_VULKAN ON)            # 需要 Vulkan → ON
set(USE_SHADERS OFF)          # 需要编译 .slang/.glsl → ON

set(SHADER_ENTRIES "")        # 仅 USE_SHADERS=ON 时有效
```

`SHADER_ENTRIES` 语法（`<输出>.spv=<源文件>:<入口点>`）：

```cmake
set(SHADER_ENTRIES
    "forward.spv=forward.slang:renderForward"
    "clear.spv=forward.slang:clearFlux"
    "post.spv=post.frag:main")
```

shader stage 由**源文件扩展名自动推导**：

| 扩展名 | stage |
|---|---|
| `.slang` | `compute`（写其它 stage 时需在模板里改这一处） |
| `.comp` / `.glsl` | `compute` |
| `.vert` | `vertex` |
| `.frag` | `fragment` |

---

## 配置 / 构建 / 运行命令

**前置**：`cmake` 必须用原生版（PATH 上若还有 devkitPro msys2 版会不支持 VS 生成器）。

```powershell
$CMAKE = "C:\Program Files\CMake\bin\cmake.exe"
cd <你的程序目录>
```

**配置**（只做一次）

```powershell
& $CMAKE -S . -B build-cmake -G "Visual Studio 17 2022" -A x64
```

**构建**

```powershell
& $CMAKE --build build-cmake --config Release
```

**运行**

```powershell
.\build-cmake\bin\Release\<PROJ_NAME>.exe
```

**干净重建**

```powershell
Remove-Item -Recurse -Force .\build-cmake
& $CMAKE -S . -B build-cmake -G "Visual Studio 17 2022" -A x64
& $CMAKE --build build-cmake --config Release
```

---

## 模板行为说明

| 行为 | 说明 |
|---|---|
| **自动定位根目录** | 从程序目录向上最多 6 层，找含 `third_party/include/GLFW/glfw3.h` 的目录作为 `VL_ROOT`。所以程序的嵌套深度变了也不用改。可用 `-DVL_ROOT=<path>` 覆盖 |
| **自动收集源码** | `file(GLOB ... CONFIGURE_DEPENDS src/*.cpp)` —— 新增/删除 `.cpp` 不用改本文件 |
| **CRT 强制 `/MD`** | `MSVC_RUNTIME_LIBRARY = MultiThreadedDLL`。必须与 GLFW 一致，否则链接报一堆 `__imp_xxx` 未解析符号 |
| **GLFW 用 IMPORTED 目标** | `third_party/build-glfw` 只生成了 `glfw3Config.cmake`，缺它内部 include 的 `glfw3Targets.cmake`（子项目模式不做 install/export），所以 `find_package(glfw3)` 不可用，改为直接声明静态库 |
| **Vulkan 用 `find_package`** | CMake 自带 `FindVulkan` 模块，经 `VULKAN_SDK` 环境变量定位，给出 `Vulkan::Vulkan` 导入目标 |
| **SPIR-V 自动拷贝** | POST_BUILD 把 `build-cmake/shaders/` 拷到可执行文件旁，程序可就近加载 |
| **输出路径** | `build-cmake/bin/<Config>/<PROJ_NAME>.exe`（VS 是多配置生成器，多一层 `Release`/`Debug`） |
| **不修改 `third_party/`** | 全程只读引用，不安装、不拷贝、不写入 |

---

## 已知注意事项

1. **`cmake` 必须是原生版**。PATH 上的 devkitPro msys2 版不支持 `-G "Visual Studio 17 2022"`，
   会报 `Could not create named generator Visual Studio 17 2022`。
2. **`link` 找不到是正常的**。MSVC 链接器目录不在默认 PATH 上；`cmake --build` 走 MSBuild，不依赖 shell 的 `link`。
   只有手动敲 `cl`/`link` 时才需要先加载 `shared/msvc_env.ps1`。
3. **`.slang` 一律按 compute 编译**。写 vertex/fragment 着色器时需在模板里改 stage 推导那一处。
4. **首次配置后 `SHADER_ENTRIES` 改动需重新配置**（改了 `CMakeLists.txt` 会自动重配）。

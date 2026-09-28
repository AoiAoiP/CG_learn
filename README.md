# CG_Learn

计算机图形学学习项目：CUDA / OpenGL / Vulkan 三条线的**代码 + 笔记**合集。
每个模块都按"能跑起来 + 写清踩过的坑"来组织，笔记与代码放在一起，避免笔记和实现对不上。

```
CG_Learn/
├─ Cuda/            CUDA 学习笔记（Markdown + 配图）
├─ OpenGL/          OpenGL 课程练习，37 个课次程序（L1-1 … L4-2-EX）
└─ vulkan_learn/    Vulkan 学习仓库，按"天"组织（day_01 / day02 / …）
```

---

## Cuda/

| 文件 | 内容 |
|---|---|
| `Cuda/Cuda Learning.md` | CUDA 学习笔记：环境配置 → 线程组织 → 程序基本框架 → 错误检测 → GPU 加速关键 → 内存组织 |
| `Cuda/images/` | 笔记配图（线程索引示意、纹理内存、VS 环境等） |

纯笔记模块，无构建脚本。

---

## OpenGL/

按课次目录组织，每课一个独立小程序，`Makefile` 单文件编译：

| 目录 | 内容 |
|---|---|
| `L1-*` | 起步：窗口、三角形、矩形、着色器、Uniform、纹理、变换、坐标系、摄像机 |
| `L2-*` | 光照：颜色、基础光照、材质、光照贴图、投光物、多光源 |
| `L3/` | 模型加载（Assimp） |
| `L4-*` | 高级：深度测试、模板测试、混合 |
| `LearningNote.md` | OpenGL 学习笔记正文 |
| `glad/` | GLAD 加载器（`include/` + `src/glad.c`） |
| `resources/` | 纹理、示意图与 `backpack/` 模型（OBJ + 贴图，含 `source_attribution.txt` 出处说明） |

> 本模块原先是一个独立 git 仓库（远程 `gitee.com/sadaba/learn-open-gl.git`）。
> 并入本仓库时移除了嵌套的 `.git`，历史备份保留在本地 `.git-backups/OpenGL.git`（已被 `.gitignore` 忽略，不参与上传）。

---

## vulkan_learn/

Vulkan 学习仓库，规则与详细说明见 [`vulkan_learn/README.md`](vulkan_learn/README.md)（含两套编译命令、铁律、当前进度表）。

```
vulkan_learn/
├─ shared/            跨项目复用：build_common.bat、msvc_env.ps1、cards.md、templates/
├─ day_01/            pg1_vulkan_window（GLFW+GLM+Vulkan）、pg2_hello_window（原生 Win32）、pg3_triangle
├─ day02/             pg4_instance、pg5_validation_layers
├─ day_99_template_test/
├─ third_party/       只读共享依赖：GLFW 3.5.1 源码 + glm 1.0.3 源码 + lib/glfw3.lib
└─ Vulkan_Tutorial_CN.pdf   中文 Vulkan 教程
```

要点（详见该目录 README）：

- **两套命令**：`build.bat` 快捷封装 + `day_NN.md` 里逐参数可读的手动 PowerShell 命令，两者等价且实测过。
- **CRT 统一 `/MD`**，源码统一 `/utf-8`（否则中文注释编译报错）。
- **`.bat` 必须纯 ASCII**（cmd.exe 按 OEM 代码页解析）。
- 手动编译前先 dot-source `shared/msvc_env.ps1` 加载 MSVC 环境。

---

## 关于本仓库的取舍

为控制体积，`.gitignore` 已排除**构建产物**：`build*/`、`*.o`、`*.obj`（`resources/backpack/backpack.obj` 模型除外）、`*.pdb`、`*.ilk`、`*.exe`、无扩展名的 `opengl_app`、以及下载残留的 `*Zone.Identifier*`。
`third_party/`（GLFW / GLM 源码与预编译 `glfw3.lib`）与中文教程 PDF 按约定**保留**，克隆后可直接编译。

克隆后需要自己生成的内容：各 `build/`、`build-cmake/` 输出目录，以及 OpenGL 各课次下的 `.o` / 可执行文件。

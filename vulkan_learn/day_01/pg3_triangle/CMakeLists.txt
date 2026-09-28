# ===========================================================================
# CMakeLists.txt 模板 — 用于 vulkan_learn/day_NN/pgN_xxx/ 下的每个程序
#
# 复制本文件到你的程序目录，然后**只改最上面"必须修改"那一节**即可。
# 其余部分是模板逻辑，通常不用动。
#
# 设计前提（本仓库的约定）：
#   * 程序位于 <vulkan_learn>/day_NN/pgN_xxx/，源码放 src/
#   * 第三方库在 <vulkan_learn>/third_party/，**只读、不安装、不修改**
#     - GLFW 头:   third_party/include/GLFW
#     - GLFW 静态库: third_party/build-glfw/src/Release/glfw3.lib
#     - GLM 头:    third_party/include/glm  （header-only）
#     - Vulkan:    由 CMake 自带 FindVulkan 经 VULKAN_SDK 定位
#   * 编译器：MSVC（x64）。GLFW 是用 /MD（动态 CRT）编的，必须保持一致。
#
# 典型用法：
#   & "C:\Program Files\CMake\bin\cmake.exe" -S . -B build-cmake -G "Visual Studio 17 2022" -A x64
#   & "C:\Program Files\CMake\bin\cmake.exe" --build build-cmake --config Release
# ===========================================================================
cmake_minimum_required(VERSION 3.20)

# ===========================================================================
# 一、必须修改：本程序的身份
# ===========================================================================
set(PROJ_NAME  pg3_trianglec)          # 可执行文件名（不加 .exe）
set(PROJ_SRC   src)                  # 源码目录（相对本文件）

# 依赖开关：按需打开
set(USE_GLFW   ON)                  # 需要开窗口 → ON
set(USE_GLM    ON)                   # 需要矩阵/向量 → ON（header-only，零成本）
set(USE_VULKAN ON)                   
set(USE_SHADERS OFF)                 # 需要编译 .slang/.glsl → ON

# 仅当 USE_SHADERS=ON 时有效
set(SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
# 语法：<输出文件名>=<源文件>:<入口点>
#   glslc  —— 每个 .glsl 编译成一个 .spv，入口点固定 main，冒号部分可省略
#   slangc —— 每个入口点单独编译成一个 .spv，所以必须写入口点
# 例：
#   "forward.spv=forward.slang:main"
#   "flux.spv=flux.slang:fluxLite"
set(SHADER_ENTRIES "")

# ===========================================================================
# 二、模板逻辑（一般不用改）
# ===========================================================================
project(${PROJ_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- 2.1 向上查找 vulkan_learn 根（含 third_party 的那一层） ----------------
#     程序在 <root>/day_NN/pgN_xxx/，深一层浅一层都不用改。
#     也可用 -DVL_ROOT=<path> 手动指定。
set(VL_ROOT "" CACHE PATH "vulkan_learn 根目录（含 third_party）")
if(NOT VL_ROOT)
    set(_probe "${CMAKE_CURRENT_SOURCE_DIR}")
    foreach(_i RANGE 1 6)
        set(_probe "${_probe}/..")
        get_filename_component(_probe "${_probe}" ABSOLUTE)
        if(EXISTS "${_probe}/third_party/include/GLFW/glfw3.h")
            set(VL_ROOT "${_probe}")
            break()
        endif()
    endforeach()
endif()
if(NOT VL_ROOT)
    message(FATAL_ERROR
        "向上找不到 vulkan_learn 根目录（即含 third_party/include/GLFW/glfw3.h 的那一层）。\n"
        "请用 -DVL_ROOT=<path> 手动指定。")
endif()
set(VL_INCLUDE_DIR "${VL_ROOT}/third_party/include")

# --- 2.2 收集源码 -----------------------------------------------------------
#     自动扫 src/ 下所有 .cpp，所以新增源文件不必改本文件。
file(GLOB PROJ_SOURCES CONFIGURE_DEPENDS "${PROJ_SRC}/*.cpp")
if(NOT PROJ_SOURCES)
    message(FATAL_ERROR "在 ${CMAKE_CURRENT_SOURCE_DIR}/${PROJ_SRC} 下没有找到任何 .cpp 文件")
endif()

add_executable(${PROJ_NAME} ${PROJ_SOURCES})

# --- 2.3 编译选项 -----------------------------------------------------------
if(MSVC)
    target_compile_options(${PROJ_NAME} PRIVATE /W3 /utf-8)
    target_compile_definitions(${PROJ_NAME} PRIVATE
        _CRT_SECURE_NO_WARNINGS
        WIN32_LEAN_AND_MEAN
        NOMINMAX)

    # ★ 关键：必须与 GLFW 的 CRT 一致（GLFW 经 CMake 编译 = 动态 CRT）
    #   若不一致，链接期会报一堆 无法解析的外部符号 __imp_xxx
    set_property(TARGET ${PROJ_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
else()
    target_compile_options(${PROJ_NAME} PRIVATE -Wall -Wextra)
endif()

# --- 2.4 依赖：GLM（header-only） -------------------------------------------
if(USE_GLM)
    if(NOT EXISTS "${VL_INCLUDE_DIR}/glm/glm.hpp")
        message(FATAL_ERROR "找不到 GLM 头文件: ${VL_INCLUDE_DIR}/glm/glm.hpp")
    endif()
endif()

# --- 2.5 依赖：GLFW（复用已编译好的静态库） --------------------------------
#   third_party/build-glfw 里只生成了 glfw3Config.cmake，缺少它内部 include 的
#   glfw3Targets.cmake（子项目模式不做 install/export），因此 find_package(glfw3)
#   不可用 —— 直接把现有静态库声明为 IMPORTED 目标。
if(USE_GLFW)
    set(_glfw_candidates
        "${VL_ROOT}/third_party/lib/glfw3.lib"
        "${VL_ROOT}/third_party/build-glfw/src/Release/glfw3.lib"
        "${VL_ROOT}/third_party/build-glfw/src/Debug/glfw3.lib")
    set(VL_GLFW_LIB "")
    foreach(_c ${_glfw_candidates})
        if(EXISTS "${_c}")
            set(VL_GLFW_LIB "${_c}")
            break()
        endif()
    endforeach()
    if(NOT VL_GLFW_LIB)
        message(FATAL_ERROR
            "找不到 GLFW 静态库。已尝试:\n  ${_glfw_candidates}\n"
            "请先编译 GLFW（见 shared/README_environment.md 第 4 节坑 1）。")
    endif()

    add_library(glfw STATIC IMPORTED)
    set_target_properties(glfw PROPERTIES
        IMPORTED_LOCATION "${VL_GLFW_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${VL_INCLUDE_DIR}")
    # GLFW 静态库自身依赖的 Windows 系统库
    set_property(TARGET glfw APPEND PROPERTY
        INTERFACE_LINK_LIBRARIES "user32;gdi32;shell32")

    target_link_libraries(${PROJ_NAME} PRIVATE glfw)
    message(STATUS "GLFW 静态库 : ${VL_GLFW_LIB}")
endif()

# --- 2.6 依赖：Vulkan -------------------------------------------------------
#   CMake 自带 FindVulkan 模块，通过 VULKAN_SDK 环境变量定位 SDK，
#   提供导入目标 Vulkan::Vulkan（自动带上 include 与 vulkan-1.lib）。
if(USE_VULKAN)
    find_package(Vulkan REQUIRED)
    target_link_libraries(${PROJ_NAME} PRIVATE Vulkan::Vulkan)
    message(STATUS "Vulkan      : ${Vulkan_LIBRARIES}")
endif()

# --- 2.7 头文件搜索路径 -----------------------------------------------------
target_include_directories(${PROJ_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/${PROJ_SRC}"
    "${VL_INCLUDE_DIR}")

# --- 2.8 着色器编译（可选） -------------------------------------------------
#   用 POST_BUILD 把 .spv 拷到可执行文件旁边，程序运行时就近加载。
if(USE_SHADERS)
    find_program(SLANGC_EXECUTABLE NAMES slangc slangc.exe
        HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
    find_program(GLSLC_EXECUTABLE NAMES glslc glslc.exe
        HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin")
    if(NOT SLANGC_EXECUTABLE AND NOT GLSLC_EXECUTABLE)
        message(FATAL_ERROR "USE_SHADERS=ON 但 PATH 上找不到 slangc 或 glslc（Vulkan SDK 自带）")
    endif()
    if(SHADER_ENTRIES STREQUAL "")
        message(FATAL_ERROR "USE_SHADERS=ON 但 SHADER_ENTRIES 为空，请填写着色器清单")
    endif()

    set(_spv_out "${CMAKE_BINARY_DIR}/shaders")
    file(MAKE_DIRECTORY "${_spv_out}")
    set(_spv_files "")

    foreach(_entry_def ${SHADER_ENTRIES})
        string(REPLACE "=" ";" _kv "${_entry_def}")
        list(GET _kv 0 _out_name)
        list(GET _kv 1 _rest)
        string(REPLACE ":" ";" _rp "${_rest}")
        list(GET _rp 0 _src_file)
        list(LENGTH _rp _n)
        if(_n GREATER 1)
            list(GET _rp 1 _entry_point)
        else()
            set(_entry_point "main")
        endif()

        set(_src_path "${SHADER_DIR}/${_src_file}")
        set(_out_path "${_spv_out}/${_out_name}")
        if(NOT EXISTS "${_src_path}")
            message(FATAL_ERROR "着色器源文件不存在: ${_src_path}")
        endif()

        if(SLANGC_EXECUTABLE)
            # 按源码扩展名推导 shader stage（slangc 必须显式给 -stage）
            get_filename_component(_src_ext "${_src_file}" EXT)
            if(_src_ext STREQUAL ".slang")
                set(_stage "compute")     # .slang 里可写任意 stage，需按需改为 vertex/fragment
            elseif(_src_ext STREQUAL ".comp" OR _src_ext STREQUAL ".glsl")
                set(_stage "compute")
            elseif(_src_ext STREQUAL ".vert")
                set(_stage "vertex")
            elseif(_src_ext STREQUAL ".frag")
                set(_stage "fragment")
            else()
                message(FATAL_ERROR
                    "无法从扩展名推导 shader stage: ${_src_file}（可用 .slang/.comp/.vert/.frag）")
            endif()

            add_custom_command(OUTPUT "${_out_path}"
                COMMAND "${SLANGC_EXECUTABLE}" "${_src_path}" -target spirv
                        -entry "${_entry_point}" -stage ${_stage} -o "${_out_path}"
                DEPENDS "${_src_path}"
                COMMENT "slangc: ${_src_file}:${_entry_point} (-stage ${_stage}) -> ${_out_name}"
                VERBATIM)
        else()
            add_custom_command(OUTPUT "${_out_path}"
                COMMAND "${GLSLC_EXECUTABLE}" "${_src_path}" -o "${_out_path}"
                DEPENDS "${_src_path}"
                COMMENT "glslc: ${_src_file} -> ${_out_name}"
                VERBATIM)
        endif()
        list(APPEND _spv_files "${_out_path}")
    endforeach()

    add_custom_target(${PROJ_NAME}_shaders DEPENDS ${_spv_files})
    add_dependencies(${PROJ_NAME} ${PROJ_NAME}_shaders)
    add_custom_command(TARGET ${PROJ_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${_spv_out}" "$<TARGET_FILE_DIR:${PROJ_NAME}>/shaders"
        COMMENT "拷贝 SPIR-V 到可执行文件旁")
    message(STATUS "着色器      : ${_spv_out}")
endif()

# --- 2.9 输出目录 -----------------------------------------------------------
#   Visual Studio 是多配置生成器，会再套一层 Release/Debug。
set_target_properties(${PROJ_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

message(STATUS "---- ${PROJ_NAME} 配置摘要 ----")
message(STATUS "VL_ROOT     : ${VL_ROOT}")
message(STATUS "源码        : ${PROJ_SOURCES}")

# 依赖开关汇总成可读文字。
# 注意：message() 不会展开生成器表达式（$<...>），必须在 configure 期用 if() 拼字符串。
set(_deps "")
if(USE_GLFW)
    set(_deps "${_deps} GLFW(on)")
else()
    set(_deps "${_deps} GLFW(off)")
endif()
if(USE_GLM)
    set(_deps "${_deps} GLM(on)")
else()
    set(_deps "${_deps} GLM(off)")
endif()
if(USE_VULKAN)
    set(_deps "${_deps} Vulkan(on)")
else()
    set(_deps "${_deps} Vulkan(off)")
endif()
if(USE_SHADERS)
    set(_deps "${_deps} shaders(on, ${_spv_out})")
else()
    set(_deps "${_deps} shaders(off)")
endif()
message(STATUS "依赖        :${_deps}")
message(STATUS "输出        : ${CMAKE_BINARY_DIR}/bin/<Config>/${PROJ_NAME}.exe")

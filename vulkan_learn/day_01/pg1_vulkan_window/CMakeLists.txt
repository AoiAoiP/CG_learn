# ===========================================================================
# vulkan_learn/day_01/pg1_vulkan_window/CMakeLists.txt
#
# pg1: GLFW + GLM + Vulkan starter program — CMake build
#
# 第三方库位置（third_party/ 不做任何改动，也不安装）：
#   GLFW 3.5.1  头文件: <root>/third_party/include/GLFW
#               静态库: <root>/third_party/build-glfw/src/Release/glfw3.lib
#   GLM 1.0.3   头文件: <root>/third_party/include/glm      （header-only）
#   Vulkan      由 CMake 自带的 FindVulkan 模块经 VULKAN_SDK 定位
# ===========================================================================
cmake_minimum_required(VERSION 3.20)
project(pg1_vulkan_window LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# 定位 vulkan_learn 根目录（即含 third_party/ 的那一层）
# 本文件在 <root>/day_01/pg1_vulkan_window/ 下，故向上两级。
# 可用 -DVL_ROOT=<path> 覆盖。
# ---------------------------------------------------------------------------
if(NOT DEFINED VL_ROOT)
    get_filename_component(VL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
endif()

set(VL_INCLUDE_DIR "${VL_ROOT}/third_party/include")
set(VL_GLFW_LIB    "${VL_ROOT}/third_party/build-glfw/src/Release/glfw3.lib")

if(NOT EXISTS "${VL_GLFW_LIB}")
    message(FATAL_ERROR
        "找不到 GLFW 静态库: ${VL_GLFW_LIB}\n"
        "请先用 CMake 编译 GLFW（见 shared/README_environment.md 第 4 节坑 1）。")
endif()
if(NOT EXISTS "${VL_INCLUDE_DIR}/GLFW/glfw3.h")
    message(FATAL_ERROR "找不到 GLFW 头文件: ${VL_INCLUDE_DIR}/GLFW/glfw3.h")
endif()
if(NOT EXISTS "${VL_INCLUDE_DIR}/glm/glm.hpp")
    message(FATAL_ERROR "找不到 GLM 头文件: ${VL_INCLUDE_DIR}/glm/glm.hpp")
endif()

# ---------------------------------------------------------------------------
# 依赖
# ---------------------------------------------------------------------------
# GLFW：third_party/build-glfw 里只生成了 glfw3Config.cmake，
# 但缺少它内部 include 的 glfw3Targets.cmake（子项目模式不做 install/export），
# 所以 find_package(glfw3) 不可用 —— 直接把现有的静态库声明为 IMPORTED 目标。
add_library(glfw STATIC IMPORTED)
set_target_properties(glfw PROPERTIES
    IMPORTED_LOCATION "${VL_GLFW_LIB}"
    INTERFACE_INCLUDE_DIRECTORIES "${VL_INCLUDE_DIR}"
)
# GLFW 静态库自身依赖的 Windows 系统库
set_property(TARGET glfw APPEND PROPERTY
    INTERFACE_LINK_LIBRARIES "user32;gdi32;shell32")

# Vulkan：CMake 自带 FindVulkan 模块，通过 VULKAN_SDK 环境变量定位 SDK
find_package(Vulkan REQUIRED)

# ---------------------------------------------------------------------------
# 目标
# ---------------------------------------------------------------------------
add_executable(pg1_vulkan_window src/vulkan_window.cpp)

target_include_directories(pg1_vulkan_window PRIVATE "${VL_INCLUDE_DIR}")

# /MD 必须与编译 GLFW 时使用的 CRT 一致（见 shared/README_environment.md 坑 2）
if(MSVC)
    target_compile_options(pg1_vulkan_window PRIVATE /W3 /utf-8)
    set_property(TARGET pg1_vulkan_window PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    target_compile_definitions(pg1_vulkan_window PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

target_link_libraries(pg1_vulkan_window PRIVATE glfw Vulkan::Vulkan)

# 产物放到 build/<Config>/ 下，与 Visual Studio 生成器的默认布局一致
set_target_properties(pg1_vulkan_window PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

message(STATUS "VL_ROOT      : ${VL_ROOT}")
message(STATUS "GLFW lib     : ${VL_GLFW_LIB}")
message(STATUS "include dir  : ${VL_INCLUDE_DIR}")
message(STATUS "Vulkan lib   : ${Vulkan_LIBRARIES}")

@echo off
REM ===========================================================================
REM vulkan_learn/day_01/pg2_hello_window/build.bat
REM
REM pg2: raw Win32 + Vulkan window (no GLFW), clear + present.
REM Shows what GLFW hides: VkSurfaceKHR creation, swapchain, fences and
REM semaphores, the acquire/record/present frame cycle.
REM
REM Usage (run from anywhere):
REM     build.bat            build Release
REM     build.bat debug      build Debug
REM     build.bat clean      remove the build directory
REM
REM NOTE: shared\ and third_party\ live two levels up (vulkan_learn\), so the
REM path to the shared build script is ..\..\shared -- not ..\shared.
REM
REM This script is a CONVENIENCE WRAPPER. The manual PowerShell equivalent is
REM documented in day_01.md so the compile steps stay visible while learning.
REM ===========================================================================
setlocal

set EXE_NAME=hello_window
set CFG_NAME=Release
set CFG_FLAGS=/O2 /DNDEBUG /Zi
if /I "%~1"=="debug" (
    set CFG_NAME=Debug
    set CFG_FLAGS=/Zi /Od /RTC1
)

call "%~dp0..\..\shared\build_common.bat" "%~dp0" %~1
exit /b %errorlevel%

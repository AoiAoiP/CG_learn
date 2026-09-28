@echo off
REM ===========================================================================
REM vulkan_learn/shared/build_common.bat
REM
REM Shared build entry point for every day_X project. THIS FILE MUST STAY PURE
REM ASCII (cmd.exe parses .bat in the OEM codepage; non-ASCII bytes get shredded
REM across line boundaries and turn into garbage commands).
REM
REM Do NOT run this directly. Each day_X project calls it with one argument:
REM     call "%~dp0..\..\shared\build_common.bat" <ProjectDir>
REM where <ProjectDir> is the folder containing the project's build.bat.
REM
REM What it does:
REM   1. locates vulkan_learn\ (walking up from the project dir until it finds
REM      third_party) so projects work at any nesting depth;
REM   2. loads the MSVC x64 environment via vcvars64.bat;
REM   3. compiles every src\*.cpp into build\<name>.exe.
REM
REM Shared, read-only inputs (never modified by a build):
REM   <root>\third_party\include    GLFW + GLM headers
REM   <root>\third_party\lib        glfw3.lib
REM   %VULKAN_SDK%                  Vulkan headers + vulkan-1.lib
REM ===========================================================================
setlocal EnableDelayedExpansion

if "%~1"=="" (
    echo [build] usage: call shared\build_common.bat ^<ProjectDir^>
    exit /b 1
)
set PROJDIR=%~1
if not exist "%PROJDIR%" (
    echo [build] project dir not found: %PROJDIR%
    exit /b 1
)
REM normalize the path (strips trailing \ and \. so no stray folders get created)
for %%P in ("%PROJDIR%") do set PROJDIR=%%~fP

REM ---- 1. locate the vulkan_learn root (the folder holding third_party) ----
set CUR=%PROJDIR%
set ROOT=
for /L %%i in (1,1,8) do (
    if exist "!CUR!\third_party\include\GLFW\glfw3.h" (
        set ROOT=!CUR!
    ) else (
        for %%P in ("!CUR!\..") do set CUR=%%~fP
    )
)
if not defined ROOT (
    echo [build] could not locate a 'third_party' folder above %PROJDIR%
    exit /b 1
)

REM ---- 2. configuration (overridable by the caller before this runs) ----
if not defined CFG_NAME set CFG_NAME=Release
if not defined CFG_FLAGS set CFG_FLAGS=/O2 /DNDEBUG /Zi
set OUT=%PROJDIR%\build

if /I "%~2"=="clean" (
    if exist "%OUT%" rmdir /s /q "%OUT%"
    echo [build] cleaned %OUT%
    exit /b 0
)

REM ---- 3. sanity checks on the toolchain ----
if not defined VULKAN_SDK ( echo [build] VULKAN_SDK is not set & exit /b 1 )
if not exist "%ROOT%\third_party\include\GLFW\glfw3.h" ( echo [build] missing GLFW headers & exit /b 1 )
if not exist "%ROOT%\third_party\lib\glfw3.lib" ( echo [build] missing %ROOT%\third_party\lib\glfw3.lib & exit /b 1 )

set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%VSWHERE%" ( echo [build] vswhere.exe not found & exit /b 1 )
for /f "usebackq tokens=*" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSPATH=%%i
if not defined VSPATH ( echo [build] no VS C++ toolset found & exit /b 1 )
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 ( echo [build] vcvars64.bat failed & exit /b 1 )

REM ---- 4. build ----
echo [build] root       : %ROOT%
echo [build] project    : %PROJDIR%
echo [build] config     : %CFG_NAME%
echo [build] Vulkan SDK : %VULKAN_SDK%

if not exist "%OUT%" mkdir "%OUT%"
pushd "%OUT%"
cl /nologo /std:c++17 /EHsc /W3 /utf-8 /MD %CFG_FLAGS% ^
   /I"%PROJDIR%\src" /I"%ROOT%\third_party\include" /I"%VULKAN_SDK%\Include" ^
   "%PROJDIR%\src\*.cpp" ^
   /Fe:%EXE_NAME%.exe ^
   /link /LIBPATH:"%VULKAN_SDK%\Lib" ^
   "%ROOT%\third_party\lib\glfw3.lib" vulkan-1.lib ^
   user32.lib gdi32.lib shell32.lib
set CLERR=%errorlevel%
popd

if not "%CLERR%"=="0" (
    echo.
    echo [build] compile FAILED, exit code %CLERR%
    exit /b %CLERR%
)

echo.
echo [build] OK: %OUT%\%EXE_NAME%.exe
endlocal & exit /b 0

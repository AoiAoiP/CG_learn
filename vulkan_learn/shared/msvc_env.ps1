# ===========================================================================
# vulkan_learn/shared/msvc_env.ps1
#
# Load the Visual Studio x64 build environment into the CURRENT PowerShell session.
#
# Why this is needed: cl.exe / link.exe are not on PATH. The usual workflow is to
# open a "Developer Command Prompt", or run vcvars64.bat -- but a .bat file cannot
# modify the PowerShell process (it runs as a child process, so its changes are lost).
#
# How it works: let cmd.exe run vcvars64.bat and then dump its whole environment
# (cmd's `set` with no arguments does exactly that), then import every variable
# into the current PowerShell session. After that you can call cl / link directly.
#
# Usage (dot-source it -- note the leading dot; that is what makes the variable
# changes land in YOUR session instead of a child scope):
#     . D:\...\vulkan_learn\shared\msvc_env.ps1
#     cl /?                      # now available
#
# NOTE: all messages here are deliberately ASCII. cmd.exe emits text in the OEM
# codepage (GBK on a Chinese Windows); mixing that with PowerShell's encoder
# corrupts output and, worse, corrupts the parsing of the environment dump itself.
# ===========================================================================

$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
if (-not (Test-Path $vswhere)) {
    throw "[msvc_env] vswhere.exe not found - is Visual Studio 2022 installed?"
}

$vsPath = & $vswhere -latest -products * `
    -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
    -property installationPath
if (-not $vsPath) { throw "[msvc_env] no VS installation with the C++ toolset found" }

$vcvars = Join-Path $vsPath 'VC\Auxiliary\Build\vcvars64.bat'
if (-not (Test-Path $vcvars)) { throw "[msvc_env] vcvars64.bat not found at $vcvars" }

# Ask cmd to run vcvars64.bat, then dump the resulting environment.
$envDump = & $env:SystemRoot\System32\cmd.exe /c "`"$vcvars`" >nul 2>&1 && set"

$count = 0
foreach ($line in $envDump) {
    if ($line -match '^([^=]+)=(.*)$') {
        $name = $matches[1]
        $value = $matches[2]
        if ($name -and $value) {
            Set-Item -Path "Env:$name" -Value $value
            if ($name -ieq 'Path') {
                # MSVC's link.exe must win over any other link.exe on PATH
                # (this machine has devkitPro's msys2 link.exe on PATH).
                $env:Path = $value
            }
            $count++
        }
    }
}

Write-Host "[msvc_env] VS          : $vsPath"
Write-Host "[msvc_env] imported    : $count environment variables"
Write-Host "[msvc_env] cl.exe      : $((Get-Command cl.exe -ErrorAction SilentlyContinue).Source)"
Write-Host "[msvc_env] link.exe    : $((Get-Command link.exe -ErrorAction SilentlyContinue).Source)"
Write-Host "[msvc_env] VULKAN_SDK  : $env:VULKAN_SDK"

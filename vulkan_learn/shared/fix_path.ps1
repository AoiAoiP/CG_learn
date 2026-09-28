# ===========================================================================
# vulkan_learn/shared/fix_path.ps1
#
# 目的：把 PATH 上「被 devkitPro/msys2 抢占的同名工具」归还给各自的 Windows 原生版本。
#
# 为什么不直接删 C:\devkitPro\msys2\usr\bin：
#   该目录里还有 make.exe，而本机没有别的 make 实现 —— 删掉就没有 make 了。
#   所以本脚本不删目录，只在 PATH 更靠前的位置插入原生工具目录，
#   让 cmd 的工具查找先命中原生版本。
#
# 解决的问题（实测）：
#   1. cmake  -> 命中 msys2 版，它不支持 -G "Visual Studio 17 2022"，且自身已损坏
#                （fatal error - couldn't create signal pipe）
#   2. cmd    -> 命中 msys2 版，绝对路径 + 管道会报 The system cannot find the path
#   3. link   -> 命中 msys2 版，会抢走 MSVC 的链接器
#   4. git    -> 命中 msys2 版（影响较小）
#
# 用法：
#     . D:\...\vulkan_learn\shared\fix_path.ps1              # 应用（当前会话 + 永久用户变量）
#     . D:\...\vulkan_learn\shared\fix_path.ps1 -WhatIfOnly   # 只看会做什么，不改
#     . D:\...\vulkan_learn\shared\fix_path.ps1 -Revert       # 回滚到备份
#
# 注意：永久变量的改动需要**重开 PowerShell 窗口**才完全生效；
#       但本脚本会同时修改当前会话的 PATH，所以本窗口立刻可用。
# ===========================================================================
param(
    [switch]$Revert,
    [switch]$WhatIfOnly
)

$msysDir = 'C:\devkitPro\msys2\usr\bin'
$nativeDirs = @(
    "$env:SystemRoot\System32",
    "$env:SystemRoot",
    "$env:SystemRoot\System32\Wbem",
    'C:\Program Files\CMake\bin',
    'C:\Program Files\Git\bin',
    'C:\Program Files\Git\usr\bin'
)

$backupFile = Join-Path $env:USERPROFILE 'path_backup_vulkan_learn.txt'
$scope = 'User'

function Get-UserPath { [Environment]::GetEnvironmentVariable('Path', $scope) }

# ---------------------------------------------------------------- 回滚
if ($Revert) {
    if (-not (Test-Path $backupFile)) {
        Write-Host "[fix_path] 找不到备份文件: $backupFile" -ForegroundColor Red
        return
    }
    $old = (Get-Content $backupFile -Raw).Trim()
    [Environment]::SetEnvironmentVariable('Path', $old, $scope)
    $env:Path = $old
    Write-Host "[fix_path] 已回滚用户 PATH（来自 $backupFile）" -ForegroundColor Green
    return
}

# ---------------------------------------------------------------- 分析
$userPath = Get-UserPath
if (-not $userPath) { $userPath = '' }

Write-Host "[fix_path] 当前用户 PATH 条目数: $(($userPath -split ';' | Where-Object {$_}).Count)"
Write-Host "[fix_path] msys2 目录是否在用户 PATH 中: $($userPath -match [regex]::Escape($msysDir))"

# 目标顺序：原生目录在前，其余保持原顺序且去重
$entries = $userPath -split ';' | Where-Object { $_ -and $_.Trim() -ne '' } | ForEach-Object { $_.Trim() }

$missingNative = $nativeDirs | Where-Object { -not ($entries | Where-Object { $_ -ieq $_ }) -and (Test-Path $_) }
$rest = $entries | Where-Object { $n = $_; -not ($nativeDirs | Where-Object { $_ -ieq $n }) }
$newEntries = @($nativeDirs | Where-Object { Test-Path $_ }) + $rest

# 去重（保留首次出现）
$seen = @{}
$dedup = foreach ($e in $newEntries) {
    $k = $e.ToLower()
    if (-not $seen.ContainsKey($k)) { $seen[$k] = $true; $e }
}
$newPath = ($dedup -join ';')

Write-Host ""
Write-Host "[fix_path] 将把以下目录提到最前（若存在）:"
$nativeDirs | Where-Object { Test-Path $_ } | ForEach-Object { Write-Host "    $_" }
Write-Host "[fix_path] msys2 目录保留在原位置（不会删除，make.exe 仍可用）"

if ($WhatIfOnly) {
    Write-Host ""
    Write-Host "[fix_path] -WhatIfOnly：未做任何修改。" -ForegroundColor Yellow
    Write-Host "[fix_path] 新的用户 PATH 将是:"
    Write-Host $newPath -ForegroundColor DarkGray
    return
}

# ---------------------------------------------------------------- 备份 + 应用
Set-Content -Path $backupFile -Value $userPath -Encoding UTF8
Write-Host ""
Write-Host "[fix_path] 已备份原用户 PATH 到: $backupFile"

[Environment]::SetEnvironmentVariable('Path', $newPath, $scope)
$env:Path = $newPath        # 当前会话立即生效

Write-Host "[fix_path] 已更新用户 PATH（永久 + 当前会话）" -ForegroundColor Green
Write-Host ""
Write-Host "[fix_path] 验证："
foreach ($t in @('cmake','cmd','link','git','make')) {
    $c = Get-Command $t -ErrorAction SilentlyContinue
    if ($c) {
        $tag = if ($c.Source -like '*msys2*') { 'msys2' } else { 'native' }
        Write-Host ("    {0,-6} [{1,-6}] {2}" -f $t, $tag, $c.Source)
    } else {
        Write-Host ("    {0,-6} [未找到]" -f $t)
    }
}
Write-Host ""
Write-Host "[fix_path] 提示：永久生效需重开 PowerShell 窗口（本窗口已可用）。回滚：-Revert" -ForegroundColor Yellow

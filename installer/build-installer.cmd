@echo off
rem ===========================================================================
rem  build-installer.cmd  —  Build-Installer.ps1 的便捷入口
rem
rem  用法 1（命令行 / 拖拽）：
rem      build-installer.cmd "D:\Out\MyApp"                       只有一个文件夹
rem      build-installer.cmd "D:\Out\MyApp" "D:\icon\app.ico"     文件夹 + 图标
rem      build-installer.cmd "D:\Out\MyApp" "D:\icon\app.ico" 1.2.0   再指定版本号
rem      也可以把文件夹（和 ico）直接拖到这个 .cmd 上。
rem
rem  用法 2（直接双击）：
rem      依次提示输入文件夹、图标（可留空）、版本号（可留空自动读 exe）。
rem
rem  想传其他参数（如 -PreserveUserData），直接调用 PowerShell 脚本：
rem      powershell -File Build-Installer.ps1 -SourceDir ... -PreserveUserData
rem ===========================================================================
setlocal
chcp 65001 >nul 2>&1
set "PS1=%~dp0Build-Installer.ps1"

if not exist "%PS1%" (
    echo [错误] 找不到 Build-Installer.ps1，请与本文件放在同一目录。
    pause
    exit /b 1
)

if not "%~1"=="" goto haveargs

rem ---------------------------------------------------------------- 交互模式
echo.
echo   ============================
echo    安装包构建助手
echo   ============================
echo.
:ask_src
set "SRC="
set /p "SRC=部署文件夹路径（含 exe 的目录，可直接把文件夹拖进这个窗口）："
if not defined SRC goto ask_src
set "SRC=%SRC:"=%"

set "ICO="
set /p "ICO=图标 .ico 路径（可留空，自动查找）："
if defined ICO set "ICO=%ICO:"=%"

set "VER="
set /p "VER=版本号（可留空，自动读取 exe 版本资源，读不到则用 1.0.0）："

set ARGS=-SourceDir "%SRC%"
if defined ICO set ARGS=%ARGS% -IconFile "%ICO%"
if defined VER set ARGS=%ARGS% -AppVersion "%VER%"

echo.
echo   开始构建...
echo.
powershell -NoProfile -ExecutionPolicy Bypass -File "%PS1%" %ARGS%
goto done

rem ---------------------------------------------------------------- 参数模式
:haveargs
set ARGS=-SourceDir "%~1"
if not "%~2"=="" set ARGS=%ARGS% -IconFile "%~2"
if not "%~3"=="" set ARGS=%ARGS% -AppVersion "%~3"
powershell -NoProfile -ExecutionPolicy Bypass -File "%PS1%" %ARGS%
set "RC=%ERRORLEVEL%"
if not "%RC%"=="0" (
    echo.
    echo [失败] 退出码 %RC%，详见上面的输出。
    pause
)
exit /b %RC%

rem ---------------------------------------------------------------- 交互模式收尾
:done
set "RC=%ERRORLEVEL%"
echo.
if not "%RC%"=="0" (
    echo [失败] 退出码 %RC%，详见上面的输出。
) else (
    echo [完成] 安装包在 installer\Output 目录下。
)
pause
exit /b %RC%

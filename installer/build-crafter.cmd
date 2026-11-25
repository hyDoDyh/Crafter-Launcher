@echo off
rem ===========================================================================
rem  build-crafter.cmd  —  一键重新打包 Crafter（本项目的预设参数）
rem
rem  改了版本号就改下面的 VER；换了 Qt 版本/构建目录就改 SRC。
rem  需要更灵活时直接用 build-installer.cmd 或 Build-Installer.ps1。
rem ===========================================================================
setlocal
chcp 65001 >nul 2>&1

set "VER=0.2.0"
set "SRC=%~dp0..\build\Desktop_Qt_6_10_1_MinGW_64_bit-Release\beta %VER%"
set "ICO=%~dp0crafter.ico"

if not exist "%SRC%\" (
    echo [错误] 找不到构建输出目录：
    echo        %SRC%
    echo        请先 Qt Creator 以 Release 构建，或把实际的部署文件夹路径填进本文件的 SRC。
    pause
    exit /b 1
)

echo.
echo   构建 Crafter %VER% 安装包...
echo.
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Build-Installer.ps1" ^
    -SourceDir "%SRC%" ^
    -IconFile "%ICO%" ^
    -AppName "Crafter" ^
    -AppVersion "%VER%" ^
    -AppId "94E22B54-079B-4B75-9F51-CBC3791F90F8" ^
    -AppUrl "https://github.com/hyDoDyh/Crafter-Launcher" ^
    -OutputName "Crafter-Setup-%VER%"

set "RC=%ERRORLEVEL%"
echo.
if not "%RC%"=="0" (
    echo [失败] 退出码 %RC%
) else (
    echo [完成] installer\Output\Crafter-Setup-%VER%.exe
)
pause
exit /b %RC%

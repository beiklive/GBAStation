@echo off
:: ============================================================
:: Windows 编译脚本（MinGW / MSYS2 环境）
:: 编译后产物：
::   build_windows\BKStation.exe          —— 主程序
::   build_windows\mgba_libretro.dll     —— libretro 核心
:: 使用方式：在 MSYS2 MinGW64 终端或已配置好 MinGW 环境的
::   cmd / PowerShell 中直接运行本脚本即可。
:: ============================================================
setlocal

:: 并行编译线程数（默认使用所有逻辑核心）
set JOBS=%NUMBER_OF_PROCESSORS%

:: 构建目录
set BUILD_DIR=build_windows

echo [1/3] 创建构建目录 %BUILD_DIR% ...
if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cd %BUILD_DIR%

echo [2/3] 运行 CMake 配置（桌面平台 / Release）...
cmake .. ^
    -G Ninja ^
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5 ^
    -DPLATFORM_DESKTOP=ON ^
    -DCMAKE_BUILD_TYPE=Release
if %ERRORLEVEL% neq 0 (
    echo [错误] CMake 配置失败，退出。
    cd ..
    exit /b 1
)

echo [3/3] 开始编译（并行线程：%JOBS%）...
cmake --build . --config Release -j %JOBS%
if %ERRORLEVEL% neq 0 (
    echo [错误] 编译失败，退出。
    cd ..
    exit /b 1
)
cd ..
echo.
echo [完成] 产物目录：%BUILD_DIR%\
endlocal


@echo off
setlocal enabledelayedexpansion

:: Initialize variables
set "BUILD_TYPE=Release"
set "GENERATOR="
set "BUILD_DIR=build"
set "COMPILER="

:: Parse command line arguments
:parse_args
if "%1" == "" goto :done_parsing
if /i "%1" == "--debug" set "BUILD_TYPE=Debug"
if /i "%1" == "--release" set "BUILD_TYPE=Release"
if /i "%1" == "--mingw" set "COMPILER=mingw"
if /i "%1" == "--msvc" set "COMPILER=msvc"
if /i "%1" == "--help" goto :show_help
shift
goto :parse_args
:done_parsing

:: Show help if requested
:show_help
if "%1" == "--help" (
    echo Usage: build_windows.bat [options]
    echo Options:
    echo   --debug     Build in debug mode
    echo   --release   Build in release mode ^(default^)
    echo   --mingw     Force MinGW compiler
    echo   --msvc      Force MSVC compiler
    echo   --help      Show this help message
    exit /b 0
)

:: Check for required tools
where cmake >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake not found. Please install CMake and add it to your PATH.
    exit /b 1
)

where git >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo Error: Git not found. Please install Git and add it to your PATH.
    exit /b 1
)

:: Setup vcpkg if not already present
if not exist "vcpkg" (
    echo Setting up vcpkg...
    git clone https://github.com/Microsoft/vcpkg.git
    if %ERRORLEVEL% NEQ 0 (
        echo Error: Failed to clone vcpkg repository.
        exit /b 1
    )
    call vcpkg\bootstrap-vcpkg.bat
    if %ERRORLEVEL% NEQ 0 (
        echo Error: Failed to bootstrap vcpkg.
        exit /b 1
    )
)

:: Set VCPKG_ROOT environment variable
set "VCPKG_ROOT=%CD%\vcpkg"

:: Auto-detect available compilers if not specified
if "%COMPILER%" == "" (
    where cl >nul 2>&1
    if %ERRORLEVEL% EQU 0 (
        set "COMPILER=msvc"
    ) else (
        where gcc >nul 2>&1
        if %ERRORLEVEL% EQU 0 (
            set "COMPILER=mingw"
        ) else (
            echo Error: No supported compiler found. Please install Visual Studio or MinGW.
            exit /b 1
        )
    )
)

:: Set CMake generator based on compiler
if "%COMPILER%" == "msvc" (
    set "GENERATOR=Visual Studio 17 2022"
    echo Using Visual Studio generator...
) else if "%COMPILER%" == "mingw" (
    set "GENERATOR=MinGW Makefiles"
    echo Using MinGW Makefiles generator...
)

:: Create build directory if it doesn't exist
if not exist "%BUILD_DIR%" mkdir "%BUILD_DIR%"

:: Configure CMake
echo Configuring CMake with %GENERATOR% generator...
cmake -B "%BUILD_DIR%" -S . ^
      -G "%GENERATOR%" ^
      -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
      -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"

if %ERRORLEVEL% NEQ 0 (
    echo Error: CMake configuration failed.
    exit /b 1
)

:: Build the project
echo Building project in %BUILD_TYPE% mode...
cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% -j %NUMBER_OF_PROCESSORS%

if %ERRORLEVEL% NEQ 0 (
    echo Error: Build failed.
    exit /b 1
)

echo.
echo Build completed successfully!
echo Executable location: %BUILD_DIR%\%BUILD_TYPE%\GameEnginePhysx.exe

:: Copy required DLLs (if using MSVC)
if "%COMPILER%" == "msvc" (
    echo Copying required DLLs...
    if exist "%BUILD_DIR%\%BUILD_TYPE%\GameEnginePhysx.exe" (
        xcopy /y "%VCPKG_ROOT%\installed\x64-windows\bin\*.dll" "%BUILD_DIR%\%BUILD_TYPE%\"
    )
)

endlocal
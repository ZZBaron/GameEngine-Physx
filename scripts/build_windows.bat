@echo off
setlocal enabledelayedexpansion

:: Initialize variables
set "BUILD_TYPE=Release"
set "GENERATOR="
set "COMPILER_NAME="
set "FOUND_COMPILER="

:: Parse command-line arguments
:parse_args
if "%1" == "" goto :done_parsing
if /i "%1" == "--debug" set "BUILD_TYPE=Debug"
if /i "%1" == "--release" set "BUILD_TYPE=Release"
if /i "%1" == "--help" goto :show_help
shift
goto :parse_args

:done_parsing
goto :check_compilers

:show_help
echo Usage: build_windows.bat [options]
echo Options:
echo   --debug     Build in debug mode
echo   --release   Build in release mode (default)
echo   --help      Show this help message
exit /b 0

:check_requirements
:: Check for required tools
where cmake >nul 2>&1 || (
    echo Error: CMake not found. Please install CMake and add it to your PATH.
    exit /b 1
)

where git >nul 2>&1 || (
    echo Error: Git not found. Please install Git and add it to your PATH.
    exit /b 1
)
goto :eof

:check_compilers
call :check_requirements

:: 1. Check for Visual Studio with C++ Build Tools
echo Checking for Visual Studio with C++ tools...
where cl >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    :: Verify C++ tools by trying to compile
    echo checking ^#include ^<iostream^> > test.cpp
    echo int main^(^) { return 0; } >> test.cpp
    cl /nologo test.cpp >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        echo Found Visual Studio with C++ tools
        set "FOUND_COMPILER=msvc"
        set "GENERATOR=Visual Studio 17 2022"
        set "COMPILER_NAME=Visual Studio"
        del test.* >nul 2>&1
        goto :setup_build
    )
    del test.* >nul 2>&1
)

:: 2. Check for Clang (LLVM)
echo Checking for Clang...
where clang++ >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Found Clang compiler
    set "FOUND_COMPILER=clang"
    where ninja >nul 2>&1
    if !ERRORLEVEL! EQU 0 (
        set "GENERATOR=Ninja"
    ) else (
        set "GENERATOR=Unix Makefiles"
    )
    set "COMPILER_NAME=Clang"
    set "CMAKE_CXX_COMPILER=clang++"
    goto :setup_build
)

:: 3. Check for MinGW-w64 in standard locations
echo Checking for MinGW-w64...
if exist "C:\msys64\mingw64\bin\g++.exe" (
    echo Found MinGW-w64 in MSYS2
    set "PATH=C:\msys64\mingw64\bin;%PATH%"
    set "FOUND_COMPILER=mingw"
    set "GENERATOR=MinGW Makefiles"
    set "COMPILER_NAME=MinGW-w64"
    set "CMAKE_CXX_COMPILER=g++"
    goto :setup_build
)

:: 4. Check standalone MinGW installation
if exist "C:\MinGW\bin\g++.exe" (
    echo Found standalone MinGW
    set "PATH=C:\MinGW\bin;%PATH%"
    set "FOUND_COMPILER=mingw"
    set "GENERATOR=MinGW Makefiles"
    set "COMPILER_NAME=MinGW"
    set "CMAKE_CXX_COMPILER=g++"
    goto :setup_build
)

:: 5. Check for standalone g++
echo Checking for standalone G++...
where g++ >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo Found standalone G++
    set "FOUND_COMPILER=gnu"
    set "GENERATOR=Unix Makefiles"
    set "COMPILER_NAME=GNU G++"
    set "CMAKE_CXX_COMPILER=g++"
    goto :setup_build
)

:: If no compiler found, exit
echo.
echo Error: No C++ compiler found. Please install one of the following:
echo 1. Visual Studio 2022 with C++ build tools ^(recommended^)
echo 2. LLVM/Clang
echo 3. MinGW-w64 ^(via MSYS2^)
echo 4. Standalone MinGW
echo 5. GNU G++
exit /b 1

:setup_build
:: Set up vcpkg if needed
if not exist "vcpkg" (
    echo.
    echo Setting up vcpkg...
    git clone https://github.com/Microsoft/vcpkg.git
    if errorlevel 1 (
        echo Error: Failed to clone vcpkg repository
        exit /b 1
    )
    call vcpkg\bootstrap-vcpkg.bat
    if errorlevel 1 (
        echo Error: Failed to bootstrap vcpkg
        exit /b 1
    )
)

:: Set VCPKG_ROOT to local vcpkg
set "VCPKG_ROOT=%CD%\vcpkg"

echo.
echo Using %COMPILER_NAME% with %GENERATOR%
echo Build type: %BUILD_TYPE%
echo.

:: Configure CMake based on compiler
if "%FOUND_COMPILER%"=="msvc" (
    cmake -B build -G "%GENERATOR%" -A x64 ^
          -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
          -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake"
) else (
    cmake -B build -G "%GENERATOR%" ^
          -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
          -DCMAKE_TOOLCHAIN_FILE="%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake" ^
          -DCMAKE_CXX_COMPILER="%CMAKE_CXX_COMPILER%"
)

if errorlevel 1 (
    echo.
    echo Error: CMake configuration failed
    exit /b 1
)

:: Build the project
echo.
echo Building project...
cmake --build build --config %BUILD_TYPE% -j %NUMBER_OF_PROCESSORS%

if errorlevel 1 (
    echo.
    echo Error: Build failed
    exit /b 1
)

echo.
echo Build completed successfully!
if "%FOUND_COMPILER%"=="msvc" (
    echo Executable location: build\%BUILD_TYPE%\GameEnginePhysx.exe
) else (
    echo Executable location: build\GameEnginePhysx.exe
)

endlocal
exit /b 0
@echo off
setlocal enabledelayedexpansion

:: Initialize variables
set "BUILD_TYPE=Release"
set "CUSTOM_COMPILER="
set "COMPILER_PATH="

:: Parse command-line arguments
:parse_args
if "%~1"=="" goto :main
if /i "%~1"=="--debug" set "BUILD_TYPE=Debug" & shift & goto :parse_args
if /i "%~1"=="--release" set "BUILD_TYPE=Release" & shift & goto :parse_args
if /i "%~1"=="--compiler" set "CUSTOM_COMPILER=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="--compiler-path" set "COMPILER_PATH=%~2" & shift & shift & goto :parse_args
if /i "%~1"=="--help" goto :help
shift
goto :parse_args

:help
echo Usage: %~nx0 [options]
echo Options:
echo   --debug           Build in debug mode
echo   --release         Build in release mode (default)
echo   --compiler        Specify compiler (msvc, clang, g++)
echo   --compiler-path   Specify path to compiler
echo   --help           Show this help message
exit /b 0

:main
:: Check for CMake
where cmake >nul 2>&1 || (
    echo Error: CMake not found. Please install CMake and add it to your PATH.
    exit /b 1
)

:: If compiler path is specified, add it to PATH
if not "%COMPILER_PATH%"=="" (
    set "PATH=%COMPILER_PATH%;%PATH%"
)

:: If custom compiler specified, use it
if not "%CUSTOM_COMPILER%"=="" (
    call :setup_custom_compiler
    if !ERRORLEVEL! neq 0 exit /b 1
) else (
    call :detect_compiler
    if !ERRORLEVEL! neq 0 exit /b 1
)

:: Create build directory
if not exist "build" mkdir "build"

:: Configure and build
echo.
echo Configuring CMake with detected settings...
echo Build type: %BUILD_TYPE%
echo Generator: %CMAKE_GENERATOR%
if not "%CMAKE_CXX_COMPILER%"=="" echo Compiler: %CMAKE_CXX_COMPILER%
echo.

:: Run CMake configure
pushd build
if not "%CMAKE_CXX_COMPILER%"=="" (
    cmake .. -G "%CMAKE_GENERATOR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_CXX_COMPILER="%CMAKE_CXX_COMPILER%"
) else (
    cmake .. -G "%CMAKE_GENERATOR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
)

if errorlevel 1 (
    echo CMake configuration failed
    exit /b 1
)

:: Build
cmake --build . --config %BUILD_TYPE%
if errorlevel 1 (
    echo Build failed
    exit /b 1
)

popd
echo.
echo Build completed successfully!
exit /b 0

:detect_compiler
:: Check for MSVC
where cl.exe >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo Found MSVC compiler
    set "CMAKE_GENERATOR=Visual Studio 17 2022"
    exit /b 0
)

:: Check for Clang
where clang++.exe >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo Found Clang compiler
    set "CMAKE_GENERATOR=MinGW Makefiles"
    set "CMAKE_CXX_COMPILER=clang++"
    exit /b 0
)

:: Check for G++
where g++.exe >nul 2>&1
if %ERRORLEVEL% equ 0 (
    echo Found G++ compiler
    set "CMAKE_GENERATOR=MinGW Makefiles"
    set "CMAKE_CXX_COMPILER=g++"
    exit /b 0
)

echo No C++ compiler found. Please install one of the following:
echo - Visual Studio with C++ tools
echo - Clang
echo - G++
echo Or specify a compiler using --compiler option
exit /b 1

:setup_custom_compiler
if /i "%CUSTOM_COMPILER%"=="msvc" (
    where cl.exe >nul 2>&1 || (
        echo Error: MSVC compiler ^(cl.exe^) not found
        exit /b 1
    )
    set "CMAKE_GENERATOR=Visual Studio 17 2022"
) else if /i "%CUSTOM_COMPILER%"=="clang" (
    where clang++.exe >nul 2>&1 || (
        echo Error: Clang compiler ^(clang++.exe^) not found
        exit /b 1
    )
    set "CMAKE_GENERATOR=MinGW Makefiles"
    set "CMAKE_CXX_COMPILER=clang++"
) else if /i "%CUSTOM_COMPILER%"=="g++" (
    where g++.exe >nul 2>&1 || (
        echo Error: G++ compiler ^(g++.exe^) not found
        exit /b 1
    )
    set "CMAKE_GENERATOR=MinGW Makefiles"
    set "CMAKE_CXX_COMPILER=g++"
) else (
    echo Error: Unsupported compiler specified: %CUSTOM_COMPILER%
    echo Supported compilers: msvc, clang, g++
    exit /b 1
)
exit /b 0
# CMakeList.txt : CMake project for GameEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("GameEngine Physx")  

# Specify the C++ standard globally
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Handle vcpkg integration more robustly
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT})
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake"
            CACHE STRING "Vcpkg toolchain file")
    endif()
endif()



find_package(OpenGL REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(glew REQUIRED)
find_package(glm REQUIRED)
find_package(Freetype REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)

# Try different possible PhysX package names
find_package(unofficial-omniverse-physx-sdk CONFIG QUIET)
if(NOT unofficial-omniverse-physx-sdk_FOUND)
    find_package(PhysX CONFIG REQUIRED)
endif()


# Define source files
set(IMGUI_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vender/imgui")
set(IMGUI_SOURCES
    "${IMGUI_DIR}/imgui.cpp"
    "${IMGUI_DIR}/imgui_demo.cpp"
    "${IMGUI_DIR}/imgui_draw.cpp"
    "${IMGUI_DIR}/imgui_tables.cpp"
    "${IMGUI_DIR}/imgui_widgets.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_glfw.cpp"
    "${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp"
)

# Group source files
set(ENGINE_SOURCES
    "GameEngine.cpp"
    "ui.cpp"
    "input.cpp"
    "camera.cpp"
    "font.cpp"
    "console.cpp"
    "debugRenderer.cpp"
    "blender.cpp"
    "stb_image_impl.cpp"
    "selection.cpp"
    "PhysXManager.cpp"
)

set(ENGINE_HEADERS
    "GameEngine.h"
    "ui.h"
    "input.h"
    "camera.h"
    "shape.h"
    "font.h"
    "animate.h"
    "console.h"
    "randomGen.h"
    "debugRenderer.h"
    "blender.h"
    "shader.h"
    "misc_funcs.h"
    "voxel.h"
    "AABB.h"
    "texture.h"
    "stb_image.h"
    "selection.h"
    "PhysXManager.h"
    "PhysXBody.h"
    "PhysXWorld.h"
    "PhysXSimulation.h"
)
# Create executable
add_executable(GameEnginePhysx 
    ${ENGINE_SOURCES}
    ${ENGINE_HEADERS}
    ${IMGUI_SOURCES}
)



# Include directories
target_include_directories(GameEnginePhysx PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

# Platform-specific configurations
if(MSVC)
    target_compile_options(GameEnginePhysx PRIVATE /W4)
else()
    target_compile_options(GameEnginePhysx PRIVATE -Wall -Wextra -Wpedantic)
endif()


# Link libraries
target_link_libraries(GameEnginePhysx PRIVATE
    OpenGL::GL
    glfw
    GLEW::GLEW
    glm::glm
    Freetype::Freetype
    Eigen3::Eigen
)

# Add PhysX library based on which version was found
if(unofficial-omniverse-physx-sdk_FOUND)
    target_link_libraries(GameEnginePhysx PRIVATE unofficial::omniverse-physx-sdk::sdk)
else()
    target_link_libraries(GameEnginePhysx PRIVATE PhysX::PhysX)
endif()



# TODO: Add tests and install targets if needed.

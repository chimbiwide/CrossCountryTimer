cmake_minimum_required(VERSION 3.22)
project(CrossCountryTimer LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
set(PLATFORM Desktop CACHE STRING "raylib platform")
set(OPENGL_VERSION OFF CACHE STRING "raylib OpenGL version")
set(USE_EXTERNAL_GLFW OFF CACHE STRING "Use system GLFW")
set(USE_AUDIO ON CACHE BOOL "raylib audio")
set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)

# raylib's root CMakeLists calls project() with no LANGUAGES, which
# enables C++. Use src/ so a C compiler is enough.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/third_party/raylib/cmake")
include(CompilerFlags)
add_subdirectory(third_party/raylib/src raylib)

add_library(pdfgen STATIC third_party/pdfgen/pdfgen.c)
target_include_directories(pdfgen PUBLIC third_party/pdfgen)
if(UNIX)
    target_link_libraries(pdfgen PUBLIC m)
endif()

add_executable(CrossCountryTimer 
    src/main.c
    src/time.c
)
target_include_directories(CrossCountryTimer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(CrossCountryTimer PRIVATE raylib pdfgen)

# whisper.cpp is C++ internally; the public API (whisper.h) is C.
option(CCT_WITH_WHISPER "Build and link whisper.cpp" ON)
if(CCT_WITH_WHISPER)
    enable_language(CXX)
    set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(WHISPER_BUILD_SERVER OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/whisper.cpp EXCLUDE_FROM_ALL)
    target_link_libraries(CrossCountryTimer PRIVATE whisper)
    target_compile_definitions(CrossCountryTimer PRIVATE CCT_HAS_WHISPER)
endif()

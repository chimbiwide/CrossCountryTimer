cmake_minimum_required(VERSION 3.22)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

if(CMAKE_HOST_WIN32 AND NOT CMAKE_GENERATOR MATCHES "MinGW Makefiles|Ninja|Visual Studio")
    message(FATAL_ERROR
        "On Windows, configure with the MinGW preset (gcc and mingw32-make on PATH):\n"
        "  cmake --preset windows\n"
        "  cmake --build --preset windows -j\n"
        "The generator '${CMAKE_GENERATOR}' needs the Visual Studio C compiler.")
endif()

if(UNIX AND NOT APPLE AND NOT DEFINED CMAKE_C_COMPILER AND NOT DEFINED ENV{CC})
    find_program(CCT_GCC16 gcc-16)
    find_program(CCT_GXX16 g++-16)
    if(CCT_GCC16 AND CCT_GXX16)
        set(CMAKE_C_COMPILER ${CCT_GCC16} CACHE FILEPATH "C compiler")
        set(CMAKE_CXX_COMPILER ${CCT_GXX16} CACHE FILEPATH "C++ compiler")
    endif()
endif()

project(CrossCountryTimer LANGUAGES C)

if(UNIX AND NOT APPLE AND CMAKE_C_COMPILER_ID STREQUAL "GNU"
   AND CMAKE_C_COMPILER_VERSION VERSION_LESS 16)
    message(FATAL_ERROR
        "#embed needs GCC 16, but ${CMAKE_C_COMPILER} is GCC ${CMAKE_C_COMPILER_VERSION}.\n"
        "Install gcc-16 or configure with -DCMAKE_C_COMPILER=<path to GCC 16>.")
endif()

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
# gamepad rumble needs raylib's SDL backend. PLATFORM is FORCEd so an older
# build directory switches over instead of keeping its cached GLFW setting.
option(CCT_WITH_SDL "Use raylib's SDL3 backend (needed for gamepad rumble)" ON)
if(CCT_WITH_SDL)
    set(PLATFORM SDL CACHE STRING "raylib platform" FORCE)
else()
    set(PLATFORM Desktop CACHE STRING "raylib platform" FORCE)
endif()
set(OPENGL_VERSION OFF CACHE STRING "raylib OpenGL version")
set(USE_EXTERNAL_GLFW OFF CACHE STRING "Use system GLFW")
set(USE_AUDIO ON CACHE BOOL "raylib audio")
# GLFW (CCT_WITH_SDL=OFF only): X11, no Wayland
if(UNIX AND NOT APPLE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
endif()

set(ZLIB_BUILD_SHARED OFF CACHE BOOL "Build zlib shared library" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "Build zlib static library" FORCE)
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "Build zlib tests" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "Install zlib" FORCE)
set(ZLIB_BUILD_MINIZIP ON CACHE BOOL "Build minizip" FORCE)
set(ZLIB_MINIZIP_BUILD_SHARED OFF CACHE BOOL "Build minizip shared library" FORCE)
set(ZLIB_MINIZIP_BUILD_STATIC ON CACHE BOOL "Build minizip static library" FORCE)
set(ZLIB_MINIZIP_BUILD_TESTING OFF CACHE BOOL "Build minizip tests" FORCE)
set(ZLIB_MINIZIP_INSTALL OFF CACHE BOOL "Install minizip" FORCE)
add_subdirectory(third_party/zlib zlib EXCLUDE_FROM_ALL)
set_target_properties(zlibstatic libminizipstatic PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)

set(EXPAT_SHARED_LIBS OFF)
set(EXPAT_BUILD_TOOLS OFF)
set(EXPAT_BUILD_EXAMPLES OFF)
set(EXPAT_BUILD_TESTS OFF)
set(EXPAT_BUILD_DOCS OFF)
set(EXPAT_BUILD_PKGCONFIG OFF)
set(EXPAT_BUILD_FUZZERS OFF)
set(EXPAT_OSSFUZZ_BUILD OFF)
set(EXPAT_ENABLE_INSTALL OFF)
add_subdirectory(third_party/expat/expat expat EXCLUDE_FROM_ALL)

if(NOT WIN32)
    find_package(Threads REQUIRED)
endif()

add_library(xlsxio_read STATIC
    third_party/xlsxio/lib/xlsxio_read.c
    third_party/xlsxio/lib/xlsxio_read_sharedstrings.c
)
add_library(xlsxio_write STATIC third_party/xlsxio/lib/xlsxio_write.c)
foreach(xlsxio_target IN ITEMS xlsxio_read xlsxio_write)
    target_compile_definitions(${xlsxio_target}
        PUBLIC BUILD_XLSXIO_STATIC
        PRIVATE BUILD_XLSXIO USE_MINIZIP
    )
    target_include_directories(${xlsxio_target}
        PUBLIC third_party/xlsxio/include
        PRIVATE third_party/xlsxio/lib third_party/zlib/contrib
    )
    set_target_properties(${xlsxio_target} PROPERTIES
        C_STANDARD 11
        C_STANDARD_REQUIRED ON
    )
endforeach()
target_link_libraries(xlsxio_read PUBLIC expat::expat MINIZIP::minizipstatic)
target_link_libraries(xlsxio_write PUBLIC MINIZIP::minizipstatic)
if(NOT WIN32)
    target_link_libraries(xlsxio_write PUBLIC Threads::Threads)
endif()

set(SDL_SHARED OFF CACHE BOOL "Build SDL shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL static library" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build SDL test library" FORCE)
set(SDL_INSTALL ON CACHE BOOL "SDL install rules" FORCE)
# X11 only; XScrnSaver and XTest would each need another -dev package
if(UNIX AND NOT APPLE)
    set(SDL_WAYLAND OFF CACHE BOOL "" FORCE)
    set(SDL_X11_XSCRNSAVER OFF CACHE BOOL "" FORCE)
    set(SDL_X11_XTEST OFF CACHE BOOL "" FORCE)
endif()
if(WIN32)
    enable_language(CXX)
endif()
add_subdirectory(third_party/SDL SDL EXCLUDE_FROM_ALL)

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
    src/timer.c
    src/font.c
    src/data.c
    src/xlsx.c
    src/pdf.c
    src/scoreboard.c
    src/transcribe.c
    src/gamepad.c
)
# no console window on Windows; app.rc is the Explorer icon
if(WIN32)
    set_target_properties(CrossCountryTimer PROPERTIES WIN32_EXECUTABLE TRUE)
    target_sources(CrossCountryTimer PRIVATE data/app.rc)
    # static MinGW runtime, so other PCs need no DLLs
    if(MINGW)
        target_link_options(CrossCountryTimer PRIVATE -static)
    endif()
endif()
target_include_directories(CrossCountryTimer PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)
# raygui as a system header, so its warnings stay out of the build
target_include_directories(CrossCountryTimer SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/third_party/raygui)
# warnings for our sources only
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(CrossCountryTimer PRIVATE -Wall -Wextra)
endif()
target_link_libraries(CrossCountryTimer PRIVATE raylib pdfgen xlsxio_read xlsxio_write)
target_link_libraries(CrossCountryTimer PRIVATE SDL3::SDL3)

option(CCT_WITH_WHISPER "Build and link whisper.cpp" ON)
if(CCT_WITH_WHISPER)
    enable_language(CXX)
    set(WHISPER_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(WHISPER_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(WHISPER_BUILD_SERVER OFF CACHE BOOL "" FORCE)
    # fixed AVX2 baseline instead of -march=native, so the exe runs on other PCs
    set(GGML_NATIVE OFF CACHE BOOL "" FORCE)
    foreach(ggml_isa IN ITEMS SSE42 AVX AVX2 BMI2 FMA F16C)
        set(GGML_${ggml_isa} ON CACHE BOOL "" FORCE)
    endforeach()
    # no OpenMP: other PCs lack libgomp
    set(GGML_OPENMP OFF CACHE BOOL "" FORCE)
    add_subdirectory(third_party/whisper.cpp EXCLUDE_FROM_ALL)
    # ggml is unusably slow unoptimized, so -O3 even in Debug
    foreach(whisper_target IN ITEMS whisper ggml ggml-base ggml-cpu)
        target_compile_options(${whisper_target} PRIVATE $<$<CONFIG:Debug>:-O3>)
    endforeach()
    target_link_libraries(CrossCountryTimer PRIVATE whisper)
    target_compile_definitions(CrossCountryTimer PRIVATE CCT_HAS_WHISPER)

    # the whisper model transcribe.c #embeds; the SHA1 is from whisper.cpp's models/README.md
    file(DOWNLOAD
        https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin
        ${CMAKE_SOURCE_DIR}/models/ggml-base.en.bin
        EXPECTED_HASH SHA1=137c40403d78fd54d454da0f9bd998f78703390c
        SHOW_PROGRESS)
endif()

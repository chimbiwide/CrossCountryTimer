# raylib, raygui, xlsxio, zlib and expat come from the timer's third_party/.
# llama.cpp and SDL are the parser's own plain clones in third_party/, so its
# SDL is independent of the timer's:
#   git clone --depth 1 --branch b11783 https://github.com/ggml-org/llama.cpp.git third_party/llama.cpp
#   git clone --depth 1 --branch release-3.4.18 https://github.com/libsdl-org/SDL.git third_party/SDL
# Configuring downloads pdfium (prebuilt, renders the PDF pages) into
# third_party/ and the model into models/.
cmake_minimum_required(VERSION 3.22)

set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

if(UNIX AND NOT APPLE AND NOT DEFINED CMAKE_C_COMPILER AND NOT DEFINED ENV{CC})
    find_program(RP_GCC16 gcc-16)
    find_program(RP_GXX16 g++-16)
    if(RP_GCC16 AND RP_GXX16)
        set(CMAKE_C_COMPILER ${RP_GCC16} CACHE FILEPATH "C compiler")
        set(CMAKE_CXX_COMPILER ${RP_GXX16} CACHE FILEPATH "C++ compiler")
    endif()
endif()

project(RosterParser LANGUAGES C CXX)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries" FORCE)
set(CCT_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)

if(NOT WIN32)
    find_package(Threads REQUIRED)
endif()

# SDL runs the parse on a worker thread. Only its core (threads) is built:
# the window comes from raylib.
set(SDL_SHARED OFF CACHE BOOL "Build SDL shared library" FORCE)
set(SDL_STATIC ON CACHE BOOL "Build SDL static library" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "Build SDL test library" FORCE)
foreach(sdl_subsystem IN ITEMS AUDIO VIDEO JOYSTICK HAPTIC HIDAPI POWER SENSOR DIALOG TRAY)
    set(SDL_${sdl_subsystem} OFF CACHE BOOL "" FORCE)
endforeach()
# without video SDL needs no X11 or Wayland, which it only allows when told
set(SDL_UNIX_CONSOLE_BUILD ON CACHE BOOL "" FORCE)
add_subdirectory(third_party/SDL SDL EXCLUDE_FROM_ALL)

# raylib on its GLFW backend: the parser has no gamepad
set(BUILD_EXAMPLES OFF CACHE BOOL "Build raylib examples" FORCE)
set(PLATFORM Desktop CACHE STRING "raylib platform" FORCE)
set(USE_EXTERNAL_GLFW OFF CACHE STRING "Use system GLFW")
if(UNIX AND NOT APPLE)
    set(GLFW_BUILD_WAYLAND OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_X11 ON CACHE BOOL "" FORCE)
endif()
list(APPEND CMAKE_MODULE_PATH "${CCT_ROOT}/third_party/raylib/cmake")
include(CompilerFlags)
add_subdirectory(${CCT_ROOT}/third_party/raylib/src raylib)

# xlsxio and what it needs, built the same way as in the timer
set(ZLIB_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZLIB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_INSTALL OFF CACHE BOOL "" FORCE)
set(ZLIB_BUILD_MINIZIP ON CACHE BOOL "" FORCE)
set(ZLIB_MINIZIP_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZLIB_MINIZIP_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(ZLIB_MINIZIP_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(ZLIB_MINIZIP_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(${CCT_ROOT}/third_party/zlib zlib EXCLUDE_FROM_ALL)
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
add_subdirectory(${CCT_ROOT}/third_party/expat/expat expat EXCLUDE_FROM_ALL)

add_library(xlsxio_read STATIC
    ${CCT_ROOT}/third_party/xlsxio/lib/xlsxio_read.c
    ${CCT_ROOT}/third_party/xlsxio/lib/xlsxio_read_sharedstrings.c
)
add_library(xlsxio_write STATIC ${CCT_ROOT}/third_party/xlsxio/lib/xlsxio_write.c)
foreach(xlsxio_target IN ITEMS xlsxio_read xlsxio_write)
    target_compile_definitions(${xlsxio_target}
        PUBLIC BUILD_XLSXIO_STATIC
        PRIVATE BUILD_XLSXIO USE_MINIZIP
    )
    target_include_directories(${xlsxio_target}
        PUBLIC ${CCT_ROOT}/third_party/xlsxio/include
        PRIVATE ${CCT_ROOT}/third_party/xlsxio/lib ${CCT_ROOT}/third_party/zlib/contrib
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

# llama.cpp, CPU only: just libllama and the vision library (mtmd)
set(GGML_CUDA OFF CACHE BOOL "" FORCE)
set(GGML_VULKAN OFF CACHE BOOL "" FORCE)
set(GGML_METAL OFF CACHE BOOL "" FORCE)
set(GGML_BLAS OFF CACHE BOOL "" FORCE)
set(GGML_OPENMP OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_COMMON OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_SERVER OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_APP OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_MTMD ON CACHE BOOL "" FORCE)
set(LLAMA_OPENSSL OFF CACHE BOOL "" FORCE)
set(MTMD_VIDEO OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/llama.cpp EXCLUDE_FROM_ALL)
# the model is unusably slow when built without optimization, so it always is
foreach(llama_target IN ITEMS ggml ggml-base ggml-cpu llama mtmd)
    target_compile_options(${llama_target} PRIVATE $<$<CONFIG:Debug>:-O3>)
endforeach()
# raylib and mtmd both compile stb_image; mtmd's copy is made private so the two don't collide
target_compile_definitions(mtmd PRIVATE STB_IMAGE_STATIC)

# pdfium renders the pages. Prebuilt by https://github.com/bblanchon/pdfium-binaries
set(PDFIUM_RELEASE https://github.com/bblanchon/pdfium-binaries/releases/download/chromium%2F8111)
if(WIN32)
    set(PDFIUM_ARCHIVE pdfium-win-x64.tgz)
    set(PDFIUM_SHA256 3452d11031050ec65e57f180215b57b9e6d465e3fbedafa8a77ad90885c7d23e)
else()
    set(PDFIUM_ARCHIVE pdfium-linux-x64.tgz)
    set(PDFIUM_SHA256 bdc60126399f39fe768566a18d4be055a7b3681bc9e248e8a6656b4f6cae0aa2)
endif()
set(PDFIUM_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/pdfium)
file(DOWNLOAD ${PDFIUM_RELEASE}/${PDFIUM_ARCHIVE} ${CMAKE_CURRENT_SOURCE_DIR}/third_party/${PDFIUM_ARCHIVE}
    EXPECTED_HASH SHA256=${PDFIUM_SHA256})
if(NOT EXISTS ${PDFIUM_DIR}/include/fpdfview.h)
    file(ARCHIVE_EXTRACT INPUT ${CMAKE_CURRENT_SOURCE_DIR}/third_party/${PDFIUM_ARCHIVE} DESTINATION ${PDFIUM_DIR})
endif()
add_library(pdfium SHARED IMPORTED)
set_target_properties(pdfium PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${PDFIUM_DIR}/include)
if(WIN32)
    set_target_properties(pdfium PROPERTIES
        IMPORTED_LOCATION ${PDFIUM_DIR}/bin/pdfium.dll
        IMPORTED_IMPLIB ${PDFIUM_DIR}/lib/pdfium.dll.lib
    )
else()
    set_target_properties(pdfium PROPERTIES IMPORTED_LOCATION ${PDFIUM_DIR}/lib/libpdfium.so)
endif()

# The model: GLM-OCR's text part at Q4_K_M (smaller and faster on a CPU than
# Q8_0, and it still reads every row), and its vision part at Q8_0. CMake
# skips a download when the file is already there with this hash.
set(RP_MODEL_URL https://huggingface.co/mradermacher/GLM-OCR-GGUF/resolve/main)
set(RP_MODEL_FILE GLM-OCR.Q4_K_M.gguf)
set(RP_MODEL_SHA256 bb27f2451f0f1b13491e04e0664cd946438a4677c32acf8ecfef79f68e862de1)
set(RP_MMPROJ_URL https://huggingface.co/ggml-org/GLM-OCR-GGUF/resolve/main)
set(RP_MMPROJ_FILE mmproj-GLM-OCR-Q8_0.gguf)
set(RP_MMPROJ_SHA256 9c4b58e33e316ed142eb5dcb41abec3844d3e6e5dc361ffb782c3fa9d175141f)
file(DOWNLOAD ${RP_MODEL_URL}/${RP_MODEL_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/models/${RP_MODEL_FILE}
    EXPECTED_HASH SHA256=${RP_MODEL_SHA256} SHOW_PROGRESS)
file(DOWNLOAD ${RP_MMPROJ_URL}/${RP_MMPROJ_FILE} ${CMAKE_CURRENT_SOURCE_DIR}/models/${RP_MMPROJ_FILE}
    EXPECTED_HASH SHA256=${RP_MMPROJ_SHA256} SHOW_PROGRESS)

add_executable(RosterParser
    src/main.c
    src/render.c
    src/vlm.c
    src/roster.c
    ${CCT_ROOT}/src/font.c
    ${CCT_ROOT}/src/xlsx.c
)
# raylib's CompilerFlags turns the default down to C99; font.c's #embed needs C23
set_target_properties(RosterParser PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED ON)
target_include_directories(RosterParser PRIVATE include ${CCT_ROOT}/include)
target_compile_definitions(RosterParser PRIVATE
    RP_MODEL_DIR="${CMAKE_CURRENT_SOURCE_DIR}/models"
    RP_MODEL_FILE="${RP_MODEL_FILE}"
    RP_MMPROJ_FILE="${RP_MMPROJ_FILE}"
)
target_link_libraries(RosterParser PRIVATE raylib xlsxio_write xlsxio_read llama mtmd pdfium SDL3::SDL3)
# GUI subsystem: double-clicking the exe opens the app window only.
# Windows looks for pdfium.dll next to the exe.
if(WIN32)
    set_target_properties(RosterParser PROPERTIES WIN32_EXECUTABLE TRUE)
    add_custom_command(TARGET RosterParser POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PDFIUM_DIR}/bin/pdfium.dll $<TARGET_FILE_DIR:RosterParser>)
endif()


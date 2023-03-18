cmake_minimum_required(VERSION 3.13)
project(genesis)

# Change this to Debug if you want to debug
set(CMAKE_BUILD_TYPE Release)

# Generates compile_commands.json which gets read by clangd (I use the extension in vscode).
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Create a symlink to the resources in the build directory
if (NOT ${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${CMAKE_SOURCE_DIR}/res ${CMAKE_BINARY_DIR}/res)
endif()

include(FetchContent)
FetchContent_Declare(
    zlib
    URL https://zlib.net/zlib-1.2.13.tar.gz
)
FetchContent_Declare(
    png
    URL https://download.sourceforge.net/libpng/libpng-1.6.39.tar.xz
)
FetchContent_Declare(
    freetype
    URL https://sourceforge.net/projects/freetype/files/freetype2/2.13.0/freetype-2.13.0.tar.xz/download
)
FetchContent_Declare(
    samplerate
    URL https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz
)
FetchContent_Declare(
    sdl2
    URL https://github.com/libsdl-org/SDL/releases/download/release-2.26.4/SDL2-2.26.4.tar.gz
)

FetchContent_MakeAvailable(zlib)

# Build zlib first then set these options so libpng will use it rather than looking for a system version which will likely fail on Windows.
set(PNG_BUILD_ZLIB ON CACHE INTERNAL "Don't use system zlib" FORCE)
set(SKIP_INSTALL_ALL ON)
set(ZLIB_LIBRARIES zlibstatic)
set(ZLIB_INCLUDE_DIRS ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})

FetchContent_MakeAvailable(png)

# Stop freetype from using find_package on these libraries. Aside from zlib, they are optional and we don't need them.
set(FT_DISABLE_ZLIB ON CACHE INTERNAL "Don't use system zlib" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE INTERNAL "Don't use system bzip2" FORCE)
set(FT_DISABLE_PNG ON CACHE INTERNAL "Don't use system png" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE INTERNAL "Don't use system harfbuzz" FORCE)
set(FT_DISABLE_BROTLI ON CACHE INTERNAL "Don't use system brotli" FORCE)

# If zlib is disabled, Freetype will us an internal version. This causes linker errors since we are building the real zlib.
# This is kind of a hack that sets the same variable as find_package but uses the zlib we're building ourselves.
set(ZLIB_FOUND ON)

# Might as well do the same for libpng since we're already building it.
set(PNG_FOUND ON)
set(PNG_INCLUDE_DIRS ${png_SOURCE_DIR} ${png_BINARY_DIR})
set(PNG_LIBRARIES png_static)

# Disable libsamplerate for SDL. It was causing linker errors on Linux since we're using it directly (on Windows this is disabled by default anyway).
# We're using libsamplerate directly for resampling so this shouldn't make any difference.
set(SDL_LIBSAMPLERATE OFF CACHE INTERNAL "Use libsamplerate" FORCE)

FetchContent_MakeAvailable(freetype samplerate sdl2)
add_executable(genesis src/main.c src/assets.c src/game.c src/pcgrandom.c src/audio.c src/font.c)

# Enable warnings on Linux. MSVC appears to have them on by default.
if (NOT MSVC)
    target_compile_options(genesis PRIVATE -Wall)
endif()

# Other libraries include the correct paths with the target_link_libraries command but not libpng so add it here.
target_include_directories(genesis PRIVATE ${PNG_INCLUDE_DIRS})

if (WIN32)
    target_link_libraries(genesis PRIVATE SDL2main)
endif()

target_link_libraries(genesis PRIVATE ${PNG_LIBRARIES} freetype samplerate SDL2-static)

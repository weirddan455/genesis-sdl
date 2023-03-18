cmake_minimum_required(VERSION 3.11)
project(genesis)
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
    sdl2
    URL https://github.com/libsdl-org/SDL/releases/download/release-2.26.4/SDL2-2.26.4.tar.gz
)
FetchContent_Declare(
    freetype
    URL https://sourceforge.net/projects/freetype/files/freetype2/2.13.0/freetype-2.13.0.tar.xz/download
)
FetchContent_Declare(
    samplerate
    URL https://github.com/libsndfile/libsamplerate/releases/download/0.2.2/libsamplerate-0.2.2.tar.xz
)

FetchContent_MakeAvailable(zlib)

# Build zlib first then set these options so libpng will use it rather than looking for a system version which will likely fail on Windows.
option(PNG_BUILD_ZLIB "Custom zlib Location, else find_package is used" ON)
set(SKIP_INSTALL_ALL ON)
set(ZLIB_LIBRARIES zlibstatic)
set(ZLIB_INCLUDE_DIRS ${zlib_SOURCE_DIR})

FetchContent_MakeAvailable(png sdl2 freetype samplerate)
add_executable(genesis src/main.c src/assets.c src/game.c src/pcgrandom.c src/audio.c src/font.c)
if (MSVC)

else ()
    target_compile_options(genesis PRIVATE -Wall)
endif()
target_link_libraries(genesis PRIVATE png_static SDL2-static freetype samplerate)

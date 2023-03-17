cmake_minimum_required(VERSION 3.11)
project(genesis)
include(FetchContent)
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
FetchContent_MakeAvailable(sdl2 png freetype samplerate)
add_executable(genesis src/main.c src/assets.c src/game.c src/pcgrandom.c src/audio.c src/font.c)
if (MSVC)

else ()
    target_compile_options(genesis PRIVATE -Wall)
endif()
target_link_libraries(genesis PRIVATE png_static SDL2-static freetype samplerate)

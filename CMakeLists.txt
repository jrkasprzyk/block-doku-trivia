# Trivia-Doku — CMake build file
#
# Expects vcpkg to provide raylib and nlohmann-json. Configure with:
#   cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake
# Then build with:
#   cmake --build build --config Release

cmake_minimum_required(VERSION 3.20)
project(trivia_doku LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC niceties: warnings on, multi-processor compile, UTF-8 source
if(MSVC)
    add_compile_options(/W4 /MP /utf-8)
endif()

find_package(raylib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(trivia_doku
    src/main.cpp
    src/Board.cpp
    src/Piece.cpp
    src/Renderer.cpp
    src/TriviaBank.cpp
)

target_include_directories(trivia_doku PRIVATE src)
target_link_libraries(trivia_doku PRIVATE
    raylib
    nlohmann_json::nlohmann_json
)

# Copy assets next to the executable so the game can find trivia.json at runtime.
# On Windows multi-config generators the exe lives in build/Release/, build/Debug/, etc.
add_custom_command(TARGET trivia_doku POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:trivia_doku>/assets
)

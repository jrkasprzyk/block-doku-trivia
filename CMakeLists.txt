# Trivia-Doku — CMake build file
#
# Expects vcpkg to provide raylib and nlohmann-json. Configure once with:
#   cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE=<path-to-vcpkg>/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release
# Then build with:
#   cmake --build build

cmake_minimum_required(VERSION 3.20)
project(trivia_doku LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /MP /utf-8 /EHsc)
endif()

# Trivia JSON — downloaded from jsdelivr at configure time and bundled with the build.
# Bump TRIVIA_VERSION here when a new release of promptukit is tagged.
set(TRIVIA_VERSION "0.1.300")
set(TRIVIA_URL
    "https://cdn.jsdelivr.net/gh/jrkasprzyk/promptukit@v${TRIVIA_VERSION}/promptukit/data/question_banks/jrb_industries_trivia.json")
set(TRIVIA_DEST "${CMAKE_CURRENT_BINARY_DIR}/jrb_industries_trivia.json")
set(TRIVIA_FALLBACK "${CMAKE_SOURCE_DIR}/assets/jrb_industries_trivia.${TRIVIA_VERSION}.json")

message(STATUS "Fetching trivia v${TRIVIA_VERSION} from jsdelivr...")
file(DOWNLOAD "${TRIVIA_URL}" "${TRIVIA_DEST}"
    STATUS _dl_status
    TIMEOUT 15
)
list(GET _dl_status 0 _dl_code)
if(_dl_code EQUAL 0)
    message(STATUS "Trivia download OK (v${TRIVIA_VERSION})")
else()
    message(WARNING "Trivia download failed (${_dl_status}) — using local fallback: ${TRIVIA_FALLBACK}")
    configure_file("${TRIVIA_FALLBACK}" "${TRIVIA_DEST}" COPYONLY)
endif()

find_package(raylib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

add_executable(trivia_doku
    src/main.cpp
    src/Board.cpp
    src/Piece.cpp
    src/Renderer.cpp
    src/TriviaBank.cpp
    resources/app.rc # Windows resource, supplies .exe icon and metadata. Ignored on other platforms.
)

target_include_directories(trivia_doku PRIVATE src)
target_link_libraries(trivia_doku PRIVATE
    raylib
    nlohmann_json::nlohmann_json
)

# Copy assets next to the executable. On Windows multi-config generators the exe
# lives in build/Release/, build/Debug/, etc.
add_custom_command(TARGET trivia_doku POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:trivia_doku>/assets
    COMMAND ${CMAKE_COMMAND} -E copy
        ${TRIVIA_DEST}
        $<TARGET_FILE_DIR:trivia_doku>/assets/jrb_industries_trivia.json
)

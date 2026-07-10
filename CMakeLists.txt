cmake_minimum_required(VERSION 3.20)
project(SingularityStrategy LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# Raylib 5.5
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG 5.5
)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(raylib)

# Flecs (stable tag)
FetchContent_Declare(
    flecs
    GIT_REPOSITORY https://github.com/SanderMertens/flecs.git
    GIT_TAG v4.0.4
)
FetchContent_MakeAvailable(flecs)

# Source files
#file(GLOB_RECURSE SOURCES "src/*.c")

#add_executable(${PROJECT_NAME} ${SOURCES})
add_executable(${PROJECT_NAME} src/main.c)

target_link_libraries(${PROJECT_NAME} PRIVATE raylib flecs::flecs_static)

target_include_directories(${PROJECT_NAME} PRIVATE src)

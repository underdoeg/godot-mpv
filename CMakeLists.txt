cmake_minimum_required(VERSION 3.25)
project(godot-mpv)

include(FetchContent)
include(ExternalProject)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

## godot-cpp
FetchContent_Declare(
        godot-cpp
        GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
        GIT_TAG 4.3
)
FetchContent_GetProperties(godot-cpp)
if (NOT godot-cpp_POPULATED)
    FetchContent_Populate(godot-cpp)
    add_subdirectory(${godot-cpp_SOURCE_DIR} ${godot-cpp_BINARY_DIR})
endif ()

#FetchContent_Declare(
#        mpv
#        GIT_REPOSITORY https://github.com/mpv-player/mpv.git
#        GIT_TAG v0.39.0
#)
#FetchContent_GetProperties(mpv)
#if (NOT mpv_POPULATED)
#    FetchContent_Populate(mpv)
#    add_subdirectory(${godot-cpp_SOURCE_DIR} ${godot-cpp_BINARY_DIR})
#endif ()

find_library(MPV_LIBRARIES mpv REQUIRED)
#find_path(MPV_INCLUDES NAMES "mpv/client.h" REQUIRED)


file(GLOB SRC_LIST "src/*")

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} godot-cpp ${MPV_LIBRARIES})

#target_link_libraries(${PROJECT_NAME} godot-cpp mpv)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_BINARY_DIR}/libgodot-mpv.so"
        "${PROJECT_SOURCE_DIR}/sample/godot-mpv/libgodot-mpv.so")


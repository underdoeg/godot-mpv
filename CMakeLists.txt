cmake_minimum_required(VERSION 3.25)
project(godot-mpv)

set(CMAKE_CXX_STANDARD 17)

find_library(MPV_LIBRARIES mpv REQUIRED)
#find_path(MPV_INCLUDES NAMES "mpv/client.h" REQUIRED)

add_subdirectory(godot-cpp)

file (GLOB SRC_LIST "src/*")

add_library(${PROJECT_NAME} SHARED ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} godot-cpp ${MPV_LIBRARIES})

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_BINARY_DIR}/libgodot-mpv.so"
        "${PROJECT_SOURCE_DIR}/sample/godot-mpv/libgodot-mpv.so")

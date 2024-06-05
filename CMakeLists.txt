cmake_minimum_required(VERSION 3.28)
project(Coal_Miner C)

set(CMAKE_C_STANDARD 23)

file(GLOB_RECURSE MAIN_SRC CONFIGURE_DEPENDS MainApp/src/*.c)

add_subdirectory(Engine)
add_executable(Coal_Miner ${MAIN_SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE Engine)

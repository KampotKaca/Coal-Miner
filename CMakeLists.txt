cmake_minimum_required(VERSION 3.28)
project(Coal_Miner C)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DRELEASE)
endif()

file(GLOB_RECURSE MAIN_SRC CONFIGURE_DEPENDS MainApp/src/*.c)

add_subdirectory(Engine)
add_executable(Coal_Miner ${MAIN_SRC})

target_link_libraries(${PROJECT_NAME} PRIVATE Engine)

target_include_directories(${PROJECT_NAME} PUBLIC MainApp/src/)

#region resources
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC RES_PATH="${CMAKE_CURRENT_SOURCE_DIR}/MainApp/res/") #DEBUG
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    file(COPY ${CMAKE_SOURCE_DIR}/MainApp/res DESTINATION ${CMAKE_BINARY_DIR})
    target_compile_definitions("${CMAKE_PROJECT_NAME}" PUBLIC RES_PATH="${CMAKE_BINARY_DIR}./res/") #BUILD
endif()

add_custom_target(copy_res ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/MainApp/res ${CMAKE_BINARY_DIR}/res
        COMMENT "Copying resource files..."
)

# Make the custom target a dependency of the main project
add_dependencies(${CMAKE_PROJECT_NAME} copy_res)
#endregion
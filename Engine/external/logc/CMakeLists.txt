file(GLOB_RECURSE LOG_C_SRC CONFIGURE_DEPENDS src/*.c)

add_definitions(-DLOG_USE_COLOR)
add_compile_options(-fdiagnostics-color=always)

add_library(logc ${LOG_C_SRC})

target_include_directories(logc PUBLIC include)
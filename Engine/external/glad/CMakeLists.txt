file(GLOB_RECURSE GLAD_SRC CONFIGURE_DEPENDS src/*.c)
add_library(glad ${GLAD_SRC})

target_include_directories(glad PUBLIC include)
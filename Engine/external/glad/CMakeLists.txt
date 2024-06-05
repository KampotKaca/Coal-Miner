file(GLOB_RECURSE GLAD_SRC CONFIGURE_DEPENDS src/*.c)
add_library(Glad ${GLAD_SRC})

target_include_directories(Glad PUBLIC include)
add_library(rb_tree INTERFACE)

target_include_directories(rb_tree
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(rb_tree_cli_lib STATIC
    rb_tree_cli_lib.cpp
)
target_link_libraries(rb_tree_cli_lib
    PUBLIC
        rb_tree
)

add_executable(rb_tree_cli rb_tree.cpp)
target_link_libraries(rb_tree_cli
    PRIVATE
        rb_tree_cli_lib
)

add_executable(std_set_cli std_set_cli.cpp)

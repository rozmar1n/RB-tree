include(FetchContent)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)

FetchContent_MakeAvailable(googletest)

add_executable(rb_balance_test
    rb_balance_test.cpp
)

add_executable(rb_memory_test
    rb_memory_test.cpp
)

add_executable(rb_distance_test
    rb_distance_test.cpp
)

target_link_libraries(rb_balance_test
    PRIVATE
        rb_tree
        GTest::gtest_main
)

target_link_libraries(rb_memory_test
    PRIVATE
        rb_tree
        GTest::gtest_main
)

target_link_libraries(rb_distance_test
    PRIVATE
        rb_tree
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(rb_balance_test)
gtest_discover_tests(rb_memory_test)
gtest_discover_tests(rb_distance_test)

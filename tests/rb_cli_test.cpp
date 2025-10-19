#include "rb_tree_cli.hpp"

#include <sstream>

#include <gtest/gtest.h>

TEST(RBTreeCliTest, ProcessesSampleInput) {
    std::istringstream input("k 10 k 20 q 8 31 q 6 9 k 30 k 40 q 15 40\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = rb::run_cli(input, output, error);

    EXPECT_EQ(exit_code, 0);
    EXPECT_EQ(output.str(), "2 0 3\n");
    EXPECT_TRUE(error.str().empty());
}

TEST(RBTreeCliTest, HandlesDescendingBoundsAndInclusiveLeft) {
    std::istringstream input(
        "k 10 q 2 7 q 3 9 k 1 k 2 k 0 k 6 q 7 2 k 10 q 3 1 q 5 3 q 9 4 k 2 q 7 8 "
        "k 2 k 3 k 1 q 2 3 q 6 1 q 2 9\n");
    std::ostringstream output;
    std::ostringstream error;

    const int exit_code = rb::run_cli(input, output, error);

    EXPECT_EQ(exit_code, 0);
    EXPECT_EQ(output.str(), "0 0 0 0 0 0 0 2 0 3\n");
    EXPECT_TRUE(error.str().empty());
}

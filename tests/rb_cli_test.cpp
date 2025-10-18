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

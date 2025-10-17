#include <algorithm>
#include <cstddef>
#include <numeric>
#include <random>
#include <vector>

#include "rb_tree.hpp"

#include <gtest/gtest.h>

TEST(RBTreeBalanceTest, EmptyTreeIsBalanced) {
    rb::Tree<int> tree;
    EXPECT_TRUE(tree.is_valid());
}

TEST(RBTreeBalanceTest, SequentialInsertionsStayBalanced) {
    rb::Tree<int> tree;
    for (int i = 0; i < 200; ++i) {
        ASSERT_TRUE(tree.insert(i));
    }
    EXPECT_TRUE(tree.is_valid());
}

TEST(RBTreeBalanceTest, InsertEraseMaintainsBalance) {
    rb::Tree<int> tree;
    std::vector<int> values(200);
    std::iota(values.begin(), values.end(), 0);

    std::mt19937 rng{12345};
    std::shuffle(values.begin(), values.end(), rng);
    for (int value : values) {
        ASSERT_TRUE(tree.insert(value));
    }

    EXPECT_TRUE(tree.is_valid());

    std::shuffle(values.begin(), values.end(), rng);
    for (std::size_t i = 0; i < values.size(); ++i) {
        ASSERT_TRUE(tree.erase(values[i]));
        if (i % 20 == 0) {
            EXPECT_TRUE(tree.is_valid());
        }
    }
    EXPECT_TRUE(tree.is_valid());
    EXPECT_TRUE(tree.empty());
}

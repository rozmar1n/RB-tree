#include "rb_tree.hpp"

#include <gtest/gtest.h>
#include <limits>

TEST(RBTreeDistanceTest, DistanceFromRootMatchesInorderRank) {
    rb::Tree<int> tree;
    tree.insert(10);
    tree.insert(5);
    tree.insert(15);
    tree.insert(3);
    tree.insert(7);

    EXPECT_EQ(tree.distance_from_root(3), 0u);
    EXPECT_EQ(tree.distance_from_root(5), 1u);
    EXPECT_EQ(tree.distance_from_root(7), 2u);
    EXPECT_EQ(tree.distance_from_root(10), 3u);
    EXPECT_EQ(tree.distance_from_root(100), std::numeric_limits<size_t>::max());
}

TEST(RBTreeDistanceTest, DistanceBetweenNodesUsesShortestPath) {
    rb::Tree<int> tree;
    tree.insert(20);
    tree.insert(10);
    tree.insert(30);
    tree.insert(5);
    tree.insert(15);
    tree.insert(25);
    tree.insert(35);

    EXPECT_EQ(tree.distance(20, 20), 0u);
    EXPECT_EQ(tree.distance(20, 10), 0u);
    EXPECT_EQ(tree.distance(5, 15), 2u);
    EXPECT_EQ(tree.distance(5, 35), 6u);
    EXPECT_EQ(tree.distance(25, 30), 1u);
    EXPECT_EQ(tree.distance(5, 99), std::numeric_limits<size_t>::max());
}

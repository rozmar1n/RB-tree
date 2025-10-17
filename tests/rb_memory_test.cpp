#include "rb_tree.hpp"

#include <gtest/gtest.h>

namespace {

struct LifetimeTracker {
    static int constructions;
    static int destructions;
    int value{};

    LifetimeTracker() { ++constructions; }
    explicit LifetimeTracker(int v) : value(v) { ++constructions; }
    LifetimeTracker(const LifetimeTracker& other) : value(other.value) {
        ++constructions;
    }
    LifetimeTracker(LifetimeTracker&& other) noexcept : value(other.value) {
        other.value = -1;
        ++constructions;
    }
    LifetimeTracker& operator=(const LifetimeTracker& other) {
        value = other.value;
        return *this;
    }
    LifetimeTracker& operator=(LifetimeTracker&& other) noexcept {
        value = other.value;
        other.value = -1;
        return *this;
    }
    ~LifetimeTracker() { ++destructions; }

    friend bool operator<(const LifetimeTracker& lhs,
                          const LifetimeTracker& rhs) {
        return lhs.value < rhs.value;
    }
};

int LifetimeTracker::constructions = 0;
int LifetimeTracker::destructions = 0;

void ResetCounters() {
    LifetimeTracker::constructions = 0;
    LifetimeTracker::destructions = 0;
}

} // namespace

TEST(RBTreeMemoryTest, DestructionReleasesAllNodes) {
    ResetCounters();
    {
        rb::Tree<LifetimeTracker> tree;
        for (int i = 0; i < 100; ++i) {
            tree.insert(LifetimeTracker{i});
        }
        EXPECT_TRUE(tree.is_valid());
    }
    EXPECT_EQ(LifetimeTracker::constructions, LifetimeTracker::destructions);
}

TEST(RBTreeMemoryTest, CopyConstructorPerformsDeepCopy) {
    ResetCounters();
    rb::Tree<LifetimeTracker> original;
    for (int i = 0; i < 10; ++i) {
        original.insert(LifetimeTracker{i});
    }

    LifetimeTracker::destructions = 0;
    const int constructions_before_copy = LifetimeTracker::constructions;
    rb::Tree<LifetimeTracker> copy(original);

    EXPECT_TRUE(original.is_valid());
    EXPECT_TRUE(copy.is_valid());
    EXPECT_EQ(LifetimeTracker::constructions,
              constructions_before_copy + 10);
    EXPECT_EQ(LifetimeTracker::destructions, 0);

    original.insert(LifetimeTracker{50});
    EXPECT_TRUE(original.erase(LifetimeTracker{50}));
    EXPECT_FALSE(copy.erase(LifetimeTracker{50}));
}

TEST(RBTreeMemoryTest, MoveConstructorTransfersOwnership) {
    ResetCounters();
    rb::Tree<LifetimeTracker> source;
    for (int i = 0; i < 5; ++i) {
        source.insert(LifetimeTracker{i});
    }
    EXPECT_TRUE(source.is_valid());

    rb::Tree<LifetimeTracker> moved(std::move(source));

    EXPECT_TRUE(moved.is_valid());
    EXPECT_TRUE(source.empty());
    EXPECT_TRUE(source.is_valid());
}

TEST(RBTreeMemoryTest, CopyAssignmentReplacesContent) {
    ResetCounters();
    rb::Tree<LifetimeTracker> lhs;
    rb::Tree<LifetimeTracker> rhs;

    for (int i = 0; i < 5; ++i) {
        lhs.insert(LifetimeTracker{i});
    }
    for (int i = 100; i < 110; ++i) {
        rhs.insert(LifetimeTracker{i});
    }

    lhs = rhs;

    EXPECT_TRUE(lhs.is_valid());
    EXPECT_TRUE(rhs.is_valid());
    EXPECT_TRUE(lhs.erase(LifetimeTracker{105}));
    EXPECT_TRUE(rhs.erase(LifetimeTracker{105}));
    EXPECT_FALSE(lhs.erase(LifetimeTracker{1}));
}

TEST(RBTreeMemoryTest, MoveAssignmentReleasesPreviousNodes) {
    ResetCounters();
    rb::Tree<LifetimeTracker> lhs;
    rb::Tree<LifetimeTracker> rhs;

    for (int i = 0; i < 20; ++i) {
        lhs.insert(LifetimeTracker{i});
    }
    for (int i = 100; i < 110; ++i) {
        rhs.insert(LifetimeTracker{i});
    }

    LifetimeTracker::destructions = 0;
    lhs = std::move(rhs);

    EXPECT_TRUE(lhs.is_valid());
    EXPECT_TRUE(rhs.empty());
    EXPECT_TRUE(rhs.is_valid());
    EXPECT_GE(LifetimeTracker::destructions, 20);
}

#include "engine/PlayerProgress.h"
#include "gtest/gtest.h"

// Test fixture for PlayerProgress tests
class PlayerProgressTest : public ::testing::Test {
protected:
    PlayerProgress progress;
};

// Test case to check the default constructor and initial state
TEST_F(PlayerProgressTest, InitialState) {
    // Assert that the initial currency is 0
    ASSERT_EQ(progress.totalCurrency, 0);
    // Assert that there are 5 levels created by default
    ASSERT_EQ(progress.levels.size(), 5);
    // Assert that the first level is unlocked
    ASSERT_TRUE(progress.levels[0].unlocked);
    // Assert that the second level is locked
    ASSERT_FALSE(progress.levels[1].unlocked);
}

// Test case to check the AddCurrency method
TEST_F(PlayerProgressTest, AddCurrency) {
    // Act: Add 50 currency
    progress.AddCurrency(50);
    // Assert: The total currency should now be 50
    ASSERT_EQ(progress.totalCurrency, 50);

    // Act: Add another 100 currency
    progress.AddCurrency(100);
    // Assert: The total currency should now be 150
    ASSERT_EQ(progress.totalCurrency, 150);
}

// Test case for completing a level
TEST_F(PlayerProgressTest, CompleteLevel) {
    // Act: Complete level 0 with 2 stars and a time of 120.5f
    progress.CompleteLevel(0, 120.5f, 2);

    // Assert: Check the stats for level 0
    ASSERT_EQ(progress.levels[0].stars, 2);
    ASSERT_FLOAT_EQ(progress.levels[0].bestTime, 120.5f);

    // Assert: Check that the next level (index 1) is now unlocked
    ASSERT_TRUE(progress.levels[1].unlocked);
}

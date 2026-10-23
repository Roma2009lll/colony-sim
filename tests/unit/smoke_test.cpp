#include <gtest/gtest.h>
#include <thread>


TEST(SmokeTest, BasicAssertions) {
    EXPECT_STRNE("hello", "world");
    EXPECT_EQ(2 + 2, 4);
}

TEST(SmokeTest, ThreadSanitizerCheck) {
    int counter = 0;
    std::thread t([&counter]() {
        int x = 10;
        (void)x;
    });
    t.join();
    counter = 1;
    EXPECT_EQ(counter, 1);
}
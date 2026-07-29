//
// Created by david on 7/24/26.
//
#include <gtest/gtest.h>
#include "chunked_string.hpp"

using ext::chunked_string;

class ChunkedStringTests : public testing::Test {
protected:
    ChunkedStringTests() = default;
    static constexpr std::array dog_breeds = {"Akita", "Afghan Hound", "Borzoi", "Australian Stumpy Tail Cattle Dog"};
};

TEST_F(ChunkedStringTests, Construction)
{
    constexpr size_t total_size = 20;
    chunked_string<dog_breeds.size()> string;
    chunked_string<dog_breeds.size()> string2{total_size};
    chunked_string<dog_breeds.size()> string3{total_size, 'q'};
    for (size_t i = 0; i < dog_breeds.size(); ++i) {
        const auto& chunk = string[i];
        EXPECT_EQ(chunk.size(), 0);
        const auto& chunk2 = string2[i];
        EXPECT_EQ(chunk2.size(), 0);
        EXPECT_NE(chunk2.capacity(), std::numeric_limits<size_t>::max());
        EXPECT_GE(chunk2.free_capacity(), (total_size / dog_breeds.size()) - 1);
        const auto& chunk3 = string3[i];
        EXPECT_GE(chunk3.size(), (total_size / dog_breeds.size()) - 1);
    }
}

TEST_F(ChunkedStringTests, AssignmentAppend)
{
    chunked_string<dog_breeds.size()> string{};
    for (size_t i = 0; i < dog_breeds.size(); ++i) {
        auto& chunk = string[i];
        chunk = dog_breeds[i];
        EXPECT_STREQ(string[i].c_str(), dog_breeds[i]);
    }
    for (ptrdiff_t i = static_cast<ptrdiff_t>(dog_breeds.size()) - 1; i >= 0; --i) {
        auto& chunk = string[i];
        chunk += dog_breeds[i];
        auto str = std::string{dog_breeds[i]} + std::string{dog_breeds[i]};
        EXPECT_STREQ(string[i].c_str(), str.c_str());
    }
}

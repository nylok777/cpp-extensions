//
// Created by david on 7/24/26.
//
#include <gtest/gtest.h>
#include "chunked_string.hpp"

// Source - https://stackoverflow.com/a/62823211
// Posted by nitronoid, modified by community. See post 'Timeline' for change history
// Retrieved 2026-07-30, License - CC BY-SA 4.0

template <std::string_view const&... Strs>
struct join
{
    // Join all strings into a single std::array of chars
    static constexpr auto impl() noexcept
    {
        constexpr std::size_t len = (Strs.size() + ... + 0);
        std::array<char, len + 1> arr{};
        auto append = [i = 0, &arr](auto const& s) mutable {
            for (auto c : s) arr[i++] = c;
        };
        (append(Strs), ...);
        arr[len] = 0;
        return arr;
    }
    // Give the joined string static storage
    static constexpr auto arr = impl();
    // View as a std::string_view
    static constexpr std::string_view value {arr.data(), arr.size() - 1};
};
// Helper to get the value out
template <std::string_view const&... Strs>
static constexpr auto join_v = join<Strs...>::value;

using ext::chunked_string;

class ChunkedStringTests : public testing::Test {
protected:
    ChunkedStringTests() = default;

    static constexpr std::array<std::string_view, 4> dog_breeds = {"Akita", "Afghan Hound", "Borzoi", "Australian Stumpy Tail Cattle Dog"};
    static constexpr auto dog_breeds_concat = []<size_t... Is>(std::index_sequence<Is...>) constexpr {
        return join_v<(dog_breeds[Is])...>;
    }(std::make_index_sequence<dog_breeds.size()>{});

    using chunked_string_t = chunked_string<dog_breeds.size()>;
};

TEST_F(ChunkedStringTests, BasicConstruction)
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

TEST_F(ChunkedStringTests, ConstructionFromStringLike)
{
    constexpr auto view = dog_breeds[0];
    std::string string{view};
    chunked_string_t ch_string{view};
    chunked_string_t ch_string2{string};
    chunked_string_t ch_string3{std::move(string)};

    EXPECT_STREQ(view.data(), ch_string[0].c_str());
    EXPECT_STREQ(view.data(), ch_string2[0].c_str());
    EXPECT_STREQ(view.data(), ch_string3[0].c_str());
}

TEST_F(ChunkedStringTests, ConstructionFromPack)
{
    std::string string{dog_breeds[0]};
    string += dog_breeds[1];
    chunked_string_t chunked_string{dog_breeds[0], dog_breeds[1]};

    EXPECT_STREQ(dog_breeds[0].data(), chunked_string[0].c_str());
    EXPECT_STREQ(dog_breeds[1].data(), chunked_string[1].c_str());

    std::string string2{chunked_string[0]};
    string2 += chunked_string[1];

    EXPECT_STREQ(string2.c_str(), string.c_str());
}

TEST_F(ChunkedStringTests, ConstructionFromStringAndOffsetArray)
{
    using diff_t = chunked_string_t::difference_type;

    std::array<diff_t, dog_breeds.size()> offsets{};
    std::ranges::transform(dog_breeds, offsets.begin(), [i = diff_t{0}](const auto& view) mutable {
        auto x = i;
        i += static_cast<diff_t>(view.size());
        return x;
    });
    std::string string;
    string.reserve(dog_breeds_concat.size());

    chunked_string_t chunked_string{dog_breeds_concat, offsets};

    for (size_t i = 0; i < dog_breeds.size(); ++i) {
        ASSERT_STREQ(dog_breeds[i].data(), chunked_string[i].c_str());
        string += chunked_string[i];
    }
    EXPECT_STREQ(string.c_str(), dog_breeds_concat.data());
}

class ChunkedStringCapacityTests : public ChunkedStringTests, public testing::WithParamInterface<size_t>{
protected:
    using Base = ChunkedStringTests;

    chunked_string_t chunked_string = []<size_t... Is>(std::index_sequence<Is...>) {
        return chunked_string_t{dog_breeds[Is]...};
    }(std::make_index_sequence<dog_breeds.size()>{});
};

TEST_P(ChunkedStringCapacityTests, Reserve)
{
    const size_t idx = GetParam();
    const auto prev_size = chunked_string.size(idx);

    ASSERT_NO_FATAL_FAILURE(chunked_string.reserve(prev_size + 10, idx));

    EXPECT_EQ(prev_size, chunked_string.size(idx));
    EXPECT_EQ(prev_size + 10, chunked_string.capacity(idx));

    for (const auto & [idx, view] : dog_breeds | std::views::enumerate) {
        EXPECT_STREQ(view.data(), chunked_string[idx].c_str());
    }
}

TEST_P(ChunkedStringCapacityTests, Resize)
{
    const size_t idx = GetParam();
    const auto prev_size = chunked_string.size(idx);

    ASSERT_NO_FATAL_FAILURE(chunked_string.resize(prev_size + 10, idx));

    EXPECT_EQ(prev_size + 10, chunked_string.size(idx));

    for (const auto & [idx, view] : dog_breeds | std::views::enumerate) {
        EXPECT_STREQ(view.data(), chunked_string[idx].c_str());
    }
}

INSTANTIATE_TEST_SUITE_P(ReserveAndResize, ChunkedStringCapacityTests, testing::Values(0, 1, 3));

TEST_F(ChunkedStringTests, AssignmentAppend)
{
    chunked_string<dog_breeds.size()> string{};
    for (size_t i = 0; i < dog_breeds.size(); ++i) {
        auto& chunk = string[i];
        chunk = dog_breeds[i];
        EXPECT_STREQ(string[i].c_str(), dog_breeds[i].data());
    }
    for (ptrdiff_t i = static_cast<ptrdiff_t>(dog_breeds.size()) - 1; i >= 0; --i) {
        auto& chunk = string[i];
        chunk += dog_breeds[i];
        auto str = std::string{dog_breeds[i]} + std::string{dog_breeds[i]};
        EXPECT_STREQ(string[i].c_str(), str.c_str());
    }
}

// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 nylok777 (https://github.com/nylok777)

//
// Created by david on 7/24/26.
//
#include <gtest/gtest.h>
#include "chunked_string.hpp"

using namespace std::string_view_literals;

// Source - https://stackoverflow.com/a/62823211
// Posted by nitronoid, modified by community. See post 'Timeline' for change history
// Retrieved 2026-07-30, License - CC BY-SA 4.0

template<const std::string_view&... Strs> struct join {
    // Join all strings into a single std::array of chars
    static constexpr auto impl() noexcept
    {
        constexpr std::size_t len = (Strs.size() + ... + 0);
        std::array<char, len + 1> arr{};
        auto append = [i = 0, &arr](const auto& s) mutable {
            for (auto c : s) arr[i++] = c;
        };
        (append(Strs), ...);
        arr[len] = 0;
        return arr;
    }
    // Give the joined string static storage
    static constexpr auto arr = impl();
    // View as a std::string_view
    static constexpr std::string_view value{arr.data(), arr.size() - 1};
};
// Helper to get the value out
template<const std::string_view&... Strs> static constexpr auto join_v = join<Strs...>::value;

using ext::chunked_string;

class ChunkedStringTests : public testing::Test {
public:
    static constexpr size_t strings_count = 4;
    static constexpr std::array<std::string_view, strings_count> strings
        = {"Akita", "Afghan Hound", "Borzoi", "Australian Stumpy Tail Cattle Dog"};
    static constexpr auto strings_concat = []<size_t... Is>(std::index_sequence<Is...>) constexpr {
        return join_v<(strings[Is])...>;
    }(std::make_index_sequence<strings.size()>{});

    using chunked_string_t = chunked_string<strings.size()>;

protected:
    ChunkedStringTests() = default;

    static constexpr auto make_test_chunked_string() -> chunked_string_t
    {
        return []<size_t... Is>(std::index_sequence<Is...>) {
            return chunked_string_t{strings[Is]...};
        }(std::make_index_sequence<strings.size()>{});
    }
};

TEST_F(ChunkedStringTests, BasicConstruction)
{
    constexpr size_t total_size = 20;
    chunked_string<strings.size()> string;
    chunked_string<strings.size()> string2{total_size};
    chunked_string<strings.size()> string3{total_size, 'q'};
    for (size_t i = 0; i < strings.size(); ++i) {
        const auto& chunk = string[i];
        EXPECT_EQ(chunk.size(), 0);
        const auto& chunk2 = string2[i];
        EXPECT_EQ(chunk2.size(), 0);
        EXPECT_NE(chunk2.capacity(), std::numeric_limits<size_t>::max());
        EXPECT_GE(chunk2.free_capacity(), (total_size / strings.size()) - 1);
        const auto& chunk3 = string3[i];
        EXPECT_GE(chunk3.size(), (total_size / strings.size()) - 1);
    }
}

TEST_F(ChunkedStringTests, ConstructionFromStringLike)
{
    constexpr auto view = strings[0];
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
    std::string string{strings[0]};
    string += strings[1];
    chunked_string_t chunked_string{strings[0], strings[1]};

    EXPECT_STREQ(strings[0].data(), chunked_string[0].c_str());
    EXPECT_STREQ(strings[1].data(), chunked_string[1].c_str());

    std::string string2{chunked_string[0]};
    string2 += chunked_string[1];

    EXPECT_STREQ(string2.c_str(), string.c_str());
}

TEST_F(ChunkedStringTests, ConstructionFromStringAndOffsetArray)
{
    using diff_t = chunked_string_t::difference_type;

    std::array<diff_t, strings.size()> offsets{};
    std::ranges::transform(strings, offsets.begin(), [i = diff_t{0}](const auto& view) mutable {
        auto x = i;
        i += static_cast<diff_t>(view.size());
        return x;
    });
    std::string string;
    string.reserve(strings_concat.size());

    chunked_string_t chunked_string{strings_concat, offsets};

    for (size_t i = 0; i < strings.size(); ++i) {
        ASSERT_STREQ(strings[i].data(), chunked_string[i].c_str());
        string += chunked_string[i];
    }
    EXPECT_STREQ(string.c_str(), strings_concat.data());
}

class ChunkedStringCapacityTests : public ChunkedStringTests,
                                   public testing::WithParamInterface<size_t> {
protected:
    using Base = ChunkedStringTests;
};

TEST_P(ChunkedStringCapacityTests, Reserve)
{
    chunked_string_t chunked_string = make_test_chunked_string();
    const size_t idx = GetParam();
    const auto prev_size = chunked_string[idx].size();

    ASSERT_NO_FATAL_FAILURE(chunked_string[idx].reserve(prev_size + 10));

    EXPECT_EQ(prev_size, chunked_string[idx].size());
    EXPECT_EQ(prev_size + 10, chunked_string[idx].capacity());

    for (const auto& [idx, view] : strings | std::views::enumerate) {
        EXPECT_STREQ(view.data(), chunked_string[idx].c_str());
    }
}

TEST_P(ChunkedStringCapacityTests, Resize)
{
    chunked_string_t chunked_string = make_test_chunked_string();
    const size_t idx = GetParam();
    const auto prev_size = chunked_string[idx].size();

    ASSERT_NO_FATAL_FAILURE(chunked_string[idx].resize(prev_size + 10));

    EXPECT_EQ(prev_size + 10, chunked_string[idx].size());

    for (const auto& [idx, view] : strings | std::views::enumerate) {
        EXPECT_STREQ(view.data(), chunked_string[idx].c_str());
    }
}

static constexpr std::array<size_t, 3> test_positions = {0, ChunkedStringTests::strings_count / 2, ChunkedStringTests::strings_count - 1};

INSTANTIATE_TEST_SUITE_P(ReserveAndResize, ChunkedStringCapacityTests, testing::ValuesIn(test_positions));

class ChunkedStringMutationTests : public ChunkedStringTests,
                                   public testing::WithParamInterface<size_t> {
protected:
    using Base = ChunkedStringCapacityTests;
    static constexpr auto value = "Irish Wolfhound";

    static void check_modification_to_empty(const chunked_string_t& chunked_string, const std::string_view& view, size_t idx)
    {
        EXPECT_STREQ(chunked_string[idx].c_str(), view.data());
        EXPECT_EQ(view.size(), chunked_string[idx].size());
        EXPECT_GE(chunked_string[idx].capacity(), view.size());
        for (size_t i = 0; i < strings.size(); ++i) {
            if (i == idx) continue;
            EXPECT_TRUE(chunked_string[i].empty());
        }
    }
    static void check_modification_to_empty(const chunked_string_t::chunk& chunk, char ch)
    {
        EXPECT_EQ(ch, chunk.back());
        EXPECT_EQ(chunk.size(), 1);
        EXPECT_GE(chunk.capacity(), 1);
    }
    static void check_modification_to_existing(const chunked_string_t& chunked_string, const std::string_view& view, size_t idx)
    {
        EXPECT_STREQ(chunked_string[idx].c_str(), view.data());
        EXPECT_EQ(chunked_string[idx].size(), view.size());
        for (size_t i = 0; i < strings_count; ++i) {
            if (i == idx) continue;
            EXPECT_STREQ(chunked_string[i].c_str(), strings[i].data());
        }
    }
};

TEST_P(ChunkedStringMutationTests, AssignAppendToEmpty)
{
    const size_t idx = GetParam();
    constexpr std::string_view string = value;
    chunked_string_t chunked_string{};
    chunked_string_t chunked_string2{};

    ASSERT_NO_FATAL_FAILURE(chunked_string[idx] = string);
    ASSERT_NO_FATAL_FAILURE(chunked_string2[idx] += string);

    {
        SCOPED_TRACE("ASSIGN");
        check_modification_to_empty(chunked_string, string, idx);
    }
    check_modification_to_empty(chunked_string2, string, idx);
}

TEST_P(ChunkedStringMutationTests, PushBackToEmpty)
{
    constexpr char ch = 'A';
    const size_t idx = GetParam();
    chunked_string_t chunked_string{};

    chunked_string[idx].push_back(ch);

    SCOPED_TRACE("PUSH_BACK");
    check_modification_to_empty(chunked_string[idx], ch);
}

TEST_P(ChunkedStringMutationTests, AssignToExisting)
{
    auto chunked_string = make_test_chunked_string();
    const size_t idx = GetParam();

    ASSERT_NO_FATAL_FAILURE(chunked_string[idx].assign(value));

    SCOPED_TRACE("ASSIGN");
    check_modification_to_existing(chunked_string, value, idx);
}

TEST_P(ChunkedStringMutationTests, AppendToExisting)
{
    auto chunked_string = make_test_chunked_string();
    const size_t idx = GetParam();
    std::string string{chunked_string[idx]};
    string += value;

    ASSERT_NO_FATAL_FAILURE(chunked_string[idx].append(value));

    SCOPED_TRACE("APPEND");
    check_modification_to_existing(chunked_string, string, idx);
}

TEST_P(ChunkedStringMutationTests, PushBackToExisting)
{
    constexpr char ch = ';';
    const size_t idx = GetParam();
    auto chunked_string = make_test_chunked_string();
    std::string string{chunked_string[idx]};
    string += ';';

    chunked_string[idx].push_back(ch);

    SCOPED_TRACE("PUSH_BACK");
    check_modification_to_existing(chunked_string, string, idx);
}

TEST_P(ChunkedStringMutationTests, EraseBackHalf)
{
    const size_t idx = GetParam();
    const auto erase_count = static_cast<ptrdiff_t>(strings[idx].size() / 2);
    const auto new_size = strings[idx].size() - erase_count;
    const std::string expected_str{strings[idx].substr(0, new_size)};
    auto chunked_string = make_test_chunked_string();
    auto& chunk = chunked_string[idx];

    chunk.erase(chunk.begin() + static_cast<ptrdiff_t>(new_size), chunk.end());

    EXPECT_STREQ(chunk.c_str(), expected_str.data());
    EXPECT_EQ(chunk.size(), expected_str.size());
    EXPECT_GT(chunk.capacity(), expected_str.size());
}

TEST_P(ChunkedStringMutationTests, EraseFrontHalf)
{
    const size_t idx = GetParam();
    const auto erase_count = static_cast<ptrdiff_t>(strings[idx].size() / 2);
    const auto expected_str = strings[idx].substr(erase_count);
    auto chunked_string = make_test_chunked_string();
    auto& chunk = chunked_string[idx];

    chunk.erase(chunk.begin(), chunk.begin() + erase_count);

    EXPECT_STREQ(chunk.c_str(), expected_str.data());
    EXPECT_EQ(chunk.size(), expected_str.size());
    EXPECT_GT(chunk.capacity(), expected_str.size());
}

TEST_P(ChunkedStringMutationTests, Clear)
{
    const size_t idx = GetParam();
    auto chunked_string = make_test_chunked_string();

    chunked_string[idx].clear();

    EXPECT_EQ(chunked_string[idx].size(), 0);
    EXPECT_EQ(chunked_string[idx].capacity(), strings[idx].size());
    for (size_t i = 0; i < strings_count; ++i) {
        if (i == idx) continue;
        EXPECT_STREQ(strings[i].data(), chunked_string[i].c_str());
    }
}

INSTANTIATE_TEST_SUITE_P(Mutation, ChunkedStringMutationTests, testing::ValuesIn(test_positions));

TEST_F(ChunkedStringMutationTests, AssignAppendToEmptyAllChunks)
{
    chunked_string_t chunked_string_ap{};
    chunked_string_t chunked_string_as{};
    for (size_t i = 0; i < strings_count; ++i) {
        chunked_string_as[i].assign(strings[i]);
        chunked_string_ap[i].append(strings[i]);
    }
    for (size_t i = 0; i < strings_count; ++i) {
        {
            auto msg = std::format("ASSIGN_{}", i);
            SCOPED_TRACE(msg.c_str());
            EXPECT_STREQ(chunked_string_as[i].c_str(), strings[i].data());
        }
        auto msg = std::format("APPEND_{}", i);
        SCOPED_TRACE(msg.c_str());
        EXPECT_STREQ(chunked_string_ap[i].c_str(), strings[i].data());
    }
}

TEST_F(ChunkedStringMutationTests, AssignToExistingAllChunks)
{
    auto chunked_string = make_test_chunked_string();
    constexpr std::string_view view = value;
    for (size_t i = 0; i < strings_count; ++i) {
        chunked_string[i].assign(view);
    }
    for (size_t i = 0; i < strings_count; ++i) {
        auto msg = std::format("ASSIGN_{}", i);
        SCOPED_TRACE(msg.c_str());
        EXPECT_STREQ(chunked_string[i], value);
        EXPECT_GE(chunked_string[i].capacity(), view.size());
    }
}

TEST_F(ChunkedStringMutationTests, AppendToExistingAllChunks)
{
    auto chunked_string = make_test_chunked_string();
    std::string string{';'};
    string += value;
    for (size_t i = 0; i < strings_count; ++i) {
        chunked_string[i].append(string);
    }
    for (size_t i = 0; i < strings_count; ++i) {
        std::string string2{strings[i]};
        string2 += string;
        auto msg = std::format("APPEND_{}", i);
        SCOPED_TRACE(msg.c_str());
        EXPECT_STREQ(chunked_string[i].c_str(), string2.c_str());
        EXPECT_GE(chunked_string[i].capacity(), string2.size());
    }
}

TEST_F(ChunkedStringMutationTests, PushBackToEmptyAllChunks)
{
    chunked_string_t chunked_string{};
    constexpr char ch = 'A';
    for (size_t i = 0; i < strings_count; ++i) {
        chunked_string[i].push_back(ch);
    }
    for (size_t idx = 0; idx < strings_count; ++idx) {
        EXPECT_EQ(chunked_string[idx].size(), 1);
        EXPECT_GT(chunked_string[idx].capacity(), 1);
        char val = chunked_string[idx, 0];
        EXPECT_EQ(val, ch);
    }
}

TEST_F(ChunkedStringMutationTests, Insert)
{
    constexpr size_t count = 3;
    constexpr std::array<std::string_view, 3> strings{"Grand Blanc et Noir", "Greater Dog", " Norwegian Elkhound"};
    constexpr std::array<std::string_view, 3> missing{"Anglo-Francais ", "Swiss Mountain ", "Black"};
    constexpr std::array<size_t, 3> insert_pos{6, 8, 0};
    chunked_string<3> chunked_string = std::apply([](auto&&... strs) { return ::chunked_string<3>{strs...}; }, strings);
    chunked_string[0].reserve(chunked_string[0].size() + missing[0].size() + 5);
    chunked_string[1].reserve(chunked_string[1].size() + missing[1].size() / 2);

    for (size_t i = 0; i < count; ++i) {
        std::string str{strings[i]};
        auto& res = chunked_string[i].insert(insert_pos[i], missing[i]);
        auto& exp = str.insert(insert_pos[i], missing[i]);
        ASSERT_STREQ(res.c_str(), exp.c_str());
    }
}

class ChunkedStringIteratorTests : public ChunkedStringTests {};

TEST_F(ChunkedStringIteratorTests, TraverseSingleChunk)
{
    auto chunked_string = make_test_chunked_string();
    std::string str;
    for (size_t i = 0; i < strings_count; ++i) {
        auto iter = chunked_string_t::iterator{chunked_string[i]};
        for (size_t j = 0; j < chunked_string[i].size(); ++j) {
            str += *iter++;
        }
        ASSERT_STREQ(str.c_str(), chunked_string[i].c_str());
        str.clear();
    }
}

TEST_F(ChunkedStringIteratorTests, TraverseChunks)
{
    std::string str;
    str.reserve(strings_concat.size());
    auto chunked_string = make_test_chunked_string();
    auto first = chunked_string.begin();
    auto last = chunked_string.end();

    for (auto it = first; it != last; ++it) {
        str += *it;
    }

    EXPECT_STREQ(str.c_str(), strings_concat.data());
}

TEST_F(ChunkedStringIteratorTests, TraverseSingleChunkBackward)
{
    auto chunked_string = make_test_chunked_string();
    std::string str;
    std::string str_rev;
    for (size_t i = 0; i < strings_count; ++i) {
        auto last = chunked_string_t::iterator{chunked_string[i], static_cast<long int>(chunked_string[i].size())};
        auto first = chunked_string_t::iterator{chunked_string[i]};
        for (auto iter = last - 1; iter != first - 1; --iter) {
            str += *iter;
        }
        str_rev.insert(str_rev.cbegin(), str.crbegin(), str.crend());
        ASSERT_STREQ(str_rev.c_str(), chunked_string[i].c_str());
        str.clear();
        str_rev.clear();
    }
}

TEST_F(ChunkedStringIteratorTests, TraverseChunksBackward)
{
    std::string str;
    std::string str_rev;
    str.reserve(strings_concat.size());
    str_rev.reserve(strings_concat.size());
    auto chunked_string = make_test_chunked_string();

    for (auto iter = chunked_string.end() - 1; iter != chunked_string.begin() - 1; --iter) {
        str += *iter;
    }
    str_rev.insert(str_rev.cbegin(), str.crbegin(), str.crend());

    EXPECT_STREQ(str_rev.c_str(), strings_concat.data());
}

TEST_F(ChunkedStringIteratorTests, Arithmetic)
{
    using diff_t = chunked_string_t::difference_type;
    auto chunked_string = make_test_chunked_string();

    for (size_t i = 0; i < strings_count; ++i) {
        bool size_is_odd = chunked_string[i].size() % 2 != 0;
        auto s = static_cast<diff_t>(chunked_string[i].size()) + size_is_odd;
        auto x = s / 2;
        auto iter = chunked_string_t::iterator{chunked_string[i]} + x;

        ASSERT_EQ(*iter, chunked_string[i].at(x));
        ASSERT_EQ(*(iter - x), *chunked_string_t::iterator{chunked_string[i]});

        if (i < strings_count - 1) {
            auto y = s - x;
            iter += y;
            auto& elem = chunked_string[i + 1].at(y - x + size_is_odd);
            auto& prev_elem = chunked_string[i].at(x);

            ASSERT_EQ(*iter, elem);
            ASSERT_EQ(prev_elem, *(iter - y));
        }
    }
}

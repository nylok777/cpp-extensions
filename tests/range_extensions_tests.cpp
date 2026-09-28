// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 nylok777 (https://github.com/nylok777)

//
// Created by david on 3/20/26.
//

#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <unordered_set>
#include <random>

#include <gtest/gtest.h>

#include "ext_ranges.hpp"
#include "ext_traits.hpp"

namespace eager_views = ext::ranges::eager_views;
namespace ranges_func = ext::functional;

template<typename V, template<typename...> class... Rs> using range_types_tmplt = testing::Types<Rs<V>...>;

using range_types = range_types_tmplt<int, std::vector, std::deque, std::list, std::forward_list, std::set, std::unordered_set>;

template<typename T> concept map_type = requires { typename T::mapped_type; };

template<typename T> concept has_key_type = requires { typename T::key_type; };

template<typename T, int MaxValue = std::numeric_limits<int8_t>::max()> class RangeTestsBase {
public:
    using RangeValue = std::ranges::range_value_t<T>;
    static constexpr int range_length = 20;
    static constexpr int max_value = MaxValue;
    T range;

    RangeTestsBase() requires(!map_type<T>)
    {
        std::mt19937 gen{std::random_device{}()};
        std::uniform_int_distribution<> rand{1, max_value};
        if constexpr (!std::ranges::sized_range<T>) {
            for (int i = 0; i < max_value; ++i) range.push_front(rand(gen));
        }
        else if constexpr (requires { typename T::key_compare; } || requires { typename T::hasher; }) {
            for (int i = 1; i <= max_value; ++i) range.insert(i);
        }
        else if constexpr (std::ranges::bidirectional_range<T>) {
            for (int i = 0; i < max_value; ++i) range.push_back(rand(gen));
        }
    }
    RangeTestsBase() requires map_type<T>
    {
        std::mt19937 gen{std::random_device{}()};
        std::uniform_real_distribution<float> rand_val{};

        for (int i = 0; i < range_length; ++i) {
            range.emplace(i, rand_val(gen));
        }
    }
};

template<typename T>
class EagerViewsTests : public testing::Test,
                        public RangeTestsBase<T, 20> {
public:
    using Base = RangeTestsBase<T, 20>;
    using Range = std::views::all_t<T>;
    using Base::max_value;
    using Base::range_length;
};

TYPED_TEST_SUITE(EagerViewsTests, range_types);

template<typename T>
class RangeFunctionsTests : public testing::Test,
                            public RangeTestsBase<T> {
public:
    using Range = T;
    using RangeValue = std::ranges::range_value_t<T>;
    template<typename U = std::string> using TargetRange = std::conditional_t<has_key_type<T>, std::vector<U>, std::set<U>>;
};

TYPED_TEST_SUITE(RangeFunctionsTests, range_types);

TYPED_TEST(EagerViewsTests, OrderedViewTest)
{
    ASSERT_FALSE(std::ranges::empty(this->range));
    auto view = eager_views::order(this->range, std::less<>{});
    EXPECT_TRUE(std::ranges::is_sorted(view));
    auto view2 = eager_views::order(this->range.begin(), this->range.end(), std::less<>{});
    EXPECT_TRUE(std::ranges::is_sorted(view2));
}

TYPED_TEST(EagerViewsTests, FilterViewTest)
{
    ASSERT_FALSE(std::ranges::empty(this->range));
    auto fn = [](auto x) { return x % 2 == 0; };
    auto view = eager_views::filter(this->range, fn);
    auto view2 = eager_views::filter(this->range.begin(), this->range.end(), fn);
    auto view_ref = std::views::filter(this->range, fn);
    EXPECT_TRUE(std::ranges::is_permutation(view, view_ref));
    EXPECT_TRUE(std::ranges::is_permutation(view2, view_ref));
}

TYPED_TEST(EagerViewsTests, SelectionViewTest)
{
    bool is_empty = std::ranges::empty(this->range);
    ASSERT_FALSE(is_empty);
    constexpr int max_value = TestFixture::max_value;
    auto fn = [k = 0]() mutable { return k += 3; };
    constexpr size_t idx_num = max_value / 3;
    std::array<int, idx_num> indices{};
    std::ranges::generate(indices, fn);

    std::array<int, idx_num> array{};
    auto a_iter = array.begin();
    for (int i = 3; i < std::ranges::distance(this->range); i += 3) {
        *a_iter = *std::ranges::next(this->range.begin(), i);
        ++a_iter;
    }

    auto view = eager_views::take_at(this->range, indices);
    auto view2 = eager_views::take_at(this->range.begin(), this->range.end(), indices);
    auto is_perm = std::ranges::is_permutation(view, array);
    auto is_perm2 = std::ranges::is_permutation(view2, array);
    EXPECT_TRUE(is_perm);
    EXPECT_TRUE(is_perm2);
}

TYPED_TEST(EagerViewsTests, SampleViewTest)
{
    ASSERT_FALSE(std::ranges::empty(this->range));
    constexpr size_t count = 5;
    std::mt19937 gen{42};
    std::vector<int> ref(count);
    std::ranges::sample(this->range, ref.begin(), count, gen);
    gen = std::mt19937{42};

    auto view = eager_views::sample(this->range, count, gen);
    gen = std::mt19937{42};

    auto view2 = eager_views::sample(this->range.begin(), this->range.end(), count, gen);
    EXPECT_TRUE(std::ranges::is_permutation(view, ref));
}

TYPED_TEST(EagerViewsTests, ConcatViewTest)
{
    ASSERT_FALSE(std::ranges::empty(this->range));
    auto range2 = this->range;
    auto view = eager_views::concat(this->range, range2);
    EXPECT_EQ(std::ranges::distance(view), std::ranges::distance(this->range) * 2);
    auto it = ++view.begin();
    auto it2 = std::ranges::next(it, std::ranges::distance(this->range));
    EXPECT_EQ(*it, *it2);
}

TYPED_TEST(RangeFunctionsTests, MapTest)
{
    auto fn = [](auto x) -> std::string { return std::to_string(x); };

    ext::rebind_t<typename TestFixture::Range, std::string> res;
    typename TestFixture::template TargetRange<> res2;
    ASSERT_NO_FATAL_FAILURE(res = ranges_func::map(this->range, fn));
    ASSERT_NO_FATAL_FAILURE(res2 = ranges_func::map<ext::extract_template<decltype(res2)>::template type>(this->range, fn));

    typename TestFixture::template TargetRange<> tr_res;
    std::ranges::transform(this->range, std::inserter(tr_res, tr_res.end()), fn);
    EXPECT_TRUE(std::ranges::is_permutation(res2, tr_res));
}

TYPED_TEST(RangeFunctionsTests, FilterTest)
{
    auto fn = [](std::integral auto x) -> bool { return x % 2 == 0; };
    typename TestFixture::Range res;
    typename TestFixture::template TargetRange<int> res2;

    ASSERT_NO_FATAL_FAILURE(res = ranges_func::filter(this->range, fn));
    ASSERT_NO_FATAL_FAILURE(res2 = ranges_func::filter<ext::extract_template<decltype(res2)>::template type>(this->range, fn));

    decltype(res2) res3;
    std::ranges::copy_if(this->range, std::inserter(res3, res3.end()), fn);
    EXPECT_TRUE(std::ranges::is_permutation(res2, res3));
}

TYPED_TEST(RangeFunctionsTests, FilterMapTest)
{
    using Range = TestFixture::Range;
    using TargetRange = TestFixture::template TargetRange<>;

    auto pred = [](std::integral auto x) -> bool { return x % 2 == 0; };
    auto fn = [](std::integral auto x) -> std::string { return std::to_string(x); };
    ext::rebind_t<Range, std::string> res;
    TargetRange res2;
    ASSERT_NO_FATAL_FAILURE(res = ranges_func::filter_map(this->range, pred, fn));
    ASSERT_NO_FATAL_FAILURE(res2 = ranges_func::filter_map<ext::extract_template<TargetRange>::template type>(this->range, pred, fn));
}

TYPED_TEST(RangeFunctionsTests, ConcatenateTest)
{
    using Range = TestFixture::Range;
    using TargetRange = TestFixture::template TargetRange<int>;
    Range in = this->range;
    Range in2 = this->range;
    Range res;
    ASSERT_NO_FATAL_FAILURE(res = ranges_func::concatenate(this->range, in, in2));

    TargetRange in3;
    TargetRange in4;
    std::ranges::copy(this->range, std::inserter(in3, in3.end()));
    std::ranges::copy(this->range, std::inserter(in4, in4.end()));
    Range res2;
    ASSERT_NO_FATAL_FAILURE(res2 = ranges_func::concatenate(this->range, in3, in4));
}

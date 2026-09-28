// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 nylok777 (https://github.com/nylok777)

//
// Created by david on 7/14/26.
//
#include <random>
#include <array>
#include <deque>
#include <forward_list>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "ext_ranges.hpp"
namespace ext_ranges = ext::ranges;
namespace eager_views = ext_ranges::eager_views;

using std::views::all_t;
using kv_pair = std::pair<char32_t, double>;

static constexpr auto divisible_by_2 = [](std::regular auto x) { return x % 2 == 0; };
static constexpr auto key_is_skull_emoji = [](auto x) { return x.first == U'💀'; };

template<typename... Args> static constexpr bool view_well_formed()
{
    return requires { (sizeof(Args) && ...); };
}

template<typename Values, template<typename, typename...> class View, typename Callable, template<typename...> class... Ranges>
static constexpr bool view_well_formed()
{
    if constexpr (requires { typename std::tuple_element_t<1, Values>; })
        return view_well_formed<View<all_t<Ranges<std::tuple_element_t<0, Values>, std::tuple_element_t<1, Values>>>, Callable>...>();
    else return view_well_formed<View<all_t<Ranges<Values>>, Callable>...>();
}

template<typename Values, template<typename, typename...> class View, template<typename, typename...> class Range, typename... Args>
static constexpr bool view_well_formed()
{
    if constexpr (requires { typename std::tuple_element_t<1, Values>; }) {
        using key = std::tuple_element_t<0, Values>;
        using value = std::tuple_element_t<1, Values>;
        return (view_well_formed<View<all_t<Range<key, value>>, Args...>>());
    }
    else {
        return (view_well_formed<View<all_t<Range<Values>>, Args...>>());
    }
}

template<typename Values, template<typename, typename...> class View, template<typename...> class... Ranges>
static constexpr bool view_well_formed()
{
    if constexpr (requires { typename std::tuple_element_t<1, Values>; })
        return view_well_formed<View<all_t<Ranges<std::tuple_element_t<0, Values>, std::tuple_element_t<1, Values>>>>...>();
    else return view_well_formed<View<all_t<Ranges<Values>>>...>();
}

template<template<typename...> class... Templates> struct type_list {};

template<size_t Idx, typename T> struct type_list_element;

template<template<typename...> class Head, template<typename...> class... Tail> struct type_list_element<0, type_list<Head, Tail...>> {
    template<typename... Args> using type = Head<Args...>;
};

template<size_t Idx, template<typename...> class Head, template<typename...> class... Tail>
struct type_list_element<Idx, type_list<Head, Tail...>> {
    template<typename... Args> using type = type_list_element<Idx - 1, type_list<Tail...>>::template type<Args...>;
};

template<size_t Idx, typename T, typename... Args> using type_list_element_t = type_list_element<Idx, T>::template type<Args...>;

template<typename T> struct type_list_size;

template<template<typename...> class... Ts> struct type_list_size<type_list<Ts...>> {
    static constexpr size_t value = sizeof...(Ts);
};

template<typename T> static constexpr size_t type_list_size_v = type_list_size<T>::value;

using non_map_ranges = type_list<std::vector, std::deque, std::list, std::forward_list, std::set, std::unordered_set>;
using map_ranges = type_list<std::map, std::unordered_map>;

template<typename Value, typename TpList, template<typename, typename...> class View, typename... Args>
static consteval bool view_well_formed()
{
    return []<size_t... Is>(std::index_sequence<Is...>) constexpr {
        return (view_well_formed<Value, View, Args..., type_list_element<Is, TpList>::template type...>());
    }(std::make_index_sequence<type_list_size_v<TpList>>{});
}

template<typename Value, typename TpList> static consteval bool concat_view_well_formed()
{
    return []<size_t... Is>(std::index_sequence<Is...>) consteval {
        if constexpr (requires { typename std::tuple_element_t<1, Value>; }) {
            using key = std::tuple_element_t<0, Value>;
            using value = std::tuple_element_t<1, Value>;
            return (view_well_formed<Value, eager_views::concat_view, type_list_element<Is, TpList>::template type,
                                     all_t<type_list_element_t<Is, TpList, key, value>>, all_t<type_list_element_t<Is, TpList, key, value>>,
                                     all_t<type_list_element_t<Is, TpList, key, value>>>()
                    && ...);
        }
        else {
            return (view_well_formed<Value, eager_views::concat_view, type_list_element<Is, TpList>::template type,
                                     all_t<type_list_element_t<Is, TpList, Value>>, all_t<type_list_element_t<Is, TpList, Value>>,
                                     all_t<type_list_element_t<Is, TpList, Value>>>()
                    && ...);
        }
    }(std::make_index_sequence<type_list_size_v<TpList>>{});
}

template<auto FuncObject, typename Range, typename... Args> static constexpr bool function_object_callable_impl()
{
    return requires { FuncObject(std::declval<Range&>(), std::declval<Args>()...); };
}

template<auto FuncObject, typename TpList, typename Values, typename... Args> static constexpr bool function_object_callable()
{
    return []<size_t... Is>(std::index_sequence<Is...>) constexpr {
        if constexpr (requires { typename std::tuple_element_t<1, Values>; }) {
            using key = std::tuple_element_t<0, Values>;
            using value = std::tuple_element_t<1, Values>;
            return (function_object_callable_impl<FuncObject, type_list_element_t<Is, TpList, key, value>, Args...>() && ...);
        }
        else {
            return (function_object_callable_impl<FuncObject, type_list_element_t<Is, TpList, Values>, Args...>() && ...);
        }
    }(std::make_index_sequence<type_list_size_v<TpList>>{});
}

template<auto FuncObject, typename TpList, typename Values> static constexpr bool concat_function_object_callable()
{
    return []<size_t... Is>(std::index_sequence<Is...>) constexpr {
        if constexpr (requires { typename std::tuple_element_t<1, Values>; }) {
            using key = std::tuple_element_t<0, Values>;
            using value = std::tuple_element_t<1, Values>;
            return (function_object_callable_impl<FuncObject, type_list_element_t<Is, TpList, key, value>&,
                                                  type_list_element_t<Is, TpList, key, value>&>()
                    && ...);
        }
        else {
            return (function_object_callable_impl<FuncObject, type_list_element_t<Is, TpList, Values>&, type_list_element_t<Is, TpList, Values>&>()
                    && ...);
        }
    }(std::make_index_sequence<type_list_size_v<TpList>>{});
}

static_assert(view_well_formed<int, non_map_ranges, eager_views::ordered_view, std::less<>>());
static_assert(view_well_formed<int, non_map_ranges, eager_views::filter_view, decltype(divisible_by_2)>());
static_assert(view_well_formed<int, non_map_ranges, eager_views::selection_view>());
static_assert(view_well_formed<int, non_map_ranges, eager_views::sample_view, std::mt19937>());
static_assert(concat_view_well_formed<int, non_map_ranges>());
static_assert(view_well_formed<kv_pair, map_ranges, eager_views::ordered_view, std::less<>>());
static_assert(view_well_formed<kv_pair, map_ranges, eager_views::filter_view, decltype(key_is_skull_emoji)>());
static_assert(view_well_formed<kv_pair, map_ranges, eager_views::selection_view>());
static_assert(view_well_formed<kv_pair, map_ranges, eager_views::sample_view, std::mt19937>());
static_assert(concat_view_well_formed<kv_pair, map_ranges>());

static_assert(function_object_callable<eager_views::order, non_map_ranges, int, std::less<>>());
static_assert(function_object_callable<eager_views::filter, non_map_ranges, int, decltype(divisible_by_2)>());
static_assert(function_object_callable<eager_views::sample, non_map_ranges, int, unsigned, std::mt19937>());
static_assert(function_object_callable<eager_views::take_at, non_map_ranges, int, std::vector<int>>());
static_assert(concat_function_object_callable<eager_views::concat, non_map_ranges, int>());

int main() {}

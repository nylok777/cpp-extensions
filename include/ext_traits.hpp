//
// Created by david on 7/2/26.
//

#ifndef CPP_EXTENSIONS_TRAITS_HPP
#define CPP_EXTENSIONS_TRAITS_HPP
#include <concepts>
#include <type_traits>

namespace ext
{
template<typename T, typename... Args> struct rebind;

template<template<typename...> class T, typename... OldArgs, typename... NewArgs>
struct rebind<T<OldArgs...>, NewArgs...> {
    using type = T<NewArgs...>;
};

template<typename T, typename... Args> using rebind_t = rebind<T, Args...>::type;

template<typename T> struct add_const_reference {
    using type = std::add_lvalue_reference_t<std::add_const_t<T>>;
};

template<typename T> using add_const_reference_t = add_const_reference<T>::type;

template<typename T> struct extract_template;

template<template<typename> class Template, typename T> struct extract_template<Template<T>> {
    template<typename Y> using type = Template<Y>;
};

template<template<typename...> class Template, typename... Args>
struct extract_template<Template<Args...>> {
    template<typename... NewArgs> using type = Template<NewArgs...>;
};

template<template<typename, auto> class Template, typename T, auto V>
struct extract_template<Template<T, V>> {
    template<typename J, auto W> using type = Template<J, W>;
};

template<template<typename, auto, typename...> class Template, typename T, auto V, typename... Args>
struct extract_template<Template<T, V, Args...>> {
    template<typename J, auto W, typename... NewArgs> using type = Template<J, W, NewArgs...>;
};

namespace detail
{
template<std::integral T, typename IntSeq> struct make_reverse_integer_sequence_helper;

template<std::integral T, T... Ints> struct make_reverse_integer_sequence_helper<T, std::integer_sequence<T, Ints...>> {
    using type = std::integer_sequence<T, (sizeof...(Ints) - 1 - Ints)...>;
};
} // namespace detail

template<std::integral T, T N>
using make_reverse_integer_sequence = detail::make_reverse_integer_sequence_helper<T, std::make_integer_sequence<T, N>>::type;
template<size_t N> using make_reverse_index_sequence = make_reverse_integer_sequence<size_t, N>::type;

static_assert(std::same_as<make_reverse_integer_sequence<int, 5>, std::integer_sequence<int, 4, 3, 2, 1, 0>>);

template<typename T> struct identity_element;

template<std::integral T> struct identity_element<T> {
    static constexpr T value = T{0};
};

template<typename T> inline constexpr T identity_element_v = identity_element<T>::value;

template<auto Value> inline constexpr auto inverse_element_v = -Value;

template<auto Lhs, auto Rhs> inline constexpr bool equals_v = Lhs == Rhs;

template<typename T = char> requires std::copyable<T>
using copyable_type = T;

struct move_only_type {
    move_only_type() = default;
    move_only_type(move_only_type&&) noexcept = default;
    move_only_type& operator=(move_only_type&&) noexcept = default;
    move_only_type(const move_only_type&) = delete;
    move_only_type& operator=(const move_only_type&) = delete;
    char data;
};

}

#endif // CPP_EXTENSIONS_TRAITS_HPP

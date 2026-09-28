// SPDX-License-Identifier: BSD-3-Clause
// Copyright 2026 nylok777 (https://github.com/nylok777)

//
// Created by david on 2026. 09. 28..
//

#ifndef CPPEXTENSIONS_MONAD_HPP
#define CPPEXTENSIONS_MONAD_HPP
#if __cplusplus <= 202002l
#include <optional>
#endif

#include "ext_traits.hpp"

namespace ext
{
template<typename M>
struct monad_traits
{
    using value_type = M::value_type;

    static constexpr bool has_value(const M& monad) { return static_cast<bool>(monad); }

    static constexpr decltype(auto) value(auto&& monad)
    {
        return std::forward<decltype(monad)>(monad).value();
    }

    template<std::invocable F> requires std::copy_constructible<value_type>
    static constexpr auto or_else(auto&& monad, F&& f)
    {
        return std::forward<decltype(monad)>(monad).or_else(std::forward < F > (f));
    }

    template<std::invocable F> requires std::move_constructible<value_type>
    static constexpr auto or_else(auto&& monad, F&& f)
    {
        return std::move(std::forward<decltype(monad)>(monad)).or_else(std::forward < F > (f));
    }

    template<class F> requires std::invocable<F, value_type>
    static constexpr auto and_then(auto&& monad, F&& f)
    {
        return std::forward<decltype(monad)>(monad).and_then(std::forward<F>(f));
    }

    template<class F> requires std::invocable<F, value_type>
    static constexpr auto transform(auto&& monad, F&& f)
    {
        return std::forward<decltype(monad)>(monad).transform(std::forward<F>(f));
    }
};

template<typename T>
struct error_monad_traits : monad_traits<T>
{
    using value_type = monad_traits<T>::value_type;
    using error_type = T::error_type;

    static constexpr decltype(auto) error(auto&& monad)
    {
        return std::forward<decltype(monad)>(monad).error();
    }

    template<class F> requires std::invocable<F, error_type> && std::copy_constructible<T>
    static constexpr auto transform_error(auto&& monad, F&& f)
    {
        return std::forward<decltype(monad)>(monad).transform_error(std::forward<F>(f));
    }

    template<class F> requires std::invocable<F, error_type> && std::move_constructible<T>
    static constexpr auto transform_error(auto&& monad, F&& f)
    {
        return std::move(std::forward<decltype(monad)>(monad)).transform_error(std::forward<F>(f));
    }
};

template<typename T, typename...E>
concept monadic = requires(T& t)
{
    typename monad_traits<T>::value_type;
    { monad_traits<T>::has_value(t) } -> std::same_as<bool>;
    { monad_traits<T>::value(t) } -> std::same_as<typename monad_traits<T>::value_type&>;
    {
        monad_traits<T>::or_else(t, [](auto&&...) -> std::remove_cvref_t<T> { return std::remove_cvref_t<T>(); })
    } -> std::same_as<std::remove_reference_t<T>>;
    {
        monad_traits<T>::and_then(t, [](auto&&) -> rebind_t<T, char, E...> { return rebind_t<T, char, E...>(); })
    } -> std::same_as<rebind_t<T, char, E...>>;
    {
        monad_traits<T>::transform(t, [](auto&&) -> char { return char(); })
    } -> std::same_as<rebind_t<T, char, E...>>;
};

template<typename T>
concept error_monad = monadic<T, typename error_monad_traits<T>::error_type> && requires(T t)
{
    typename error_monad_traits<T>::error_type;
    { error_monad_traits<T>::error(t) } -> std::convertible_to<typename error_monad_traits<T>::error_type>;
    {
        error_monad_traits<T>::transform_error(t, [](auto&&) { return char(); })
    } -> std::same_as<rebind_t<T, typename error_monad_traits<T>::value_type, char>>;
};

#if __cplusplus <= 202002l
template<typename T>
struct monad_traits<std::optional<T>>
{
    using value_type = std::optional<T>::value_type;

    static constexpr bool has_value(const std::optional<T>& opt) { return opt.has_value(); }

    static constexpr decltype(auto) value(auto&& opt)
    {
        return std::forward<decltype(opt)>(opt).value();
    }

    template<std::invocable F>
    static constexpr auto or_else(auto&& opt, F&& f)
        requires std::copy_constructible<std::optional<T>>
    {
        using invoke_res = std::invoke_result_t<F>;
        static_assert(
            std::same_as<std::remove_cvref_t<invoke_res>, std::optional<T>>,
            "function passed as argument must return an " "std::optional<T>");
        if (!static_cast<const std::optional<T>&>(std::forward<decltype(opt)>(opt)).has_value())
            return std::forward < F > (f)();

        return std::forward<decltype(opt)>(opt);
    }

    template<std::invocable F>
    static constexpr auto or_else(auto&& opt, F&& f)
        requires std::move_constructible<std::optional<T>>
    {
        using invoke_res = std::invoke_result_t<F>;
        static_assert(
            std::same_as<std::remove_cvref_t<invoke_res>, std::optional<T>>,
            "function passed as argument must return an " "std::optional<T>");
        if (!static_cast<const std::optional<T>&>(std::forward<decltype(opt)>(opt)).has_value())
            return std::forward < F > (f)();

        return std::move(std::forward<decltype(opt)>(opt));
    }

    template<typename F>
    static constexpr decltype(auto) and_then(auto&& opt, F&& f)
        requires std::same_as<std::remove_cvref_t<decltype(opt)>, std::optional<T>>
        && std::invocable<F, value_type>
    {
        using invoke_res = std::invoke_result_t<F, value_type>;
        static_assert(
            std::same_as<rebind_t<invoke_res, value_type>, std::optional<T>>,
            "function passed as argument must return an " "std::optional");
        if (static_cast<const std::optional<T>&>(std::forward<decltype(opt)>(opt)).has_value())
            return std::forward<F>(f)(std::forward<decltype(opt)>(opt).value());

        return std::invoke_result_t<F, value_type>{std::nullopt};
    }

    template<typename F>
    static constexpr decltype(auto) transform(auto&& opt, F&& f)
        requires std::same_as<std::remove_cvref_t<decltype(opt)>, std::optional<T>>
        && std::invocable<F, value_type>
    {
        if (static_cast<const std::optional<T>&>(std::forward<decltype(opt)>(opt)).has_value())
            return std::make_optional(std::forward<F>(f)(std::forward<decltype(opt)>(opt).value()));

        return std::optional<std::invoke_result_t<F, value_type>>{std::nullopt};
    }
};
#endif
}
#endif //CPPEXTENSIONS_MONAD_HPP

//
// Created by david on 7/2/26.
//

#ifndef CPP_EXTENSIONS_TRAITS_HPP
#define CPP_EXTENSIONS_TRAITS_HPP
#include <concepts>
#include <type_traits>
#if __cplusplus <= 202002l
#include <optional>
#endif

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

template<typename T = std::byte> requires std::copyable<T>
using copyable_type = T;

template<typename T = std::unique_ptr<std::byte>> requires(!std::copyable<T>) && std::movable<T>
using move_only_type = T;

template<typename M> struct monad_traits {
    using value_type = M::value_type;

    static constexpr bool has_value(const M& monad) { return static_cast<bool>(monad); }
    static constexpr decltype(auto) value(auto&& monad)
    {
        return std::forward<decltype(monad)>(monad).value();
    }

    template<std::invocable F> requires std::copy_constructible<value_type>
    static constexpr auto or_else(auto&& monad, F&& f)
    {
        return std::forward<decltype(monad)>(monad).or_else(std::forward<F>(f));
    }

    template<std::invocable F> requires std::move_constructible<value_type>
    static constexpr auto or_else(auto&& monad, F&& f)
    {
        return std::move(std::forward<decltype(monad)>(monad)).or_else(std::forward<F>(f));
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

template<typename T> struct error_monad_traits : monad_traits<T> {
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

#if __cplusplus <= 202002l
template<typename T> struct monad_traits<std::optional<T>> {
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
        static_assert(std::same_as<std::remove_cvref_t<invoke_res>, std::optional<T>>,
                      "function passed as argument must return an " "std::optional<T>");
        if (!static_cast<const std::optional<T>&>(std::forward<decltype(opt)>(opt)).has_value())
            return std::forward<F>(f)();

        return std::forward<decltype(opt)>(opt);
    }

    template<std::invocable F>
    static constexpr auto or_else(auto&& opt, F&& f)
        requires std::move_constructible<std::optional<T>>
    {
        using invoke_res = std::invoke_result_t<F>;
        static_assert(std::same_as<std::remove_cvref_t<invoke_res>, std::optional<T>>,
                      "function passed as argument must return an " "std::optional<T>");
        if (!static_cast<const std::optional<T>&>(std::forward<decltype(opt)>(opt)).has_value())
            return std::forward<F>(f)();

        return std::move(std::forward<decltype(opt)>(opt));
    }

    template<typename F>
    static constexpr decltype(auto) and_then(auto&& opt, F&& f)
        requires std::same_as<std::remove_cvref_t<decltype(opt)>, std::optional<T>>
                 && std::invocable<F, value_type>
    {
        using invoke_res = std::invoke_result_t<F, value_type>;
        static_assert(std::same_as<rebind_t<invoke_res, value_type>, std::optional<T>>,
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

#endif // CPP_EXTENSIONS_TRAITS_HPP

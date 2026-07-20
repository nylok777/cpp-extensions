//
// Created by david on 12/10/25.
//

#ifndef RANGES_EXT_HPP
#define RANGES_EXT_HPP
#include <algorithm>
#include <memory>
#include <ranges>

#include "ext_concepts.hpp"
#include "ext_ranges.hpp"

namespace ext::ranges
{
using namespace std::string_literals;

template<std::ranges::forward_range Range>
constexpr std::integral auto size_or_distance(Range&& range)
{
    if constexpr (std::ranges::sized_range<Range>)
        return std::ranges::size(std::forward<Range>(range));
    else return std::ranges::distance(std::forward<Range>(range));
}

template<std::ranges::input_range Range, typename Out>
requires std::output_iterator<Out, std::ranges::range_value_t<Range>>
constexpr void move_or_copy(Range&& range, Out out)
{
    if constexpr (std::is_rvalue_reference_v<Range>)
        std::ranges::move(std::forward<Range>(range), out);
    else std::ranges::copy(std::forward<Range>(range), out);
}

template<std::ranges::forward_range In, typename Out>
constexpr void move_n(In&& inp, Out out, size_t n)
{
    auto start = std::ranges::begin(std::forward<In>(inp));
    auto end = std::ranges::next(start, n);
    for (auto it = start; it != end; ++it) {
        *out++ = std::move(*it);
    }
}

template<std::ranges::bidirectional_range Range, typename T>
requires std::convertible_to<T, std::ranges::range_value_t<Range>>
constexpr void append(Range& range, T&& value)
{
    std::back_inserter(range) = std::forward<T>(value);
}

template<std::ranges::bidirectional_range Range, std::ranges::bidirectional_range... RangeN>
constexpr void append(Range& r1, RangeN&&... rn)
{
    if constexpr (std::is_rvalue_reference_v<RangeN...>)
        (std::ranges::move(rn, std::inserter(r1, std::ranges::end(r1))), ...);
    else (std::ranges::copy(rn, std::inserter(r1, std::ranges::end(r1))), ...);
}

template<std::ranges::forward_range Range>
constexpr void prepend(Range& range, std::convertible_to<std::ranges::range_value_t<Range>> auto&& value)
{
    std::front_inserter(range) = std::forward<decltype(value)>(value);
}

template<std::ranges::forward_range Range>
constexpr void insert(Range& range, std::convertible_to<std::ranges::range_value_t<Range>> auto&& value)
{
    std::inserter(range, std::ranges::end(range)) = std::forward<decltype(value)>(value);
}

template<std::ranges::forward_range InRange, std::ranges::forward_range Out>
constexpr void combine(InRange&& range, Out& out)
{
    if constexpr (back_insert_range<Out>) {
        move_or_copy(std::forward<InRange>(range), std::back_inserter(out));
    }
    else if constexpr (front_insert_range<Out>) {
        move_or_copy(std::forward<InRange>(range), std::front_inserter(out));
    }
    else {
        move_or_copy(std::forward<InRange>(range), std::inserter(out, std::ranges::end(out)));
    }
}

namespace eager_views
{
template<std::ranges::random_access_range Range, bool IsConst> class eager_iterator {
    using ra_iter
        = std::conditional_t<IsConst, std::ranges::iterator_t<const Range>, std::ranges::iterator_t<Range>>;
    using base_iter = std::iter_value_t<ra_iter>;
    ra_iter current;

public:
    using iterator_tag = std::random_access_iterator_tag;
    using difference_type = std::ranges::range_difference_t<Range>;
    using value_type = std::iter_value_t<base_iter>;
    using size_type = std::ranges::range_size_t<Range>;
    using reference = std::iter_reference_t<base_iter>;
    using const_reference = const std::iter_reference_t<base_iter>;
    using pointer = base_iter;

    eager_iterator() = default;
    explicit constexpr eager_iterator(ra_iter it) : current(it) {}

    constexpr reference operator*() { return **current; }
    constexpr pointer operator->() { return *current; }
    constexpr const_reference operator*() const { return **current; }
    constexpr pointer operator->() const { return *current; }

    constexpr eager_iterator& operator++()
    {
        ++current;
        return *this;
    }

    constexpr eager_iterator operator++(int)
    {
        auto temp = *this;
        ++(*this);
        return temp;
    }

    constexpr eager_iterator& operator--()
    {
        --current;
        return *this;
    }

    constexpr eager_iterator operator--(int)
    {
        auto temp = *this;
        --(*this);
        return temp;
    }

    constexpr eager_iterator& operator+=(const difference_type rhs)
    {
        current += rhs;
        return *this;
    }

    constexpr friend eager_iterator operator+(const eager_iterator& lhs, const difference_type rhs)
    {
        auto temp = lhs;
        temp += rhs;
        return temp;
    }

    constexpr friend eager_iterator operator+(const difference_type rhs, const eager_iterator& lhs)
    {
        return lhs + rhs;
    }

    constexpr eager_iterator& operator-=(const difference_type rhs)
    {
        current -= rhs;
        return *this;
    }

    constexpr friend eager_iterator operator-(const eager_iterator& lhs, const difference_type rhs)
    {
        auto temp = lhs;
        temp -= rhs;
        return temp;
    }

    constexpr friend difference_type operator-(const eager_iterator& lhs, const eager_iterator& rhs)
    {
        return lhs.current - rhs.current;
    }

    constexpr reference operator[](const size_type idx) { return **(current + idx); }

    constexpr const_reference operator[](const size_type idx) const { return **(current + idx); }

    auto operator<=>(const eager_iterator&) const = default;
};

template<std::ranges::forward_range V, template<typename...> class ContainerTp = std::vector>
requires indexed_sequence<ContainerTp<copyable_type<>>> && reservable<ContainerTp<copyable_type<>>>
class eager_view_base : public std::ranges::view_interface<eager_view_base<V>> {
public:
    using base_iterator_type = std::ranges::iterator_t<V>;
    using container_type = ContainerTp<base_iterator_type>;
    using iterator = eager_iterator<container_type, false>;
    using const_iterator = eager_iterator<container_type, true>;
    using reverse_iterator = std::reverse_iterator<iterator>;

    static_assert(std::random_access_iterator<iterator>);
    static_assert(std::random_access_iterator<const_iterator>);

    constexpr eager_view_base() : iters_(std::make_shared<container_type>()) {}

    constexpr iterator begin() { return iterator(iters_->begin()); }
    constexpr iterator end() { return iterator(iters_->end()); }
    constexpr const_iterator begin() const { return const_iterator{iters_->begin()}; }
    constexpr const_iterator end() const { return const_iterator{iters_->end()}; }
    constexpr const_iterator cbegin() { return const_iterator{iters_->cbegin()}; }
    constexpr const_iterator cend() { return const_iterator{iters_->end()}; }
    constexpr reverse_iterator rbegin() { return reverse_iterator{end()}; }
    constexpr reverse_iterator rend() { return reverse_iterator{begin()}; }
    constexpr auto erase(iterator first, iterator last)
    {
        iters_->erase(first.current, last.current);
    }

protected:
    container_type& base() { return *iters_; }

private:
    std::shared_ptr<container_type> iters_;
};

static_assert(
    std::ranges::random_access_range<eager_view_base<std::ranges::views::all_t<std::vector<int>>>>);

namespace detail
{
template<template<typename...> class V, typename... Args>
concept view_possible = requires { V<Args...>{std::declval<Args>()...}; };

template<typename V, typename... Args>
concept view_possible_tmplt_ctor = requires { V{std::declval<Args>()...}; };

template<std::ranges::view V> requires std::ranges::forward_range<V>
class iterator_view : public std::ranges::view_interface<iterator_view<V>> {
    V view_;

public:
    class iterator {
    public:
        using iterator_tag = std::forward_iterator_tag;
        using value_type = std::ranges::iterator_t<V>;
        using difference_type = std::ranges::range_difference_t<V>;
        using reference = value_type&;
        using const_reference = const value_type&;
        using pointer = value_type*;

    private:
        value_type iter_;

    public:
        iterator() = default;
        iterator(value_type it) : iter_(it) {}

        auto operator++() -> iterator&
        {
            ++iter_;
            return *this;
        }

        auto operator++(int) -> iterator
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }

        auto operator*() const -> const_reference { return iter_; }

        bool operator==(const iterator& other) const { return other.iter_ == iter_; }
    };

    iterator_view() = default;
    explicit iterator_view(const V& v) : view_(v) {}

    auto begin() -> iterator { return {std::ranges::begin(view_)}; }
    auto end() -> iterator { return {std::ranges::end(view_)}; }
};

static_assert(std::forward_iterator<iterator_view<std::ranges::views::all_t<std::vector<int>>>::iterator>);
static_assert(std::ranges::forward_range<iterator_view<std::ranges::views::all_t<std::vector<int>>>>);
} // namespace detail

template<std::ranges::forward_range V, class Gen>
requires std::ranges::view<V> && std::uniform_random_bit_generator<std::remove_reference_t<Gen>>
class sample_view final : public eager_view_base<V> {
    using view_base = eager_view_base<V>;
    V base_;

public:
    sample_view() = default;

    sample_view(V&& range, std::integral auto sample_count, Gen&& gen) : base_(std::move(range))
    {
        if (sample_count < 1) {
            throw std::logic_error{
                "sample_count cannot be less than 1. sample_count value: " + std::to_string(sample_count)
            };
        }
        if (auto size = size_or_distance(base_); size < sample_count)
            throw std::out_of_range{
                "sample_count cannot be greater than range size. sample_count value: "
                + std::to_string(sample_count)
                + "\t range size:"
                + std::to_string(size)
            };

        auto& iters = view_base::base();
        iters.reserve(sample_count);
        std::ranges::sample(detail::iterator_view<V>{base_}, std::back_inserter(iters),
                            sample_count, std::move(gen));
    }
};

template<typename R, typename Count, typename Gen>
sample_view(R&&, Count, Gen&&) -> sample_view<std::views::all_t<R>, Gen>;

namespace detail
{
struct Sample {
    template<typename R, std::integral Count, typename Gen>
    requires view_possible_tmplt_ctor<sample_view<std::views::all_t<R>, Gen>, std::views::all_t<R>, Count, Gen>
    constexpr auto operator()(R&& r, Count sample_count, Gen&& gen) const
    {
        return sample_view{std::forward<R>(r), sample_count, std::forward<Gen>(gen)};
    }
};
} // namespace detail

inline constexpr detail::Sample sample;

template<std::ranges::forward_range V, std::ranges::forward_range... Rest>
requires std::ranges::view<V> && (std::ranges::view<Rest> && ...) && (std::same_as<V, Rest> && ...)
class concat_view final : public eager_view_base<V> {
    using view_base = eager_view_base<V>;
    V base_first_;
    std::tuple<Rest...> base_rest_;

public:
    concat_view() = default;

    constexpr explicit concat_view(V&& first, Rest&&... rest)
        : base_first_(std::move(first)), base_rest_(std::tuple<Rest...>{std::move(rest)...})
    {
        auto rest_total_size
            = std::apply([](auto&&... args) { return (size_or_distance(args) + ... + 0); }, base_rest_);
        auto& iters = view_base::base();
        iters.reserve(size_or_distance(base_first_) + rest_total_size);

        std::ranges::copy(detail::iterator_view<V>{base_first_}, std::back_inserter(iters));
        std::apply(
            [&iters](auto&... args) {
                (std::ranges::copy(detail::iterator_view<Rest>{args}, std::back_inserter(iters)), ...);
            },
            base_rest_);
    }
};

template<typename R, typename... Rn>
concat_view(R&&, Rn&&...)
    -> concat_view<std::ranges::views::all_t<R>, std::ranges::views::all_t<Rn>...>;

namespace detail
{
struct Concat {
    template<typename First, typename... Rest>
    requires view_possible<concat_view, std::ranges::views::all_t<First>, std::ranges::views::all_t<Rest>...>
    constexpr auto operator()(First&& first, Rest&&... rest) const
    {
        return concat_view{std::forward<First>(first), std::forward<Rest>(rest)...};
    }
};
} // namespace detail

inline constexpr detail::Concat concat;

template<std::ranges::forward_range V, std::indirect_unary_predicate<std::ranges::iterator_t<V>> F>
requires std::ranges::view<V>
class filter_view final : public eager_view_base<V> {
    using view_base = eager_view_base<V>;
    V base_;

public:
    using iterator = view_base::iterator;
    using const_iterator = view_base::const_iterator;

    filter_view() = default;

    constexpr filter_view(V&& range, std::same_as<F> auto&& pred) : base_(std::move(range))
    {
        auto& iters = view_base::base();
        detail::iterator_view<V> view{base_};
        std::ranges::copy_if(view.begin(), view.end(), std::back_inserter(iters),
                             std::forward<decltype(pred)>(pred), [](const auto& a) { return *a; });
        iters.shrink_to_fit();
    }
};

template<typename R, typename F>
filter_view(R&&, F&&) -> filter_view<std::ranges::views::all_t<R>, F>;

namespace detail
{
struct Filter {
    template<typename Range, typename Pred>
    requires view_possible<filter_view, std::ranges::views::all_t<Range>, std::remove_reference_t<Pred>>
    constexpr auto operator()(Range&& range, Pred&& pred) const
    {
        return filter_view{std::forward<Range>(range), std::forward<Pred>(pred)};
    }
};
} // namespace detail

inline constexpr detail::Filter filter;

template<std::ranges::forward_range V> requires std::ranges::view<V>
class selection_view final : public eager_view_base<V> {
    using view_base = eager_view_base<V>;
    V base_;

public:
    using iterator = view_base::iterator;
    using const_iterator = view_base::const_iterator;

    selection_view() = default;

    template<std::ranges::sized_range R> requires std::integral<std::ranges::range_value_t<R>>
    selection_view(V&& range, const R& indices) : base_(std::move(range))
    {
        auto base_size = size_or_distance(base_);
        const auto indices_size = std::ranges::size(indices);
        if (base_size < indices_size)
            throw std::out_of_range{"indices size cannot be larger than range size"};

        auto& iters = view_base::base();
        iters.reserve(indices_size);
        const auto start = std::ranges::begin(base_);
        const auto end = std::ranges::end(base_);

        for (auto idx : indices) {
            auto it = std::ranges::next(start, idx);
            if (it == end)
                throw std::out_of_range{"index "s + std::to_string(idx) + "out of bounds"};
            iters.push_back(it);
        }
    }
};

template<typename R, typename Indices>
selection_view(R&&, Indices&&) -> selection_view<std::ranges::views::all_t<R>>;

namespace detail
{
struct Selection {
    template<typename V, typename R>
    requires view_possible_tmplt_ctor<selection_view<std::views::all_t<V>>, std::views::all_t<V>, R>
    constexpr auto operator()(V&& v, R&& indices) const
    {
        return selection_view{std::forward<V>(v), std::forward<R>(indices)};
    }
};
} // namespace detail

inline constexpr detail::Selection take_at;

template<std::ranges::forward_range V,
         std::relation<std::ranges::range_value_t<V>, std::ranges::range_value_t<V>> Ordering>
requires std::ranges::view<V>
class ordered_view final : public eager_view_base<V> {
    using view_base = eager_view_base<V>;
    V base_;

public:
    using iterator = view_base::iterator;
    using const_iterator = view_base::const_iterator;

    ordered_view() = default;

    constexpr ordered_view(V&& range, std::same_as<Ordering> auto&& ordering)
        : base_(std::move(range))
    {
        auto& iters = view_base::base();
        iters.reserve(size_or_distance(base_));
        std::ranges::copy(detail::iterator_view<V>{base_}, std::back_inserter(iters));
        std::ranges::sort(iters, std::forward<decltype(ordering)>(ordering),
                          [](auto it) { return *it; });
    }
};

template<std::ranges::forward_range R,
         std::relation<std::ranges::range_value_t<R>, std::ranges::range_value_t<R>> Rel>
ordered_view(R&&, Rel&&) -> ordered_view<std::ranges::views::all_t<R>, Rel>;

namespace detail
{
struct Ordered {
    template<typename R, typename Ordering>
    requires view_possible<ordered_view, std::ranges::views::all_t<R>, Ordering>
    constexpr auto operator()(R&& range, Ordering&& ordering) const
    {
        return ordered_view{std::forward<R>(range), std::forward<Ordering>(ordering)};
    }
};
} // namespace detail

inline constexpr detail::Ordered order;

} // namespace eager_views
} // namespace ext::ranges

namespace ext::functional
{
namespace detail
{
template<typename, typename...> struct range_sentinel {};

template<template<typename, typename...> class Out = range_sentinel,
         template<typename, typename...> class In,
         typename... Callables,
         typename... Views,
         typename T>
constexpr std::ranges::range auto apply_function(const In<T>& range,
                                                 std::tuple<Callables...> callables,
                                                 std::tuple<Views...> views)
{
    constexpr auto size = std::tuple_size_v<decltype(callables)>;
    auto view = [&]<size_t... Is>(std::index_sequence<Is...>) {
        return (std::forward<decltype(range)>(range)
                | ...
                | std::get<Is>(views)(std::get<Is>(callables)));
    }(std::make_index_sequence<size>{});

    if constexpr (std::same_as<Out<T>, range_sentinel<T>>)
        return In<std::remove_cvref_t<std::ranges::range_value_t<decltype(view)>>>{
            std::ranges::begin(view), std::ranges::end(view)
        };
    else
        return Out<std::remove_cvref_t<std::ranges::range_value_t<decltype(view)>>>{
            std::ranges::begin(view), std::ranges::end(view)
        };
}
} // namespace detail

template<template<typename, typename...> class Out = detail::range_sentinel, typename Fn>
constexpr std::ranges::range auto map(std::ranges::input_range auto&& range, Fn&& fn)
{
    return detail::apply_function<Out>(std::forward<decltype(range)>(range),
                                       std::tuple<Fn>{std::forward<Fn>(fn)},
                                       std::tuple{std::views::transform});
}

template<template<typename, typename...> class Out = detail::range_sentinel, typename Pred>
constexpr std::ranges::range auto filter(std::ranges::input_range auto&& range, Pred&& pred)
{
    return detail::apply_function<Out>(std::forward<decltype(range)>(range), std::tuple<Pred>{pred},
                                       std::tuple{std::views::filter});
}

template<template<typename, typename...> class Out = detail::range_sentinel, typename Pred, typename Fn>
constexpr std::ranges::range auto filter_map(std::ranges::input_range auto&& range, Pred&& pred, Fn&& fn)
{
    return detail::apply_function<Out>(
        std::forward<decltype(range)>(range),
        std::tuple<Pred, Fn>{std::forward<Pred>(pred), std::forward<Fn>(fn)},
        std::tuple{std::views::filter, std::views::transform});
}

template<std::ranges::forward_range First, std::ranges::forward_range... Rest>
requires(std::convertible_to<std::ranges::range_value_t<Rest>, std::ranges::range_value_t<First>> && ...)
constexpr auto concatenate(First&& first, Rest&&... rest) -> std::remove_cvref_t<First>
{
    std::remove_cvref_t<First> range = std::forward<First>(first);
    if constexpr (reservable<First>) {
        auto size = (ranges::size_or_distance(rest) + ... + 0);
        range.reserve(std::ranges::size(range) + size);
    }
    (ranges::combine(std::forward<Rest>(rest), range), ...);
    return range;
}
} // namespace ext::functional
#endif // RANGES_EXT_HPP

//
// Created by david on 5/19/26.
//

#ifndef CPP_EXTENSIONS_CONCEPTS_HPP
#define CPP_EXTENSIONS_CONCEPTS_HPP

#include "ext_traits.hpp"

namespace ext
{
template<typename T, typename OtherTp> concept convertible_from = std::convertible_to<OtherTp, T>;

template<typename From, typename To> concept constructible_into = std::constructible_from<To, From>;

template<typename C, typename T> concept comparator = std::strict_weak_order<C, T, T>;

template<typename T> inline constexpr bool enable_char_type = false;

// clang-format off
template<typename T>
concept character =
    std::same_as<std::remove_cvref_t<T>, char> ||
    std::same_as<std::remove_cvref_t<T>, signed char> ||
    std::same_as<std::remove_cvref_t<T>, unsigned char> ||
    std::same_as<std::remove_cvref_t<T>, wchar_t> ||
    std::same_as<std::remove_cvref_t<T>, char8_t> ||
    std::same_as<std::remove_cvref_t<T>, char16_t> ||
    std::same_as<std::remove_cvref_t<T>, char32_t> ||
    enable_char_type<T>;

template<typename T>
concept unsigned_character = character<T> && std::unsigned_integral<T>;

template<typename T>
concept signed_character = character<T> && std::signed_integral<T>;

template<typename T, typename Ch>
concept string_like = character<Ch> && std::convertible_to<T, std::basic_string_view<Ch>>;

template<typename T>
concept any_string_like =
    string_like<T, char> ||
    string_like<T, char8_t> ||
    string_like<T, char16_t> ||
    string_like<T, char32_t> ||
    string_like<T, wchar_t>;

template<typename T>
concept boolean = std::same_as<std::remove_cvref_t<T>, bool>;

template<typename T>
concept integer_like =
    std::numeric_limits<std::remove_cvref_t<T>>::is_specialized &&
    std::numeric_limits<std::remove_cvref_t<T>>::is_integer;

template<typename T>
concept integer =
    (std::integral<T> || integer_like<T>) &&
    !character<T> &&
    !boolean<T>;

template<typename T>
concept unsigned_integer =
    integer<T> &&
    (std::unsigned_integral<T> || !std::numeric_limits<std::remove_cvref_t<T>>::is_signed);

template<typename T>
concept signed_integer =
    integer<T> &&
    (std::signed_integral<T> || std::numeric_limits<std::remove_cvref_t<T>>::is_signed);
// clang-format on

template<typename T, typename... Us> concept one_of = (std::same_as<T, Us> || ...);

template<typename Op, typename T>
concept operation_on = requires(T a, T b, Op op) {
    { op(a, b) } -> std::same_as<T>;
};

template<typename T, typename Op> concept magma = operation_on<Op, T>;

template<typename T, typename Op> concept monoid = magma<T, Op> && requires(T a, T b, Op op) { typename identity_element<T>; };

template<typename T, typename Op>
concept group = monoid<T, Op> && requires(T a, T b, Op op) {
    { op(a, inverse_element_v<a>) } -> std::convertible_to<std::integral_constant<T, identity_element_v<T>>>;
    { op(b, inverse_element_v<b>) } -> std::convertible_to<std::integral_constant<T, identity_element_v<T>>>;
};

namespace detail
{
template<typename T, size_t I>
concept can_get_element = requires(T&& t) {
    typename std::tuple_element_t<I, T>;
    std::get<I>(std::forward<T>(t));
};
} // namespace detail

template<typename T>
concept tuple_like = integer<typename std::tuple_size<T>::value_type> && []<size_t... I>(std::index_sequence<I...>) {
    return (detail::can_get_element<T, I> && ...);
}(std::make_index_sequence<std::tuple_size_v<T>>());

template<typename T> concept pair_like = tuple_like<T> && std::tuple_size_v<T> == 2;

template<class T, typename K, typename V>
concept mapping = requires(T& map, K& key) {
    { map[key] } -> std::convertible_to<V>;
};

template<typename T> concept reservable = requires(T& t) { t.reserve(1); };

template<typename T> concept resizable = requires(T& t) { t.resize(1); };

template<typename T> concept shrinkable = requires(T& t) { t.shrink_to_fit(); };

template<typename ValueTp, template<typename> class StorageTp, typename PositionTp>
concept insertable = (std::forward_iterator<PositionTp> || integer<PositionTp>)
                     && requires(ValueTp value, PositionTp pos, StorageTp<ValueTp> store) { store.insert(pos, value); };

template<typename ValueTp, template<typename> class StorageTp, typename PositionTp>
concept insertable_after = requires(ValueTp value, PositionTp pos, StorageTp<ValueTp> store) { store.insert_after(pos, value); };

template<typename ValueTp, template<typename> class StorageTp>
concept appendable = requires(ValueTp value, StorageTp<ValueTp> store) { store.push_back(std::forward<ValueTp>(value)); };

template<typename ValueTp, template<typename> class StorageTp>
concept prependable = requires(ValueTp value, StorageTp<ValueTp> store) { store.push_front(std::forward<ValueTp>(value)); };

template<typename T> inline constexpr bool enable_insert_range = false;
template<typename T> inline constexpr bool enable_forward_insert_range = false;
template<typename T> inline constexpr bool enable_back_insert_range = false;
template<typename T> inline constexpr bool enable_front_insert_range = false;

template<typename T>
concept insert_range
    = std::ranges::input_range<T>
      && (enable_insert_range<T>
          || requires(std::ranges::iterator_t<T> iter, std::ranges::range_value_t<T> value, T& range) { range.insert(iter, value); });

template<typename T>
concept forward_insert_range
    = std::ranges::input_range<T>
      && (enable_forward_insert_range<T>
          || requires(std::ranges::iterator_t<T> iter, std::ranges::range_value_t<T> value, T& range) { range.insert_after(iter, value); });

template<typename T>
concept back_insert_range
    = std::ranges::input_range<T>
      && (enable_back_insert_range<T> || requires(std::ranges::range_value_t<T> value, T& range) { range.push_back(value); });

template<typename T>
concept front_insert_range
    = std::ranges::input_range<T>
      && (enable_front_insert_range<T> || requires(std::ranges::range_value_t<T> value, T& range) { range.push_front(value); });

template<typename U, typename T> concept collection_of = std::same_as<std::ranges::range_value_t<U>, T>;

template<class U> concept iterable = std::ranges::forward_range<U> && std::ranges::forward_range<std::add_const_t<U>>;

template<class U, typename T> concept iterable_of = iterable<U> && collection_of<U, T>;

template<typename U>
concept const_correct_iterable = iterable<U> && requires(U& u) {
    { std::ranges::cbegin(u) } -> std::same_as<std::ranges::iterator_t<std::add_const_t<U>>>;
    { std::ranges::cend(u) } -> std::sentinel_for<std::ranges::iterator_t<std::add_const_t<U>>>;
};

template<typename U> concept unsized_range = !std::ranges::sized_range<U> || std::ranges::disable_sized_range<U>;

template<typename U>
concept container_like = const_correct_iterable<U> && requires(U& u) {
    { std::ranges::empty(u) } -> boolean;
};

namespace adt
{
enum class iterator_category : int8_t {
    forward,
    bidirectional,
    random_access,
    contiguous,
};

enum class value_insertion_category : int8_t {
    none = 0,
    before = (1 << 0),
    after = (1 << 1),
    back = (1 << 2),
    front = (1 << 3),
    any = before | after | back | front,
};

constexpr value_insertion_category operator|(value_insertion_category lhs, value_insertion_category rhs) noexcept
{
    using underlying_type = std::underlying_type_t<value_insertion_category>;
    return static_cast<value_insertion_category>(static_cast<underlying_type>(lhs) | static_cast<underlying_type>(rhs));
}
constexpr value_insertion_category operator&(value_insertion_category lhs, value_insertion_category rhs) noexcept
{
    using underlying_type = std::underlying_type_t<value_insertion_category>;
    return static_cast<value_insertion_category>(static_cast<underlying_type>(lhs) & static_cast<underlying_type>(rhs));
}
template<integer_like IntLike> constexpr bool operator==(value_insertion_category lhs, IntLike rhs)
{
    return static_cast<IntLike>(lhs) == rhs;
}
constexpr bool any_insertion_accepted(value_insertion_category flag)
{
    return (flag == value_insertion_category::any) || (flag != value_insertion_category::none);
}
constexpr bool contains_insertion_category(value_insertion_category flag, value_insertion_category category)
{
    return flag != value_insertion_category::any && (flag & category) == category;
}

struct container_options {
    bool fixed = false;
    bool unsized = false;
    iterator_category iter_category = iterator_category::forward;
    value_insertion_category insertion_categories = value_insertion_category::any;

    constexpr container_options as_fixed() const { return {true, unsized, iter_category, insertion_categories}; }
    constexpr container_options as_unsized() const { return {fixed, true, iter_category, insertion_categories}; }
    constexpr container_options operator|(iterator_category category) const
    {
        if (category <= iter_category) return *this;
        return {fixed, unsized, category, insertion_categories};
    }
    constexpr container_options operator^(value_insertion_category category) const { return {fixed, unsized, iter_category, category}; }
};
} // namespace adt

namespace detail
{
template<typename Range, adt::value_insertion_category Category>
concept insertion_supported
    = (!contains_insertion_category(Category, adt::value_insertion_category::before) || insert_range<Range>)
      && (!contains_insertion_category(Category, adt::value_insertion_category::back) || back_insert_range<Range>)
      && (!contains_insertion_category(Category, adt::value_insertion_category::front) || front_insert_range<Range>)
      && (!contains_insertion_category(Category, adt::value_insertion_category::after) || forward_insert_range<Range>);

template<typename Range, adt::iterator_category Category>
concept iterator_category_supported = (Category == adt::iterator_category::contiguous && std::ranges::contiguous_range<Range>)
                                      || (Category >= adt::iterator_category::random_access && std::ranges::random_access_range<Range>)
                                      || (Category >= adt::iterator_category::bidirectional && std::ranges::bidirectional_range<Range>)
                                      || (Category == adt::iterator_category::forward && std::ranges::forward_range<Range>);
} // namespace detail

template<typename T> inline constexpr bool enable_fixed_capacity = false;

template<typename T>
concept dynamic_capacity
    = !enable_fixed_capacity<T> && (insert_range<T> || back_insert_range<T> || front_insert_range<T> || forward_insert_range<T>);

template<typename T> concept fixed_capacity = !dynamic_capacity<T> || enable_fixed_capacity<T>;

template<typename U, adt::container_options Options = {}>
concept container = container_like<U>
                    && ((Options.fixed && fixed_capacity<U>)
                        || (!Options.fixed && adt::any_insertion_accepted(Options.insertion_categories))
                        || (!Options.fixed && dynamic_capacity<U>))
                    && ((Options.unsized && unsized_range<U>) || (!Options.unsized && std::ranges::sized_range<U>))
                    && detail::iterator_category_supported<U, Options.iter_category>
                    && detail::insertion_supported<U, Options.insertion_categories>;

template<typename U, adt::container_options Options = {}>
concept sequence
    = container<U, Options> && std::indirectly_writable<std::ranges::iterator_t<std::remove_cvref_t<U>>, std::ranges::range_value_t<U>>;

template<typename U, adt::container_options Options = {}>
concept indexed_sequence = sequence<U, (Options | adt::iterator_category::random_access)>;

template<typename U, adt::container_options Options = {}>
concept contiguous_sequence = indexed_sequence<U, Options | adt::iterator_category::contiguous>;

template<typename U, adt::container_options Options = {.insertion_categories = adt::value_insertion_category::after | adt::value_insertion_category::front}>
concept unsized_sequence = sequence<U, Options.as_unsized()>;

template<typename U, adt::container_options Options = {}> concept fixed_container = container<U, Options.as_fixed()>;

template<typename U, adt::container_options Options = {}> concept fixed_sequence = sequence<U, Options.as_fixed()>;

namespace detail
{
template<typename U, typename T>
concept membership_queryable = std::equality_comparable<T> && std::copyable<T> && requires(const U& u, const T& t) {
    { u.contains(t) } -> boolean;
};
} // namespace detail

template<typename U>
concept searchable_range = std::ranges::range<U>
                           && detail::membership_queryable<U, std::ranges::range_value_t<U>>
                           && requires(const U& u, const std::ranges::range_value_t<U>& value) {
                                  { u.find(value) } -> std::same_as<std::ranges::iterator_t<const U&>>;
                              };

template<typename U, adt::container_options Options = {}> concept finite_set = container<U, Options> && searchable_range<U>;

template<typename U, adt::container_options Options = {}>
concept mutable_set = Options.insertion_categories != adt::value_insertion_category::none
                      && finite_set<U, Options>
                      && requires(U& u, std::ranges::iterator_t<const U> pos, const std::ranges::range_value_t<U>& value) {
                             u.erase(pos);
                             u.insert(value);
                         };

template<typename U, adt::container_options Options = {}>
concept frozen_set = finite_set<U, Options.as_fixed() ^ adt::value_insertion_category::none>;

template<typename U, adt::container_options Options = {}>
concept ordered_set = finite_set<U, Options> && requires {
    typename U::key_compare;
    typename U::value_compare;
} && std::same_as<typename U::key_compare, typename U::value_compare> && std::indirect_strict_weak_order<typename U::key_compare, std::ranges::iterator_t<U>>;

template<typename U, adt::container_options Options = {}>
concept associative_array = container<U, Options> && requires {
    typename U::key_type;
    typename U::mapped_type;
} && mapping<U, typename U::key_type, typename U::mapped_type>;

template<class P>
concept smart_pointer = std::destructible<P> && requires(P& p) {
    typename P::element_type;
    { *p } -> std::same_as<std::add_lvalue_reference_t<typename P::element_type>>;
    { p.operator->() } -> std::same_as<std::add_pointer_t<typename P::element_type>>;
    { p.get() } -> std::same_as<std::add_pointer_t<typename P::element_type>>;
};

template<typename T, typename... E>
concept monadic = requires(T& t) {
    typename monad_traits<T>::value_type;
    { monad_traits<T>::has_value(t) } -> boolean;
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
concept error_monad = monadic<T, typename error_monad_traits<T>::error_type> && requires(T t) {
    typename error_monad_traits<T>::error_type;
    { error_monad_traits<T>::error(t) } -> std::convertible_to<typename error_monad_traits<T>::error_type>;
    {
        error_monad_traits<T>::transform_error(t, [](auto&&) { return char(); })
    } -> std::same_as<rebind_t<T, typename error_monad_traits<T>::value_type, char>>;
};
} // namespace ext
#endif // CPP_EXTENSIONS_CONCEPTS_HPP

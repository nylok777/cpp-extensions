//
// Created by david on 7/2/26.
//
#include <array>
#include <bitset>
#include <cmath>
#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#if __cplusplus >= 202302l
#include <expected>
#endif

#ifdef TEST_ABSEIL
#include <absl/container/btree_set.h>
#include <absl/container/btree_map.h>
#include <absl/container/flat_hash_set.h>
#include <absl/container/flat_hash_map.h>
#include <absl/container/node_hash_set.h>
#include <absl/container/node_hash_map.h>
#include <absl/container/linked_hash_set.h>
#include <absl/container/linked_hash_map.h>
#include <absl/container/fixed_array.h>
#include <absl/container/inlined_vector.h>
#endif

#ifdef TEST_BOOST
#include <boost/multiprecision/integer.hpp>
#include <boost/safe_numerics/safe_integer.hpp>
#include <boost/container/devector.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/small_vector.hpp>
#include <boost/container/stable_vector.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/container/slist.hpp>
#include <boost/smart_ptr.hpp>
#endif

#ifdef TEST_QT
#include <QMap>
#include <QSet>
#include <QSharedPointer>
#endif

#include "ext_concepts.hpp"

struct non_default_type {
    template<ext::constructible_into<std::string> T1, ext::signed_integer T2>
    non_default_type(T1&& arg1, T2&& arg2) : x(std::forward<T1>(arg1)), y(std::forward<T2>(arg2))
    {}
    std::string x;
    int64_t y;
};
static_assert(!std::is_default_constructible_v<non_default_type>);

struct a_type {
    std::string name;
    int64_t x;
    int64_t y;
    int64_t z;
    char8_t icon;
    double distance_from_origin() const { return std::sqrt((x * x) + (y * y) + (z * z)); }
    friend bool operator==(const a_type& lhs, const a_type& rhs) { return lhs.name == rhs.name && lhs.icon == rhs.icon; }
    friend auto operator<=>(const a_type& lhs, const a_type& rhs) -> std::weak_ordering
    {
        return static_cast<int64_t>(lhs.distance_from_origin()) <=> static_cast<int64_t>(rhs.distance_from_origin());
    }
};

template<> struct std::hash<a_type> {
    std::size_t operator()(const a_type& x) const noexcept
    {
        size_t h1 = std::hash<char8_t>{}(x.icon);
        size_t h2 = std::hash<std::string>{}(x.name);
        return h1 ^ (h2 << 1);
    }
};

struct small_type {
    int x, y, z;
};

using move_only_type = std::unique_ptr<a_type>;

using comparator_archetype = std::less<>;

static_assert(ext::integer<int>);
static_assert(ext::integer<size_t>);
static_assert(!ext::integer<bool>);
static_assert(!ext::integer<char>);

static_assert(ext::comparator<comparator_archetype, a_type>);

static constexpr size_t fixed_container_size = 512;

using unsized_linked_list = std::forward_list<a_type>;
using doubly_linked_list = std::list<a_type>;
using dynamic_array = std::vector<a_type>;
using double_ended_queue = std::deque<a_type>;
using fixed_array = std::array<small_type, fixed_container_size>;
using tree_set = std::set<a_type>;
using tree_map = std::map<char8_t, small_type>;
using hash_set = std::unordered_set<a_type>;
using hash_map = std::unordered_map<char8_t, small_type>;

// sized and dynamic
using typical_container_types = std::tuple<doubly_linked_list, dynamic_array, double_ended_queue, tree_set, tree_map, hash_set, hash_map>;

#define test_container_tuple(pred, tuple)                                                                                                  \
    []<size_t... Is>(std::index_sequence<Is...>) consteval {                                                                               \
        return (pred<std::tuple_element_t<Is, tuple>> && ...);                                                                             \
    }(std::make_index_sequence<std::tuple_size_v<tuple>>{})

static_assert(ext::iterable<unsized_linked_list>);
static_assert(ext::iterable<const unsized_linked_list&>);
static_assert(ext::iterable_of<const unsized_linked_list&, a_type>);

static_assert(test_container_tuple(ext::container, typical_container_types));

static_assert(ext::sequence<doubly_linked_list>);
static_assert(!ext::sequence<unsized_linked_list>);
static_assert(ext::unsized_sequence<unsized_linked_list>);
static_assert(ext::fixed_sequence<fixed_array>);
static_assert(ext::indexed_sequence<double_ended_queue>);
static_assert(ext::contiguous_sequence<dynamic_array>);
static_assert(ext::contiguous_sequence<fixed_array, {.fixed = true, .insertion_categories = ext::adt::value_insertion_category::none}>);

static_assert(ext::mutable_set<hash_set>);
static_assert(ext::mutable_set<tree_set>);

static_assert(ext::associative_array<hash_map>);
static_assert(ext::associative_array<tree_map>);

static_assert(ext::smart_pointer<std::shared_ptr<std::string>>);
static_assert(ext::smart_pointer<std::unique_ptr<std::string>>);

static_assert(ext::available_from<std::string, const char*>);
static_assert(ext::available_from<std::string_view, std::string>);
static_assert(ext::available_from<std::vector<float>, int>);

static_assert(ext::monadic<std::optional<move_only_type>>);
static_assert(ext::monadic<std::optional<a_type>>);
static_assert(ext::monadic<std::optional<int>>);
static_assert(ext::monadic<std::optional<std::tuple<int, float, std::u8string>>>);
#if __cplusplus >= 202302l
static_assert(ext::error_monad<std::expected<double, int>>);
static_assert(ext::error_monad<std::expected<std::vector<int>, std::string>>);
static_assert(ext::error_monad<std::expected<move_only_type, std::string>>);
static_assert(ext::error_monad<std::expected<std::tuple<float, float, std::u8string>, std::string>>);
#endif

#ifdef TEST_ABSEIL
template<template<typename...> class MapTemplate> using add_kv_to_map = MapTemplate<char8_t, small_type>;
template<template<typename...> class SetTemplate> using add_key_to_set = SetTemplate<a_type>;

template<template<typename...> class... MapTemplates> using def_map_types = std::tuple<add_kv_to_map<MapTemplates>...>;
template<template<typename...> class... SetTemplates> using def_set_types = std::tuple<add_key_to_set<SetTemplates>...>;

using abseil_map_types = def_map_types<absl::flat_hash_map, absl::node_hash_map, absl::btree_map, absl::linked_hash_map>;
using abseil_set_types = def_set_types<absl::flat_hash_set, absl::node_hash_set, absl::btree_set, absl::linked_hash_set>;

static_assert(test_container_tuple(ext::mutable_set, abseil_set_types));
static_assert(test_container_tuple(ext::associative_array, abseil_map_types));
static_assert(ext::ordered_set<absl::btree_set<a_type>>);

static_assert(ext::fixed_sequence<absl::FixedArray<small_type, fixed_container_size>>);
static_assert(ext::contiguous_sequence<absl::InlinedVector<small_type, fixed_container_size>>);
#endif

#ifdef TEST_BOOST
static_assert(ext::integer<boost::multiprecision::int128_t>);
static_assert(ext::integer<boost::multiprecision::int256_t>);
static_assert(ext::integer<boost::multiprecision::int512_t>);
static_assert(ext::integer<boost::multiprecision::cpp_int>);
static_assert(ext::integer<boost::safe_numerics::safe<int>>);

template<> inline constexpr bool ext::enable_fixed_capacity<boost::container::static_vector<small_type, fixed_container_size>> = true;

static_assert(ext::mutable_set<boost::container::flat_set<a_type>>);
static_assert(ext::associative_array<boost::container::flat_map<char8_t, small_type>>);
static_assert(ext::sequence<boost::container::slist<a_type>>);
static_assert(ext::indexed_sequence<boost::container::stable_vector<a_type>>);
static_assert(ext::contiguous_sequence<boost::container::static_vector<small_type, fixed_container_size>, {.fixed = true}>);
static_assert(ext::contiguous_sequence<boost::container::devector<a_type>>);
static_assert(ext::contiguous_sequence<boost::container::small_vector<a_type, 20>>);

static_assert(ext::smart_pointer<boost::intrusive_ptr<a_type>>);
#endif

#ifdef TEST_QT
template<> inline constexpr bool ext::enable_char_type<QChar> = requires { sizeof(std::char_traits<QChar>); };

static_assert(ext::string_like<QString, char16_t>);
static_assert(ext::any_string_like<QString>);
static_assert(ext::character<QChar>);

static_assert(ext::mutable_set<QSet<a_type>>);

static_assert(ext::associative_array<QMap<char8_t, small_type>>);
static_assert(ext::associative_array<QHash<char8_t, small_type>>);

static_assert(ext::contiguous_sequence<QList<a_type>>);

// no element_type typedef
static_assert(!ext::smart_pointer<QScopedPointer<a_type>>);

static_assert(ext::smart_pointer<QSharedPointer<a_type>>);
#endif

void some_procedure() {}

consteval bool some_comptime_function() { return true; }

int main() {}

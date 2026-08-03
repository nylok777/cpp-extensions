//
// Created by david on 7/24/26.
//
#include "chunked_string.hpp"

using namespace std::string_view_literals;

int main()
{
    constexpr auto str = "abcdefg"sv;
    constexpr auto str2 = "hijklmnop"sv;
    ext::chunked_string<2> chunked_string{str, str2};
    const auto& chunked_string_ref = chunked_string;

    static_assert(std::same_as<ext::chunked_string<2>::chunk&, decltype(chunked_string[0])>);
    static_assert(std::same_as<char&, decltype(chunked_string[0, 0])>);

    static_assert(std::same_as<const ext::chunked_string<2>::chunk&, decltype(chunked_string_ref[0])>);
    static_assert(std::same_as<const char&, decltype(chunked_string_ref[0, 0])>);
}
//
// Created by david on 7/28/26.
//

#ifndef CPPEXTENSIONS_CHUNKED_STRING_HPP
#define CPPEXTENSIONS_CHUNKED_STRING_HPP
#include "basic_chunked_string.hpp"

namespace ext
{
template<size_t ChunkNum, character Char = char>
using chunked_string = basic_chunked_string<Char, ChunkNum, std::basic_string, std::array>;

template<size_t N>
using u8chunked_string = chunked_string<N, char8_t>;
template<size_t N>
using u16chunked_string = chunked_string<N, char16_t>;
template<size_t N>
using u32chunked_string = chunked_string<N, char32_t>;
template<size_t N>
using wchunked_string = chunked_string<N, wchar_t>;
}

#endif // CPPEXTENSIONS_CHUNKED_STRING_HPP

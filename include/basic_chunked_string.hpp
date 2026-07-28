//
// Created by david on 7/24/26.
//

#ifndef CPPEXTENSIONS_BASIC_CHUNKED_STRING_HPP
#define CPPEXTENSIONS_BASIC_CHUNKED_STRING_HPP
#if __cplusplus >= 202302l
#include <algorithm>
#include <span>
#include <stdexcept>
#include <ranges>

#include "ext_concepts.hpp"

namespace ext
{
template<character Char, size_t ChunkNum, template<typename> class CharSequence, template<typename, size_t N> class ChunkSequence>
requires contiguous_sequence<CharSequence<Char>> && contiguous_sequence<ChunkSequence<copyable_type<>, ChunkNum>, {.fixed = true}>
class basic_chunked_string {
public:
    class chunk;
    using char_sequence = CharSequence<Char>;
    using chunk_sequence = ChunkSequence<chunk, ChunkNum>;
    using size_type = std::ranges::range_size_t<char_sequence>;
    using difference_type = std::ranges::range_difference_t<char_sequence>;

    template<bool> class iterator_base;

private:
    char_sequence m_data;
    chunk_sequence m_chunks;

public:
    class chunk {
        friend basic_chunked_string;
        friend class iterator_base<false>;
        friend class iterator_base<true>;

        std::span<Char> m_string;
        size_type m_offset;
        basic_chunked_string* m_parent;
        size_type m_idx;

    public:
        using iterator = std::span<Char>::iterator;
        using const_iterator = std::span<Char>::const_iterator;

        constexpr chunk() : m_offset(~0), m_parent(nullptr), m_idx(~0) {}
        constexpr operator std::basic_string_view<Char>() const { return std::basic_string_view<Char>{m_string}; }
        constexpr operator const Char*() const { return m_string.data(); }

        constexpr decltype(auto) operator[](this auto& self, size_type i) { return std::forward_like<decltype(self)>(self.m_string[i]); }
        constexpr decltype(auto) at(this auto& self, size_type i)
        {
            return i < self.size() ? self[i] : throw std::out_of_range{"Index out of range."};
        }
        constexpr decltype(auto) front(this auto& self) { return std::forward_like<decltype(self)>(self.m_string.front()); }
        constexpr decltype(auto) back(this auto& self) { return std::forward_like<decltype(self)>(self.m_string.back()); }

        constexpr auto size() const -> size_type { return m_string.size(); }
        constexpr auto length() const -> size_type { return m_string.size(); }
        constexpr auto max_size() const -> size_type { return m_parent->m_data.max_size(); }
        constexpr auto capacity() const -> size_type
        {
            return ChunkNum - m_idx == 1 ? m_parent->m_data.size() - m_offset : (*m_parent)[m_idx + 1].offset() - m_offset - 1;
        }
        constexpr auto free_capacity() const -> size_type { return capacity() - m_string.size(); }

        constexpr void reserve(size_type size) { resize_sequence<true>(size); }
        constexpr void resize(size_type size) { resize_sequence<false>(size); }

        template<string_like<Char> StringTp> constexpr chunk& operator=(StringTp&& str)
        {
            assign(std::forward<StringTp>(str));
            return *this;
        }
        template<string_like<Char> StringTp> constexpr chunk& operator+=(StringTp&& str)
        {
            append(std::forward<StringTp>(str));
            return *this;
        }
        constexpr chunk& operator+=(Char ch)
        {
            push_back(ch);
            return *this;
        }
        constexpr void push_back(Char ch)
        {
            if (free_capacity() < 2) reserve(size() * 2);
            auto it = cast_iterator(end());
            *it = ch;
            *++it = Char{'\0'};
            reset(cast_iterator(begin()), it);
        }
        constexpr void pop_back()
        {
            m_string.back() = Char{'\0'};
            reset(begin(), end() - 1);
        }
        template<string_like<Char> StringTp> constexpr chunk& assign(StringTp&& str)
        {
            constexpr bool is_string = std::same_as<std::basic_string<Char>, char_sequence>;
            integer auto str_size = std::ranges::size(str);

            if (str_size <= capacity()) {
                if constexpr (is_string)
                    m_parent->m_data.replace(cast_iterator(begin()), cast_iterator(begin()) + str_size, std::forward<StringTp>(str));
                else std::ranges::copy(std::forward<StringTp>(str), cast_iterator(begin()));
                *(begin() + str_size) = Char{'\0'};
                reset(begin(), begin() + str_size);
                return *this;
            }

            chunk* next = m_parent->next_chunk(m_idx);
            m_parent->change_data(std::forward<StringTp>(str), m_offset, (next ? next->m_offset : -1));
            for (chunk& chnk : m_parent->m_chunks) chnk.reset<true>(this, std::abs(static_cast<difference_type>(chnk.size()) -
                static_cast<difference_type>(str_size)));

            return *this;
        }
        constexpr chunk& assign(const Char* c_str) { return assign(std::basic_string_view<Char>{c_str}); }
        template<string_like<Char> StringTp> constexpr chunk& append(StringTp&& str)
        {
            constexpr bool is_string = std::same_as<std::basic_string<Char>, char_sequence>;

            integer auto str_size = static_cast<const StringTp&>(str).size();

            if (str_size <= free_capacity()) {
                if constexpr (is_string)
                    m_parent->m_data.replace(cast_iterator(end()), cast_iterator(end()) + str_size, std::forward<StringTp>(str));
                else std::ranges::copy(std::forward<StringTp>(str), cast_iterator(end()));
                *(end() + str_size) = Char{'\0'};
                reset(cast_iterator(begin()), cast_iterator(end()) + str_size);
                return *this;
            }

            chunk* next = m_parent->next_chunk(m_idx);
            m_parent->change_data(std::forward<StringTp>(str), m_offset + size(), (next ? next->m_offset : -1));
            for (chunk& chnk : m_parent->m_chunks)
                chnk.reset<true>(this, std::abs(static_cast<difference_type>(chnk.size()) - static_cast<difference_type>(str_size)));

            return *this;
        }
        constexpr chunk& append(const Char* c_str) { return append(std::basic_string_view<Char>{c_str}); }
        constexpr chunk& erase(const_iterator first, const_iterator last)
        {
            auto dist = std::distance(first, last);
            if (dist < 0 || std::to_address(first) < std::to_address(begin()) || std::to_address(last) > std::to_address(end()))
                return *this;
            if (std::to_address(last) < std::to_address(end()))
                std::shift_left(last, end(), dist);
            reset(begin(), end() - dist);
            if (end() != m_parent->m_data.end())
                *end() = Char{'\0'};
            return *this;
        }
        constexpr void clear()
        {
            *begin() = Char{'\0'};
            reset(begin(), begin());
        }

        constexpr auto begin() -> iterator { return m_string.begin(); }
        constexpr auto end() -> iterator { return m_string.end(); }
        constexpr auto begin() const -> const_iterator { return m_string.begin(); }
        constexpr auto end() const -> const_iterator { return m_string.end(); }

        constexpr friend std::ostream& operator<<(std::ostream& stream, const chunk& chunk)
        {
            return stream << static_cast<std::basic_string_view<Char>>(chunk);
        }

    private:
        chunk(std::contiguous_iterator auto first, std::sentinel_for<decltype(first)> auto sent, size_type offset, basic_chunked_string* parent, size_type idx)
            : m_string(first, sent), m_offset(offset), m_parent(parent), m_idx(idx)
        {}
        constexpr auto offset() const -> size_type { return m_offset; }
        constexpr auto index() const -> size_type { return m_idx; }
        template<typename Self> constexpr auto cast_iterator(this Self&, std::contiguous_iterator auto iter)
        {
            if constexpr (std::is_const_v<Self>) return std::ranges::const_iterator_t<char_sequence>{std::to_address(iter)};
            else return std::ranges::iterator_t<char_sequence>{std::to_address(iter)};
        }
        constexpr void reset(std::contiguous_iterator auto first, std::contiguous_iterator auto last)
        {
            m_string = std::span<Char>{cast_iterator(first), cast_iterator(last)};
        }
        constexpr void reset(std::contiguous_iterator auto first, std::contiguous_iterator auto last, size_type extra_offset)
        {
            m_string = std::span<Char>{cast_iterator(first), cast_iterator(last)};
            m_offset += extra_offset;
        }
        template<bool DataShifted>
        constexpr void reset(const chunk* modifier, size_type extra_len)
        {
            auto first = m_parent->m_data.begin() + m_offset;
            auto last = first + size();

            if (this < modifier) reset(first, last);
            else if (this == modifier) reset(first, last + extra_len);
            else if (this > modifier) {
                if constexpr (DataShifted) {
                    first += extra_len;
                    last += extra_len;
                }
                else last = std::shift_right(first, last, extra_len);
                reset(first, last, extra_len);
            }
        }
        template<bool ReserveOnly> constexpr void resize_sequence(size_type size)
        {
            if (size <= m_string.size()) return;
            size_type extra = size - m_string.size();
            m_parent->m_data.resize(m_parent->m_data.size() + extra);
            m_parent->reset_chunks<!ReserveOnly>(*this, extra);
        }
    };

    constexpr basic_chunked_string() = default;
    constexpr basic_chunked_string(size_type size) : m_data(size, Char{'\0'})
    {
        size_type n = size / ChunkNum;
        size_type i = 0;
        for (size_type offset = 0; chunk& chnk : m_chunks) {
            auto it = m_data.begin() + offset;
            chnk = chunk{it, it /*+ 1*/, offset, this, i++};
            offset += n;
        }
    }
    constexpr basic_chunked_string(size_type size, Char ch) : m_data(size, ch)
    {
        size_type n = size / ChunkNum;
        size_type i = 0;
        for (size_type offset = 0; chunk& chnk : m_chunks) {
            auto it = m_data.begin() + offset;
            *(it + n) = Char{'\0'};
            chnk = chunk{it, it + n, offset, this, i++};
            offset += n + 1;
        }
    }

    template<typename Self> constexpr decltype(auto) operator[](this Self& self, size_type chunk_idx)
    {
        return self.m_chunks[chunk_idx];
    }
    template<typename Self> constexpr decltype(auto) get_chunk(this Self& self, size_type chunk_idx) { return self[chunk_idx]; }
    template<typename Self> constexpr decltype(auto) chunks(this Self& self) { return self.m_chunks; }

    constexpr auto size(size_type chunk_idx) const -> size_type
    {
        return m_chunks[chunk_idx].size();
    }
    constexpr auto size() const -> size_type
    {
        size_type acc = 0;
        for (const auto& chunk : m_chunks) acc += chunk.size();
        return acc;
    }
    constexpr auto capacity(size_type chunk_idx) const -> size_type
    {
        return m_chunks[chunk_idx].capacity();
    }
    constexpr auto capacity() const -> size_type { return m_data.size(); }

    constexpr void push_back(Char ch, size_type chunk_idx) { m_chunks[chunk_idx].push_back(ch); }

private:
    static consteval bool check_index(size_type idx) { return idx < ChunkNum && idx >= 0; }
    template<typename Self> constexpr auto char_data(this Self& self) { return std::forward_like<Self>(self.m_data.data()); }
    template<string_like<Char> StringTp> constexpr void change_data(StringTp&& str, difference_type pos1, difference_type pos2)
    {
        char_sequence seq;
        seq.reserve(m_data.size() + std::ranges::size(str));
        auto out = std::back_inserter(seq);
        std::copy(m_data.begin(), m_data.begin() + pos1, out);
        std::ranges::copy(std::forward<StringTp>(str), out);
        if (pos2 > 0)
            std::copy(m_data.begin() + pos2, m_data.end(), out);
        else for (size_type i = seq.size(); i < m_data.size(); ++i) out = Char{'\0'};
        m_data = std::move(seq);
    }
    constexpr auto next_chunk(size_type idx) -> chunk*
    {
        if (idx >= ChunkNum - 1) return nullptr;
        return &m_chunks[idx + 1];
    }
    template<bool Resized> constexpr void reset_chunks(chunk& selected_chunk, [[maybe_unused]] size_type extra_offset)
    {
        auto reset_fn = [this](auto& chunk, size_type size) {
            auto first = m_data.begin() + chunk.offset();
            chunk.reset(first, first + size);
        };

        if (&selected_chunk == &m_chunks.back())
            for (chunk& chunk : std::views::counted(m_chunks.begin(), m_chunks.size() - 1)) reset_fn(chunk, chunk.size());
        else {
            auto take_while = [&](auto cmp) {
                return std::views::take_while([&selected_chunk, cmp](auto& chunk) -> bool { return cmp(&chunk, &selected_chunk); });
            };

            for (chunk& chunk : m_chunks | take_while(std::less<>{})) reset_fn(chunk, chunk.size());

            for (chunk& chunk : m_chunks | std::views::reverse | take_while(std::greater<>{})) {
                auto first = m_data.begin() + chunk.offset();
                auto end = std::shift_right(first, first + chunk.size(), extra_offset);
                chunk.reset(first + extra_offset, end, extra_offset);
            }
        }
        if constexpr (Resized) reset_fn(selected_chunk, selected_chunk.size() + extra_offset);
        else reset_fn(selected_chunk, selected_chunk.size());
    }

public:
    template<bool IsConst> class iterator_base {
    public:
        using difference_type = std::ranges::range_difference_t<char_sequence>;
        using value_type = Char;
        using reference = std::conditional_t<IsConst, const Char&, Char&>;
        using const_reference = const Char&;

        auto operator*() -> reference { return *m_char_iter; }
        auto operator*() const -> const_reference { return *m_char_iter; }

        constexpr auto operator++() -> iterator_base&
        {
            if (!m_chunk_iter || !m_char_iter) return *this;
            if (m_char_iter != m_chunk_iter->end()) ++m_char_iter;
            else if (m_chunk_iter != m_chunk_iter->m_parent->m_chunks.end() - 1) m_char_iter = (++m_chunk_iter)->begin();
            return *this;
        }
        constexpr auto operator++(int) -> iterator_base
        {
            auto tmp = *this;
            ++*this;
            return tmp;
        }
        constexpr auto operator--() -> iterator_base&
        {
            if (!m_chunk_iter || !m_char_iter) return *this;
            if (m_char_iter != m_chunk_iter->begin()) --m_char_iter;
            else if (m_chunk_iter != m_chunk_iter->m_parent->m_chunks.begin()) m_char_iter = (--m_chunk_iter)->end() - 1;
            return *this;
        }
        constexpr auto operator--(int) -> iterator_base
        {
            auto tmp = *this;
            --*this;
            return tmp;
        }
        constexpr auto operator+=(difference_type n) -> iterator_base&
        {
            difference_type x = m_chunk_iter->end() - m_char_iter;
            if (n < x) m_char_iter += n;
            else if (m_chunk_iter != m_chunk_iter->m_parent->m_chunks.end() - 1) m_char_iter = (++m_chunk_iter)->begin() + (n - x);
            else m_char_iter = m_chunk_iter->end();
            return *this;
        }
        constexpr auto friend operator+(const iterator_base& iter, const difference_type n) -> iterator_base
        {
            auto j = iter;
            j += n;
            return j;
        }
        constexpr auto operator-=(difference_type n) -> iterator_base&
        {
            difference_type x = m_char_iter - m_chunk_iter->begin();
            if (n < x) m_char_iter -= n;
            else if (m_chunk_iter != m_chunk_iter->m_parent->m_chunks.begin()) m_char_iter = (--m_chunk_iter)->end() - 1 - (n - x);
            else m_char_iter = m_chunk_iter->begin();
            return *this;
        }
        constexpr auto friend operator-(const iterator_base& iter, const difference_type n) -> iterator_base
        {
            auto j = iter;
            j -= n;
            return j;
        }
        template<typename Self> constexpr auto operator[](this Self& self, difference_type n) { *(self + n); }

    private:
        using char_iter = std::conditional_t<IsConst, typename chunk::const_iterator, typename chunk::iterator>;
        using chunk_iter = std::conditional_t<IsConst, std::ranges::const_iterator_t<chunk_sequence>, std::ranges::iterator_t<chunk_sequence>>;
        chunk_iter m_chunk_iter = nullptr;
        char_iter m_char_iter = nullptr;

    public:
        iterator_base(chunk* chunk) : m_chunk_iter(chunk), m_char_iter((m_chunk_iter ? m_chunk_iter->begin() : nullptr)) {}
        iterator_base(chunk* chunk, Char* iter) : m_chunk_iter(chunk), m_char_iter(iter) {}
        template<bool OtherConst> requires(!OtherConst || IsConst)
        iterator_base(const iterator_base<OtherConst>& other) : m_chunk_iter(other.m_chunk_iter), m_char_iter(other.m_char_iter)
        {}
    };

    using iterator = iterator_base<false>;
    using const_iterator = iterator_base<true>;

    template<typename Self> constexpr auto begin(this Self& self)
    {
        return std::conditional_t<std::is_const_v<Self>, const_iterator, iterator>{&self.m_chunks[0]};
    }
    template<typename Self> constexpr auto end(this Self& self)
    {
        return std::conditional_t<std::is_const_v<Self>, const_iterator, iterator>{
            std::to_address(self.m_chunks.end() - 1), std::to_address(self.m_chunks.back().end())
        };
    }
};
} // namespace ext
#endif
#endif // CPPEXTENSIONS_BASIC_CHUNKED_STRING_HPP

//
// Created by david on 7/24/26.
//

#ifndef CPPEXTENSIONS_BASIC_CHUNKED_STRING_HPP
#define CPPEXTENSIONS_BASIC_CHUNKED_STRING_HPP
#if __cplusplus >= 202302l
#include "basic_chunked_string.hpp"

#include <algorithm>
#include <span>
#include <stdexcept>
#include <ranges>

#include "ext_concepts.hpp"

namespace ext
{

namespace detail { template<typename T, typename Ch> concept is_basic_string = std::same_as<std::basic_string<Ch>, T>; }

template<character Char, size_t ChunkNum, template<typename> class CharSequence, template<typename, size_t N> class ChunkSequence>
requires contiguous_sequence<CharSequence<Char>> && contiguous_sequence<ChunkSequence<copyable_type<>, ChunkNum>, {.fixed = true}>
class basic_chunked_string {
public:
    class chunk;
    template<bool> class iterator_base;

    using char_sequence = CharSequence<Char>;
    using chunk_sequence = ChunkSequence<chunk, ChunkNum>;
    using size_type = std::ranges::range_size_t<char_sequence>;
    using difference_type = std::ranges::range_difference_t<char_sequence>;

    using iterator = iterator_base<false>;
    using const_iterator = iterator_base<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

private:
    char_sequence m_data;
    chunk_sequence m_chunks;

public:
    constexpr basic_chunked_string() : m_data(), m_chunks()
    {
        for (size_type i = 0; chunk& chunk : m_chunks) chunk = {this, i++};
    }

    explicit constexpr basic_chunked_string(size_type size) : m_data(size + ChunkNum, Char{'\0'})
    {
        auto fn = [n = size / ChunkNum, this](auto x) {
            difference_type offset = (n * x) + static_cast<difference_type>(x > 0);
            return chunk{this, x, offset, 0};
        };
        for (size_type i = 0; i < ChunkNum; ++i) {
            if constexpr (back_insert_range<chunk_sequence>) m_chunks.push_back(fn(i));
            else m_chunks[i] = fn(i);
        }
    }

    constexpr basic_chunked_string(size_type size, Char ch) : m_data(ChunkNum - 1 + size, ch)
    {
        auto fn = [n = (size / ChunkNum), this](size_type x) {
            difference_type offset = (n * x) + static_cast<difference_type>(x > 0);
            m_data[offset + n] = Char{'\0'};
            return chunk{this, x, offset, n};
        };
        for (size_type i = 0; i < ChunkNum; ++i) {
            if constexpr (back_insert_range<chunk_sequence>) m_chunks.push_back(fn(i));
            else m_chunks[i] = fn(i);
        }
    }

    template<string_like<Char> Str> requires std::constructible_from<char_sequence, Str>
    constexpr basic_chunked_string(Str&& string) : m_data(std::forward<Str>(string))
    {
        auto len = m_data.size();
        m_data.resize(ChunkNum - 1 + len);
        auto fn = [this](size_type idx, difference_type offset, size_type len) {
            chunk chunk{this, idx, offset, len};
            if constexpr (back_insert_range<chunk_sequence>) m_chunks.push_back(std::move(chunk));
            else m_chunks[idx] = std::move(chunk);
        };
        fn(0, 0, len);
        for (size_type i = 1; i < ChunkNum; ++i) {
            difference_type offset = len + i;
            fn(i, offset, 0);
        }
    }

    template<string_like<Char>... Strs> requires(std::is_object_v<std::remove_cvref_t<Strs>> && ...)
    constexpr basic_chunked_string(Strs&&... strings)
    {
        std::array<size_type, sizeof...(Strs)> sizes{(strings.size())...};
        std::array<difference_type, sizeof...(Strs)> offsets;

        size_type total_size = 0;
        std::ranges::transform(sizes, offsets.begin(), [i = difference_type{0}, &total_size](size_type s) mutable {
            total_size += s;
            difference_type offset = i;
            i += static_cast<difference_type>(s) + 1;
            return offset;
        });
        m_data.resize(ChunkNum - 1 + total_size);

        auto fn = [i = size_type{0}, this, &offsets, &sizes]<typename T>(T&& str) mutable {
            std::ranges::copy(std::forward<T>(str), m_data.begin() + offsets[i]);
            chunk chunk{this, i, offsets[i], sizes[i]};
            if constexpr (back_insert_range<chunk_sequence>) m_chunks.push_back(std::move(chunk));
            else m_chunks[i] = std::move(chunk);
            ++i;
        };
        (fn(std::forward<Strs>(strings)), ...);
    }

    template<string_like<Char> Str, template<typename, integer auto> class Array>
    requires std::convertible_to<Str, char_sequence> || std::constructible_from<char_sequence, Str>
    constexpr basic_chunked_string(Str&& str, const Array<difference_type, ChunkNum>& offsets) : m_data(std::forward<Str>(str))
    {
        if (auto s = ChunkNum - 1 + m_data.size(); m_data.capacity() < s) m_data.reserve(s);
        if constexpr (back_insert_range<chunk_sequence>) m_chunks.reserve(ChunkNum);
        auto fn = [this, idx = size_type{0}](auto offset, auto len) mutable {
            chunk chunk{this, idx, offset, len};
            if constexpr (back_insert_range<chunk_sequence>) m_chunks.push_back(std::move(chunk));
            else m_chunks[idx++] = std::move(chunk);
        };
        int idx = 0;
        for (auto [x, y] : offsets | std::views::adjacent<2>) {
            m_data.insert(m_data.begin() + y + idx, Char{'\0'});
            fn(x + idx++, static_cast<size_type>(y - x));
        }
        fn(offsets[idx] + idx, m_data.size());
    }

    constexpr auto& operator[](this auto&& self, size_type chunk_idx)
    {
        return std::forward_like<decltype(self)>(self.m_chunks[chunk_idx]);
    }
    constexpr auto& operator[](this auto&& self, size_type chunk_idx, size_type pos)
    {
        return std::forward_like<decltype(self)>(self.m_chunks[chunk_idx][pos]);
    }
    constexpr auto& at(this auto&& self, size_type chunk_idx)
    {
        return chunk_idx < ChunkNum ? std::forward_like<decltype(self)>(self.m_chunks[chunk_idx])
                                    : throw std::out_of_range{"Index out of" " range."};
    }
    constexpr auto& at(this auto&& self, size_type chunk_idx, size_type pos)
    {
        return std::forward_like<decltype(self)>(self.at(chunk_idx).at(pos));
    }

    constexpr auto& back(this auto&& self, size_type chunk_idx)
    {
        return std::forward_like<decltype(self)>(self.m_chunks[chunk_idx].back());
    }
    constexpr auto& front(this auto&& self, size_type chunk_idx)
    {
        return std::forward_like<decltype(self)>(self.m_chunks[chunk_idx].front());
    }

    constexpr auto size() const -> size_type
    {
        size_type acc = 0;
        for (const auto& chunk : m_chunks) acc += chunk.size();
        return acc;
    }
    constexpr auto capacity() const -> size_type { return m_data.size() - (ChunkNum - 1); }

    constexpr auto begin(this auto& self)
    {
        return std::conditional_t<std::is_const_v<decltype(self)>, const_iterator, iterator>{self.m_chunks[0]};
    }
    constexpr auto end(this auto& self)
    {
        auto chunk_iter = self.m_chunks.end() - 1;
        return std::conditional_t<std::is_const_v<decltype(self)>, const_iterator, iterator>{chunk_iter, chunk_iter->end()};
    }
    constexpr auto cbegin() -> const_iterator { return const_iterator{m_chunks[0]}; }
    constexpr auto cend() -> const_iterator { return const_iterator{m_chunks.back(), m_chunks.back().end()}; }
    constexpr auto rbegin(this auto& self)
    {
        auto chunk_iter = self.end();
        return std::conditional_t<std::is_const_v<decltype(self)>, const_reverse_iterator, reverse_iterator>{chunk_iter};
    }
    constexpr auto rend(this auto& self)
    {
        auto chunk_iter = self.begin();
        return std::conditional_t<std::is_const_v<decltype(self)>, const_reverse_iterator, reverse_iterator>{chunk_iter};
    }

private:
    constexpr auto next_chunk(size_type idx) -> chunk*
    {
        if (idx >= ChunkNum - 1) return nullptr;
        return &m_chunks[idx + 1];
    }
    constexpr auto prev_chunk(size_type idx) -> chunk*
    {
        if (idx == 0) return nullptr;
        return &m_chunks[idx - 1];
    }
    constexpr auto begin_char() -> std::ranges::iterator_t<char_sequence> { return m_data.begin(); }
    constexpr auto end_char() -> std::ranges::iterator_t<char_sequence> { return m_data.end(); }
};

template<character Char, size_t ChunkNum, template<typename> class CharSequence, template<typename, size_t N> class ChunkSequence>
requires contiguous_sequence<CharSequence<Char>> && contiguous_sequence<ChunkSequence<copyable_type<>, ChunkNum>, {.fixed = true}>
class basic_chunked_string<Char, ChunkNum, CharSequence, ChunkSequence>::chunk {
    friend basic_chunked_string;
    friend class iterator_base<false>;
    friend class iterator_base<true>;

    std::span<Char> m_string;
    basic_chunked_string* m_parent;
    difference_type m_offset;
    size_type m_idx;

public:
    using iterator = std::span<Char>::iterator;
    using const_iterator = std::span<Char>::const_iterator;

    constexpr chunk() : m_string(), m_parent(nullptr), m_offset(0), m_idx(0) {}
    constexpr operator std::basic_string_view<Char>() const { return std::basic_string_view<Char>{m_string}; }
    constexpr operator const Char*() const { return m_string.data(); }
    constexpr auto c_str() const -> const Char* { return m_string.data(); }

    constexpr auto& operator[](this auto&& self, size_type i) { return std::forward_like<decltype(self)>(self.m_string[i]); }
    constexpr auto& at(this auto&& self, size_type i)
    {
        return i < self.size() ? std::forward_like<decltype(self)>(self[i]) : throw std::out_of_range{"Index out of range."};
    }
    constexpr auto& back(this auto& self) { return std::forward_like<decltype(self)>(self.m_string.back()); }
    constexpr auto& front(this auto& self) { return std::forward_like<decltype(self)>(self.m_string.front()); }

    constexpr auto size() const -> size_type { return m_string.size(); }
    constexpr auto length() const -> size_type { return m_string.size(); }
    constexpr auto max_size() const -> size_type { return m_parent->m_data.max_size(); }
    constexpr bool empty() const { return m_string.empty(); }

    constexpr auto capacity() const -> size_type
    {
        chunk* next = m_parent->next_chunk(m_idx);
        difference_type x;
        if (next == nullptr) x = static_cast<difference_type>(m_parent->m_data.size()) - m_offset;
        else x = next->m_offset - m_offset - 1;
        return static_cast<size_type>(std::max(difference_type{0}, x));
    }

    constexpr void resize(size_type size) { resize_sequence<false>(size); }
    constexpr void reserve(size_type size) { resize_sequence<true>(size); }

    constexpr auto free_capacity() const -> size_type
    {
        size_type c = capacity();
        if (c == 0) return c;
        return c - m_string.size();
    }

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
        if (m_string.data() == nullptr || free_capacity() == 0) reserve(8);
        else if (free_capacity() == 0) reserve(std::max<size_type>(8, size() * 2));
        auto it = m_parent->begin_char() + m_offset + size();
        *it = ch;
        *++it = Char{'\0'};
        reset(m_offset, size() + 1);
    }
    template<string_like<Char> StringTp> constexpr chunk& assign(StringTp&& str)
    {
        auto str_size = std::ranges::size(str);

        if (str_size <= capacity()) modify(std::forward<StringTp>(str), m_offset);
        else modify(std::forward<StringTp>(str), m_offset, str_size);
        return *this;
    }
    template<string_like<Char> StringTp> constexpr chunk& append(StringTp&& str)
    {
        auto str_size = std::ranges::size(str);

        if (str_size <= free_capacity()) modify(std::forward<StringTp>(str), m_offset + size());
        else modify(std::forward<StringTp>(str), m_offset + size(), str_size + size());
        return *this;
    }
    template<string_like<Char> StringTp> constexpr chunk& insert(size_type pos, StringTp&& str)
    {
        using string_t = std::conditional_t<std::is_lvalue_reference_v<StringTp>, StringTp, std::remove_reference_t<StringTp>>;

        auto str_size = std::ranges::size(str);
        size_type cap = capacity();
        size_type free_cap = cap - size();

        if (str_size <= free_cap) {
            std::shift_right(begin() + pos, begin() + cap, str_size);
            std::ranges::copy(std::forward<StringTp>(str), begin() + pos);
            reset(m_offset, size() + str_size);
            return *this;
        }

        string_t val = std::forward<StringTp>(str);

        if constexpr (detail::is_basic_string<char_sequence, Char>) {
            if (free_cap > 0) {
                std::contiguous_iterator auto insert_pos = m_parent->begin_char() + m_offset + pos;
                std::shift_right(insert_pos, insert_pos - pos + cap, free_cap);
                m_parent->m_data.replace(insert_pos, insert_pos + free_cap, val);
                auto dist = m_offset + size() + str_size;
                m_parent->begin_char()[dist] = Char{'\0'};
            }
            else {
                char_sequence seq;
                seq.reserve(m_parent->m_data.size() + str_size);
                std::ranges::copy(m_parent->m_data, std::back_inserter(seq));
                seq.insert(seq.begin() + m_offset + pos, val.begin(), val.end());
                m_parent->m_data = std::move(seq);
            }
        }
        else {
            char_sequence seq;
            seq.reserve(m_parent->m_data.size() + str_size - free_cap);
            std::ranges::copy(m_parent->m_data, std::back_inserter(seq));
            auto first = seq.begin() + m_offset;
            if (free_cap > 0) std::copy(val.begin(), val.begin() + free_cap, first + pos);
            seq.insert(first + free_cap + pos, val.begin() + free_cap, val.end());
            m_parent->m_data = std::move(seq);
        }

        reset(m_offset, size() + str_size);
        chunk::reset_neighbours(*this, m_parent->m_chunks);

        return *this;
    }
    constexpr chunk& assign(const Char* c_str) { return assign(std::basic_string_view<Char>{c_str}); }
    constexpr chunk& append(const Char* c_str) { return append(std::basic_string_view<Char>{c_str}); }

    constexpr void pop_back()
    {
        if (m_string.empty()) return;
        m_string.back() = Char{'\0'};
        reset(begin(), end() - 1);
    }
    constexpr void clear()
    {
        *begin() = Char{'\0'};
        reset(m_offset, 0);
    }
    constexpr chunk& erase(iterator first, iterator last)
    {
        auto dist = std::distance(first, last);
        if (dist < 0 || std::to_address(first) < std::to_address(begin()) || std::to_address(last) > std::to_address(end())) return *this;
        if (std::to_address(last) < std::to_address(end())) {
            std::shift_left(begin(), end(), dist);
            reset(m_offset, (size() % 2) + dist);
        }
        else {
            reset(m_offset, size() - dist);
        }
        if (m_parent->begin_char() + m_offset + size() != m_parent->end_char()) *end() = Char{'\0'};
        return *this;
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
    constexpr chunk(basic_chunked_string* ptr, size_type idx) : m_string(), m_parent(ptr), m_offset(idx), m_idx(idx) {}
    constexpr chunk(basic_chunked_string* ptr, size_type idx, difference_type offset, size_type len)
        : m_string(ptr->begin_char() + offset, len), m_parent(ptr), m_offset(offset), m_idx(idx)
    {}

    constexpr void reset() { m_string = std::span<Char>{m_parent->begin_char() + m_offset, size()}; }
    constexpr void reset(difference_type offset, size_type length)
    {
        auto first = m_parent->begin_char() + offset;
        m_string = std::span<Char>{first, first + length};
        m_offset = offset;
    }
    static constexpr void reset_neighbours(chunk& modified, chunk_sequence& chunks)
    {
        auto pred = [&modified](const auto& x, auto&& fn) -> bool { return std::forward<decltype(fn)>(fn)(&x, &modified); };
        for (chunk& chunk_ : chunks | std::views::take_while(std::bind_back(pred, std::less<>{}))) chunk_.reset();
        for (chunk& chunk_ : chunks | std::views::drop_while(std::bind_back(pred, std::less_equal<>{}))) {
            chunk* prev = chunk_.m_parent->prev_chunk(chunk_.m_idx);
            difference_type new_offset = prev->m_offset + prev->size() + 1;
            chunk_.reset(new_offset, chunk_.size());
        }
    }
    static constexpr void reset_neighbours(chunk& modified, chunk_sequence& chunks, difference_type n)
    {
        auto pred = [&modified](const auto& x) { return &x != &modified; };
        for (chunk& chunk_ : chunks | std::views::take_while(pred)) chunk_.reset();
        for (chunk& chunk_ : std::views::reverse(chunks) | std::views::take_while(pred)) chunk_.shift(n);
    }
    constexpr void shift(difference_type n)
    {
        auto first = m_parent->begin_char() + m_offset;
        first = std::shift_right(first, first + size() + n, n);
        if (m_idx < ChunkNum - 1) first[size()] = Char{'\0'};
        reset(m_offset + n, size());
    }
    template<bool ReserveOnly> constexpr void resize_sequence(size_type size)
    {
        if (size <= m_string.size()) return;
        size_type extra = size - m_string.size();
        m_parent->m_data.resize(m_parent->m_data.size() + extra);

        auto pred = [this](const auto& x) { return &x != this; };
        for (chunk& chunk_ : m_parent->m_chunks | std::views::take_while(pred)) chunk_.reset();
        for (chunk& chunk_ : std::views::reverse(m_parent->m_chunks) | std::views::take_while(pred))
            chunk_.shift(static_cast<difference_type>(extra));

        if constexpr (ReserveOnly) reset(m_offset, this->size());
        else reset(m_offset, size);
    }

    template<string_like<Char> StringTp> constexpr void modify(StringTp&& str, difference_type n)
    {
        auto str_size = std::ranges::size(str);
        auto first = m_parent->begin_char() + n;
        auto last = first + str_size;
        if constexpr (std::same_as<std::basic_string<Char>, char_sequence>)
            m_parent->m_data.replace(first, last, std::forward<StringTp>(str));
        else std::ranges::copy(std::forward<StringTp>(str), first);
        *last = Char{'\0'};
        if (first == m_parent->begin_char() + m_offset) reset(n, str_size);
        else reset(m_offset, size() + str_size);
    }

    template<string_like<Char> StringTp> constexpr void modify(StringTp&& str, difference_type pos, size_type new_size)
    {
        chunk* next = m_parent->next_chunk(m_idx);
        difference_type pos2 = next && next->capacity() > 0 ? next->m_offset : -1;
        char_sequence seq;
        seq.reserve(std::max(m_parent->m_data.size() + new_size, new_size + (ChunkNum - 1)));
        auto out = std::back_inserter(seq);
        if (m_parent->m_data.empty() && pos < ChunkNum)
            for (int i = 0; i < pos; ++i) out = Char{'\0'};
        else std::copy(m_parent->begin_char(), m_parent->begin_char() + pos, out);
        std::ranges::copy(std::forward<StringTp>(str), out);
        if (pos2 > 0) {
            out = Char{'\0'};
            std::copy(m_parent->begin_char() + pos2, m_parent->end_char(), out);
        }
        else
            for (integer auto i = seq.size(); i < seq.capacity(); ++i) out = Char{'\0'};
        m_parent->m_data = std::move(seq);
        reset(m_offset, new_size);
        chunk::reset_neighbours(*this, m_parent->m_chunks);
    }
};

template<character Char, size_t ChunkNum, template<typename> class CharSequence, template<typename, size_t N> class ChunkSequence>
requires contiguous_sequence<CharSequence<Char>> && contiguous_sequence<ChunkSequence<copyable_type<>, ChunkNum>, {.fixed = true}>
template<bool IsConst>
class basic_chunked_string<Char, ChunkNum, CharSequence, ChunkSequence>::iterator_base {
public:
    using difference_type = std::ranges::range_difference_t<char_sequence>;
    using value_type = Char;
    using reference = std::conditional_t<IsConst, const Char&, Char&>;
    using const_reference = const Char&;

    auto operator*() -> reference { return *m_char_iter; }
    auto operator*() const -> const_reference { return *m_char_iter; }

    constexpr auto operator++() -> iterator_base&
    {
        if (!m_chunk_iter || m_char_iter == char_iter{nullptr}) return *this;
        if (++m_char_iter != m_chunk_iter->end()) return *this;
        if (m_chunk_iter->m_idx + 1 < ChunkNum) m_char_iter = (++m_chunk_iter)->begin();
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
        if (!m_chunk_iter || m_char_iter == char_iter{nullptr}) return *this;
        if (--m_char_iter != m_chunk_iter->begin() - 1) return *this;
        if (m_chunk_iter->m_idx > 0) m_char_iter = (--m_chunk_iter)->end() - 1;
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
        else if (m_chunk_iter->m_idx + 1 < ChunkNum) m_char_iter = (++m_chunk_iter)->begin() + (n - x);
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
        if (n <= x) m_char_iter -= n;
        else if (m_chunk_iter->m_idx > 0) m_char_iter = (--m_chunk_iter)->end() - /*1 -*/ (n - x);
        else m_char_iter = m_chunk_iter->begin() - 1;
        return *this;
    }
    constexpr auto friend operator-(const iterator_base& iter, const difference_type n) -> iterator_base
    {
        auto j = iter;
        j -= n;
        return j;
    }
    constexpr auto&& operator[](this auto&& self, difference_type n) { return std::forward_like<decltype(self)>(*(self + n)); }
    friend constexpr bool operator==(const iterator_base&, const iterator_base&) = default;

private:
    using char_iter = std::conditional_t<IsConst, typename chunk::const_iterator, typename chunk::iterator>;
    using chunk_iter = std::conditional_t<IsConst, std::ranges::const_iterator_t<chunk_sequence>, std::ranges::iterator_t<chunk_sequence>>;
    chunk_iter m_chunk_iter = nullptr;
    char_iter m_char_iter = nullptr;

public:
    iterator_base(nullptr_t) = delete;
    iterator_base(chunk& chunk) : m_chunk_iter(&chunk), m_char_iter(m_chunk_iter->begin()) {}
    iterator_base(chunk& chunk, difference_type pos) : m_chunk_iter(&chunk), m_char_iter(m_chunk_iter->begin() + pos) {}
    iterator_base(chunk& chunk_, std::ranges::iterator_t<chunk> char_iter) : m_chunk_iter(&chunk_), m_char_iter(char_iter) {}
    iterator_base(std::ranges::iterator_t<chunk_sequence> chunk_iter, std::ranges::iterator_t<chunk> char_iter)
        : m_chunk_iter(chunk_iter), m_char_iter(char_iter)
    {}
    template<bool OtherConst> requires(!OtherConst || IsConst)
    iterator_base(const iterator_base<OtherConst>& other) : m_chunk_iter(other.m_chunk_iter), m_char_iter(other.m_char_iter)
    {}
};

} // namespace ext
#endif
#endif

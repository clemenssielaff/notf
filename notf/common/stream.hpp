#pragma once

#include "notf/meta/fwd.hpp"

#include <cstring>
#include <ostream>
#include <streambuf>
#include <vector>

NOTF_OPEN_NAMESPACE

// binary writes =================================================================================================== //

/// Write the binary representation of an arbitrary value into the stream.
/// Only works for trivial types, for everything else you need to define your own serialization.
template<class T, class = std::enable_if_t<std::is_trivially_copyable_v<T>>>
void write_value(const T& value, std::ostream& out) {
    out.write(std::launder(reinterpret_cast<const char*>(&value)), sizeof(T));
}

/// Write raw data / ascii string into the stream.
void write_data(const char* data, const size_t size, std::ostream& out) { out.write(data, size); }

// vecbuffer ======================================================================================================= //

/// Buffer writing directly into a std::vector, growing it on-the-fly as neccessary.
/// For best performance (and without downsides) use a DefaultInitAllocator for the vector.
/// Example:
///
///     std::vector<char, DefaultInitAllocator<char>> vector;
///     VectorBuffer buffer(vector);
///     std::ostream stream(&buffer);
///     stream << "Hello" << 123;
///
template<class CharT, class AllocT, class TraitsT = std::char_traits<CharT>>
class VectorBuffer : public std::basic_streambuf<CharT, TraitsT> {

    // types ----------------------------------------------------------------------------------- //
private:
    using char_t = typename std::basic_streambuf<CharT>::char_type;
    using vector_t = std::vector<CharT, AllocT>;

    /// Minimal capacity of the vector in words.
    /// GCC grows the size by power of 2, which means that you can save on quite a few re-allocations when you start
    /// out with a large enough capacity.
    static constexpr const size_t start_capacity = 32;

    // methods --------------------------------------------------------------------------------- //
public:
    /// Constructor.
    /// @param vector   Vector to write into. Any existing content of the vector will be overwritten from the start.
    VectorBuffer(vector_t& vector) : m_vector(vector) {
        if (m_vector.size() < start_capacity) { m_vector.resize(start_capacity); }
        this->setp(m_vector.data(), m_vector.data() + m_vector.size());
    }

private:
    /// Write `count` characters from the `input` array into the buffer.
    std::streamsize xsputn(const char_t* input, const std::streamsize count) final {
        // allocate as much space as you need to fit the input
        if (const auto* new_end = this->pptr() + count; new_end >= this->epptr()) {
            const auto previous_size = this->pptr() - m_vector.data();
            m_vector.resize(new_end - m_vector.data());
            this->setp(m_vector.data() + previous_size, m_vector.data() + m_vector.size());
        }

        // copy the input into the buffer and advance pptr
        std::memcpy(this->pptr(), input, count);
        this->pbump(count);

        // all characters were successfully written into the buffer (or we ran out of memory)
        return count;
    }

    // fields ---------------------------------------------------------------------------------- //
private:
    /// The vector to write into.
    vector_t& m_vector;
};

NOTF_CLOSE_NAMESPACE

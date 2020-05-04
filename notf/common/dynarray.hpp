#pragma once

#include "notf/meta/types.hpp"

#include <boost/smart_ptr/make_shared.hpp>

NOTF_OPEN_NAMESPACE

// dyn array ======================================================================================================== //

namespace detail {

/// Array with a size known only at runtime.
template<class T, class EqualT, bool IsShared>
class DynArray {

    // types ------------------------------------------------------------------------------------ //
public:
    using value_type = T;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference = T&;
    using const_reference = const T&;
    using pointer = T*;
    using const_pointer = const T*;

private:
    template<class U>
    class Iterator : public std::iterator<std::random_access_iterator_tag, U> {

        // methods ---------------------------------------------------------- //
    public:
        /// Value constructor.
        constexpr explicit Iterator(U* position) : m_pos(position) {}

        /// Pre-increment.
        Iterator& operator++() {
            ++m_pos;
            return *this;
        }

        /// Pre-decrement.
        Iterator& operator--() {
            --m_pos;
            return *this;
        }

        /// Post-increment.
        Iterator operator++(int) {
            Iterator temp(*this);
            operator++();
            return temp;
        }

        /// Post-decrement.
        Iterator operator--(int) {
            Iterator temp(*this);
            operator--();
            return temp;
        }

        /// Value increment.
        /// @param offset   How far to move the iterator forwards.
        Iterator& operator+=(const difference_type offset) {
            m_pos += offset;
            return *this;
        }

        /// Value increment.
        /// @param offset   How far to move the iterator backwards.
        Iterator& operator-=(const difference_type offset) {
            m_pos -= offset;
            return *this;
        }

        /// @{
        /// Create a new iterator pointing to a offset ahead of this one.
        /// @param offset   How far to move forwards.
        Iterator operator+(const difference_type offset) const { return Iterator(m_pos + offset); }
        friend Iterator operator+(const difference_type offset, const Iterator& it) { return Iterator(it + offset); }
        /// @}

        /// Create a new iterator pointing to a offset behind of this one.
        /// @param offset   How far to move backwards.
        Iterator operator-(const difference_type offset) const { return Iterator(m_pos - offset); }

        /// Distance between this and the other iterator.
        difference_type operator-(const Iterator& rhs) const { return m_pos - rhs.m_pos; }

        /// Dereferences the iterator.
        U& operator*() { return *m_pos; }

        /// Access to the current element.
        U* operator->() { return m_pos; }

        /// Array-like access to the iterator.
        U& operator[](difference_type n) const { return m_pos[n]; }

        /// @{
        /// Comparison operators
        constexpr bool operator==(const Iterator& rhs) const noexcept { return m_pos == rhs.m_pos; }
        constexpr bool operator!=(const Iterator& rhs) const noexcept { return m_pos != rhs.m_pos; }
        constexpr bool operator>(const Iterator& rhs) const noexcept { return m_pos > rhs.m_pos; }
        constexpr bool operator<(const Iterator& rhs) const noexcept { return m_pos < rhs.m_pos; }
        constexpr bool operator>=(const Iterator& rhs) const noexcept { return m_pos >= rhs.m_pos; }
        constexpr bool operator<=(const Iterator& rhs) const noexcept { return m_pos <= rhs.m_pos; }
        /// @}

        // fields --------------------------------------------------------------- //
    private:
        /// Current element.
        U* m_pos;
    };

public:
    using iterator = Iterator<T>;
    using const_iterator = Iterator<const T>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    // methods ---------------------------------------------------------------------------------- //
public:
    /// Empty default constructor.
    DynArray() noexcept = default;

    /// Default initializing constructor.
    /// Is only enabled for trivially destructible types, otherwise we might accidentally call `delete` on memory that
    /// hasn't been initialized yet.
    /// @param size     Size of this array.
    template<class X = T, std::enable_if_t<std::is_trivially_destructible_v<X>, int> = 0>
    DynArray(size_type size) : m_size(size), m_data(_produce_data(m_size)) {}

    /// Fill constructor.
    /// @param size     Size of this array.
    /// @param value    Value to fill the array with.
    DynArray(size_type size, const_reference value) : m_size(size), m_data(_produce_data(m_size)) { fill(value); }

    /// Copy constructor.
    /// @param other    Array to copy.
    DynArray(const DynArray& other) : m_size(other.size()) {
        if constexpr (IsShared) {
            m_data = other.m_data;
        } else {
            m_data = _produce_data(m_size);
            std::copy(other.cbegin(), other.cend(), m_data.get());
        }
    }

    /// Copy assignment.
    /// @param rhs      Array to assign from.
    DynArray& operator=(const DynArray& rhs) {
        DynArray temp(rhs);
        swap(temp);
        return *this;
    }

    /// Move constructor.
    /// @param other    Array to move from.
    DynArray(DynArray&& other) noexcept { swap(other); }

    /// Move assignment.
    /// @param rhs      Array to move from.
    DynArray& operator=(DynArray&& rhs) noexcept {
        swap(rhs);
        return *this;
    }

    /// Iterator constructor.
    template<class Itr>
    DynArray(Itr first, Itr last) : m_size(std::distance(first, last)), m_data(_produce_data(m_size)) {
        for (size_type i = 0; first != last; ++first, ++i) {
            m_data.get()[i] = *first;
        }
    }

    /// @{
    /// Initializer list constructor.
    /// Has two implementations, one using straight-up copies for trivially copyable T, the other using ingest<T> to
    /// allow moving from the initializer list.
    /// @param list Initializer list.
    template<class X = T, class = std::enable_if_t<std::is_trivially_copyable_v<X>>>
    DynArray(std::initializer_list<X> list) : m_size(list.size()), m_data(_produce_data(m_size)) {
        std::copy(list.cbegin(), list.cend(), m_data.get());
    }
    template<class X = T, class = std::enable_if_t<!std::is_trivially_copyable_v<X>>>
    DynArray(std::initializer_list<ingest<X>> list) : m_size(list.size()), m_data(_produce_data(m_size)) {
        size_type i = 0;
        for (auto& element : list) {
            if (element.is_movable()) {
                m_data.get()[i++] = element.force_move();
            } else {
                m_data.get()[i++] = element;
            }
        }
    }
    /// @}

    /// @{
    /// Access to the raw data contained in this array.
    const_pointer data() const noexcept { return m_data.get(); }
    pointer data() noexcept { return m_data.get(); }
    /// @}

    /// @{
    /// Array-style access to an element in this array.
    /// @param index    Index of the element, is not bound checked;
    /// @returns        Element at the given index.
    const_reference operator[](const size_type index) const noexcept {
        NOTF_ASSERT(index < m_size);
        return data()[index];
    }
    reference operator[](const size_type index) noexcept { return NOTF_FORWARD_CONST_AS_MUTABLE(operator[](index)); }
    /// @}

    /// @{
    /// Bound-checked access to an element in this array.
    /// @param index    Index of the element, is not bound checked;
    /// @returns        Element at the given index.
    /// @throws         IndexError if the index is larger than the array size.
    const_reference at(const size_type index) const {
        if (index >= m_size) { NOTF_THROW(IndexError, "Invalid index {} for a DynArray of size {}", index, m_size); }
        return data()[index];
    }
    reference at(const size_type index) { return NOTF_FORWARD_CONST_AS_MUTABLE(at(index)); }
    /// @}

    /// The number of elements in this array.
    size_type size() const noexcept { return m_size; }

    /// Whether or not this array is empty.
    bool empty() const noexcept { return m_size != 0; }

    /// @{
    /// Get a forward-iterator to the begin of the array.
    iterator begin() noexcept { return iterator(data()); }
    const_iterator begin() const noexcept { return const_iterator(data()); }
    const_iterator cbegin() const noexcept { return const_iterator(data()); }
    /// @}

    /// @{
    /// Get a forward-iterator to one past the end of the array.
    iterator end() noexcept { return iterator(data() + m_size); }
    const_iterator end() const noexcept { return const_iterator(data() + m_size); }
    const_iterator cend() const noexcept { return const_iterator(data() + m_size); }
    /// @}

    /// @{
    /// Get a reverse-iterator to the end of the array.
    reverse_iterator rbegin() noexcept { return reverse_iterator(data() + m_size - 1); }
    const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(data() + m_size - 1); }
    const_reverse_iterator crbegin() const noexcept { return const_reverse_iterator(data() + m_size - 1); }
    /// @}

    /// @{
    /// Get a reverse-iterator to one before the start of the array.
    reverse_iterator rend() noexcept { return reverse_iterator(data() - 1); }
    const_reverse_iterator rend() const noexcept { return const_reverse_iterator(data() - 1); }
    const_reverse_iterator crend() const noexcept { return const_reverse_iterator(data() - 1); }
    /// @}

    /// Equality operator.
    constexpr bool operator==(const DynArray& rhs) const noexcept {
        if (m_size != rhs.m_size) { return false; }
        for (std::size_t i = 0; i < m_size; ++i) {
            if (!std::equal_to<T>{}(data()[i], rhs[i])) { return false; }
        }
        return true;
    }

    /// Inequality operator.
    constexpr bool operator!=(const DynArray& rhs) const noexcept { return !operator==(rhs); }

    /// Fills the entire array with copies of the given value.
    /// @param value    Value to fill the array with.
    void fill(const_reference value) { std::fill(data(), data() + m_size, value); }

    /// Swaps the contents of this array with other.
    /// @param other    Array to swap contents with.
    void swap(DynArray& other) noexcept {
        std::swap(m_size, other.m_size);
        m_data.swap(other.m_data);
    }

private:
    static auto _produce_data(const std::size_t size) {
        if constexpr (IsShared) {
            return boost::make_shared<T[]>(size);
        } else {
            return std::make_unique<T[]>(size);
        }
    }

    // fields ----------------------------------------------------------------------------------- //
private:
    /// Number of elements in this array.
    size_type m_size = 0;

    /// Pointer to the array data.
    decltype(_produce_data(0)) m_data = nullptr;
};

} // namespace detail

/// Array with a size known only at runtime.
/// Uses an shared_ptr to manage the data.
template<class T, class EqualT = std::equal_to<T>>
using DynArray = detail::DynArray<T, EqualT, false>;

/// Array with a size known only at runtime.
/// Uses an `shared_ptr` to manage the data.
template<class T, class EqualT = std::equal_to<T>>
using SharedDynArray = detail::DynArray<T, EqualT, true>;

NOTF_CLOSE_NAMESPACE
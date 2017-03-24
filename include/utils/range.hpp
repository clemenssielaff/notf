#pragma once

#include <assert.h>
#include <iterator>

/** Simple Range implementation to be used with std::vectors and other ordered containers. */
template <typename Iterator>
class Range {

public: // methods
    /** Constructor.
     * @param begin     Iterator to the first element in the Range.
     * @param distance  Distance from the first to the last element in the Range.
     */
    Range(Iterator begin, size_t distance)
        : m_begin(std::move(begin)), m_distance(distance) {}

    /** Iterator to the first element in the Range. */
    Iterator begin() const { return m_begin; }

    /** Iterator to one element past the last in the Range. */
    Iterator end() const
    {
        Iterator result = m_begin;
        std::advance(result, m_distance);
        return result;
    }

    /** Returns the number of elements in the Range. */
    size_t size() const { return m_distance + 1; }

    /** Shifts the Range's end by one element to the right. */
    void grow_one() { ++m_distance; }

private: // fields
    /** Iterator to the first element in the Range. */
    const Iterator m_begin;

    /** Distance from the first to the last element in the Range. */
    size_t m_distance;
};

/** Creates a Range from a given Container and two indices. */
template <typename Container>
Range<typename Container::iterator> make_range(Container& container, const size_t first, const size_t last)
{
    using Iterator = typename Container::iterator;
    if (first > last) {
        assert(false);
        return make_range(container, first, first);
    }
    Iterator begin = std::begin(container);
    std::advance(begin, first);
    return Range<Iterator>(std::move(begin), last - first);
}

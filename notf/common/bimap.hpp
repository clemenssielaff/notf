#pragma once

#include <optional>
#include <set>

#include "notf/meta/assert.hpp"
#include "notf/meta/exception.hpp"

NOTF_OPEN_NAMESPACE

// bimap ============================================================================================================ //

/// Is a bit hacky, but okay for now.
/// from https://stackoverflow.com/a/21917041
template<class A, class B, class = std::enable_if_t<!std::is_same_v<A, B>>>
class Bimap {

    // types ----------------------------------------------------------------------------------- //
private:
    /// Value types.
    using AB = std::pair<A, B*>;
    using BA = std::pair<B, A*>;

    /// Comparison functor ignoring the second element of a pair.
    template<class T>
    struct first_only_less {
        constexpr bool operator()(const T& lhs, const T& rhs) const { return lhs.first < rhs.first; }
    };

    // methods --------------------------------------------------------------------------------- //
public:
    /// @{
    /// Returns the value assocated with the given one.
    std::optional<A> get(const B& b) const {
        if (auto itr = _find(b); itr != m_ba.end()) {
            return *itr->second;
        } else {
            return {};
        }
    }
    std::optional<B> get(const A& a) const {
        if (auto itr = _find(a); itr != m_ab.end()) {
            return *itr->second;
        } else {
            return {};
        }
    }
    /// @}

    /// @{
    /// Checks if this Bimap contains the given value.
    bool contains(const A& a) const { return _find(a) != m_ab.end(); }
    bool contains(const B& b) const { return _find(b) != m_ba.end(); }
    /// @}

    /// @{
    /// Inserts a new value pair into the Bimap. Both values must not be already present.
    /// @throws NotUniqueError  If one or both values are already in the Bimap.
    void set(A a, B b) {
        if (contains(a) || contains(b)) { NOTF_THROW(NotUniqueError); }
        auto iter_ab = m_ab.emplace(std::make_pair(std::forward<A>(a), nullptr)).first;
        auto iter_ba = m_ba.emplace(std::make_pair(std::forward<B>(b), const_cast<A*>(&iter_ab->first))).first;
        *const_cast<B**>(&iter_ab->second) = const_cast<B*>(&iter_ba->first);
    }
    void set(B b, A a) { set(std::forward<A>(a), std::forward<B>(b)); }
    /// @}

    /// @{
    /// Removes the pair containing the given value from the Bimap.
    /// Does nothing if the value is not in the Bimap.
    void remove(const A& a) {
        if (auto ab_itr = _find(a); ab_itr != m_ab.end()) {
            if (auto ba_itr = _find(*ab_itr->second); ba_itr != m_ba.end()) { m_ba.erase(ba_itr); }
            m_ab.erase(ab_itr);
        }
    }
    void remove(const B& b) {
        if (auto ba_itr = _find(b); ba_itr != m_ba.end()) {
            if (auto ab_itr = _find(*ba_itr->second); ab_itr != m_ab.end()) { m_ab.erase(ab_itr); }
            m_ba.erase(ba_itr);
        }
    }
    /// @}

private:
    /// @{
    /// Finds a given value in the Bimap.
    auto _find(const A& a) const { return m_ab.find(std::make_pair(a, nullptr)); }
    auto _find(const B& b) const { return m_ba.find(std::make_pair(b, nullptr)); }
    /// @}

    // fields ---------------------------------------------------------------------------------- //
private:
    /// Map A->B
    std::set<AB, first_only_less<AB>> m_ab;

    /// Map B->A
    std::set<BA, first_only_less<BA>> m_ba;
};

NOTF_CLOSE_NAMESPACE

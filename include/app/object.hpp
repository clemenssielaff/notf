#pragma once

#include <bitset>
#include <memory>
#include <vector>

#include "app/component.hpp"

namespace { // anomymous

void null_destructor(void*) {}

} // namespace anoymous

namespace untitled {

using std::bitset;
using std::vector;
using std::shared_ptr;

///
/// \brief
///
/// Several objects can share the same components.
///
class Object {

public: // methods
    ///
    /// \brief Default constructor.
    ///
    explicit Object()
        : m_components()
        , m_component_kinds()
        , m_monitor{ this, null_destructor }
    {
    }

    ///
    /// \brief Copy constructor.
    ///
    /// \param other    Object to copy.
    ///
    explicit Object(const Object& other)
        : m_components(other.m_components)
        , m_component_kinds(other.m_component_kinds)
        , m_monitor{ this, null_destructor }
    {
    }

    ///
    /// \brief Move constructor.
    ///
    /// \param other    Object to move.
    ///
    Object(Object&& other)
        : Object()
    {
        swap(*this, other);
    }

    ///
    /// \brief operator =
    ///
    /// \param other    Object to assign from.
    ///
    /// \return This object after copying other's data.
    ///
    Object& operator=(Object other)
    {
        swap(*this, other);
        return *this;
    }

    ///
    /// \brief std::swap implementation for Qbjects.
    ///
    /// \param a    Object a.
    /// \param b    Object b.
    ///
    friend void swap(Object& a, Object& b)
    {
        using std::swap;
        swap(a.m_components, b.m_components);
        swap(a.m_component_kinds, b.m_component_kinds);
    }

    ///
    /// \brief Checks if this Object contains a Component of the given kind.
    ///
    /// \param kind Component kind to check for.
    ///
    /// \return True iff this Object contains a Component of the given kind - false otherwise.
    ///
    bool has_component_kind(Component::KIND kind) const { return m_component_kinds[static_cast<size_t>(kind)]; }

protected: // methods
    ///
    /// \brief Attaches a new Component to this Object.
    ///
    /// Each Object can only have one instance of each Component kind.
    /// In debug builds, attaching a Component of a kind allready attached to this Object will throw an assert.
    /// In release builds, it will just return false;
    ///
    /// \param kind The kind of Component to attach to this Object.
    ///
    /// \return True on success. False, if a Component of the type is already attached.
    ///
    bool attach_component(shared_ptr<Component> component);

private: // fields
    ///
    /// \brief All components of this Object.
    ///
    vector<shared_ptr<Component> > m_components;

    ///
    /// \brief Bitmask representing the existing Component kinds of this Object.
    ///
    bitset<componentKindCount()> m_component_kinds;

    ///
    /// \brief Monitor used by weak_ptr in lambdas to check if this instance has been deleted.
    ///
    /// Must not be copied, moved or swapped!
    ///
    shared_ptr<void> m_monitor;
};

} // namespace untitled

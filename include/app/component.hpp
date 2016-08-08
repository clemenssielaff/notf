#pragma once

#include <memory>

#include "common/devel.hpp"

namespace signal {

class Component : public std::enable_shared_from_this<Component> {

    friend class Application;

public: // enum
    /// \brief Component kind enum.
    /// This enum controls a lot more than is immediately apparent.
    /// It acts as a unique identifier of each Component type, as index for the Widget components member
    /// and defines the order in which Components are updated by the Application.
    /// All of these uses are orthogonal, which is why we allow this kind of overloading.
    /// Just be sure you know what you are doing when you modify the enum.
    enum class KIND {
        SHAPE = 0, // first entry must be zero
        TEXTURE,
        _count, // must always be the last entry
    };

public: // methods
    /// \brief Defautl Constructor.
    explicit Component();

    /// \brief Virtual Destructor.
    virtual ~Component();

    /// \brief This Component's type.
    virtual KIND get_kind() const = 0;

    /// \brief Checks whether this Component has been flaged as dirty.
    bool is_dirty() const { return m_is_dirty; }

    /// \brief Registers this Component to be updated by the Application before the next frame.
    void update();

public: // static methods
    /// \brief The number of available components.
    static constexpr auto get_count() { return to_number(KIND::_count); }

private: // methods for Application
    /// \brief Lets the Component know that it has been cleaned.
    void set_clean() { m_is_dirty = false; }

private: // fields
    /// \brief Dirty flag for Components.
    bool m_is_dirty;
};

} // namespace signal

#pragma once

#include "app/forwards.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

class Scene {

    // types ---------------------------------------------------------------------------------------------------------//
protected:
    /// Token object to make sure that object instances can only be created by a call to `_create`.
    class Token {
        friend class Scene;
        Token() {} // not "= default", otherwise you could construct a Token via `Token{}`.
    };

    // methods -------------------------------------------------------------------------------------------------------//
private:
    NOTF_ALLOW_MAKE_SMART_FROM_PRIVATE

    /// Constructor.
    Scene(const Token&) {}

protected:
    /// Factory method for this type and all of its children.
    /// You need to call this function from your own factory in order to get a Token instance.
    template<typename T, typename... Ts>
    static std::shared_ptr<T> _create(Ts&&... args)
    {
        static_assert(std::is_base_of<Scene, T>::value, "Scene::_create can only create instances of subclasses of "
                                                        "Scene");
        const Token token;
        return NOTF_MAKE_UNIQUE_FROM_PRIVATE(T, std::forward<Ts>(args)...);
    }

public:
    NOTF_NO_COPY_OR_ASSIGN(Scene)

    /// Destructor.
    virtual ~Scene();

    /// Handles an untyped event.
    /// @returns True iff the event was handled.
    virtual bool handle_event(Event& event) = 0;
};

NOTF_CLOSE_NAMESPACE

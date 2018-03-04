#pragma once

#include "app/forwards.hpp"
#include "common/size2.hpp"
#include "utils/make_smart_enabler.hpp"

NOTF_OPEN_NAMESPACE

// ===================================================================================================================//

class Scene {

    // types ---------------------------------------------------------------------------------------------------------//
protected:
    /// Token object to make sure that object instances can only be created by a call to `_create`.
    class Token {
        friend class Scene;
        Token() = default;
    };

    // methods -------------------------------------------------------------------------------------------------------//
protected:
    /// Constructor.
    Scene(const Token&) {}

    /// Factory method for this type and all of its children.
    /// You need to call this function from your own factory in order to get a Token instance.
    template<typename T, typename... Ts>
    static std::shared_ptr<T> _create(Ts&&... args)
    {
        static_assert(std::is_base_of<GraphicsProducer, T>::value, "Scene::_create can only create instances of "
                                                                   "subclasses of GraphicsProducer");
        const Token token;
#ifdef NOTF_DEBUG
        auto result = std::shared_ptr<T>(new T(token, std::forward<Ts>(args)...));
#else
        auto result = std::make_shared<make_shared_enabler<T>>(token, std::forward<Ts>(args)...);
#endif
        return result;
    }

public:
    NOTF_NO_COPY_OR_ASSIGN(Scene)

    /// Destructor.
    virtual ~Scene();

    /// Factory.
    static ScenePtr create();

    // event handlers ---------------------------------------------------------

    /// Called when the a mouse button is pressed or released, the mouse is moved inside the Window, the mouse wheel
    /// scrolled or the cursor enters or exists the client area of a Window.
    /// @param event    MouseEvent.
    /// @returns        True iff the Scene handled the event and it doesn't need to be propagated further.
    virtual bool propagate(MouseEvent&& event) = 0;

    /// Called when a key is pressed, repeated or released.
    /// @param event    KeyEvent.
    /// @returns        True iff the Scene handled the event and it doesn't need to be propagated further.
    virtual bool propagate(KeyEvent&& event) = 0;

    /// Called when an unicode code point is generated.
    /// @param event    CharEvent.
    /// @returns        True iff the Scene handled the event and it doesn't need to be propagated further.
    virtual bool propagate(CharEvent&& event) = 0;

    /// Called when the Window containing the Scene is resized.
    /// @param size     New size.
    virtual void resize(Size2i size) = 0;
};

NOTF_CLOSE_NAMESPACE

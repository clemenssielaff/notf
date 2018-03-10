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
        Token() {} // not "= default", otherwise you could construct a Token via `Token{}`.
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
        static_assert(std::is_base_of<Scene, T>::value, "Scene::_create can only create instances of subclasses of "
                                                        "Scene");
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

    // event handlers ---------------------------------------------------------

    /// Called when the a mouse button is pressed or released, the mouse is moved inside the Window, the mouse wheel
    /// scrolled or the cursor enters or exists the client area of a Window.
    /// @param event    MouseEvent.
    virtual void propagate(MouseEvent& event) = 0;

    /// Called when a key is pressed, repeated or released.
    /// @param event    KeyEvent.
    virtual void propagate(KeyEvent& event) = 0;

    /// Called when an unicode code point is generated.
    /// @param event    CharEvent.
    virtual void propagate(CharEvent& event) = 0;

    /// Called when the cursor enters or exits the Window's client area or the window is about to be closed.
    /// @param event    WindowEvent.
    virtual void propagate(WindowEvent& event) = 0;

    /// Called when the Window containing the Scene is resized.
    /// @param size     New size.
    virtual void resize(const Size2i& size) = 0;
};

NOTF_CLOSE_NAMESPACE

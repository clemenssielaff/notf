#include "app/scene/widget/hierarchy.hpp"

#include <unordered_set>

#include "app/io/char_event.hpp"
#include "app/io/focus_event.hpp"
#include "app/io/key_event.hpp"
#include "app/io/mouse_event.hpp"
#include "app/scene/widget/root_layout.hpp"
#include "app/scene/widget/widget.hpp"
#include "utils/reverse_iterator.hpp"

namespace { // anonymous
NOTF_USING_NAMESPACE

/// Helper function that propagates an event up the Item hierarchy.
/// @param widget    Widget receiving the original event.
/// @param signal    Signal to trigger on each ScreenItem in the hierarchy.
/// @param event     Event object that is passed as an argument to the Signals.
/// @param notified  ScreenItems that have already been notified of this event and that must not handle it again.
/// @return          True iff the ScreenItem handled the event.
template<class Event, typename... Args>
bool propagate_to_hierarchy(const Widget* widget, Signal<Args...> ScreenItem::*signal, Event& event,
                            std::unordered_set<const ScreenItem*>& notified_items)
{
    const ScreenItem* screen_item = widget;
    while (screen_item) {
        // don't propagate the event to items that have already seen (but not handled) it
        if (notified_items.count(screen_item)) {
            return false;
        }
        notified_items.insert(screen_item);

        // fire the signal and return if it was handled
        (screen_item->*signal)(event);
        if (event.was_handled()) {
            return true;
        }
        screen_item = make_raw(screen_item->layout());
    }
    return false;
}

} // namespace

//====================================================================================================================//

NOTF_OPEN_NAMESPACE

namespace detail {

struct traverse {

    class Iterator {
    public:
        explicit Iterator(long _num = 0); // : num(_num) {}
        Iterator& operator++()
        {
            //            num = TO >= FROM ? num + 1 : num - 1;
            return *this;
        }
        Iterator operator++(int)
        {
            iterator retval = *this;
            ++(*this);
            return retval;
        }
        bool operator==(const Iterator& other) const; // { return num == other.num; }
        bool operator!=(const Iterator& other) const; // { return !(*this == other); }
        Widget& operator*() const;             // { return num; }
    };

    /// Constructor.
    /// @param hierarchy    ItemHierarchy to traverse.
    traverse(ItemHierarchy* hierarchy) : m_hierarchy(*hierarchy) {}

    Iterator begin(); // { return iterator(FROM); }

    Iterator end(); //  { return iterator(TO >= FROM ? TO + 1 : TO - 1); }

    ItemHierarchy& m_hierarchy;
};

// TODO: continue here
// by implementing the traverse Iterator
// then remove all risky_ptr from Item and replace them with raw pointers and throw exception when nullptr is requested
// then make everything compile (I think that will be a bit of work)
// then commit, make a tag and create a new example with the application, window etc.
// move graphics_context and fontManager into Window
// make fog example with the time as property to exercise the redraw and timing mechanism
// next step: create proper item hierarchy and see if you can reactivate the layouts

} // namespace detail

using detail::traverse;

//====================================================================================================================//

ItemHierarchy::ItemHierarchy(const Token& token) : Scene(token), m_root(RootLayout::Access<ItemHierarchy>::create()) {}

void ItemHierarchy::propagate(MouseEvent& event)
{
    WidgetPtr mouse_item = m_mouse_item.lock();
    std::unordered_set<const ScreenItem*> notified_items;

    // call the appropriate event signal
    if (event.action == MouseAction::MOVE) {
        if (mouse_item) {
            mouse_item->on_mouse_move(event);
            if (event.was_handled()) {
                return;
            }
            notified_items.insert(mouse_item.get());
        }
        for (const auto& widget : traverse(this)) {
            if (propagate_to_hierarchy(&widget, &ScreenItem::on_mouse_move, event, notified_items)) {
                return;
            }
        }
    }

    else if (event.action == MouseAction::SCROLL) {
        if (mouse_item) {
            mouse_item->on_mouse_scroll(event);
            if (event.was_handled()) {
                return;
            }
            notified_items.insert(mouse_item.get());
        }
        for (const auto& widget : traverse(this)) {
            if (propagate_to_hierarchy(&widget, &ScreenItem::on_mouse_scroll, event, notified_items)) {
                return;
            }
        }
    }

    else if (event.action == MouseAction::PRESS) {
        assert(!mouse_item);
        for (auto& widget : traverse(this)) {
            if (propagate_to_hierarchy(&widget, &ScreenItem::on_mouse_button, event, notified_items)) {
                WidgetPtr new_focus_widget = std::static_pointer_cast<Widget>(widget.shared_from_this());
                m_mouse_item               = new_focus_widget;

                // do nothing if the item already has the focus
                WidgetPtr old_focus_widget = m_keyboard_item.lock();
                if (new_focus_widget == old_focus_widget) {
                    return;
                }

                // send the mouse item a 'focus gained' event and notify its hierarchy, if it handles it
                FocusEvent focus_gained_event(FocusAction::GAINED, old_focus_widget, new_focus_widget);
                new_focus_widget->on_focus_changed(focus_gained_event);
                if (focus_gained_event.was_handled()) {

                    // let the previously focused Widget know that it lost the focus
                    if (old_focus_widget) {
                        FocusEvent focus_lost_event(FocusAction::LOST, old_focus_widget, new_focus_widget);
                        ScreenItem* handler = old_focus_widget.get();
                        while (handler) {
                            handler->on_focus_changed(focus_lost_event);
                            handler = make_raw(handler->layout());
                        }
                    }

                    // notify the new focused Widget's hierarchy
                    m_keyboard_item     = new_focus_widget;
                    ScreenItem* handler = make_raw(new_focus_widget->layout());
                    while (handler) {
                        handler->on_focus_changed(focus_gained_event);
                        handler = make_raw(handler->layout());
                    }
                }
                else {
                    // if it doesn't handle the focus event, the focus remains untouched
                }
                return;
            }
        }
    }

    else if (event.action == MouseAction::RELEASE) {
        if (mouse_item) {
            m_mouse_item.reset();
            mouse_item->on_mouse_button(event);
            if (event.was_handled()) {
                return;
            }
            notified_items.insert(mouse_item.get());
        }
        for (const auto& widget : traverse(this)) {
            if (propagate_to_hierarchy(&widget, &ScreenItem::on_mouse_button, event, notified_items)) {
                return;
            }
        }
    }

    else {
        assert(0);
    }
}

void ItemHierarchy::propagate(KeyEvent& event)
{
    std::unordered_set<const ScreenItem*> notified_items;
    if (WidgetPtr keyboard_item = m_keyboard_item.lock()) {
        propagate_to_hierarchy(keyboard_item.get(), &ScreenItem::on_key, event, notified_items);
    }
    else {
        // if there is no keyboard item, let the RootLayout fire its signal
        m_root->on_key(event);
    }
}

void ItemHierarchy::propagate(CharEvent& event)
{
    std::unordered_set<const ScreenItem*> notified_items;
    if (WidgetPtr keyboard_item = m_keyboard_item.lock()) {
        propagate_to_hierarchy(keyboard_item.get(), &ScreenItem::on_char_input, event, notified_items);
    }
    else {
        // if there is no keyboard item, let the RootLayout fire its signal
        m_root->on_char_input(event);
    }
}

void ItemHierarchy::resize(const Size2i& size) { RootLayout::Access<ItemHierarchy>(*m_root).set_grant(size); }

NOTF_CLOSE_NAMESPACE

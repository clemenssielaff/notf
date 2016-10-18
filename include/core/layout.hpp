#pragma once

#include <assert.h>
#include <memory>
#include <type_traits>
#include <unordered_map>

#include "common/signal.hpp"
#include "common/size2r.hpp"
#include "common/transform2.hpp"
#include "core/abstract_item.hpp"

namespace signal {

class LayoutObject;
class Widget;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/// \brief Visibility states, all but one mean that the LayoutObject is not visible, but all for different reasons.
enum class VISIBILITY {
    INVISIBLE, // LayoutObject is not displayed
    HIDDEN, // LayoutObject is not INVISIBLE but one of its parents is, so it cannot be displayed
    UNROOTED, // LayoutObject and all ancestors are not INVISIBLE, but the Widget is not a child of a RootWidget
    VISIBLE, // LayoutObject is displayed
};

/// \brief Coordinate Spaces to pass to get_transform().
enum class SPACE {
    PARENT, // returns transform in local coordinates, relative to the parent LayoutObject
    WINDOW, // returns transform in global coordinates, relative to the Window
    SCREEN, // returns transform in screen coordinates, relative to the screen origin
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * \brief Layout or Widget, something that can be put into a Layout.
 */
class LayoutObject : public AbstractItem {

protected: // classes
    /// \brief Abstract baseclass providing an Iterator over all child LayoutObjects of a LayoutObject subclass.
    class ChildIteratorBase {

    public:
        /// \brief Virtual Destructor
        virtual ~ChildIteratorBase();

        /// \brief Iteration method returning the next available LayoutObject, an invalid means the iteration is over.
        virtual const std::shared_ptr<LayoutObject>& next() = 0;

    protected: // methods
        /// \brief Default Constructor.
        explicit ChildIteratorBase() = default;

        /// \brief Factory function to create a new ChildIterator.
        template <typename T, typename = std::enable_if_t<std::is_base_of<ChildIteratorBase, T>::value>, typename... ARGS>
        static std::unique_ptr<T> create_iterator(ARGS&&... args)
        {
            return std::make_unique<MakeSmartEnabler<T>>(std::forward<ARGS>(args)...);
        }

        /// \brief Returns a reference to an invalid LayoutObject to be used as last returned item in next().
        const std::shared_ptr<LayoutObject>& get_nil() const
        {
            static const std::shared_ptr<LayoutObject> nil;
            return nil;
        }
    };

public: // abstract methods
    /// \brief Looks for a Widget at a given local position.
    /// \param local_pos    Local coordinates where to look for the Widget.
    /// \return The Widget at a given local position or an empty shared_ptr if there is none.
    virtual std::shared_ptr<Widget> get_widget_at(const Vector2& local_pos) const = 0;

    /// \brief Tells the containing layout to redraw (potentially cascading up the Widget ancestry).
    virtual void redraw() = 0;

protected: // abstract methods
    /// \brief Produces an Iterator over all children of this LayoutObject subclass.
    virtual std::unique_ptr<ChildIteratorBase> get_child_iterator() const = 0;

    /// \brief Removes a child LayoutObject.
    /// \param parent   Child LayoutObject to remove.
    virtual void remove_child(std::shared_ptr<LayoutObject> child) = 0;

public: // methods
    /// \brief Virtual destructor.
    virtual ~LayoutObject() override;

    /// \brief Returns true iff this LayoutObject has a parent
    bool has_parent() const { return !m_parent.expired(); }

    /// \brief Tests, if `ancestor` is an ancestor of this LayoutObject.
    /// \param ancestor Potential ancestor
    /// \return True iff `ancestor` is an ancestor of this LayoutObject, false otherwise.
    bool is_ancestor_of(const std::shared_ptr<LayoutObject> ancestor);

    /// \brief Tests if a given LayoutObject is a child of this LayoutObject.
    bool has_child(const std::shared_ptr<LayoutObject> candidate);

    /// \brief Returns the parent LayoutObject containing this LayoutObject, may be invalid.
    std::shared_ptr<LayoutObject> get_parent() const { return m_parent.lock(); }

    /// \brief Returns the LayoutObject of the hierarchy containing this LayoutObject.
    /// Is invalid if this LayoutObject is unrooted.
    std::shared_ptr<const LayoutObject> get_root_item() const;

    /// \brief Checks the visibility of this LayoutObject.
    VISIBILITY get_visibility() const { return m_visibility; }

    /// \brief Returns the unscaled size of this LayoutObject in pixels.
    const Size2r& get_size() const { return m_size; }

    /// \brief Returns this LayoutObject's transformation in the given space.
    Transform2 get_transform(const SPACE space) const
    {
        Transform2 result = Transform2::identity();
        switch (space) {
        case SPACE::WINDOW:
            get_window_transform(result);
            break;

        case SPACE::SCREEN:
            result = get_screen_transform();
            break;

        case SPACE::PARENT:
            result = get_parent_transform();
            break;

        default:
            assert(0);
        }
        return result;
    }

public: // signals
    /// \brief Emitted, when the visibility of this LayoutObject has changed.
    /// \param New visiblity.
    Signal<VISIBILITY> visibility_changed;

    /// \brief Emitted when this LayoutObject got a new parent.
    /// \param New parent.
    Signal<std::shared_ptr<LayoutObject>> parent_changed;

    /// \brief Emitted when this LayoutObject' size changed.
    /// \param New size.
    Signal<Size2r> size_changed;

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Application-unique Handle of this Item.
    explicit LayoutObject(const Handle handle)
        : AbstractItem(handle)
        , m_parent()
        , m_visibility(VISIBILITY::VISIBLE)
        , m_size()
        , m_transform(Transform2::identity())
    {
    }

    /// \brief Sets a new LayoutObject to contain this LayoutObject.
    void set_parent(std::shared_ptr<LayoutObject> parent);

    /// \brief Removes the current parent of this LayoutObject.
    void unparent() { set_parent({}); }

    /// \brief Shows (if possible) or hides this LayoutObject.
    void change_visibility(const bool is_visible);

    /// \brief Updates the size of this LayoutObject.
    void set_size(const Size2r size)
    {
        m_size = size;
        size_changed(m_size);
    }

private: // methods
    /// \brief Recursive function to let all children emit visibility_changed when the parent's visibility changed.
    void cascade_visibility(const VISIBILITY visibility);

    /// \brief Recursive implementation to produce the LayoutObject's transformation in window space.
    void get_window_transform(Transform2& result) const;

    /// \brief Returns the LayoutObject's transformation in screen space.
    Transform2 get_screen_transform() const;

    /// \brief Returns the LayoutObject's transformation in parent space.
    Transform2 get_parent_transform() const { return m_transform; }

protected: // fields
    /// \brief The parent LayoutObject, may be invalid.
    std::weak_ptr<LayoutObject> m_parent;

    /// \brief Visibility state of this LayoutObject.
    VISIBILITY m_visibility;

    /// \brief Unscaled size of this LayoutObject in pixels.
    Size2r m_size;

    /// \brief 2D transformation of this LayoutObject in local space.
    Transform2 m_transform;

    CALLBACKS(LayoutObject)
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class LayoutItem {

public:
    /// \brief Returns the LayoutObject owned by this Item.
    std::shared_ptr<LayoutObject>& get_object() { return m_object; }

private:
    /// \brief The LayoutObject owned by this Item.
    std::shared_ptr<LayoutObject> m_object;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename LayoutItemType, typename = std::enable_if_t<std::is_base_of<LayoutItem, LayoutItemType>::value>>
class Layout : public LayoutObject {

private: // classes
    /// \brief Iterator over all child LayoutObjects of a LayoutObject subclass.
    class ChildIterator : public ChildIteratorBase {

    protected: // methods
        /// \brief Value Constructor.
        /// \param layout   Layout whose children to iterate over.
        explicit ChildIterator(const Layout& layout)
            : ChildIteratorBase()
            , m_group_iterator(layout.m_items.cbegin())
            , m_group_end(layout.m_items.cend())
            , m_child_iterator((*m_group_iterator).cbegin())
            , m_child_end((*m_group_iterator).cend())
        {
        }

        /// \brief Iteration method returning the next available LayoutObject, an invalid means the iteration is over.
        virtual const std::shared_ptr<LayoutObject>& next() override
        {
            // if the child iterator is valid, return and increment
            if (m_child_iterator != m_child_end) {
                return (m_child_iterator++)->second->get_object();
            }

            // if both iterators are through, return nil
            else if (m_group_iterator == m_group_end) {
                return get_nil();
            }

            // if the child iterator is through, go to the next group
            m_group_iterator++;
            if (m_group_end != m_group_iterator) {
                m_child_iterator = (*m_group_iterator).cbegin();
                m_child_end = (*m_group_iterator).cend();
            }
            return next();
        }

    public: // static methods
        static std::unique_ptr<ChildIterator> create(const Layout& layout)
        {
            return create_iterator<ChildIterator>(layout);
        }

    private: // fields
        /// \brief Group iterator.
        std::vector<std::unordered_map<Handle, std::unique_ptr<LayoutItem>>>::const_iterator m_group_iterator;

        /// \brief Last group iterator.
        const std::vector<std::unordered_map<Handle, std::unique_ptr<LayoutItem>>>::const_iterator m_group_end;

        /// \brief Child iterator
        std::unordered_map<Handle, std::unique_ptr<LayoutItem>>::const_iterator m_child_iterator;

        /// \brief Last child iterator.
        std::unordered_map<Handle, std::unique_ptr<LayoutItem>>::const_iterator m_child_end;
    };

public: // methods
    /// \brief Shows (if possible) or hides this LayoutObject.
    void set_visible(const bool is_visible) { change_visibility(is_visible); }

protected: // methods
    /// \brief Value Constructor.
    /// \param handle   Application-unique Handle of this Item.
    explicit Layout(const Handle handle)
        : LayoutObject(handle)
    {
    }

    /// \brief Produces an Iterator over all children of this LayoutObject subclass.
    virtual std::unique_ptr<ChildIteratorBase> get_child_iterator() const override { return ChildIterator::create(*this); }

protected: // fields
    /// \brief LayoutItems contained in this Layout
    std::vector<std::unordered_map<Handle, std::unique_ptr<LayoutItem>>> m_items;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} // namespace signal

// TODO: redraw methods
// As is it set up right now, there is no strong relationship between a Widget and its child widgets, they can be
// positioned all over the place.
// Unlike in Qt, we cannot say that a widget must update all of its child widgets when it redraws but that is a good
// thing as you might not need that.
// Instead, every widget that changes should register with the Window's renderer.
// Just before rendering, the Renderer then figures out what Widgets to redraw, only consulting their bounding box
// overlaps and z-values, ignoring the Widget hierarchy.

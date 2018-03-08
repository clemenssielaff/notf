#include "app/scene/widget/screen_item.hpp"

#include "app/core/window.hpp"
#include "app/scene/widget/controller.hpp"
#include "app/scene/widget/layout.hpp"
#include "common/log.hpp"

namespace { // anonymous

static constexpr float g_alpha_cutoff = 1.f / (255 * 2);

} // namespace

NOTF_OPEN_NAMESPACE

template<>
const Matrix3f ScreenItem::xform<ScreenItem::Space::WINDOW>() const
{
    Matrix3f result = Matrix3f::identity();
    _window_transform(result);
    return result;
}

void ScreenItem::set_offset_xform(const Matrix3f transform)
{
    if (transform == m_offset_transform) {
        return;
    }
    m_offset_transform = std::move(transform);
    on_xform_changed(m_offset_transform * m_layout_transform);
    _redraw();
}

float ScreenItem::opacity(bool effective) const
{
    if (abs(m_opacity) < g_alpha_cutoff) {
        return 0;
    }
    if (effective) {
        if (risky_ptr<const Layout> parent_layout = first_ancestor<Layout>()) {
            return m_opacity * parent_layout->opacity();
        }
    }
    return m_opacity;
}

void ScreenItem::set_opacity(float opacity)
{
    opacity = clamp(opacity, 0, 1);
    if (abs(m_opacity - opacity) <= precision_high<float>()) {
        return;
    }
    m_opacity = opacity;
    on_opacity_changed(m_opacity);
    _redraw();
}

bool ScreenItem::is_visible() const
{
    // explicitly marked as not visible
    if (!m_is_visible) {
        return false;
    }

    // bounding rect too small
    if (m_size.area() <= precision_low<float>()) {
        return false;
    }

    // fully transparent
    if (opacity() < g_alpha_cutoff) {
        return false;
    }

    // fully scissored
    if (m_scissor_layout) {
        Aabrf content_aabr = m_content_aabr;
        transformation_between(this, m_scissor_layout).transform(content_aabr);
        Aabrf scissor_aabr(m_scissor_layout->size());
        m_scissor_layout->xform<Space::PARENT>().transform(scissor_aabr);
        if (!scissor_aabr.intersects(content_aabr)) {
            return false;
        }
    }

    // visible
    return true;
}

void ScreenItem::set_visible(bool is_visible)
{
    if (is_visible == m_is_visible) {
        return;
    }
    m_is_visible = is_visible;
    _update_parent_layout();
    on_visibility_changed(m_is_visible);
}

void ScreenItem::set_scissor(const Layout* scissor_layout)
{
    if (!has_ancestor(scissor_layout)) {
        log_critical << "Cannot set Layout " << scissor_layout->name() << " as scissor of Item " << name()
                     << " because it is not an ancestor of " << name();
        scissor_layout = nullptr;
    }
    _set_scissor(scissor_layout);
}

void ScreenItem::_update_from_parent()
{
    Item::_update_from_parent();
    if (m_scissor_layout && !has_ancestor(m_scissor_layout)) {
        log_critical << "Item \"" << name() << "\" was moved out of the child hierarchy from its scissor layout: \""
                     << m_scissor_layout->name() << "\" and will no longer be scissored by it";
        m_scissor_layout = nullptr;
    }
}

void ScreenItem::_update_parent_layout()
{
    risky_ptr<Layout> parent_layout = first_ancestor<Layout>();
    while (parent_layout) {
        // if the parent Layout's Claim changed, we also need to update the grandparent ...
        if (Layout::Access<ScreenItem>(*parent_layout).update_claim()) {
            parent_layout = parent_layout->first_ancestor<Layout>();
        }

        // ... otherwise, we have reached the end of the propagation through the ancestry
        // and continue to relayout all children from the parent downwards
        else {
            parent_layout->_relayout();
            break;
        }
    }
}

bool ScreenItem::_set_claim(const Claim claim)
{
    if (claim == m_claim) {
        return false;
    }
    m_claim = std::move(claim);
    _update_parent_layout();
    return true;
}

bool ScreenItem::_set_grant(const Size2f grant)
{
    if (grant == m_grant) {
        return false;
    }
    m_grant = std::move(grant);
    _relayout();
    return true;
}

bool ScreenItem::_set_size(const Size2f size)
{
    if (size == m_size) {
        return false;
    }
    m_size = std::move(size);
    on_size_changed(m_size);
    _redraw();
    return true;
}

void ScreenItem::_set_content_aabr(const Aabrf aabr) { m_content_aabr = std::move(aabr); }

void ScreenItem::_set_layout_xform(const Matrix3f transform)
{
    if (transform == m_layout_transform) {
        return;
    }
    m_layout_transform = std::move(transform);
    on_xform_changed(m_offset_transform * m_layout_transform);
    _redraw();
}

void ScreenItem::_set_scissor(const Layout* scissor_layout)
{
    if (scissor_layout == m_scissor_layout) {
        return;
    }
    m_scissor_layout = scissor_layout;
    on_scissor_changed(m_scissor_layout);
    _redraw();
}

void ScreenItem::_window_transform(Matrix3f& result) const
{
    if (const risky_ptr<const ScreenItem> parent_layout = first_ancestor<Layout>()) {
        parent_layout->_window_transform(result);
        result.premult(m_offset_transform * m_layout_transform);
    }
}

//====================================================================================================================//

risky_ptr<ScreenItem> get_screen_item(Item* item)
{
    if (ScreenItem* screen_item = dynamic_cast<ScreenItem*>(item)) {
        return screen_item;
    }
    else if (Controller* controller = dynamic_cast<Controller*>(item)) {
        return controller->root_item();
    }
    return nullptr;
}

Matrix3f transformation_between(const ScreenItem* source, const ScreenItem* target)
{
    risky_ptr<const ScreenItem> common_ancestor = get_screen_item(make_raw(source->common_ancestor(target)));
    if (!common_ancestor) {
        std::stringstream ss;
        ss << "Cannot find common ancestor for Items " << source->name() << " and " << target->name();
        throw std::runtime_error(ss.str());
    }

    Matrix3f source_branch = Matrix3f::identity();
    for (risky_ptr<const ScreenItem> it = source; it != common_ancestor; it = it->first_ancestor<Layout>()) {
        source_branch *= it->xform<ScreenItem::Space::PARENT>();
    }

    Matrix3f tarbranch = Matrix3f::identity();
    for (risky_ptr<const ScreenItem> it = target; it != common_ancestor; it = it->first_ancestor<Layout>()) {
        tarbranch *= it->xform<ScreenItem::Space::PARENT>();
    }
    tarbranch.inverse();

    source_branch *= tarbranch;
    return source_branch;
}

NOTF_CLOSE_NAMESPACE

/*

It's thinky time one more time every time!
Today's problem: what the fuck are we going to do about the ScreenItem redraw thing?
Meaning, how does a ScreenItem notify someone that it needs to be re-rendered?
Before, whe had a dedicated `redraw` method for that which notified the Window that it needed to redraw, which in turn
notified its RenderManager and awayyyiiii... it rendered.
Ain't nobody got the architecture for that (anymore).
Items don't know what a Window is, nor should they.
And they certainly shouldn't keep some sort of reference to the RenderManager etc.
I guess, there could be a scene flag that an Item can set if it wants to be redrawn.
Buttsies: there is already a mechanism that must also force a redraw and it is pretty much exactly the same stuff that
we have here: the PropertyManager.
The PropertyManager keeps track of Properties, which is everything that is something that influences how something is
drawn on screen.
A property can be owned by a 2D Widget, or by a 3D something or by something that is something else.
And everything can be connected to everything else, accros scenes and dimensions and time and space and so on.
I like that in gerneral - a good reason to keep the PropertyManager alive as a concept.
So there it is.
Of course, there's nothing keeping us from making all the redrawable stuff in a ScreenItem (position, size, opacity,
visibility etc.) all Properties.
And why the funky knot?
Because we would have to figure out how the PropertyManager would work IN PRACTICE!
And believe it or not - I haven't really done that yet.
Time to rectify the situation.

Problem (1): When I change a property, how does the SceneManager know what to redraw and shit?
The PropertyManager must have some mechanism to connect a Property to a Layer / RenderTarget in order to notify it
so it knows that it's a dirty dirty Layer or RenderTarget.
Simple: put a reference or pointer into each Property.
DONE ... Except that sucks.
Is it an owning pointer?
That would probably be bad idea because I'm not too sure yet on how to make sure that Properties are always deleted.
I guess that would be problem (2).

Problem (2): Property lifetime.
How can we make sure that a Property is created before it is accessed?
Put it into the constructor of the thing owning the Property - that's what's for.
How can we make sure that a Property is deleted after is is no longer used?
Put it in the destructor of the thing owning the Property.
Of course, we also have to make sure that everything that references the property no longer does so, but I believe that
the PropertyManager handles those situations exactly ... ?
(lemme check)
Jup. Actually the PropertyGraph but all the same.
So we actually should be fine here.
PROBLEM SOLVED, bitches.

Problem (3): Lifetime revisited
Except not really, my friend.
The problem is in the threading - if the Constructor creates a Property and you immediately query its value from the
graph (PropertyGraph that is), it doesn't exist yet.
But then ... didn't we always say that Properties are not to be read except by other properties?
I don't think that will work for this scenario.
ScreenItems need access to their transformation and to that of their parents etc. and they need it now!
Right now we have this super-advanced, wait free super lockfree queue thing where you just push changes to the graph.
No reading allowed, because who knows when dat shit's updated by the RenderThread.
BUT (big butt) who sais that only the RenderThread has access?
Well ... nobody, but if everyone at any time can request Properties, then this super waitfree queue thing doesn't work
regardless.
Okay, back to the drawing board.

Who is interested in Properties?
The RenderThread when it evaluates the Scenes.
The SceneManager when it propagates events.
The Timer and potential other things that update Properties directly.

Longer thought dump:
Lock Property on Item level (call it PropertyGroup).
We use mutexes, all other synchronization primitives (spinlock) seem to be too frickle.
Also, when we lock on Item level (and we don't have a mutex per Property), the number of mutexes goes way down.
Locking on Item level also opens the possibility of relating items to Layer/RenderTargets.
All hierarchies (no matter if widget or scenegraph or whatever) can be represented as trees.
Have PropertyIdentifiers be a PAIR of numbers: ItemID and PropertyID (which can be a ushort), or bitshift magic or...
Make Item the base thing for ALL scenes - the fact that it is also the base for SceneItem and Controller is coincidal.

Lock patterns:
    Individual properties/Items:
        Create
            Aquire PropertyGroup lock (is returned by factory if the PropertyGroup didn't exist yet)
                Add new properties with default values
                For each dependency (does not have to be all at once - use try_lock to try another one if locked?):
                    Aquire PropertyGroup lock of dependency
                        add corresponding property as dependent
                        release PropertyGroup lock of dependency
                    else (dependency PropertyGroup has been deleted)
                        drop back to default value (must be supplied in addition to a expression)
                Release PropertyGroup lock
             else (PropertyGroup existed but was deleted)
                throw exception (this would be really unexpected, if only the Item itself is allowed to create)

        Read
            Aquire PropertyGroup lock
                Read values
                Release PropertyGroup lock

                Aquire lock of ALL dependencies at once (we know there are no cyclic dependencies so we should be fine)
                    Evaluate expression
                    Release All Locks
                else
                    drop back to default value
                Release PropertyGroup lock
            else (PropertyGroup existed but was deleted or never existed)
                throw exception (possible but hopefully unlikely) -- or use std::optional?

        Update
            Aquire PropertyGroup lock
                Update values
                For each OLD dependency (does not have to be all at once - use try_lock to try another one if locked?):
                    Aquire PropertyGroup lock of dependency
                        remove corresponding property as dependent
                        release PropertyGroup lock of dependency
                    else (dependency PropertyGroup has been deleted)
                        ignore
                For each NEW dependency (does not have to be all at once - use try_lock to try another one if locked?):
                    Aquire PropertyGroup lock of dependency
                        add corresponding property as dependent
                        release PropertyGroup lock of dependency
                    else (dependency PropertyGroup has been deleted)
                        drop back to default value (must be supplied in addition to a expression)
                Release PropertyGroup lock
            else (PropertyGroup existed but was deleted or never existed)
                throw exception (possible but hopefully unlikely) -- or use std::optional?

        Delete
            Aquire PropertyGroup lock
                get all dependent
                Delete properties
                For each dependency (does not have to be all at once - use try_lock to try another one if locked?):
                    Aquire PropertyGroup lock of dependency
                        remove corresponding property as dependent
                        release PropertyGroup lock of dependency
                    else (dependent PropertyGroup has been deleted)
                        ignore
                Release PropertyGroup lock
             else (PropertyGroup existed but was deleted)
                throw exception (this would be really unexpected, if only the Item itself is allowed to create)
            For each dependent
                Aquire PropertyGroup lock of dependent
                    remove expression from dependent and drop back to default value
                    release PropertyGroup lock of dependency
                else (dependent PropertyGroup has been deleted)
                    ignore

    Complete Subtree
        ...

NOOOOOO
I got this all wrong.
There are only 2 threads that are relvant here: the SceneThread and the RenderThread.
This means that we will have to give up on the notion that everybody can set Properties from everywhere ... but seriosly
if they basically get unusable because of that, we don't win anythin.
Instead:
SceneManager receives events from App AND ALSO TIMER ETC.
It handles one event at a time with a lock on the COMPLETE SCENE
It then releases the lock and tries to lock it again for the next event - either immediately or on request.
The RenderThread does the same thing.
Draw a frame, release the lock wait for the next frame to be requested, rinse and repeat.
The only problem here is that the SceneThread might starve, if every event causes a redraw because the RenderThread
would immediately snatch up the lock and render.
However, on systems that use vertical sync (so, most of them except mine...) one complete loop will take 16ms every
time.
... well, only the call to glfwSwapBuffers does that, for which I don't need to lock anymore.
If I want to limit the render thread on my machine, I could put a wait right after swap buffers.
*/

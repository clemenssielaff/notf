#pragma once

/* Move SceneManager into app/core
 * Move Layer there as well.
 * The Layer class will become more powerfull with both a producer AND an item hierarchy ... or some other way to
 * delegate events - I suspect that a 3D scene will need a completely different approach than an Item Hierarchy.
 * Maybe we should call that abstraction "scene"?
 *
 * So the ItemHierarchy is in fact a Scene.
 * I like that.
 *
 * I played with the thought of putting the ability for mouse-over tooltips etc. into a separate Layer, but when the
 * Layer class is becoming so much more complicated (and expensive) than a regular Item Hierarchy, I wonder if we
 * shouldn't put some though in right now to make sure that you can properly create tooltips in an Item Hierarchy.
 * Were's the problem you ask?
 * Well, usually, the Layout of a Widget determines its size AND its depth. A tooltip however should be placed over
 * whatever it refers to. Actually, let's say a "Glow" because I just realized that a tooltip only needs the mouse
 * position ... yes fair enough.
 * So: we have one or more widgets that want to have a nice "glow" around their borders. The glow is supposed to be
 * drawn in front of everything, but its position is still determined by the widget. Apart from the fact that a glow
 * should probably be a post-effect (...) let's assume that I come up with a good use case for an Item that has a
 * different depth from what its' layout would suggest.
 *
 * I don't like it. Even if we can make it work, how do we handle events and shit?
 * Actually, isn't that what our signals are for anyway? You have two widgets and one of them changes in size and lets
 * the other one know. And then the other one responds in changing ITS size as well... yes.
 * That's the way.
 * Whew...
 * No double-hierarchy or stuff like that which would be annoying and stupid and brittle and I wouldn't like it.
 *
 * Okay signals now. Let's get to how THAT would work.
 * The application gets an event, actually GLFW but whatevs. It filters out the events that are relevant for the visuals
 * (user-input events, I suppose) and passes them on to the SceneHandler, which I just invented and is going to be
 * awesome. It will go through its Layers in order from front to back and ask each one to handle the event. One of them
 * will eventually or not, but after one did the event is not propagated further back.
 * The scene, in our case an ItemHierarchy, will find an Item to handle the event and then do it.
 * Doing it in this case consist of creating the appropriate Command object (let's ignore for now how the
 * synchronization of Command objects from multiple sources over the network would work ...), pushing it on the stack
 * and executing its "doIt" function.
 * This will update the Widget and potentially fire some of its signals, which in turn updates all associated widgets
 * in one go. I don't see how this could go wrong.
 * Actually ... if the Signals have no cycles (circles?) in them it should work. A Widget is not allowed to generate
 * any more events (only the Application can do that) and no more events means no more event handling and everything
 * should be fine.
 * Undoing that stuff is probably another matter, especially if the changes made by the signal are destructive ... but
 * as I said, that's a problem for future Clemens (that sucker).
 * That's about 80% there, I suppose.
 *
 * Time to get going!
 */

NOTF_OPEN_NAMESPACE}

# from uuid import uuid4 as uuid
# from enum import Enum, unique, auto
# from logging import warning
# from typing import NamedTuple, List, TypeVar, Type, Union, Optional, Callable, Dict, Any
# from .reactive import Operator, Publisher
# from .structured_buffer import StructuredBuffer
#
#
# class Property(Operator):
#     @unique
#     class Visibility(Enum):
#         """
#         Changing this Property ...
#         """
#         INVISIBLE = auto()  # .. does not require any action from the renderer.
#         REDRAW = auto()  # ... requires the Node to be drawn again as it is (only transformed etc.).
#         REFRESH = auto()  # ... requires the Node to update and then redraw.
#
#     def __init__(self, name: str, value: Any, is_visible: bool):
#         """
#         :param name: Node-unique name of the Property.
#         :param value: Initial value of the Property.
#         :param is_visible: Whether or not changes in this Property should cause a redraw of the Node or not.
#         :raise ValueError: If the given value cannot be represented as an Element.
#         """
#         element: Element = Element(value)
#
#         super().__init__(element.output_schema)
#
#         self._name = name
#         """
#         Node-unique name of the Property.
#         Is constant.
#         """
#
#         self._render_value: StructuredBuffer = StructuredBuffer.create(element)
#         """
#         The value of this Property as it is currently being displayed on screen.
#         """
#
#         self._modified_value: Optional[StructuredBuffer] = None
#         """
#         The most current value of this Property.
#         Is only set if the value was changed since the last frame. Before each frame, this value is copied into the
#         `_render_data` field and reset to None.
#         """
#
#         self.callback: Optional[Callable] = None
#         """
#         Optional callback executed before the value of the Property would be updated.
#         Must take a value of the Property's kind and return either a (modified) value of the same kind or None, to
#         reject the update completely.
#         """
#
#         self.hash = 0 if is_visible == Property.Visibility.INVISIBLE else hash(element)
#         """
#         The hash of the most current value of this Property.
#         Is always 0 if this is an invisible property.
#         """
#
#     def __del__(self):
#         super().complete()
#
#     @property
#     def name(self) -> str:
#         """
#         Node-unique name of this Property.
#         """
#         return self._name
#
#     @property
#     def is_visible(self) -> bool:
#         """
#         Visible Properties will cause a redraw of the Node on each update, invisible ones do not.
#         """
#         return self.hash != 0
#
#     @property
#     def value(self) -> StructuredBuffer:
#         """
#         The value of this Property, depending on whether this is the render thread or not, this function will return the
#         "render value" or "modified value" if one exists.
#         """
#         if is_this_the_render_thread():
#             return self._render_value  # the renderer always sees the unmodified value
#         if self._modified_value is not None:
#             return self._modified_value  # return a modified value to the UI thread, if one exists
#         else:
#             return self._render_value  # if no modified copy exists, return the unmodified value
#
#     @value.setter
#     def value(self, value: StructuredBuffer):
#         if not is_this_the_main_thread():
#             raise RuntimeError("Properties can only be set from the main thread")
#
#         if value is None:
#             raise ValueError("Properties cannot be None")
#
#         if not isinstance(value, StructuredBuffer):
#             raise ValueError("Property values must be wrapped in a Structured Buffer instance")
#
#         # do nothing if the property value would not change
#         if value == (self._modified_value if self._modified_value is not None else self._render_value):
#             return
#
#         # give the property callback a chance to modified or veto the update
#         if self.callback is not None:
#             value = self.callback(value)
#             if value is None:
#                 return
#
#         # update the modified value, it will be copied into the render value before the next frame
#         self._modified_value = value
#
#         # update the hash if this Property is visible
#         if self.is_visible:
#             self.hash = hash(self._modified_value)
#
#         # publish the updated value
#         self.publish(self._modified_value)
#
#     def clean(self):
#         assert is_this_the_main_thread()
#         if self._modified_value:
#             self._render_value = self._modified_value
#             self._modified_value = None
#
#     def on_next(self, publisher: Publisher, value: Optional[StructuredBuffer] = None):
#         """
#         Abstract method called by any upstream publisher.
#
#         :param publisher    The Publisher publishing the value, for identification purposes only.
#         :param value        Published value, can be None (but this will throw in error for Properties).
#         """
#         self.value = value
#
#     def on_error(self, publisher: Publisher, exception: BaseException):
#         warning("Caught propagated error from Publisher {}: {}".format(publisher, str(exception)))
#
#     def complete(self):
#         raise NotImplementedError("Properties cannot be completed manually")
#
#
# class Node:
#     T = TypeVar('T')
#
#     class Data(NamedTuple):
#         parent: 'Node'  # mandatory
#         children: List['Node'] = []
#         is_enabled: bool = True
#         is_visible: bool = True
#         is_dirty: bool = True
#
#     class Definition:
#         """
#         Instead of offering a `create_property` method on the Node itself (alongside `create_signal` and `_slot`
#         methods) Node definition at runtime is done using this private helper class that should not be constructed by
#         the user but only passed into the `finalized` method as an output argument.
#         This way, we don't have to store an `is_finalized` flag in the Node and the existence of the `finalize` method
#         allows us to have access to the completed Node instance, since the constructor is finished by the time
#         `finalize` is called.
#         """
#
#         def __init__(self):
#             self._properties: Dict[str, Property] = {}
#
#         @property
#         def properties(self):
#             return self._properties
#
#         def add_property(self, name: str, value: Any, is_visible: bool) -> Property:
#             """
#             Creates and returns a new Property instance, that will be attached to the node.
#             :param name: Node-unique name of the Property.
#             :param value: Initial value of the Property.
#             :param is_visible: Whether or not changes in this Property should cause a redraw of the Node or not.
#             :return: New Property instance.
#             """
#             if name in self._properties:
#                 raise NameError("Node already has a property named {}".format(name))
#             new_property = Property(name, value, is_visible)
#             self._properties[name] = new_property
#             return new_property
#
#     def __init__(self, graph: 'Graph', parent: 'Node'):
#         self._graph: Graph = graph
#         self.__data: 'Node.Data' = self.Data(parent)
#         self._uuid: uuid = uuid()
#         self._properties: Dict[str, Property] = {}
#         # TODO: signals and slots
#
#     def _define_node(self, definition: Definition):
#         """
#         Is called right after the constructor has finished.
#         :param definition:  Node Definition. Is changed in-place to define the node instance.
#         """
#         raise NotImplementedError("Node subclasses must implement their virtual `_define_node` method")
#
#     def _apply_definition(self, definition: Definition):
#         """
#         Applies the given Node Definition to this instance.
#         :param definition: Node Definition to apply. Is constant.
#         """
#         self._properties = definition.properties
#         # TODO: automatic update when visible properties are updated
#
#     @property
#     def _data(self) -> Data:
#         """
#         A private getter for the Node data that can change depending on whether you ask for it from the main thread or
#         the render thread.
#         """
#         # TODO: render data & modified data
#         return self.__data
#
#     @property
#     def uuid(self) -> uuid:
#         return self._uuid
#
#     @property
#     def name(self) -> str:
#         return self._graph.registry.get_name(self)
#
#     @name.setter
#     def name(self, value: str):
#         self._graph.registry.set_name(self, value)
#
#     def create_child(self, node_type: Type['Node']) -> 'Node':
#         """
#         Create a new child Node of this Node.
#         By tying node creation to the existence of its parent node, we can make sure that a node can never not have a
#         valid parent. We can also call `_define_node` on the new instance.
#         :param node_type: Subclass of Node to instantiate as a child of this Node.
#         :return:          The new child Node instance.
#         """
#         # create the node instance
#         if not issubclass(node_type, Node):
#             raise TypeError("Nodes can only have other Nodes as children")
#         child = node_type(self._graph, self)
#
#         # finalize the node
#         definition = self.Definition()
#         child._define_node(definition)
#         child._apply_definition(definition)
#
#         # register the node as a new child
#         self._data.children.append(child)
#         return child
#
#     def property(self, name: str) -> Property:
#         """
#         Returns the Property requested by name.
#         :raises KeyError: If this Node kind has no Property by the given name.
#         """
#         if name in self._properties:
#             return self._properties[name]
#         raise KeyError("Node has no Property named: {}. Available Property names are: {}".format(name, ", ".join(
#             self._properties.keys())))
#
#
# class RootNode(Node):
#     """
#     The Root Node is its own parent.
#     """
#
#     def __init__(self, graph: 'Graph'):
#         super().__init__(graph, self)
#
#     def _define_node(self, definition: Node.Definition):
#         pass
#
#
# class Graph:
#     """
#     The UI Graph.
#     """
#
#     class NodeRegistry:
#         def __init__(self):
#             self._nodes: Dict[Union[Node, str], Union[Node, str]] = {}  # str <-> Node bimap
#
#         def __len__(self) -> int:
#             """
#             Number of node/name pairs in the registry.
#             """
#             assert len(self._nodes) % 2 == 0
#             return len(self._nodes) // 2
#
#         def get_name(self, node: Node) -> str:
#             name = self._nodes.get(node, None)
#             if name is None:
#                 # create a default name and register the node with it
#                 name = str(node.uuid)
#                 self._nodes[name] = node
#                 self._nodes[node] = name
#             return name
#
#         def set_name(self, node: Node, name: str) -> str:
#             # delete the old node/name pair if it exists
#             old_name = self._nodes.get(node, None)
#             if old_name is not None:
#                 assert node in self._nodes
#                 del self._nodes[old_name]
#                 del self._nodes[node]
#
#             # make sure the name is unique
#             proposal = name
#             counter = 1
#             while proposal in self._nodes:
#                 proposal = "{}{:>2}".format(name, counter)
#                 counter += 1
#
#             # (re-)register the node under its new name
#             self._nodes[proposal] = node
#             self._nodes[node] = proposal
#
#             # return the actual new name of the node
#             return proposal
#
#         def get_node(self, name: str) -> Optional[Node]:
#             return self._nodes.get(name, None)
#
#     def __init__(self):
#         self.root: RootNode = RootNode(self)
#         self.registry: Graph.NodeRegistry = Graph.NodeRegistry()

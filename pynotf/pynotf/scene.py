from __future__ import annotations
from typing import List, Optional, Dict, Set, Type, Any, Callable, NamedTuple
from weakref import ref as weak_ref

from .value import Value
from .logic import Circuit, ValueSignal, Operation, Emitter, Receiver


#######################################################################################################################

class Property(Receiver, Emitter):  # final
    """
    Properties are values that combined fully define the state of the Node.
    If you take the entire Scene, take all Properties of all the Nodes, serialize the Scene alongside the Properties and
    re-load them, you have essentially restored the entire UI application at the point of serialization.
    """

    def __init__(self, circuit: Circuit, element_id: Circuit.Element.ID, operation: Operation, node: 'Node',
                 value: Value):
        """
        Constructor.
        Should only be called from a Node constructor with a valid Node.Type - hence we don't check the operation for
        validity and just assume that it ingests and produces the correct Value type.
        :param circuit:     The Circuit containing this Element.
        :param element_id:  The Circuit-unique identification number of this Element.
        :param operation: Operations applied to every new Value of this Property, can be a noop.
        :param node: Node that this Property lives on.
        :param value: Initial value of the Property, also defines its Schema.
        """
        assert value.schema == operation.get_input_schema()
        assert value.schema == operation.get_output_schema()
        Receiver.__init__(self, operation.get_input_schema())
        Emitter.__init__(self, operation.get_output_schema())

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Circuit.Element.ID = element_id  # is constant
        self._operation: Operation = operation  # is constant
        self._node: Node = node  # is constant
        self._value: Value = value

    def get_id(self) -> Circuit.Element.ID:  # final, noexcept
        """
        The Circuit-unique identification number of this Element.
        """
        return self._element_id

    def get_circuit(self) -> 'Circuit':  # final, noexcept
        """
        The Circuit containing this Element.
        """
        return self._circuit

    def get_value(self) -> Value:
        """
        The current value of this Property.
        """
        return self._value

    def set_value(self, value: Any):
        """
        Applies a new Value to this Property. The Value must be of the Property's type and is transformed by the
        Property's Operation before assignment.
        :param value: New value to assign to this Property.
        :raise TypeError: If the given Value type does not match the one stored in this Property.
        """
        if value is None:
            raise TypeError("Property values cannot be none")

        if not isinstance(value, Value):
            try:
                value = Value(value)
            except ValueError as exception:
                raise TypeError(str(exception)) from exception
        assert isinstance(value, Value)

        if value.schema != self._value.schema:
            raise TypeError("Cannot change the type of a Property")

        try:
            result = self._operation(value)
        except Exception as exception:
            self._fail(exception)
        else:
            if result != self._value:
                assert result.schema == self._value.schema
                self._value = result
                self._emit(self._value)

    def _on_value(self, signal: ValueSignal):
        """
        Abstract method called by any upstream Emitter.
        :param signal   The ValueSignal associated with this call.
        """
        self.set_value(signal.get_value())


#######################################################################################################################

class InputPlug(Receiver):
    """
    An Input Plug (IPlug) accepts Values from the Circuit and forwards them to an optional Callback on its Node.
    """

    def __init__(self, circuit: 'Circuit', element_id: Circuit.Element.ID, schema: Value.Schema, node: 'Node'):
        """
        Constructor.
        It should only be possible to create-connect an InputPlug when constructing a new Node instance.
        :param circuit:     The Circuit containing this Element.
        :param element_id:  The Circuit-unique identification number of this Element.
        :param schema:      Value type accepted by this Plug.
        :param node:        Node on which the Plug lives.
        """
        Receiver.__init__(self, schema)

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Circuit.Element.ID = element_id  # is constant
        self._node: Node = node  # is constant
        self._callback: Optional[Callable] = None

    def get_id(self) -> Circuit.Element.ID:  # final, noexcept
        """
        The Circuit-unique identification number of this Element.
        """
        return self._element_id

    def get_circuit(self) -> 'Circuit':  # final, noexcept
        """
        The Circuit containing this Element.
        """
        return self._circuit

    def set_callback(self, callback: Optional[Callable]):
        """
        Callbacks must take two arguments (Node.Self, ValueSignal) and return nothing.
        :param callback:    New Callback to call when a new Value is received or None to uninstall the Callback.
        """
        self._callback = callback

    # private
    def _on_value(self, signal: ValueSignal):  # final
        """
        Forwards the Signal unmodified to the registered Callback or ignores it, if no Callback is set.
        Exceptions in the Callback will be reported and the Signal will remain unmodified, even if the Callback accepted
        or blocked it prior to failure.
        :param signal   The ValueSignal associated with this call.
        """
        assert signal.get_value().schema == self.get_input_schema()

        # don't do anything if no Callback is registered
        if self._callback is None:
            return

        # create a copy of the Signal so the original remains unmodified if the Callback fails
        signal_copy: ValueSignal = ValueSignal(signal.get_source(), signal.get_value(), signal.is_blockable())
        if signal.is_blocked():
            signal_copy.block()
        elif signal.is_accepted():
            signal.accept()

        # perform the callback
        try:
            self._callback(Node.Self(self._node), signal_copy)

        # report any exception that might escape the user-defined code
        except Exception as exception:
            self.get_circuit().handle_error(Circuit.Error(
                weak_ref(self),
                Circuit.Error.Kind.USER_CODE_EXCEPTION,
                str(exception)
            ))

        # only modify the original signal if the callback succeeded
        else:
            if signal.is_blockable() and not signal.is_blocked():
                if signal_copy.is_blocked():
                    signal.block()
                elif signal_copy.is_accepted():
                    signal.accept()  # accepting more than once does nothing


#######################################################################################################################

class OutputPlug(Emitter):
    """
    Output Plugs (OPlugs) are Emitters that live on the Node and than can be invoked to propagate non-Property Values
    into the Circuit as needed. They are public, meaning that they can be triggered from everywhere, although most will
    probably be triggered from code living on the Node itself. The reason why we allow them to be triggered from outside
    of the Node is that they themselves do not depend on the Node itself, nor do they modify it in any way. It is
    therefore feasible to have for example a single Node with an OPlug "FormChanged" which is triggered from the various
    form elements as needed.
    """

    def __init__(self, circuit: 'Circuit', element_id: Circuit.Element.ID, schema: Value.Schema):
        """
        Constructor.
        It should only be possible to create-connect an InputPlug when constructing a new Node instance.
        :param circuit:     The Circuit containing this Element.
        :param element_id:  The Circuit-unique identification number of this Element.
        :param schema:      Value type accepted by this Plug.
        """
        Emitter.__init__(self, schema)

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Circuit.Element.ID = element_id  # is constant

    def get_id(self) -> Circuit.Element.ID:  # final, noexcept
        """
        The Circuit-unique identification number of this Element.
        """
        return self._element_id

    def get_circuit(self) -> 'Circuit':  # final, noexcept
        """
        The Circuit containing this Element.
        """
        return self._circuit

    def emit(self, value: Any):
        """
        This method should only be callable from within Node Callbacks.
        If the given Value does not match the output Schema of this Emitter, an exception is thrown. This is so that the
        Callback is interrupted and the ValueSignal remains unchanged. The exception will be caught in the IPlug
        `_on_value` function and handled by the Circuit's error handler.
        :param value:       Value to emit.
        :raise TypeError:   If the given value does not match the output Schema of this Emitter.
        """
        # implicit value conversion
        if not isinstance(value, Value):
            try:
                value = Value(value)
            except ValueError:
                raise TypeError("Emitters can only emit Values or things that are implicitly convertible to one")

        # ensure that the value can be emitted by the Emitter
        if self.get_output_schema() != value.schema:
            raise TypeError(f"Cannot emit Value from Emitter {self.get_id()}."
                            f"  Emitter schema: {self.get_output_schema()}"
                            f"  Value schema: {value.schema}")

        self._emit(value)


#######################################################################################################################

# noinspection PyProtectedMember
class Scene:
    """
    The Scene contains the Root Node and a map of all named Nodes.
    It also cleans up the complete hierarchy on removal.
    Modification of the Scene must only occur in the Logic thread.

    Nodes can be named. They have a name associated with them during construction and it can not be changed afterwards.
    Names are unique in the Scene, if you try to create two Nodes with the same name, the creation of the second one
    will fail.
    Root Nodes have the reserved name '/', we can therefore be sure that only one Root Node can exist in the Scene.
    """

    def __init__(self):
        self._named_nodes: Dict[str, weak_ref] = dict()
        self._expired_nodes: Set[Node] = set()

        # create the root node and name it '/'
        self._root: Node = Node(None, self, '/')

    def __del__(self):
        # remove all nodes in the scene depth-first, so that child nodes don't outlive their ancestors
        self._root._remove_self()
        self._root = None

    def create_node(self, node_type: Type[Node], *args, **kwargs) -> 'Node.Handle':
        return self._root.create_child(node_type, *args, **kwargs)

    def get_node(self, name: str) -> Optional['Node.Handle']:
        """
        Looks for a Node with the given name and returns it if one is found.
        :param name:    Name of the Node to look for.
        :return:        Valid handle to the requested Node or None, if none was found.
        """
        weak_node: Optional[weak_ref] = self._named_nodes.get(name, None)
        if weak_node is None:
            return None
        node: Node = weak_node()
        assert node is not None  # named nodes should remove themselves from the dict on destruction
        return Node.Handle(node)

    def remove_expired_nodes(self):
        """
        Is called at the end of the event loop by an Executor to remove all expired Nodes.
        """
        for node_handle in self._expired_nodes:
            node: Optional[Node] = node_handle._node()
            if node is None:
                continue
            parent: Node = node._parent
            del node
            parent.remove_child(node_handle)
        self._expired_nodes.clear()


#######################################################################################################################

# noinspection PyProtectedMember
class Node:
    class Handle:
        def __init__(self, node: 'Node'):
            assert isinstance(node, Node)
            self._node: Optional[weak_ref] = weak_ref(node)

        def is_valid(self) -> bool:
            return self._node() is not None

        def get_name(self) -> Optional[str]:
            node: Node = self._node()
            if node is not None:
                return node.get_name()

    class Definition:
        """
       Instead of implementing `_add_property`, `_add_input` and `_add_output` methods in the Node class, a Node must be
       fully defined on construction, by passing in a Node.Definition object. Like a Schema describes a type of Value,
       the Node.Definition describes a type of Node without making any assumptions about the actual state of the Node.

       Properties and IPlugs must be constructed with a reference to the Node instance that they live on. Therefore,
       we only store the construction arguments here and build the object in the Node constructor.
       """

        class Property(NamedTuple):
            default: Value
            operation: Optional[Operation]

        def __init__(self):
            """
            Default Constructor.
            """
            self._input_plugs: Dict[str, Value.Schema] = {}
            self._output_plugs: Dict[str, Value.Schema] = {}
            self._properties: Dict[str, Node.Definition.Property] = {}

        def add_input(self, name: str, schema: Value.Schema = Value.Schema()):
            """
            Stores the arguments need to construct a Slot instance on the node.
            :param name: Node-unique name of the Slot.
            :param schema: Scheme of the Value received by the Slot.
            :raise NameError: If the Node already has a Property, Signal or Slot with the given name.
            """
            if not self._is_name_available(name):
                raise NameError(f'Node already has a member named "{name}"')
            self._input_plugs[name] = schema

        def add_output(self, name: str, schema: Value.Schema = Value.Schema()):
            """
            Stores the arguments need to construct a Output Plug instance on the node.
            :param name:        Node-unique name of the OPlug.
            :param schema:      Scheme of the Value emitted by the OPlug.
            :raise NameError:   If the Node already has a Property, Signal or Slot with the given name.
            """
            if not self._is_name_available(name):
                raise NameError(f'Node already has a member named "{name}"')
            self._output_plugs[name] = schema

        def add_property(self, name: str, default: Any, operation: Optional[Operation] = None):
            """
            Stores the arguments need to construct a Property on the node.
            :param name: Node-unique name of the Property.
            :param default:     Initial value of the Property.
            :param operation:   Operation applied to every new Value of this Property.
            :raise NameError:   If the Node already has a Property, IPlug or OPlug with the given name.
            """
            if not self._is_name_available(name):
                raise NameError(f'Node already has a member named "{name}"')

            # implicit value conversion
            if not isinstance(default, Value):
                try:
                    default = Value(default)
                except ValueError:
                    raise TypeError(f"Cannot convert {str(default)} to a Property default Value")

            # noinspection PyCallByClass
            self._properties[name] = Node.Definition.Property(default, operation)

        def _is_name_available(self, name: str) -> bool:
            """
            Properties, Signals and Slots share the same namespace on a Node. Therefore we need to check whether any
            one of the three dictionaries already contains a given name before it can be accepted as new.
            :param name: Name to test
            :return: Whether or not the given name would be a valid new name for a Property, Signal or Slot.
            """
            return (name not in self._input_plugs and
                    name not in self._output_plugs and
                    name not in self._properties)

    class Self:
        """
        Special kind of Node Handle passed to Callbacks which allows the user to access the Node's private data Value
        as well as its Properties and OPlugs.
        """

        def __init__(self, node: Node):
            self._node: Node = node

    def __init__(self, parent: Optional['Node'], scene: Scene, name: Optional[str] = None):
        self._parent: Optional[Node] = parent
        self._scene: Scene = scene
        self._children: List[Node] = []
        self._name: Optional[str] = name
        print(f'Creating Node "{self.get_name() or str(id(self))}"')

    def __del__(self):  # virtual noexcept
        print(f'Deleting Node "{self.get_name() or str(id(self))}"')

    def get_parent(self) -> Optional['Node.Handle']:
        if self._parent is None:
            return None
        return Node.Handle(self._parent)

    def get_name(self) -> Optional[str]:
        return self._name

    def create_child(self, node_type: Type[Node], *args, **kwargs) -> 'Node.Handle':
        """
        Creates a new child of this Node.
        :param node_type:
        :param args:
        :param kwargs:
        :return:
        """
        assert issubclass(node_type, Node)
        node: Node = node_type(self, self._scene, *args, **kwargs)

        name: Optional[str] = node.get_name()
        if name is not None:
            if name in self._scene._named_nodes:
                raise NameError('Cannot create another Node with the name "{}"'.format(name))
            self._scene._named_nodes[name] = weak_ref(node)

        self._children.append(node)
        return Node.Handle(node)

    def remove_child(self, node_handle: 'Node.Handle'):
        """
        Removes a child of this Node.
        :param node_handle:     Child Node to remove.
        """
        node: Optional[Node] = node_handle._node()
        if node is None:
            return
        assert node in self._children
        node._remove_self()
        self._children.remove(node)
        del node
        assert not node_handle.is_valid()

    # private
    def _remove_self(self):
        """
        This method is called when this Node is being removed.
        It recursively calls this method on all descendant nodes while still being fully initialized.
        """
        # recursively remove all children
        for child in self._children:
            child._remove_self()
        self._children.clear()

        if self._name is not None:
            assert self._scene._named_nodes[self._name] == weak_ref(self)
            del self._scene._named_nodes[self._name]

        # self._scene._expired_nodes.add(self)


########################################################################################################################

class LeafNode(Node):
    pass


class TestNode(Node):
    def __init__(self, parent: Optional['Node'], scene: Scene):
        Node.__init__(self, parent, scene)
        self._a: Node.Handle = self.create_child(LeafNode)

    def clear(self):
        self.remove_child(self._a)


def main():
    scene: Scene = Scene()
    scene.create_node(TestNode)


if __name__ == '__main__':
    main()

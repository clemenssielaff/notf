from __future__ import annotations
from typing import List, Optional, Dict, Set, Type as Class, Any
from weakref import ref as weak_ref

from .value import Value
from .logic import Circuit, Element, ValueSignal, Operation, AbstractEmitter, AbstractReceiver


#######################################################################################################################

class Property(AbstractReceiver, AbstractEmitter):  # final
    """
    Properties are values that combined fully define the state of the Node.
    If you take the entire Scene, take all Properties of all the Nodes, serialize the Scene alongside the Properties and
    re-load them, you have essentially restored the entire UI application at the point of serialization.
    """

    def __init__(self, circuit: Circuit, element_id: Element.ID, operation: Operation, node: 'Node', value: Value):
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
        AbstractReceiver.__init__(self, operation.get_input_schema())
        AbstractEmitter.__init__(self, operation.get_output_schema())

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Element.ID = element_id  # is constant
        self._operation: Operation = operation  # is constant
        self._node: Node = node  # is constant
        self._value: Value = value

    def get_id(self) -> Element.ID:  # final, noexcept
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

class InputPlug:
    pass


class OutputPlug:
    pass


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

    def create_node(self, node_type: Class[Node], *args, **kwargs) -> 'Node.Handle':
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

    class Type:
        def __init__(self, input_schemas: Dict[str, Value.Schema], output_schemas: Dict[str, Value.Schema]):
            self._input_plug_schemas: Dict[str, Value.Schema] = {}
            self._property_operations: Dict[str, Operation] = {}
            self._output_plug_schemas: Dict[str, Value.Schema] = {}

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

    def create_child(self, node_type: Class[Node], *args, **kwargs) -> 'Node.Handle':
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

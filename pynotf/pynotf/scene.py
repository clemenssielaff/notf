from __future__ import annotations
from typing import List, Optional, Dict, Set, Any, Callable, NamedTuple
from weakref import ref as weak_ref

from .value import Value
from .logic import Circuit, ValueSignal, Emitter, Receiver, Operator


#######################################################################################################################

class Property(Receiver, Emitter):  # final
    """
    Properties are values that combined fully define the state of the Widget.
    If you take the entire Scene, take all Properties of all the Nodes, serialize the Scene alongside the Properties and
    re-load them, you have essentially restored the entire UI application at the point of serialization.
    """

    def __init__(self, circuit: Circuit, element_id: Circuit.Element.ID, operation: Operator.Operation, node: 'Widget',
                 value: Value):
        """
        Constructor.
        Should only be called from a Widget constructor with a valid Widget.Type - hence we don't check the operation for
        validity and just assume that it ingests and produces the correct Value type.
        :param circuit:     The Circuit containing this Element.
        :param element_id:  The Circuit-unique identification number of this Element.
        :param operation: Operations applied to every new Value of this Property, can be a noop.
        :param node: Widget that this Property lives on.
        :param value: Initial value of the Property, also defines its Schema.
        """
        assert value.schema == operation.get_input_schema()
        assert value.schema == operation.get_output_schema()
        Receiver.__init__(self, operation.get_input_schema())
        Emitter.__init__(self, operation.get_output_schema())

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Circuit.Element.ID = element_id  # is constant
        self._operation: Operator.Operation = operation  # is constant
        self._node: Widget = node  # is constant
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
    An Input Plug (IPlug) accepts Values from the Circuit and forwards them to an optional Callback on its Widget.
    """

    def __init__(self, circuit: 'Circuit', element_id: Circuit.Element.ID, schema: Value.Schema, node: 'Widget'):
        """
        Constructor.
        It should only be possible to create-connect an InputPlug when constructing a new Widget instance.
        :param circuit:     The Circuit containing this Element.
        :param element_id:  The Circuit-unique identification number of this Element.
        :param schema:      Value type accepted by this Plug.
        :param node:        Widget on which the Plug lives.
        """
        Receiver.__init__(self, schema)

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Circuit.Element.ID = element_id  # is constant
        self._node: Widget = node  # is constant
        self._callback: Optional[Widget.InputCallback] = None

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

    def set_callback(self, callback: Optional[Widget.InputCallback]):
        """
        Callbacks must take two arguments (Widget.Self, ValueSignal) and return nothing.
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
            self._callback(Widget.Self(self._node), signal_copy)

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
    Outward facing Emitter on a Widget.
    """

    def __init__(self, circuit: 'Circuit', element_id: Circuit.Element.ID, schema: Value.Schema):
        """
        Constructor.
        It should only be possible to create-connect an InputPlug when constructing a new Widget instance.
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
        This method should only be callable from within Widget Callbacks.
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
class Widget:
    class Handle:
        def __init__(self, node: 'Widget'):
            assert isinstance(node, Widget)
            self._node: Optional[weak_ref] = weak_ref(node)

        def is_valid(self) -> bool:
            return self._node() is not None

        def get_name(self) -> Optional[str]:
            node: Widget = self._node()
            if node is not None:
                return node.get_name()

        def get_parent(self) -> Optional[Widget.Handle]:
            node: Widget = self._node()
            if node is not None:
                return node.get_parent()

    class Definition:
        """
       Instead of implementing `_add_property`, `_add_input` and `_add_output` methods in the Widget class, a Widget must be
       fully defined on construction, by passing in a Widget.Definition object. Like a Schema describes a type of Value,
       the Widget.Definition describes a type of Widget without making any assumptions about the actual state of the Widget.

       Properties and IPlugs must be constructed with a reference to the Widget instance that they live on. Therefore,
       we only store the construction arguments here and build the object in the Widget constructor.
       """

        class Property(NamedTuple):
            default: Value
            operation: Optional[Operator.Operation]

        def __init__(self):
            """
            Default Constructor.
            """
            self._input_plugs: Dict[str, Value.Schema] = {}
            self._output_plugs: Dict[str, Value.Schema] = {}
            self._properties: Dict[str, Widget.Definition.Property] = {}

        def add_input(self, name: str, schema: Value.Schema = Value.Schema()):
            """
            Stores the arguments need to construct a Slot instance on the node.
            :param name: Widget-unique name of the Slot.
            :param schema: Scheme of the Value received by the Slot.
            :raise NameError: If the Widget already has a Property, Signal or Slot with the given name.
            """
            if not self._is_name_available(name):
                raise NameError(f'Widget already has a member named "{name}"')
            self._input_plugs[name] = schema

        def add_output(self, name: str, schema: Value.Schema = Value.Schema()):
            """
            Stores the arguments need to construct a Output Plug instance on the node.
            :param name:        Widget-unique name of the OPlug.
            :param schema:      Scheme of the Value emitted by the OPlug.
            :raise NameError:   If the Widget already has a Property, Signal or Slot with the given name.
            """
            if not self._is_name_available(name):
                raise NameError(f'Widget already has a member named "{name}"')
            self._output_plugs[name] = schema

        def add_property(self, name: str, default: Any, operation: Optional[Operator.Operation] = None):
            """
            Stores the arguments need to construct a Property on the node.
            :param name: Widget-unique name of the Property.
            :param default:     Initial value of the Property.
            :param operation:   Operation applied to every new Value of this Property.
            :raise NameError:   If the Widget already has a Property, IPlug or OPlug with the given name.
            """
            if not self._is_name_available(name):
                raise NameError(f'Widget already has a member named "{name}"')

            # implicit value conversion
            if not isinstance(default, Value):
                try:
                    default = Value(default)
                except ValueError:
                    raise TypeError(f"Cannot convert {str(default)} to a Property default Value")

            # noinspection PyCallByClass
            self._properties[name] = Widget.Definition.Property(default, operation)

        def _is_name_available(self, name: str) -> bool:
            """
            Properties, Signals and Slots share the same namespace on a Widget. Therefore we need to check whether any
            one of the three dictionaries already contains a given name before it can be accepted as new.
            :param name: Name to test
            :return: Whether or not the given name would be a valid new name for a Property, Signal or Slot.
            """
            return (name not in self._input_plugs and
                    name not in self._output_plugs and
                    name not in self._properties)

    class Self:
        """
        Special kind of Widget Handle passed to Callbacks which allows the user to access the Widget's private data Value
        as well as its Properties and OPlugs.
        """

        def __init__(self, node: Widget):
            self._node: Widget = node

    InputCallback = Callable[[Self, ValueSignal], None]

    def __init__(self, parent: Optional['Widget'], scene: Scene, name: Optional[str] = None):
        self._parent: Optional[Widget] = parent
        self._scene: Scene = scene
        self._children: List[Widget] = []
        self._name: Optional[str] = name
        print(f'Creating Widget "{self.get_name() or str(id(self))}"')

    def __del__(self):  # virtual noexcept
        print(f'Deleting Widget "{self.get_name() or str(id(self))}"')

    def get_parent(self) -> Optional['Widget.Handle']:
        if self._parent is None:
            return None
        return Widget.Handle(self._parent)

    def get_name(self) -> Optional[str]:
        return self._name

    def create_child(self, definition: Widget.Definition, name: str) -> 'Widget.Handle':
        """
        Creates a new child of this Widget.
        :param definition:  Definition object of the Widget to create.
        :param name:        Parent-unique name of the Widget.
        :return:            A Handle to the created Widget.
        """
        assert issubclass(node_type, Widget)
        node: Widget = node_type(self, self._scene, *args, **kwargs)

        name: Optional[str] = node.get_name()
        if name is not None:
            if name in self._scene._named_nodes:
                raise NameError('Cannot create another Widget with the name "{}"'.format(name))
            self._scene._named_nodes[name] = weak_ref(node)

        self._children.append(node)
        return Widget.Handle(node)

    def remove(self):
        """
        Marks this Widget as expired. It will be deleted at the end of the Event.
        """
        self._scene._expired_nodes.add(Widget.Handle(self))

    # private for Scene
    def _remove_child(self, node_handle: 'Widget.Handle'):
        """
        Removes a child of this Widget.
        :param node_handle:     Child Widget to remove.
        """
        node: Optional[Widget] = node_handle._node()
        assert node is not None
        assert node in self._children
        node._remove_self()
        self._children.remove(node)
        del node
        assert not node_handle.is_valid()

    # private
    def _remove_self(self):
        """
        This method is called when this Widget is being removed.
        It recursively calls this method on all descendant nodes while still being fully initialized.
        """
        # recursively remove all children
        for child in self._children:
            child._remove_self()
        self._children.clear()


#######################################################################################################################

# noinspection PyProtectedMember
class Scene:

    def __init__(self):
        """
        Default Constructor.
        """
        # Expired nodes are kept around until the end of an Event.
        # We store Handles here an not owning references to the Nodes to keep ownership contained to the parent only.
        self._expired_nodes: Set[Widget.Handle] = set()

        # create the root node and name it '/'
        self._root: Widget = Widget(None, self, '/')

    def __del__(self):
        """
        Destructor.
        """
        self._root._remove_self()

    def create_node(self, definition: Widget.Definition, name: str) -> Widget.Handle:
        """
        Creates a new first-level Widget directly underneath the root.
        :param definition:  Definition object of the Widget to create.
        :param name:        Parent-unique name of the Widget.
        :return:            A Handle to the created Widget.
        """
        return self._root.create_child(definition, name)

    def remove_expired_nodes(self):
        """
        Is called at the end of the event loop by an Executor to remove all expired Nodes.
        """
        # first, we need to make sure we find the expired root(s), so we do not delete a child node before its parent
        # if they both expired during the same event
        expired_roots: Set[Widget.Handle] = set()
        for node in self._expired_nodes:
            parent: Optional[Widget.Handle] = node.get_parent()
            while parent is not None:
                if parent in self._expired_nodes:
                    break
            else:
                expired_roots.add(node)
        self._expired_nodes.clear()

        # delete all expired nodes in the correct order
        for node in expired_roots:
            parent_handle: Optional[Widget.Handle] = node.get_parent()
            assert parent_handle is not None  # root removal is handled manually at the destruction of the Scene
            parent: Optional[Widget] = parent_handle._node()
            assert parent is not None
            parent._remove_child(node)
            del parent

    # def get_node(self, name: str) -> Optional[Widget.Handle]:
    #     """
    #     Looks for a Widget with the given name and returns it if one is found.
    #     :param name:    Name of the Widget to look for.
    #     :return:        Valid handle to the requested Widget or None, if none was found.
    #     """
    #     weak_node: Optional[weak_ref] = self._named_nodes.get(name, None)
    #     if weak_node is None:
    #         return None
    #     node: Widget = weak_node()
    #     assert node is not None  # named nodes should remove themselves from the dict on destruction
    #     return Widget.Handle(node)

from __future__ import annotations
from typing import Optional, Dict, Set, Any, NamedTuple
from weakref import ref as weak_ref

from .value import Value
from .callback import Callback as CallbackBase
from .logic import Circuit, ValueSignal, Emitter, Receiver


#######################################################################################################################

class Property(Receiver, Emitter):  # final

    class Operation(CallbackBase):
        """
        A Property Operator is a Callback that is run on every new value that the Property receives via `_on_value`.

        The signature of a Property.Operation is as follows:

            (value: Value, widget: Widget.View, data: Value) -> (Optional[Value], Optional[Value])

        Parameter:
            value:  The new Value to operate on.
            widget: Widget.View object to inspect other Properties on the same Widget.
            data:   Private instance data that can be modified by the Operation and will be passed back the next time.

        Return type:
            first:  New Value of the Property or None to retain the existing one.
            second: New data to store in the Operation or None to retain the existing data.
        """

        def __init__(self, source: str, data: Optional[Value] = None):
            """
            Constructor.
            :param source:  Function body of a function adhering to the signature of a Property.Operation.
            :param data:    Initial private data Value. Is empty by default.
            """
            CallbackBase.__init__(self, dict(value=Value, widget=Widget.View, data=Value), source, dict(Value=Value))

            # initialize an empty data Value that can be used by the Callback function to store state
            self._data: Value = data or Value()

        def __call__(self, value: Value, widget: Widget.View) -> Optional[Value]:
            """
            Wraps Callback.__call__ to add the private data argument.
            :param value:       New value for the Callback to operate on.
            :param widget:      Widget.View object to inspect other Properties on the same Widget.
            :return:            Callback result.
            :raise TypeError:   If the callback did not return the correct result types.
            :raise Exception: From user-defined code.
            """
            # execute the Callback and add the private data as argument
            result = CallbackBase.__call__(self, value, widget, self._data)  # this could throw any exception

            # the return type should be a Value, Value pair
            if not (isinstance(result, tuple) and len(result) == 2):
                raise TypeError("Property.Callbacks must return (Optional[Value], Optional[Value])")
            new_value, new_data = result
            # TODO: with mutable Values, we should be able to modify the data argument directly (and not return it)

            # check the returned value
            if new_value is not None and not isinstance(new_value, Value):
                try:
                    new_value = Value(new_value)
                except ValueError:
                    raise TypeError("Property.Callbacks must return the new value or None as the first return value")

            # update the private data
            if new_data is not None:
                success = True
                if not isinstance(new_data, Value):
                    try:
                        new_data = Value(new_data)
                    except ValueError:
                        success = False
                if success and new_data.schema != self._data.schema:
                    success = False
                if not success:
                    raise TypeError("Property.Callbacks must return the data or None as the second return value")
                else:
                    self._data = new_data

            return new_value

    class Callback(CallbackBase):
        """
        Callback invoked by a Property after its Value changed.
        """

        def __init__(self, source: str):
            CallbackBase.__init__(self, dict(value=Value, widget=Widget.Self), source)

    def __init__(self, circuit: Circuit, element_id: Circuit.Element.ID, widget: 'Widget', value: Value,
                 operation: Optional[Operation]):
        """
        Constructor.
        Should only be called from a Widget constructor with a valid Widget.Definition - hence we don't check the
        operation for validity at runtime and just assume that it ingests and produces the correct Value type.
        :param circuit:     The Circuit containing this Element.
        :param element_id:  The Circuit-unique identification number of this Element.
        :param widget: Widget that this Property lives on.
        :param value: Initial value of the Property, also defines its Schema.
        :param operation: Operations applied to every new Value of this Property, can be a noop.
        """
        # various asserts, all of this should be guaranteed by the Widget.Definition
        assert not value.schema.is_empty()

        Receiver.__init__(self, value.schema)
        Emitter.__init__(self, value.schema)

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Circuit.Element.ID = element_id  # is constant
        self._operation: Optional[Property.Operation] = operation  # is constant
        self._widget: Widget = widget  # is constant
        self._callback: Optional[Property.Callback] = None
        self._value: Value = value

    def get_id(self) -> Circuit.Element.ID:  # final, noexcept
        """
        The Circuit-unique identification number of this Element.
        """
        return self._element_id

    def get_circuit(self) -> Circuit:  # final, noexcept
        """
        The Circuit containing this Element.
        """
        return self._circuit

    def get_value(self) -> Value:  # noexcept
        """
        The current value of this Property.
        """
        return self._value

    def set_callback(self, callback: Optional[Property.Callback]):
        """
        Callbacks must take two arguments (Value, Widget.Self) and return nothing.
        :param callback:    New Callback to call when the Property changes its Value.
        """
        self._callback = callback

    def set_value(self, value: Any):
        """
        Applies a new Value to this Property. The Value must be of the Property's type and is transformed by the
        Property's Operation before assignment.
        :param value: New value to assign to this Property.
        :raise TypeError: If the given Value type does not match the one stored in this Property.
        :raise Exception: From user-defined code.
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

        # since the Operation contains user-defined code, this might throw
        if self._operation is not None:
            value = self._operation(value, Widget.View(self._widget))
            assert value.schema == self._value.schema

        # return early if the value didn't change
        if value == self._value:
            return

        # update the property's value
        self._value = value

        # invoke the registered callback, if there is one
        if self._callback is not None:
            self._callback(self._value, Widget.Self(self._widget))

        # lastly, emit the changed value into the circuit
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

    class Callback(CallbackBase):
        """
        Callback invoked by an InputPlug.
        """

        def __init__(self, source: str):
            CallbackBase.__init__(self, dict(signal=ValueSignal, widget=Widget.Self), source)

    def __init__(self, circuit: Circuit, element_id: Circuit.Element.ID, widget: 'Widget', schema: Value.Schema):
        """
        Constructor.
        It should only be possible to create-connect an InputPlug when constructing a new Widget instance.
        :param circuit:     The Circuit containing this Element.
        :param element_id:  The Circuit-unique identification number of this Element.
        :param widget:      Widget on which the Plug lives.
        :param schema:      Value type accepted by this Plug.
        """
        Receiver.__init__(self, schema)

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Circuit.Element.ID = element_id  # is constant
        self._widget: Widget = widget  # is constant
        self._callback: Optional[InputPlug.Callback] = None

    def get_id(self) -> Circuit.Element.ID:  # final, noexcept
        """
        The Circuit-unique identification number of this Element.
        """
        return self._element_id

    def get_circuit(self) -> Circuit:  # final, noexcept
        """
        The Circuit containing this Element.
        """
        return self._circuit

    def set_callback(self, callback: Optional[InputPlug.Callback]):
        """
        Callbacks must take two arguments (ValueSignal, Widget.Self) and return nothing.
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
            self._callback(Widget.Self(self._widget), signal_copy)

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

    def __init__(self, circuit: Circuit, element_id: Circuit.Element.ID, schema: Value.Schema):
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

    def get_circuit(self) -> Circuit:  # final, noexcept
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
    class Definition:
        """
       Instead of implementing `_add_property`, `_add_input` and `_add_output` methods in the Widget class, a Widget
       must be fully defined on construction, by passing in a Widget.Type object. Like a Schema describes a type of
       Value, the Widget.Type describes a type of Widget without making any assumptions about the actual state of the
       Widget.
       A Widget.Definition object is used to describe a Type and is mutable, because the Type object itself is const.

       Properties and IPlugs must be constructed with a reference to the Widget instance that they live on. Therefore,
       we only store the construction arguments here and build the object in the Widget constructor.

       Widget Types have a name which stored in the Scene to ensure that no two Types with the same name can exist at
       the same time.

       All methods of this class are not thread-safe.
       """

        class PropertyDefinition(NamedTuple):
            default: Value
            operation: Optional[Property.Operation]

        def __init__(self):
            """
            Default Constructor.
            """
            self._properties: Dict[str, Widget.Definition.PropertyDefinition] = {}
            self._input_plugs: Dict[str, Value.Schema] = {}
            self._output_plugs: Dict[str, Value.Schema] = {}

        def add_property(self, name: str, default: Any, operation: Optional[Property.Operation] = None):
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

            # property values cannot be None
            if default.is_none():
                raise ValueError("Properties can not store a None value")

            # noinspection PyCallByClass
            self._properties[name] = Widget.Definition.PropertyDefinition(default, operation)

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

    class Type:
        """
        Type objects should only be created by the Scene.
        """

        def __init__(self, name: str, definition: Widget.Definition):  # noexcept
            """
            Constructor.
            :param name: All Widgets created with this Definition will return this name as their type identifier.
            :param definition: Widget Definition to build the Type from. Would be an r-value in C++.
            """
            self._name: str = name  # is constant
            self._properties: Dict[str, Widget.Definition.PropertyDefinition] = definition._properties  # is constant
            self._input_plugs: Dict[str, Value.Schema] = definition._input_plugs  # is constant
            self._output_plugs: Dict[str, Value.Schema] = definition._output_plugs  # is constant

        def get_name(self) -> str:
            """
            Scene-unique name of the Widget type.
            """
            return self._name

        def get_properties(self) -> Dict[str, Widget.Definition.PropertyDefinition]:
            """
            All Properties defined for this Widget Type.
            """
            return self._properties

        def get_input_plugs(self) -> Dict[str, Value.Schema]:
            """
            All Input Plugs defined for this Widget Type.
            """
            return self._input_plugs

        def get_output_plugs(self) -> Dict[str, Value.Schema]:
            """
            All Output Plugs defined for this Widget Type.
            """
            return self._output_plugs

    class Path:
        class Error(Exception):
            pass

        def __str__(self) -> str:
            return ""  # TODO

    class View:
        """
        A View onto a Widget instance. A View is only able to inspect Property Values and nothing else.
        It is the Widget accessor with the lowest level of access.
        """

        def __init__(self, widget: Widget):
            """
            Constructor.
            :param widget: Widget that is being viewed.
            """
            self._widget: Widget = widget

        def __getitem__(self, property_name: str) -> Optional[Value]:
            """
            Returns the value of the Property with the given name or None if the Widget has no Property by the name.
            :param property_name: Name of the Property providing the Value.
            :return: The Property's Value or None if no Property by the given name exists on the Widget.
            """
            prop: Optional[Property] = self._widget._properties.get(property_name)
            if prop is None:
                return None
            return prop.get_value()

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

    class Self:
        """
        Special kind of Widget Handle passed to Callbacks which allows the user to access the Widget's private data
        Value as well as its Properties and OPlugs.
        """

        def __init__(self, widget: Widget):
            self._widget: Widget = widget

    def __init__(self, definition: Widget.Type, parent: Optional[Widget], scene: Scene, name: str):
        """
        Constructor.
        :param definition: Widget Definition that determines the Type of this Widget.
        :param parent: Parent Widget. May only be None if this is the root.
        :param scene: Scene containing this Widget.
        :param name: Parent-unique name of this Widget.
        """

        # store constant arguments
        self._parent: Optional[Widget] = parent  # is constant
        self._scene: Scene = scene  # is constant
        self._name: str = name  # is constant
        self._type: str = definition.get_name()  # is constant

        self._children: Dict[str, Widget] = {}

        # apply widget.definition
        circuit: Circuit = self._scene.get_circuit()
        self._properties: Dict[str, Property] = {
            name: circuit.create_element(Property, self, prop.default, prop.operation)
            for name, prop in definition.get_properties().items()
        }
        self._input_plugs: Dict[str, InputPlug] = {
            name: circuit.create_element(InputPlug, self, schema)
            for name, schema in definition.get_input_plugs().items()
        }
        self._output_plugs: Dict[str, OutputPlug] = {
            name: circuit.create_element(OutputPlug, schema)
            for name, schema in definition.get_output_plugs().items()
        }

    def get_type(self) -> str:
        """
        The Scene-unique name of this Widget's Type.
        """
        return self._type

    def get_parent(self) -> Optional[Widget.Handle]:
        """
        The parent of this Widget, is only None if this is the root Widget.
        """
        if self._parent is None:
            return None
        else:
            return Widget.Handle(self._parent)

    def get_name(self) -> str:
        """
        The parent-unique name of this Widget instance.
        """
        return self._name

    def get_path(self) -> Widget.Path:
        return self.Path()  # TODO: get_path

    def create_child(self, type_name: str, child_name: str) -> Widget.Handle:
        """
        Creates a new child of this Widget.
        :param type_name:   Name of the Type of the Widget to create
        :param child_name:  Parent-unique name of the child Widget.
        :return:            A Handle to the created Widget.
        :raise NameError:           If the Scene does not know a Type of the given type name.
        :raise Widget.Path.Error:   If the Widget already has a child Widget with the given name.
        """
        # ensure the name is unique
        if child_name in self._children:
            raise Widget.Path.Error(f'Cannot create another child Widget of "{self.get_path()}" named "{child_name}"')

        # get the widget type from the scene
        widget_type: Optional[Widget.Type] = self._scene._get_widget_type(type_name)
        if widget_type is None:
            raise NameError(f'No Widget Type named "{type_name}" is registered with the Scene')

        # create the child widget
        node: Widget = Widget(widget_type, self, self._scene, child_name)
        self._children[child_name] = node

        # return a handle
        return Widget.Handle(node)

    def remove(self):
        """
        Marks this Widget as expired. It will be deleted at the end of the Event.
        """
        self._scene._expired_widgets.add(Widget.Handle(self))

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
        del self._children[node.get_name()]
        del node
        assert not node_handle.is_valid()

    # private
    def _remove_self(self):
        """
        This method is called when this Widget is being removed.
        It recursively calls this method on all descendant nodes while still being fully initialized.
        """
        # recursively remove all children
        for child in self._children.values():
            child._remove_self()
        self._children.clear()


#######################################################################################################################

# noinspection PyProtectedMember
class Scene:

    def __init__(self):
        """
        Default Constructor.
        """
        # Expired Widgets are kept around until the end of an Event.
        # We store Handles here to keep ownership contained to the parent Widget only.
        self._expired_widgets: Set[Widget.Handle] = set()

        self._root: Widget = Widget(Widget.Type("", Widget.Definition()), None, self, '/')  # is constant
        self._circuit: Circuit = Circuit()  # is constant

        self._widget_types: Dict[str, Widget.Type] = {}

    def __del__(self):
        """
        Destructor.
        """
        self._root._remove_self()

    def get_circuit(self) -> Circuit:
        """
        The Circuit managed by this Scene.
        """
        return self._circuit

    def register_widget_type(self, type_name: str, definition: Widget.Definition):
        if type_name in self._widget_types:
            raise NameError(f'Another Widget type with the name "{type_name}" exists already in the Scene')
        else:
            self._widget_types[type_name] = Widget.Type(type_name, definition)

    # def get_widget(self, path: Widget.Path) -> Optional[Widget.Handle]:
    #     """
    #
    #     :param path:
    #     :return:
    #     """
    #     return self._root.find_widget(path)

    def create_widget(self, type_name: str, name: str) -> Widget.Handle:
        """
        Creates a new first-level Widget directly underneath the root.
        :param type_name:   Name of the Type of the Widget to create
        :param name:        Root-unique name of the Widget.
        :return:            A Handle to the created Widget.
        """
        return self._root.create_child(type_name, name)

    def remove_expired_widgets(self):
        """
        Is called at the end of an Event to remove all expired Widgets.
        """
        # first, we need to make sure we find the expired root(s), so we do not delete a child node before its parent
        # if they both expired during the same event
        expired_roots: Set[Widget.Handle] = set()
        for widget in self._expired_widgets:
            parent: Optional[Widget.Handle] = widget.get_parent()
            while parent is not None:
                if parent in self._expired_widgets:
                    break
            else:
                expired_roots.add(widget)
        self._expired_widgets.clear()

        # delete all expired nodes in the correct order
        for widget in expired_roots:
            parent_handle: Optional[Widget.Handle] = widget.get_parent()
            assert parent_handle is not None  # root removal is handled manually at the destruction of the Scene
            parent: Optional[Widget] = parent_handle._node()
            assert parent is not None
            parent._remove_child(widget)
            del parent

    # private for Widget
    def _get_widget_type(self, type_name: str) -> Optional[Widget.Type]:
        """
        Should be private and visible to Widget only.
        :param type_name: Name of the Widget Type.
        :return: Requested Widget Type or None if the Scene doesn't know a Type by that name.
        """
        return self._widget_types.get(type_name)

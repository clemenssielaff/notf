from __future__ import annotations
from typing import Optional, Dict, NamedTuple, List, Any

from pynotf.value import Value
from pynotf.logic import Circuit, Receiver, Emitter

from .output_plug import OutputPlug
from .path import Path


########################################################################################################################

class PropertyDefinition(NamedTuple):
    default_value: Value
    operation: Optional[Property.Operation] = None


########################################################################################################################

class WidgetDefinition(NamedTuple):
    """
    Fully describes a Widget Type.
    A Widget.Definition can be created by anyone and does not have to valid. Before you can use it as a Widget.Type, it
    has to be validated.
    See each field of this tuple for details on what constraints apply.
    """
    # the Scene-unique name of the Widget Type
    # cannot be empty and must not contain Path control symbols like '/' and '.' so the type of a widget is a valid
    # default name for an instance
    type_name: str

    # all Properties and input/output Plugs share the same namespace
    # names cannot be empty and must not contain Path control symbols like '/' and '.'
    properties: Dict[str, PropertyDefinition]
    input_plugs: Dict[str, Value.Schema]
    output_plugs: Dict[str, Value.Schema]

    # callbacks that can be attached to an input plug
    input_callbacks: Dict[str, InputPlug.Callback]

    # callbacks that can be attached to a property
    property_callbacks: Dict[str, Property.Callback]

    # state machine of the Widget
    state_machine: StateMachine


########################################################################################################################

class WidgetType(WidgetDefinition):
    """
    Instead of implementing  `_add_property`, `_add_input` and `_add_output` methods in the Widget class, a Widget
    must be fully defined on construction, by passing in a Widget.Type object. Like a Schema describes a type of
    Value, the Widget.Type describes a type of Widget without making any assumptions about the actual state of the
    Widget.

    A Widget.Type is equivalent to a Widget.Definition - the only difference between the two is that the Type has been
    validated whereas Widget.Definitions can be created in an invalid state.
    """

    class ConsistencyError(Exception):
        """
        Error raised when a Widget.Definition failed to validate.
        """
        pass

    @classmethod
    def create(cls, definition: WidgetDefinition) -> WidgetType:
        """
        Widget Type Factory.
        Takes and validates a Widget Definition.
        Only the Scene should be able to create a Widget.Type from a Widget.Definition.
        :param definition: Widget Definition to build the Type from. Would be an r-value in C++.
        """
        forbidden_symbols: List[str] = [Path.get_widget_delimiter(), Path.get_element_delimiter()]

        # type name
        if len(definition.type_name) == 0:
            raise cls.ConsistencyError('The name of a Widget Type cannot be empty.')
        for forbidden_symbol in forbidden_symbols:
            if forbidden_symbol in definition.type_name:
                raise cls.ConsistencyError('Name "{} "is invalid because it contains "{}".\n'
                                           "Widget Type names may not contain symbols '{}'".format(
                    definition.type_name, forbidden_symbol, "', '".join(forbidden_symbols))
                )

        # property default values
        for name, property_definition in definition.properties.items():
            if property_definition.default_value.is_none():
                raise cls.ConsistencyError(f'Invalid Property "{name}", Properties can not store a None Value.')

        # shared namespace of properties and plugs
        for source, kind in ((definition.properties, "Property"),
                             (definition.input_plugs, "Input Plug"),
                             (definition.output_plugs, "Output Plug")):
            for name, target_definition in source.items():
                for forbidden_symbol in forbidden_symbols:
                    if forbidden_symbol in name:
                        raise cls.ConsistencyError(
                            "{} names may not contain '{}'".format(kind, "', '".join(forbidden_symbols)))
                if name in definition.properties and source is not definition.properties:
                    raise cls.ConsistencyError(f'Cannot add {kind} "{name}" to Widget Type {definition.type_name} '
                                               f'because it already contains a Property with the same name.')
                if name in definition.input_plugs and source is not definition.input_plugs:
                    raise cls.ConsistencyError(f'Cannot add {kind} "{name}" to Widget Type {definition.type_name} '
                                               f'because it already contains an Input Plug with the same name.')
                if name in definition.output_plugs and source is not definition.output_plugs:
                    raise cls.ConsistencyError(f'Cannot add {kind} "{name}" to Widget Type {definition.type_name} '
                                               f'because it already contains an Output Plug with the same name.')

        return WidgetType(*definition)


########################################################################################################################

class WidgetView:
    """
    A View onto a Widget instance. A View is only able to inspect Property Values and nothing else.
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
        prop: Optional[Property] = self._widget.get_property(property_name)
        if prop is None:
            return None
        else:
            return prop.get_value()


########################################################################################################################

class WidgetHandle(WidgetView):
    """
    Mutability Widget Handle passed to Callbacks which allows the user to modify the Widget.
    """

    def __init__(self, widget: Widget):
        """
        Constructor.
        :param widget: Widget that is being handled.
        """
        WidgetView.__init__(self, widget)

    def __setitem__(self, property_name: str, value: Any):
        """
        Updates the Property with the given name to value
        :param property_name: Name of the Property to update.
        :param value: Value to update the Property with.
        :raise NameError: if the Widget has no Property by the given name.
        :raise TypeError: If the given value does not match the Property's.
        """
        prop: Optional[Property] = self._widget.get_property(property_name)
        if prop is None:
            raise NameError(f'Widget "{self._widget.get_path()}" has no Property named "{property_name}"')
        else:
            prop.set_value(value)

    def emit(self, output_name: str, value: Any) -> None:
        """
        Emits a new Value from one of the Widget's Output Plugs.
        :param output_name: Name of the Output Plug to emit from.
        :param value: Value to emit.
        :raise NameError: If the Widget has no Output Plug by the given name.
        :raise ValueError: If `value` could not be converted into a Value instance.
        :raise TypeError: If the given Value is of the wrong type for the Output Plug.
        """
        output_plug: Optional[OutputPlug] = self._widget.get_output_plug(output_name)
        if output_plug is None:
            raise NameError(f'Widget "{self._widget.get_path()}" has no Output Plug named "{output_name}"')

        output_plug.emit(value)

    def transition_into(self, state_name: str):
        """
        Transitions from the current into another State with the given name.
        Re-transitioning into the current State has to be allowed explicitly in the State Machine Description, otherwise
        it is considered a TransitionError.
        :param state_name: Name of the State to transition into.
        :raise NameError: If the State Machine does not have a State with the given name.
        :raise TransitionError: If you cannot enter the target State from the current one.
        """
        self._widget.transition_into(state_name)

    def set_input_callback(self, input_name: str, callback_name: str):
        """
        Sets or removes the Callback of an Input Plug.
        :param input_name: Name of the Input Plug to modify.
        :param callback_name: New Callback to call on input or the empty string to uninstall the current Callback.
        :raise NameError: If the Widget has no Input Plug by the given name.
        :raise NameError: If the Widget has no Input Callback by the given name.
        """
        input_plug: Optional[InputPlug] = self._widget.get_input_plug(input_name)
        if input_plug is None:
            raise NameError(f'Widget "{self._widget.get_path()}" has no Input Plug named "{input_name}"')

        if callback_name == "":
            input_callback = None
        else:
            input_callback: Optional[InputPlug.Callback] = self._widget.get_input_callback(callback_name)
            if input_callback is None:
                raise NameError(f'Widget "{self._widget.get_path()}" has no Input Callback named "{callback_name}"')

        input_plug.set_callback(input_callback)

    def set_property_callback(self, property_name: str, callback_name: str):
        """
        Sets or removes the Callback of a Property.
        :param property_name: Name of the Property to modify.
        :param callback_name: New Callback to call on input or the empty string to uninstall the current Callback.
        :raise NameError: If the Widget has no Property by the given name.
        :raise NameError: If the Widget has no Property Callback by the given name.
        """
        prop: Optional[Property] = self._widget.get_property(property_name)
        if prop is None:
            raise NameError(f'Widget "{self._widget.get_path()}" has no Property named "{property_name}"')

        if callback_name == "":
            property_callback = None
        else:
            property_callback: Optional[Property.Callback] = self._widget.get_property_callback(callback_name)
            if property_callback is None:
                raise NameError(f'Widget "{self._widget.get_path()}" has no Property Callback named "{callback_name}"')

        prop.set_callback(property_callback)

    def connect_input_to(self, input_name: str, output_path: Path):
        """
        Connect an Input Plug of this Widget to an Emitter in the Circuit.
        :param input_name: Name of the Input Plug to connect.
        :param output_path: Path to the Emitter in the Circuit to connect to.
        :raise NameError: If the Widget has no Input Plug by the given name.
        :raise Path.Error: If the Path to the Emitter could not be resolved.
        """
        input_plug: Optional[InputPlug] = self._widget.get_input_plug(input_name)
        if input_plug is None:
            raise NameError(f'Widget "{self._widget.get_path()}" has no Input Plug named "{input_name}"')

        try:
            emitter: Optional[Emitter] = self._widget.find_output(output_path)
        except Path.Error as path_error:
            raise Path.Error(f'Failed to connect input "{input_name}" to "{output_path}".\n{str(path_error)}')
        if emitter is None:
            raise Path.Error(f'Widget "{output_path.get_widget_path()}" '
                             f'has no output named "{output_path.get_element()}"')

        input_plug.connect_to(emitter)

    def connect_output_to(self, output_name: str, input_path: Path):
        """
        Connect an Output Plug of this Widget to a Receiver in the Circuit.
        :param output_name: Name of the Output Plug to connect.
        :param input_path: Path to the Receiver in the Circuit to connect to.
        :raise NameError: If the Widget has no Output Plug by the given name.
        :raise Path.Error: If the Path to the Receiver could not be resolved.
        """
        output_plug: Optional[OutputPlug] = self._widget.get_output_plug(output_name)
        if output_plug is None:
            raise NameError(f'Widget "{self._widget.get_path()}" has no Output Plug named "{output_name}"')

        try:
            receiver: Optional[Receiver] = self._widget.find_input(input_path)
        except Path.Error as path_error:
            raise Path.Error(f'Failed to connect output "{output_name}" to "{input_path}".\n{str(path_error)}')
        if receiver is None:
            raise Path.Error(f'Widget "{input_path.get_widget_path()}" '
                             f'has no input named "{input_path.get_element()}"')

        receiver.connect_to(output_plug)

    def disconnect_input(self, input_name: str, output_path: Optional[Path] = None):
        """
        Disconnects an Input Plug from a specific or all upstream Emitters.
        :param input_name: Name of the Input Plug to disconnect.
        :param output_path: Optional Path to a specific Emitter to disconnect from.
        :raise NameError: If the Widget has no Input Plug by the given name.
        :raise Path.Error: If the Path to the Emitter could not be resolved.
        """
        input_plug: Optional[InputPlug] = self._widget.get_input_plug(input_name)
        if input_plug is None:
            raise NameError(f'Widget "{self._widget.get_path()}" has no Input Plug named "{input_name}"')

        if output_path is None:
            input_plug.disconnect_upstream()

        else:
            try:
                emitter: Optional[Emitter] = self._widget.find_output(output_path)
            except Path.Error as path_error:
                raise Path.Error(f'Failed to resolve "{output_path}".\n{str(path_error)}')
            if emitter is None:
                raise Path.Error(f'Widget "{output_path.get_widget_path()}" '
                                 f'has no output named "{output_path.get_element()}"')

            input_plug.disconnect_from(emitter)


    def disconnect_output(self, output_name: str, input_path: Path):
        """
        Disconnects an Output Plug from a specific downstream Receiver.
        Unlike the `disconnect_input` function, we do not allow the unspecified removal of all outgoing connections,
        since an Emitter should not know nor care about who receives its Signals.
        :param output_name: Name of the Output Plug to disconnect.
        :param input_path: Path to a specific Receiver to disconnect from.
        :raise NameError: If the Widget has no Output Plug by the given name.
        :raise Path.Error: If the Path to the Receiver could not be resolved.
        """
        output_plug: Optional[OutputPlug] = self._widget.get_output_plug(output_name)
        if output_plug is None:
            raise NameError(f'Widget "{self._widget.get_path()}" has no Output Plug named "{output_name}"')

        try:
            receiver: Optional[Receiver] = self._widget.find_input(input_path)
        except Path.Error as path_error:
            raise Path.Error(f'Failed to resolve "{input_path}".\n{str(path_error)}')
        if receiver is None:
            raise Path.Error(f'Widget "{input_path.get_widget_path()}" '
                             f'has no input named "{input_path.get_element()}"')

        receiver.disconnect_from(output_plug)


########################################################################################################################

class Widget:
    """

    """
    Definition = WidgetDefinition
    PropertyDefinition = PropertyDefinition
    Type = WidgetType
    Path = Path
    View = WidgetView
    Handle = WidgetHandle

    def __init__(self, definition: Type, parent: Optional[Widget], scene: 'Scene', name: str):
        """
        Constructor.
        :param definition: Widget Definition that determines the Type of this Widget.
        :param parent: Parent Widget. May only be None if this is the root.
        :param scene: Scene containing this Widget.
        :param name: Parent-unique name of this Widget.
        """
        # store constant fields (in C++ these could even be public)
        self._parent: Optional[Widget] = parent  # is constant
        self._scene: Scene = scene  # is constant
        self._name: str = name  # is constant
        self._type: str = definition.type_name  # is constant
        self._input_callbacks: Dict[str, InputPlug.Callback] = definition.input_callbacks  # is constant
        self._property_callbacks: Dict[str, Property.Callback] = definition.property_callbacks  # is constant

        # initialize mutable fields
        self._children: Dict[str, Widget] = {}
        self._state_machine: StateMachine = definition.state_machine

        # apply widget.definition
        circuit: Circuit = self._scene.get_circuit()
        self._properties: Dict[str, Property] = {
            name: circuit.create_element(Property, self, prop.default_value, prop.operation)
            for name, prop in definition.properties.items()
        }
        self._input_plugs: Dict[str, InputPlug] = {
            name: circuit.create_element(InputPlug, self, schema)
            for name, schema in definition.input_plugs.items()
        }
        self._output_plugs: Dict[str, OutputPlug] = {
            name: circuit.create_element(OutputPlug, schema)
            for name, schema in definition.output_plugs.items()
        }

        # enter the initial state
        self._state_machine.initialize(self)

    def get_type(self) -> str:
        """
        The Scene-unique name of this Widget's Type.
        """
        return self._type

    def get_name(self) -> str:
        """
        The parent-unique name of this Widget instance.
        """
        return self._name

    def get_path(self) -> Path:
        """
        Returns the absolute Path of this Widget.
        """
        path: List[str] = []
        next_widget: Optional[Widget] = self
        while next_widget is not None:
            path.append(next_widget.get_name())
            next_widget = next_widget._parent
        return Path.create(list(reversed(path)))

    def get_parent(self) -> Optional[Widget]:
        """
        The parent of this Widget, is only None if this is the root Widget.
        """
        return self._parent

    def get_property(self, name: str) -> Optional[Property]:
        """
        Returns the Property with the given name or None if the Widget has no Property by the name.
        :param name: Name of the Property.
        :return: The Property with the given name or None if the Widget has no Property by the name.
        """
        return self._properties.get(name)

    def get_property_callback(self, name: str) -> Optional[Property.Callback]:
        """
        Returns the Property Callback associated with the given name or None, if none exists.
        :param name: Name of the Property Callback.
        """
        return self._property_callbacks.get(name)

    def get_input_plug(self, name: str) -> Optional[InputPlug]:
        """
        Returns the Input Plug associated with the given name or None, if none exists.
        :param name: Name of the Input Plug.
        """
        return self._input_plugs.get(name)

    def get_input_callback(self, name: str) -> Optional[InputPlug.Callback]:
        """
        Returns the Input Callback associated with the given name or None, if none exists.
        :param name: Name of the Input Callback.
        """
        return self._input_callbacks.get(name)

    def get_output_plug(self, name: str) -> Optional[OutputPlug]:
        """
        Returns the Output Plug associated with the given name or None, if the Widget has no output plug by the name.
        :param name: Name of the Output Plug.
        """
        return self._output_plugs.get(name)

    def find_widget(self, path: Path) -> Optional[Widget]:
        """
        Tries to find a Widget in the Scene from a given Path.
        :param path: Path of the Widget to find.
        :return: Handle of the Widget or None if the Path doesn't point to a Widget.
        """
        if path.is_absolute():
            return self._scene.find_widget(path)  # absolute paths start from the scene
        else:
            return self._find_widget(path, 0)  # start following the relative path starting here

    def find_input(self, path: Path) -> Optional[Receiver]:
        """
        Finds a Receiver in the Circuit - either an Input Plug or a Property,
        :param path: Path leading to a Receiver in the Circuit.
        :raise Path.Error: If the Path did not resolve to an Element or the input is an internal Property.
        :return: The target input or None, if none could be found (but the Path resolved successfully).
        """
        # if the path does not denote an element, there is no way that we will find one
        element_name: Optional[str] = path.get_element()
        if element_name is None:
            raise Path.Error(f'Path "{path}" does not point to an Element')

        # if we cannot find the widget, we won't find an input on it
        target_widget: Optional[Widget] = self.find_widget(path)
        if target_widget is None:
            raise Path.Error(f'Could not find a Widget with the path: "{path}"')

        # first, try if the path leads to an input plug
        input_plug: Optional[InputPlug] = target_widget.get_input_plug(element_name)
        if input_plug is not None:
            return input_plug

        # maybe the path is to a property?
        input_property: Optional[Property] = target_widget.get_property(element_name)
        if input_property is not None:
            # TODO: check if the property allows external connections and raise an Error if it doesn't
            #  this way we can let the user know what's happening instead of pretending that the property doesn't exist
            return input_property

        # the element name is not an input on the target widget
        return None

    def find_output(self, path: Path) -> Optional[Emitter]:
        """
        Finds an Emitter in the Circuit - either an Output Plug or a Property,
        :param path: Path leading to an Emitter in the Circuit.
        :raise Path.Error: If the Path did not resolve to an Element.
        :return: The target output or None, if none could be found (but the Path resolved successfully).
        """
        # if the path does not denote an element, there is no way that we will find one
        element_name: Optional[str] = path.get_element()
        if element_name is None:
            raise Path.Error(f'Path "{path}" does not point to an Element')

        # if we cannot find the widget, we won't find an output on it
        target_widget: Optional[Widget] = self.find_widget(path)
        if target_widget is None:
            raise Path.Error(f'Failed to find a Widget with the path: "{path}"')

        # first, try if the path leads to an output plug
        output_plug: Optional[OutputPlug] = target_widget.get_output_plug(element_name)
        if output_plug is not None:
            return output_plug

        # maybe the path is to a property?
        output_property: Optional[Property] = target_widget.get_property(element_name)
        if output_property is not None:
            return output_property

        # the element name is not an output on the target widget
        return None

    def create_child(self, type_name: str, child_name: str) -> Widget:
        """
        Creates a new child of this Widget.
        :param type_name:   Name of the Type of the Widget to create
        :param child_name:  Parent-unique name of the child Widget.
        :return:            A Handle to the created Widget.
        :raise NameError:           If the Scene does not know a Type of the given type name.
        :raise Path.Error:   If the Widget already has a child Widget with the given name.
        """
        # validate the name
        if not self._validate_name(child_name):
            raise Path.Error(f'Cannot create a Widget with an invalid name "{child_name}".\n'
                             f'Widget names must be alphanumeric and cannot be empty.')

        # ensure the name is unique
        if child_name in self._children:
            raise Path.Error(f'Cannot create another child Widget of "{self.get_path()}" named "{child_name}"')

        # get the widget type from the scene
        widget_type: Optional[WidgetType] = self._scene._get_widget_type(type_name)
        if widget_type is None:
            raise NameError(f'No Widget Type named "{type_name}" is registered with the Scene')

        # create the child widget
        child_widget: Widget = Widget(widget_type, self, self._scene, child_name)
        self._children[child_name] = child_widget

        # return the child widget
        return child_widget

    def transition_into(self, state_name: str):
        """
        Transitions from the current into another State with the given name.
        Re-transitioning into the current State has to be allowed explicitly in the State Machine Description, otherwise
        it is considered a TransitionError.
        :param state_name: Name of the State to transition into.
        :raise NameError: If the State Machine does not have a State with the given name.
        :raise TransitionError: If you cannot enter the target State from the current one.
        """
        self._state_machine.transition_into(state_name)

    def remove(self):
        """
        Marks this Widget as expired. It will be deleted at the end of the Event.
        """
        self._scene._expired_widgets.add(self)

    # private for Scene
    def _find_widget(self, path: Path, step: int) -> Optional[Widget]:
        """
        Recursive implementation to follow a Path to a particular Widget.
        :param path: Path of the Widget to find.
        :param step: Current position in the Path.
        :return: Handle of the Widget or None if the Path doesn't point to a Widget.
        """
        # if this is the final step, we have reached the target widget
        if step == len(path):
            assert path.get_widget(len(path) - 1) == self._name
            return self

        # if this is not the final step, get the next step
        next_child: Optional[str] = path.get_widget(step)
        assert next_child is not None

        # the two dots are a special instruction to go up one level in the hierarchy
        if next_child == '..':
            return self._parent._find_widget(path, step + 1)

        # if the next widget from the path is not a child of this one, return None
        child_widget: Optional[Widget] = self._children.get(next_child)
        if child_widget is None:
            return None

        # if the child widget is found, advance to the next step in the path and let the child continue the search
        return child_widget._find_widget(path, step + 1)

    def _remove_child(self, widget: Widget):
        """
        Removes a child of this Widget.
        :param widget:  Child Widget to remove.
        """
        assert widget in self._children
        widget._remove_self()
        del self._children[widget.get_name()]
        del widget

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

    @staticmethod
    def _validate_name(name: str) -> bool:
        """
        Tests whether the given name is valid Widget name.
        :param name: Name to test.
        :return: True if the name is valid.
        """
        return name.isalnum()


########################################################################################################################

from .scene import Scene
from .property import Property
from .input_plug import InputPlug
from .statemachine import StateMachine

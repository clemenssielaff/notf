from __future__ import annotations
from typing import Optional, Dict, NamedTuple, List

from pynotf.value import Value
from pynotf.logic import Circuit

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
    plug_callbacks: Dict[str, InputPlug.Callback]

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
        forbidden_symbols: List[str] = [Path.get_widget_delimiter(), Path.get_property_delimiter()]

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
        return self._widget.get_property(property_name)


########################################################################################################################

class WidgetHandle:
    """
    Mutability Widget Handle passed to Callbacks which allows the user to access the Widget's private data
    Value as well as its Properties and OPlugs.
    """

    def __init__(self, widget: Widget):
        self._widget: Widget = widget


########################################################################################################################

class Widget:
    """

    """
    Definition = WidgetDefinition
    Type = WidgetType
    Path = Path
    StateMachine = StateMachine
    Property = Property
    InputPlug = InputPlug
    OutputPlug = OutputPlug
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

        # store constant arguments
        self._parent: Optional[Widget] = parent  # is constant
        self._scene: Scene = scene  # is constant
        self._name: str = name  # is constant
        self._type: str = definition.type_name  # is constant

        self._children: Dict[str, Widget] = {}

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

    def get_path(self) -> Widget.Path:
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

    def get_property(self, name: str) -> Optional[Value]:
        """
        Returns the value of the Property with the given name or None if the Widget has no Property by the name.
        :param name: Name of the Property providing the Value.
        :return: The Property's Value or None if no Property by the given name exists on the Widget.
        """
        prop: Optional[Property] = self._properties.get(name)
        if prop is None:
            return None
        else:
            return prop.get_value()

    def find_widget(self, path: Widget.Path) -> Optional[Widget]:
        """
        Tries to find a Widget in the Scene from a given Path.
        :param path: Path of the Widget to find.
        :return: Handle of the Widget or None if the Path doesn't point to a Widget.
        """
        if path.is_absolute():
            return self._scene.find_widget(path)  # absolute paths start from the scene
        else:
            return self._find_widget(path, 0)  # start following the relative path starting here

    def create_child(self, type_name: str, child_name: str) -> Widget:
        """
        Creates a new child of this Widget.
        :param type_name:   Name of the Type of the Widget to create
        :param child_name:  Parent-unique name of the child Widget.
        :return:            A Handle to the created Widget.
        :raise NameError:           If the Scene does not know a Type of the given type name.
        :raise Widget.Path.Error:   If the Widget already has a child Widget with the given name.
        """
        # validate the name
        if not self._validate_name(child_name):
            raise Widget.Path.Error(f'Cannot create a Widget with an invalid name "{child_name}".\n'
                                    f'Widget names must be alphanumeric and cannot be empty.')

        # ensure the name is unique
        if child_name in self._children:
            raise Widget.Path.Error(f'Cannot create another child Widget of "{self.get_path()}" named "{child_name}"')

        # get the widget type from the scene
        widget_type: Optional[WidgetType] = self._scene._get_widget_type(type_name)
        if widget_type is None:
            raise NameError(f'No Widget Type named "{type_name}" is registered with the Scene')

        # create the child widget
        child_widget: Widget = Widget(widget_type, self, self._scene, child_name)
        self._children[child_name] = child_widget

        # return the child widget
        return child_widget

    def remove(self):
        """
        Marks this Widget as expired. It will be deleted at the end of the Event.
        """
        self._scene._expired_widgets.add(self)

    # private for Scene
    def _find_widget(self, path: Widget.Path, step: int) -> Optional[Widget]:
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

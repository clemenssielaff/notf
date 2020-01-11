from __future__ import annotations
from typing import Optional, Dict, Any, NamedTuple, List
from weakref import ref as weak_ref

from pynotf.value import Value
from pynotf.logic import Circuit

from .output_plug import OutputPlug
from .path import Path


#######################################################################################################################

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
        def __init__(self, widget: Widget):
            assert isinstance(widget, Widget)
            self._widget: Optional[weak_ref] = weak_ref(widget)

        def is_valid(self) -> bool:
            return self._widget() is not None

        def get_type(self) -> Optional[str]:
            widget: Widget = self._widget()
            if widget is not None:
                return widget.get_type()

        def get_name(self) -> Optional[str]:
            widget: Widget = self._widget()
            if widget is not None:
                return widget.get_name()

        def get_path(self) -> Optional[Path]:
            widget: Widget = self._widget()
            if widget is not None:
                return widget.get_path()

        def get_parent(self) -> Optional[Widget.Handle]:
            widget: Widget = self._widget()
            if widget is not None:
                return widget.get_parent()

    class Self:
        """
        Special kind of Widget Handle passed to Callbacks which allows the user to access the Widget's private data
        Value as well as its Properties and OPlugs.
        """

        def __init__(self, widget: Widget):
            self._widget: Widget = widget

    Path = Path

    def __init__(self, definition: Widget.Type, parent: Optional[Widget], scene: 'Scene', name: str):
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

    def get_parent(self) -> Optional[Widget.Handle]:
        """
        The parent of this Widget, is only None if this is the root Widget.
        """
        if self._parent is None:
            return None
        else:
            return Widget.Handle(self._parent)

    def find_widget(self, path: Widget.Path) -> Optional[Widget.Handle]:
        """
        Tries to find a Widget in the Scene from a given Path.
        :param path: Path of the Widget to find.
        :return: Handle of the Widget or None if the Path doesn't point to a Widget.
        """
        if path.is_absolute():
            return self._scene.find_widget(path)  # absolute paths start from the scene
        else:
            return self._find_widget(path, 0)  # start following the relative path starting here

    def create_child(self, type_name: str, child_name: str) -> Widget.Handle:
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
    def _find_widget(self, path: Widget.Path, step: int) -> Optional[Widget.Handle]:
        """
        Recursive implementation to follow a Path to a particular Widget.
        :param path: Path of the Widget to find.
        :param step: Current position in the Path.
        :return: Handle of the Widget or None if the Path doesn't point to a Widget.
        """
        # if this is the final step, we have reached the target widget
        if step == len(path):
            assert path.get_widget(len(path) - 1) == self._name
            return Widget.Handle(self)

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

    def _remove_child(self, node_handle: 'Widget.Handle'):
        """
        Removes a child of this Widget.
        :param node_handle:     Child Widget to remove.
        """
        node: Optional[Widget] = node_handle._widget()
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

    @staticmethod
    def _validate_name(name: str) -> bool:
        """
        Tests whether the given name is valid Widget name.
        :param name: Name to test.
        :return: True if the name is valid.
        """
        return name.isalnum()


#######################################################################################################################

from .scene import Scene
from .property import Property
from .input_plug import InputPlug

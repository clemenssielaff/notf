from typing import Optional, Dict, Set

from pynotf.logic import Circuit


#######################################################################################################################

class Scene:

    def __init__(self):
        """
        Default Constructor.
        """
        # Expired Widgets are kept around until the end of an Event.
        # We store Handles here to keep ownership contained to the parent Widget only.
        self._expired_widgets: Set[Widget] = set()

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

    def register_widget_type(self, type_name: str, definition: 'Widget.Definition'):
        """
        Register a new Widget Type with the Scene.
        :param type_name: Name of the new Widget Type.
        :param definition: Definition of the Type.
        """
        if type_name in self._widget_types:
            raise NameError(f'Another Widget type with the name "{type_name}" exists already in the Scene')
        else:
            self._widget_types[type_name] = Widget.Type(type_name, definition)

    def find_widget(self, path: 'Widget.Path') -> Optional['Widget']:
        """
        Find a Widget from an absolute Path.
        :param path: Absolute Path of the Widget to find.
        :return: Handle of the Widget or None if the Path doesn't point to a Widget.
        """
        if path.is_relative():
            raise Widget.Path.Error(f'Cannot query a Widget from the Scene with relative path "{str(path)}"')
        return self._root._find_widget(path, 0)

    def create_widget(self, type_name: str, name: str) -> 'Widget':
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
        expired_roots: Set[Widget] = set()
        for widget in self._expired_widgets:
            parent: Optional[Widget] = widget.get_parent()
            while parent is not None:
                if parent in self._expired_widgets:
                    break
            else:
                expired_roots.add(widget)
        self._expired_widgets.clear()

        # delete all expired nodes in the correct order
        for widget in expired_roots:
            parent: Optional[Widget] = widget.get_parent()
            assert parent is not None  # root removal is handled manually at the destruction of the Scene
            parent._remove_child(widget)
            del parent

    # private for Widget
    def _get_widget_type(self, type_name: str) -> Optional['Widget.Type']:
        """
        Should be private and visible to Widget only.
        :param type_name: Name of the Widget Type.
        :return: Requested Widget Type or None if the Scene doesn't know a Type by that name.
        """
        return self._widget_types.get(type_name)


#######################################################################################################################

from .widget import Widget

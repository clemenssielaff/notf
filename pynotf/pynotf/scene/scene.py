from typing import Optional, Dict, Set

from pynotf.logic import Circuit


#######################################################################################################################

class Scene:

    @staticmethod
    def _get_root_type():
        return Widget.Type.create(
            Widget.Definition(
                type_name='_root',
                properties={},
                input_plugs={},
                output_plugs={},
                plug_callbacks={},
                property_callbacks={},
                state_machine=StateMachine(StateMachine.Description(
                    states={'root': StateMachine.State(StateMachine.Callback())},
                    initial_state='root'
                ))
            )
        )

    def __init__(self):
        """
        Default Constructor.
        """
        # Expired Widgets are kept around until the end of an Event.
        # We store Handles here to keep ownership contained to the parent Widget only.
        self._expired_widgets: Set[Widget] = set()

        self._widget_types: Dict[str, Widget.Type] = {}

        self._circuit: Circuit = Circuit()  # is constant

        # create the root widget last to make sure all other members of the scene are initialized
        self._root: Widget = Widget(self._get_root_type(), None, self, '/')  # is constant

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

    def register_widget_type(self, definition: 'Widget.Definition'):
        """
        Register a new Widget Type with the Scene.
        :param definition: Definition of the Type.
        :raise Widget.Type.ConsistencyError: If the Definition fails to validate.
        """
        widget_type: Widget.Type = Widget.Type.create(definition)

        if widget_type.type_name in self._widget_types:
            raise NameError(f'Another Widget type with the name "{widget_type.type_name}" exists already in the Scene')
        else:
            self._widget_types[widget_type.type_name] = widget_type

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
from .statemachine import StateMachine

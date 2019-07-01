from uuid import uuid4 as uuid
from enum import Enum, unique, auto
from typing import NamedTuple, List, TypeVar, Generic, Union, Dict, Optional, Any
from .app import this_is_the_render_thread
from .reactive import Operator

T = TypeVar('T')


class Property:
    @unique
    class Visibility(Enum):
        """
        Changing this Property ...
        """
        INVISIBLE = auto()  # .. does not require any action from the renderer.
        REDRAW = auto()  # ... requires the Node to be drawn again as it is (only transformed etc.).
        REFRESH = auto()  # ... requires the Node to update and then redraw.

    class PropertyOperator(Operator):
        pass  # TODO

    def __init__(self, data: Generic[T]):
        self._data: T = data
        self._modified_data: Optional[T] = None

    @property
    def data(self):
        if this_is_the_render_thread():
            return self._data  # the renderer always sees the unmodified data
        if self._modified_data is not None:
            return self._modified_data  # return a modified value to the UI thread, if one exists
        else:
            return self._data  # if no modified copy exists, return the unmodified data

    @data.setter
    def data(self, value: T):
        pass


class Node:
    class Data(NamedTuple):
        parent: 'Node'  # mandatory
        children: List['Node'] = []
        is_enabled: bool = True
        is_visible: bool = True
        is_dirty: bool = True

    def __init__(self, graph: 'Graph', parent: 'Node'):
        self._graph: Graph = graph
        self.__data: 'Node.Data' = self.Data(parent)
        self._uuid: uuid = uuid()

    @property
    def _data(self) -> Data:
        return self.__data

    @property
    def uuid(self) -> uuid:
        return self._uuid

    @property
    def name(self) -> str:
        return self._graph.registry.get_name(self)

    @name.setter
    def name(self, value: str):
        self._graph.registry.set_name(self, value)

    def create_child(self, node_type: Generic[T]) -> T:
        if not issubclass(node_type, Node):
            raise TypeError("Nodes can only have other Nodes as children")
        child = node_type(self._graph, self)
        self._data.children.append(child)
        return child


class RootNode(Node):
    """
    The Root Node is its own parent.
    """

    def __init__(self, graph: 'Graph'):
        super().__init__(graph, self)


class Graph:
    """
    The UI Graph.
    """

    class NodeRegistry:
        def __init__(self):
            self._nodes: Dict[Union[Node, str], Union[Node, str]] = {}  # str <-> Node bimap

        def __len__(self) -> int:
            """
            Number of node/name pairs in the registry.
            """
            assert len(self._nodes) % 2 == 0
            return len(self._nodes) // 2

        def get_name(self, node: Node) -> str:
            name = self._nodes.get(node, None)
            if name is None:
                # create a default name and register the node with it
                name = str(node.uuid)
                self._nodes[name] = node
                self._nodes[node] = name
            return name

        def set_name(self, node: Node, name: str) -> str:
            # delete the old node/name pair if it exists
            old_name = self._nodes.get(node, None)
            if old_name is not None:
                assert node in self._nodes
                del self._nodes[old_name]
                del self._nodes[node]

            # make sure the name is unique
            proposal = name
            counter = 1
            while proposal in self._nodes:
                proposal = "{}{:>2}".format(name, counter)
                counter += 1

            # (re-)register the node under its new name
            self._nodes[proposal] = node
            self._nodes[node] = proposal

            # return the actual new name of the node
            return proposal

        def get_node(self, name: str) -> Optional[Node]:
            return self._nodes.get(name, None)

    def __init__(self):
        self.root: RootNode = RootNode(self)
        self.registry: Graph.NodeRegistry = Graph.NodeRegistry()

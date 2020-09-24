from __future__ import annotations

from enum import IntEnum
from logging import warning
from typing import Tuple, Optional, Dict, List, NamedTuple, Iterable, Iterator, Union

from pyrsistent import field

from pycnotf import V2f, Size2f, M3f
from pynotf.data import Value, RowHandle, Table, TableRow, RowHandleList, Bonsai, RowHandleMap, Path, Claim
import pynotf.core as core

__all__ = ('Node', 'Scene', 'NodeRow')


# DATA #################################################################################################################

# TODO: Node states carry a lot of weight right now and I think they are a crutch.
#  Instead, you should be able to to create child and dynamic operators and change the layout and claim etc. without
#  changing the state.
class NodeStateDescription(NamedTuple):
    operators: Dict[str, Tuple[core.OperatorIndex, Value]]
    connections: List[Tuple[Path, Path]]
    design: core.Design
    children: Dict[str, NodeDescription]
    layout: core.LayoutDescription
    claim: Claim = Claim()


class NodeInterops(NamedTuple):
    names: Bonsai = Bonsai()
    elements: List[core.Operator] = []


class DisplayKind(IntEnum):
    NOP = 0
    REDRAW = 1
    RELAYOUT = 2


class NodeDescription(NamedTuple):
    interops: Dict[str, Tuple[Value, DisplayKind]]
    states: Dict[str, NodeStateDescription]
    transitions: List[Tuple[str, str]]
    initial_state: str


# EMPTY_NODE_DESCRIPTION: Value = Value(
#     interops=dict(
#     )
# )

WIDGET_BUILTIN_NAMESPACE = 'widget'

# TODO: Interop means "Interface Operator" ... I even forgot that myself and had no idea what they were...
BUILTIN_NODE_INTEROPS: Dict[str, (Value, int)] = {  # interop name : (default value, is displayable)
    f'{WIDGET_BUILTIN_NAMESPACE}.opacity': (Value(1), 1),
    f'{WIDGET_BUILTIN_NAMESPACE}.visibility': (Value(1), 1),
    f'{WIDGET_BUILTIN_NAMESPACE}.depth': (Value(0), 2),
    f'{WIDGET_BUILTIN_NAMESPACE}.xform': (Value(1, 0, 0, 1, 0, 0), 1),  # used for painting
    f'{WIDGET_BUILTIN_NAMESPACE}.claim': (Claim().get_value(), 2),
}


class NodeRow(TableRow):
    __table_index__: int = core.TableIndex.NODES
    description = field(type=NodeDescription, mandatory=True)
    parent = field(type=RowHandle, mandatory=True)
    interops = field(type=NodeInterops, mandatory=True, initial=NodeInterops())
    layout = field(type=RowHandle, mandatory=True, initial=RowHandle())
    state = field(type=str, mandatory=True, initial='')
    child_names = field(type=RowHandleMap, mandatory=True, initial=RowHandleMap())
    network = field(type=RowHandleList, mandatory=True, initial=RowHandleList())


# UTILITIES ############################################################################################################

def create_node_state_from_value(description: Value) -> NodeStateDescription:
    if description.get_schema() != Node.STATE.get_schema():
        raise ValueError(f'Cannot build a Node State from an incompatible Value Schema: {description.get_schema()!s}.\n'
                         f'A valid Node State Value has the Schema: {Node.STATE.get_schema()!s}')
    return NodeStateDescription(
        operators={str(op[0]): (int(op[1]), op[2]) for op in description['operators']},
        connections=[(Path(str(path[0])), Path(str(path[1]))) for path in description['connections']],
        design=core.Design.from_value(description['design']),
        children={str(child[0]): create_node_description_from_value(child[1]) for child in description['children']},
        layout=core.LayoutDescription(
            type=int(description['layout'][0]),
            args=description['layout'][1],
        ),
        claim=Claim.from_value(description['claim']),
    )


def create_node_description_from_value(description: Value) -> NodeDescription:
    if description.get_schema() != Node.VALUE.get_schema():
        raise ValueError(f'Cannot build a Node from an incompatible Value Schema: {description.get_schema()!s}.\n'
                         f'A valid Node Value has the Schema: {Node.VALUE.get_schema()!s}')

    node_description: NodeDescription = NodeDescription(
        interops={str(op[0]): (op[1], DisplayKind(int(op[2]))) for op in description['interops']},
        states={str(state[0]): create_node_state_from_value(state[1]) for state in description['states']},
        transitions=[(str(trans[0]), str(trans[1])) for trans in description['transitions']],
        initial_state=str(description['initial']),
    )

    # ensure none of the interops use the the built in widget namespace
    for operator_name in node_description.interops:
        assert not operator_name.startswith(f'{WIDGET_BUILTIN_NAMESPACE}.')

    # add built-in interops
    node_description.interops.update(BUILTIN_NODE_INTEROPS)

    return node_description


# SCENE ################################################################################################################

class Scene:

    def __init__(self):
        self._root_node: Optional[Node] = None
        self._hitboxes: List[core.Sketch.Hitbox] = []
        self._size: Size2f = Size2f()

        # Transitions cannot happen right away, while and event is still being handled.
        # Instead, they are collected here and executed at the end of an event
        self._transitions: List[Tuple[Node, str]] = []

    def initialize(self, root_description_value: Value):
        self._root_node = Node.create_node(create_node_description_from_value(root_description_value))
        self._root_node.initialize()  # initialize AFTER assigning it to `self._root_node` for absolute path lookup
        self.perform_node_transitions()

    def set_size(self, size: Size2f):
        if size != self._size:
            self._size = size
            self._root_node.relayout_down(self._size)

    def get_size(self) -> Size2f:
        return self._size

    def clear(self):
        if self._root_node:
            self._root_node.remove()

    def create_node(self, name: str, description: Value) -> Node:
        return self._root_node.create_child(name, create_node_description_from_value(description))

    # TODO: facts should not be "gotten" from the Scene, instead they are only managed by Services
    #   this used to return a Fact, but the root node interops that we were using as "Facts" should really be part of
    #   the "os.input" service
    def get_fact(self, name: str, ) -> Optional[core.Operator]:
        return self._root_node.get_interop(name)

    def get_node(self, path: Union[Path, str]) -> Node:
        """
        Finds and returns a Node in the Scene using an absolute Path.
        :param path: Absolute Path (or one that is relative to the root).
        :returns: The requested Node.
        :raise: Path.Error if the Path does not identify a Node.
        """
        return self._root_node.get_node(path)

    def paint(self, painter: core.Painter) -> None:
        self._root_node.paint(painter)
        self._hitboxes = painter.get_hitboxes()

    def iter_hitboxes(self, pos: V2f) -> Iterable[core.Sketch.Hitbox]:
        """
        Returns the Operators connected to all hitboxes that overlap the given position in reverse draw order.
        """
        for hitbox in self._hitboxes:
            if hitbox.shape.contains(pos):
                yield hitbox

    def schedule_node_transition(self, node: Node, target_state: str) -> None:
        self._transitions.append((node, target_state))

    def perform_node_transitions(self) -> None:
        for node, target_state in self._transitions:
            node._transition_into(target_state)
        self._transitions.clear()


# NODE ################################################################################################################

class Node:
    VALUE: Value = Value(
        interops=[("name", Value(), 0)],  # name, initial value, display flag
        states=[("name", Value())],  # name, node state
        transitions=[('from', 'to')],
        initial="initial",
    )

    STATE: Value = Value(
        operators=[('name', 0, Value())],  # path, operator index, args (any)
        connections=[('from', 'to')],  # path, path
        design=Value(),  # Design
        children=[('name', Value())],  # node description
        layout=(0, Value()),  # layout index, args
        claim=dict(
            horizontal=dict(preferred=0, min=0, max=0, scale_factor=0, priority=0),
            vertical=dict(preferred=0, min=0, max=0, scale_factor=0, priority=0),
        )
    )

    def __init__(self, handle: RowHandle):
        assert handle.is_null() or handle.table == core.TableIndex.NODES
        self._handle: RowHandle = handle

    @staticmethod
    def create_node(description: NodeDescription, parent: Optional[Node] = None, name: str = "") -> Node:
        node_table: Table = core.get_app().get_table(core.TableIndex.NODES)
        parent_handle: RowHandle = RowHandle() if parent is None else parent.get_handle()

        # ensure the node name is unique among its siblings
        if parent_handle:
            assert name and name not in node_table[parent.get_handle()]['child_names']

        # create the new node row
        node_handle: RowHandle = node_table.add_row(
            description=description,
            parent=parent_handle,
        )

        # create and attach the interops once the node row has been established
        node_table[node_handle]['interops'] = NodeInterops(
            names=Bonsai([interop_name for interop_name in description.interops.keys()]),
            elements=[core.Operator.create(
                description=core.OpRelay.create(interop_description[0]),
                node=node_handle,
                effect=(core.Operator.Effect.REDRAW if interop_description[1] == 1 else
                        (core.Operator.Effect.REDRAW if interop_description[1] == 2 else core.Operator.Effect.NONE)),
                name=interop_name,
            ) for interop_name, interop_description in description.interops.items()]
        )

        # register the node with its parent
        if parent_handle:
            node_table[parent_handle]['child_names'] = node_table[parent_handle]['child_names'].set(name, node_handle)

        return Node(node_handle)

    def initialize(self) -> None:
        """
        Initialize a new node by transitioning into its initial_state state.
        """
        node_table: Table = core.get_app().get_table(core.TableIndex.NODES)
        assert not node_table[self._handle]['state']
        self.transition_into(node_table[self._handle]['description'].initial_state)

    def get_handle(self) -> RowHandle:
        return self._handle

    def is_valid(self) -> bool:
        return core.get_app().get_table(core.TableIndex.NODES).is_handle_valid(self._handle)

    # TODO: it might be faster to just store the name inside the node as well..
    def get_name(self) -> str:
        node_table: Table = core.get_app().get_table(core.TableIndex.NODES)
        parent_handle: RowHandle = node_table[self._handle]['parent']
        if parent_handle.is_null():
            return '/'
        for child_name, child_handle in node_table[parent_handle]['child_names'].items():
            if child_handle == self._handle:
                return child_name
        assert False

    def get_parent(self) -> Optional[Node]:
        parent_handle: RowHandle = core.get_app().get_table(core.TableIndex.NODES)[self._handle]['parent']
        if not parent_handle:
            return None
        return Node(parent_handle)

    def get_interop(self, name: str) -> Optional[core.Operator]:
        interops: NodeInterops = core.get_app().get_table(core.TableIndex.NODES)[self._handle]['interops']
        index: Optional[int] = interops.names.get(name)
        if index is None:
            return None
        return interops.elements[index]

    def get_layout(self) -> core.Layout:
        return core.Layout(core.get_app().get_table(core.TableIndex.NODES)[self._handle]['layout'])

    # TODO: this name is weird: Composition is not just one thing but the composition of multiple
    #  the way I use it here, to describe the placement of a singular node within a larger scheme is not right
    def get_composition(self) -> core.NodeComposition:
        parent: Optional[Node] = self.get_parent()
        if not parent:
            return core.NodeComposition(M3f.identity(), core.get_app().get_scene().get_size(), 1)  # root composition

        node_composition: Optional[core.NodeComposition] = parent.get_layout().get_node_composition(self)
        assert node_composition
        return node_composition

    def get_state(self) -> str:
        return core.get_app().get_table(core.TableIndex.NODES)[self._handle]['state']

    def get_sketch(self) -> core.Sketch:
        node_table: Table = core.get_app().get_table(core.TableIndex.NODES)
        design: core.Design = node_table[self._handle]['description'].states[self.get_state()].design
        return design.sketch(self)

    def get_path(self) -> Path:
        """
        Returns the absolute Path to this Node.
        """
        names: List[str] = [self.get_name()]
        next_node: Optional[Node] = self.get_parent()
        while next_node:
            names.append(next_node.get_name())
            next_node = next_node.get_parent()
        return Path.assemble(reversed(names))

    def get_child(self, name: str) -> Optional[Node]:
        child_handle: Optional[RowHandle] = \
            core.get_app().get_table(core.TableIndex.NODES)[self._handle]['child_names'].get(name)
        if child_handle is None:
            return None
        return Node(child_handle)

    def get_node(self, path: Union[Path, str]) -> Node:
        """
        Finds and returns a Node in the Scene by Path.
        :param path: Absolute or relative Path to a Node.
        :returns: The requested Node.
        :raise: Path.Error if the Path does not identify a Node.
        """
        if isinstance(path, str):  # convenience conversion
            path = Path(path)
        assert isinstance(path, Path)

        # defer absolute paths to the root node
        if path.is_absolute() and self.get_parent() is not None:
            return core.get_app().get_scene().get_node(path)

        # service paths are always an error
        if path.is_service_path():
            raise Path.Error(f'Cannot use Service Path "{path}" to identify a Node')

        # step through the path
        path_iterator: Iterator[str] = path.get_iterator()
        node: Node = self
        next_step: Optional[str] = next(path_iterator, None)
        while next_step:

            # next step is up
            if next_step == Path.STEP_UP:
                parent: Optional[Node] = node.get_parent()
                if parent is None:
                    raise Path.Error(f'Cannot follow Path "{path}" up past the root (from "{self.get_path()}")')
                node = parent

            # next step is down
            else:
                next_child: Optional[Node] = node.get_child(next_step)
                if next_child is None:
                    raise Path.Error(f'Node "{str(node.get_path())}" has no child named "{next_step}"')
                node = next_child

            # advance the path iterator
            next_step = next(path_iterator, None)

        # success
        return node

    def set_claim(self, claim: Claim) -> None:
        # TODO: relayout upwards
        if not self.get_parent():
            return
        self.get_interop('widget.claim').update(claim.get_value())

    def paint(self, painter: core.Painter):
        painter.paint(self)
        for node_handle in self.get_layout().get_composition().order:
            painter.paint(Node(node_handle))

    def create_child(self, name: str, description: NodeDescription) -> Node:
        child_node: Node = self.create_node(description, self, name)
        child_node.initialize()
        return child_node

    def remove(self):
        # unregister from the parent
        node_table: Table = core.get_app().get_table(core.TableIndex.NODES)
        parent_handle: RowHandle = node_table[self._handle]['parent']
        if parent_handle:
            child_names: RowHandleMap = node_table[parent_handle]['child_names']
            node_table[parent_handle]['child_names'] = child_names.remove(self.get_name())

        # remove all children first and then yourself
        self._remove_recursively()

    def transition_into(self, state: str) -> None:
        core.get_app().get_scene().schedule_node_transition(self, state)

    def _transition_into(self, state: str) -> None:
        # ensure the transition is allowed
        node_table: Table = core.get_app().get_table(core.TableIndex.NODES)
        current_state: str = node_table[self._handle]['state']
        node_description: NodeDescription = node_table[self._handle]['description']
        assert current_state == '' or (current_state, state) in node_description.transitions

        # remove the dynamic dependencies from the last state
        # TODO: remove children, or think of a better way to transition between states
        self._clear_dependencies()

        # enter the new state
        node_table[self._handle]['state'] = state
        new_state: NodeStateDescription = node_description.states[state]

        # initialize the design
        new_state.design.initialize(self)
        # TODO: I am not yet happy with Design initialization
        #   Ideally, we'd want to initialize _all_ nodes (especially Value Nodes like Expression and Interop) right 
        #   when the node is created. But at least for expressions, that means that the expression needs to be 
        #   evaluated, which only works if the scope is initialized, with only works if the grant is available, which
        #   only works if the parent is fully initialized including all of its children, so it can layout them.
        #   The current approach is one that branches on the evaluation of a Design node, if it is the first time it
        #   behaves differently from the res. This is not a _big_ problem, I think branch prediction should pick up
        #   on that pattern pretty quickly, but it smells.

        # create new child nodes
        child_nodes: List[Node] = []
        for name, child_description in new_state.children.items():
            child_nodes.append(self.create_child(name, child_description))

        # create the new layout
        layout: core.Layout = core.Layout.create(new_state.layout.type, new_state.layout.args)
        for child_node in child_nodes:
            layout.add_node(child_node)
        node_table[self._handle]['layout'] = layout.get_handle()

        # create new operators
        network: Dict[str, RowHandle] = {}
        for name, (kind, args) in new_state.operators.items():
            assert name not in network
            network[name] = core.Operator.create(
                description=core.OPERATOR_VTABLE[kind][core.OperatorVtableIndex.CREATE](args),
                node=self._handle,
                name=name,
            ).get_handle()
        node_table[self._handle]['network'] = RowHandleList(network.values())

        # TODO: there is no way to get internal operators by name outside of this method

        def find_operator(path: Path) -> core.Operator:
            """
            Finds an Operator to connect to or from.
            Accepts a Path as an input with one quirk: simple Paths are taken to identify an internal Operator.
            Usually, such a Path would identify a child Node, but since we are looking only for Operators, this should
            be unambiguous.
            """
            result: Optional[core.Operator] = None
            if path.is_simple():  # simple paths identify a dynamic operator in this context
                result = core.Operator(network.get(str(path)))
            elif path.is_absolute():
                result = core.get_app().get_operator(path)
            else:
                interop_name: Optional[str] = path.get_interop_name()
                if interop_name is None:
                    raise RuntimeError(f'Connections must be made between Operators, but "{path}" identifies a Node')
                node: Optional[core.Node] = self.get_node(path)
                if node is not None:
                    result = node.get_interop(interop_name)

            if result is None:
                raise NameError(f'Could not find Operator "{self.get_path() + path}"')
            return result

        # create new connections
        for source, sink in new_state.connections:
            find_operator(source).subscribe(find_operator(sink))

        # update the claim
        self.set_claim(new_state.claim)  # causes a potential re-layout

    def relayout_down(self, grant: Size2f) -> None:
        # update the layout with the new grant
        layout: core.Layout = self.get_layout()
        old_layout: core.LayoutComposition = layout.get_composition()
        new_layout: core.LayoutComposition = layout.perform(grant)

        # update all child nodes
        for child_name, child_layout in new_layout.nodes.items():
            assert isinstance(child_name, str)
            assert isinstance(child_layout, core.NodeComposition)
            old_node_layout: Optional[core.NodeComposition] = old_layout.nodes.get(child_name)
            if old_node_layout and old_node_layout.grant == child_layout.grant:
                continue  # no change in grant
            child_node: Optional[Node] = self.get_child(child_name)
            if not child_node:
                warning(f'core.Layout contained unknown child node "{child_name}"')
                continue
            child_node.relayout_down(grant)

    def _remove_recursively(self):
        """
        Unlike `remove`, this does not remove this Node from its parent as we know that the parent is also in the
        process of removing itself from the Scene.
        """
        # remove all children
        node_table: Table = core.get_app().get_table(core.TableIndex.NODES)
        for child_handle in node_table[self._handle]['child_names'].values():
            Node(child_handle)._remove_recursively()

        # remove dynamic network
        self._clear_dependencies()

        # remove interops
        interops: NodeInterops = node_table[self._handle]['interops']
        for operator in interops.elements:
            operator.remove()

        # remove the node
        node_table.remove_row(self._handle)

    def _clear_dependencies(self) -> None:
        node_table: Table = core.get_app().get_table(core.TableIndex.NODES)

        # network
        for operator in node_table[self._handle]['network']:
            core.Operator(operator).remove()
        node_table[self._handle]['network'] = RowHandleList()

        # layout
        self.get_layout().remove()

from __future__ import annotations

from logging import warning
from typing import Tuple, Optional, Dict, List, NamedTuple, Iterable, Iterator

from pyrsistent import field

from pycnotf import V2f, Size2f
from pynotf.data import Value, RowHandle, Table, TableRow, RowHandleList, Bonsai, RowHandleMap, Path, Claim
import pynotf.core as core


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
    names: Bonsai
    elements: List[core.Operator]


class NodeDescription(NamedTuple):
    interface: Dict[str, Value]
    states: Dict[str, NodeStateDescription]
    transitions: List[Tuple[str, str]]
    initial_state: str


# EMPTY_NODE_DESCRIPTION: Value = Value(
#     interface=dict(
#     )
# )

WIDGET_BUILTIN_NAMESPACE = 'widget'

BUILTIN_NODE_INTEROPS: Dict[str, Value] = {
    f'{WIDGET_BUILTIN_NAMESPACE}.opacity': Value(1),
    f'{WIDGET_BUILTIN_NAMESPACE}.visibility': Value(1),
    f'{WIDGET_BUILTIN_NAMESPACE}.depth': Value(0),
    f'{WIDGET_BUILTIN_NAMESPACE}.xform': Value(1, 0, 0, 1, 0, 0),
    f'{WIDGET_BUILTIN_NAMESPACE}.claim': Claim().get_value(),
}


class NodeRow(TableRow):
    __table_index__: int = core.TableIndex.NODES
    description = field(type=NodeDescription, mandatory=True)
    parent = field(type=RowHandle, mandatory=True)
    interface = field(type=NodeInterops, mandatory=True)
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
        interface={str(op[0]): op[1] for op in description['interface']},
        states={str(state[0]): create_node_state_from_value(state[1]) for state in description['states']},
        transitions=[(str(trans[0]), str(trans[1])) for trans in description['transitions']],
        initial_state=str(description['initial']),
    )

    # ensure none of the interface use the the built in widget namespace
    for operator_name in node_description.interface:
        assert not operator_name.startswith(f'{WIDGET_BUILTIN_NAMESPACE}.')

    # add built-in interface operators
    node_description.interface.update(BUILTIN_NODE_INTEROPS)

    return node_description


# SCENE ################################################################################################################

class Scene:

    def __init__(self):
        self._root_node: Optional[Node] = None
        self._hitboxes: List[core.Sketch.Hitbox] = []
        self._size: Size2f = Size2f()

    def initialize(self, root_description_value: Value):
        # create the root node
        root_description: NodeDescription = create_node_description_from_value(root_description_value)
        self._root_node = Node(core.get_app().get_table(core.TableIndex.NODES).add_row(
            description=root_description,
            parent=RowHandle(),  # empty row handle as parent
            interface=NodeInterops(
                names=Bonsai([interop_name for interop_name in root_description.interface.keys()]),
                elements=[core.Operator.create(core.OpRelay.create(value)) for value in
                          root_description.interface.values()],
            )
        ))
        self._root_node.transition_into(root_description.initial_state)

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
        return self._root_node.create_child(name, description)

    def get_fact(self, name: str, ) -> Optional[core.Fact]:
        fact_handle: Optional[core.Operator] = self._root_node.get_interop(name)
        if fact_handle:
            return core.Fact(fact_handle)
        else:
            return None

    def get_node(self, path: Path) -> Optional[Node]:
        return self._root_node.get_descendant(path)

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


# NODE ################################################################################################################

class Node:
    VALUE: Value = Value(
        interface=[("name", Value())],  # any value
        states=[("name", Value())],  # node state
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

    def get_handle(self) -> RowHandle:
        return self._handle

    def is_valid(self) -> bool:
        return core.get_app().get_table(core.TableIndex.NODES).is_handle_valid(self._handle)

    def get_parent(self) -> Optional[Node]:
        parent_handle: RowHandle = core.get_app().get_table(core.TableIndex.NODES)[self._handle]['parent']
        if not parent_handle:
            return None
        return Node(parent_handle)

    def get_layout(self) -> core.Layout:
        return core.Layout(core.get_app().get_table(core.TableIndex.NODES)[self._handle]['layout'])

    def get_composition(self) -> core.NodeComposition:
        parent: Optional[Node] = self.get_parent()
        if not parent:
            return core.NodeComposition(core.Xform(), core.get_app().get_scene().get_size(), 1)  # root composition

        node_composition: Optional[Optional[core.NodeComposition]] = parent.get_layout().get_node_composition(self)
        assert node_composition
        return node_composition

    def create_child(self, name: str, description_value: Value) -> Node:
        return self._create_child(name, create_node_description_from_value(description_value))

    def _create_child(self, name: str, description: NodeDescription) -> Node:
        # ensure the child name is unique
        node_table: Table = core.get_app().get_table(core.TableIndex.NODES)
        assert name not in node_table[self._handle]['child_names']

        # create the child node entry
        child_handle: RowHandle = node_table.add_row(
            description=description,
            parent=self._handle,
            interface=NodeInterops(
                names=Bonsai([interop_name for interop_name in description.interface.keys()]),
                elements=[core.Operator.create(core.OpRelay.create(initial_value)) for
                          initial_value in description.interface.values()],
            )
        )
        node_table[self._handle]['child_names'] = node_table[self._handle]['child_names'].set(name, child_handle)

        # initialize the child node by transitioning into its initial_state state
        child_node: Node = Node(child_handle)
        child_node.transition_into(description.initial_state)

        return child_node

    # TODO: it might be faster to just store the name inside the node as well..
    def get_name(self) -> str:
        node_table: Table = core.get_app().get_table(core.TableIndex.NODES)
        parent_handle: RowHandle = node_table[self._handle]['parent']
        if parent_handle.is_null():
            return '/'
        child_names: RowHandleMap = node_table[parent_handle]['child_names']
        for index, (child_name, child_handle) in enumerate(child_names.items()):
            if child_handle == self._handle:
                return child_name
        assert False

    def get_path(self) -> Path:
        names: List[str] = [self.get_name()]
        next_node: Optional[Node] = self.get_parent()
        while next_node:
            names.append(next_node.get_name())
            next_node = next_node.get_parent()
        return Path._create(f'/{"/".join(reversed(names))}')

    def get_child(self, name: str) -> Optional[Node]:
        child_handle: Optional[RowHandle] = core.get_app().get_table(core.TableIndex.NODES)[self._handle][
            'child_names'].get(name)
        if child_handle is None:
            return None
        return Node(child_handle)

    def get_descendant(self, path: Path) -> Optional[Node]:
        path_iterator: Iterator[str] = path.get_iterator()
        node: Node = self
        next_step: Optional[str] = next(path_iterator, None)
        while next_step:

            # next step is up
            if next_step == Path.STEP_UP:
                parent: Optional[Node] = node.get_parent()
                if parent is None:
                    raise Path.Error(f'Node "{str(self.get_path())}" has no parent')
                node = parent

            # next step is down
            else:
                next_child: Optional[Node] = node.get_child(next_step)
                if next_child is None:
                    raise Path.Error(f'Node "{str(self.get_path())}" has no child')
                node = next_child

            # advance the path iterator
            next_step = next(path_iterator, None)

        # success
        return node

    def paint(self, painter: core.Painter):
        painter.paint(self)
        for node_handle in self.get_layout().get_composition().order:
            painter.paint(Node(node_handle))

    def get_interop(self, name: str) -> Optional[core.Operator]:
        interface: NodeInterops = core.get_app().get_table(core.TableIndex.NODES)[self._handle]['interface']
        index: Optional[int] = interface.names.get(name)
        if index is None:
            return None
        return interface.elements[index]

    def get_state(self) -> str:
        return core.get_app().get_table(core.TableIndex.NODES)[self._handle]['state']

    def get_sketch(self) -> core.Sketch:
        node_table: Table = core.get_app().get_table(core.TableIndex.NODES)
        design: core.Design = node_table[self._handle]['description'].states[self.get_state()].design
        return design.sketch(self)

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
        #   behaves differently from the rest. This is not a _big_ problem, I think branch prediction should pick up
        #   on that pattern pretty quickly, but it smells.

        # create new child nodes
        child_nodes: List[Node] = []
        for name, child_description in new_state.children.items():
            child_nodes.append(self._create_child(name, child_description))

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
                core.OPERATOR_VTABLE[kind][core.OperatorVtableIndex.CREATE](args)).get_handle()
        node_table[self._handle]['network'] = RowHandleList(network.values())

        def find_operator(path: Path) -> Optional[core.Operator]:
            if path.is_node_path():
                # TODO: interpreting single-name relative node paths as dynamic operators is not good
                assert path.is_relative() and len(path) == 1  # interpret single name paths as dynamic operator
                return core.Operator(network.get(str(path)))

            else:
                assert path.is_interop_path()
                node: Optional[Node]
                if path.is_absolute():
                    node = core.get_app().get_scene().get_node(path)
                else:
                    assert path.is_relative()
                    node = self.get_descendant(path)
                if node:
                    return node.get_interop(path.get_interop_name())

            return None

        # create new connections
        for source, sink in new_state.connections:
            find_operator(source).subscribe(find_operator(sink))

        # update the claim
        self.set_claim(new_state.claim)  # causes a potential re-layout

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

        # remove interface operators
        interface: NodeInterops = node_table[self._handle]['interface']
        for operator in interface.elements:
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

    def set_claim(self, claim: Claim) -> None:
        # TODO: relayout upwards
        if not self.get_parent():
            return
        self.get_interop('widget.claim').set_value(claim.get_value())
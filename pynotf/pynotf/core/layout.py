from __future__ import annotations

from enum import IntEnum, auto, unique
from typing import Tuple, Callable, Optional, Dict, NamedTuple, Iterable

from pyrsistent._checked_types import CheckedPMap as ConstMap
from pyrsistent import field

from pycnotf import Size2f, Aabrf, M3f
from pynotf.data import Value, RowHandle, Table, TableRow, RowHandleList, Claim
import pynotf.core as core

__all__ = ('Layout', 'LayoutRow', 'LayoutDescription', 'NodeComposition', 'LayoutComposition', 'LayoutIndex')


# DATA #################################################################################################################

class LayoutDescription(NamedTuple):
    type: int
    args: Value


class NodeComposition(NamedTuple):
    xform: M3f  # layout xform
    grant: Size2f  # grant size
    opacity: float = 0


class NodeCompositions(ConstMap):
    __key_type__ = str
    __value_type__ = NodeComposition


class LayoutComposition(NamedTuple):
    nodes: NodeCompositions = NodeCompositions()  # node name -> composition
    order: RowHandleList = RowHandleList()  # all nodes in draw order
    aabr: Aabrf = Aabrf()  # union of all child aabrs
    claim: Claim = Claim()  # union of all child claims


class LayoutRow(TableRow):
    __table_index__: int = core.TableIndex.LAYOUTS
    layout_index = field(type=int, mandatory=True)
    args = field(type=Value, mandatory=True)  # must be a named record
    node_value_initial = field(type=Value, mandatory=True)
    nodes = field(type=RowHandleList, mandatory=True, initial=RowHandleList())
    node_values = field(type=core.ValueList, mandatory=True, initial=core.ValueList())
    composition = field(type=LayoutComposition, mandatory=True, initial=LayoutComposition())


# LAYOUT ###############################################################################################################

class Layout:

    def __init__(self, handle: RowHandle):
        assert handle.is_null() or handle.table == core.TableIndex.LAYOUTS
        self._handle: RowHandle = handle

    @staticmethod
    def create(layout_index: int, args: Value) -> Layout:
        return Layout(LAYOUT_VTABLE[layout_index][LayoutVtableIndex.CREATE](args))

    def get_handle(self) -> RowHandle:
        return self._handle

    def is_valid(self) -> bool:
        return core.get_app().get_table(core.TableIndex.LAYOUTS).is_handle_valid(self._handle)

    def get_argument(self, name) -> Value:
        """
        :param name: Name of the argument to access.
        :return: The requested argument.
        :raise: KeyError
        """
        return core.get_app().get_table(core.TableIndex.LAYOUTS)[self._handle]["args"][name]

    def clear(self) -> None:
        core.get_app().get_table(core.TableIndex.LAYOUTS)[self._handle]['nodes'] = RowHandleList()

    def add_node(self, node: core.Node) -> None:
        layout_table: Table = core.get_app().get_table(core.TableIndex.LAYOUTS)
        current_nodes: RowHandleList = layout_table[self._handle]['nodes']
        if node in current_nodes:
            current_nodes = current_nodes.remove(node)  # re-add at the end
        layout_table[self._handle]['nodes'] = current_nodes.append(node.get_handle())

    def get_composition(self) -> LayoutComposition:
        return core.get_app().get_table(core.TableIndex.LAYOUTS)[self._handle]['composition']

    def get_node_composition(self, node: core.Node) -> Optional[NodeComposition]:
        return self.get_composition().nodes.get(node.get_name())

    def perform(self, grant: Size2f) -> LayoutComposition:
        layout_table: Table = core.get_app().get_table(core.TableIndex.LAYOUTS)
        layout_index: LayoutIndex = layout_table[self._handle]['layout_index']
        composition: LayoutComposition = LAYOUT_VTABLE[layout_index][LayoutVtableIndex.LAYOUT](self, grant)
        layout_table[self._handle]['composition'] = composition
        return composition

    def get_nodes(self) -> Iterable[LayoutNodeView]:
        """
        Returns a core.Node View for each core.Node in this layout, in the order that they were added.
        """
        node_list: RowHandleList = core.get_app().get_table(core.TableIndex.LAYOUTS)[self._handle]['nodes']
        return (LayoutNodeView(core.Node(handle)) for handle in node_list)

    def get_order(self) -> RowHandleList:
        """
        Returns the handles of all core.Nodes in this Layout in draw-order.
        """
        node_list: RowHandleList = core.get_app().get_table(core.TableIndex.LAYOUTS)[self._handle]['nodes']
        return RowHandleList(
            sorted(node_list, key=lambda h: float(core.Node(h).get_interop('widget.depth').get_value())))

    def remove(self) -> None:
        if self.is_valid():
            core.get_app().get_table(core.TableIndex.LAYOUTS).remove_row(self._handle)


class LayoutNodeView:
    def __init__(self, node: core.Node):
        self._node: core.Node = node

    @property
    def name(self) -> str:
        return self._node.get_name()

    @property
    def opacity(self) -> float:
        return float(self._node.get_interop('widget.opacity').get_value())

    @property
    def claim(self) -> Claim:
        return Claim(self._node.get_interop('widget.claim').get_value())


# LAYOUT REGISTRY ######################################################################################################

class LtOverlayout:
    @staticmethod
    def create(args: Value) -> RowHandle:
        return core.get_app().get_table(core.TableIndex.LAYOUTS).add_row(
            layout_index=LayoutIndex.OVERLAY,
            args=args,
            node_value_initial=Value(),
        )

    @staticmethod
    def layout(self: Layout, grant: Size2f) -> LayoutComposition:
        compositions: Dict[str, NodeComposition] = {}
        for node_view in self.get_nodes():
            node_grant: Size2f = Size2f(
                width=max(node_view.claim.horizontal.min, min(node_view.claim.horizontal.max, grant.width)),
                height=max(node_view.claim.vertical.min, min(node_view.claim.vertical.max, grant.height)),
            )
            assert node_view.name not in compositions
            compositions[node_view.name] = NodeComposition(
                xform=M3f.identity(),
                grant=node_grant,
                opacity=node_view.opacity,
            )
        return LayoutComposition(
            nodes=NodeCompositions(compositions),
            order=self.get_order(),
            aabr=Aabrf(grant),
            claim=Claim(),
        )


@unique
class LayoutIndex(core.IndexEnum):
    OVERLAY = auto()
    FLEXBOX = auto()


@unique
class LayoutVtableIndex(IntEnum):
    CREATE = 0
    LAYOUT = 1

from .layouts import LtFlexbox

LAYOUT_VTABLE: Tuple[
    Tuple[
        Optional[Callable[[Value], RowHandle]],  # create
        Optional[Callable[[Layout, Size2f], LayoutComposition]],  # layout
    ], ...] = (
    (LtOverlayout.create, LtOverlayout.layout),
    (LtFlexbox.create, LtFlexbox.layout),
)

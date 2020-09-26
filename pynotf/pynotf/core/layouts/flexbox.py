from __future__ import annotations

from typing import Dict, NamedTuple, Optional, TypeVar, List
from enum import Enum, auto, unique

from pycnotf import Size2f, Aabrf
from pynotf.data import Value, RowHandle, Claim
import pynotf.core as core

from pynotf.core.layout import Layout, LayoutComposition, NodeComposition, NodeCompositions, LayoutIndex

__all__ = ('LtFlexbox',)

T = TypeVar('T')


# HELPER STRUCTS #######################################################################################################

# TODO: there is a C++ class for Padding
class Padding:
    __slots__ = ('left', 'top', 'right', 'bottom')

    def __init__(self, left: float, top: float, right: float, bottom: float):
        self.left: float = left
        self.top: float = top
        self.right: float = right
        self.bottom: float = bottom


class FlexSize(NamedTuple):
    """
    Helper struct denoting two sizes, but since the layout can be rotated either way we use main and cross instead of
    width and height.
    """
    main: float
    cross: float


class LayoutAdapter:
    """
    Helper struct used to abstract away whether we are laying out individual Nodes or stacks.
    """
    __slots__ = ('maximum', 'preferred', 'scaling', 'result')

    def __init__(self, maximum: float, preferred: float, scaling: float, result: float):
        self.maximum: float = maximum
        self.preferred: float = preferred
        self.scaling: float = scaling
        self.result: float = result

    def __repr__(self) -> str:
        return f'LayoutAdapter(preferred={self.preferred}, maximum={self.maximum}, ' \
               f'scaling={self.scaling}, result={self.result})'


@unique
class Direction(Enum):
    """
    Direction in which items in a Layout can be stacked.
    """
    LEFT_TO_RIGHT = auto()
    TOP_TO_BOTTOM = auto()
    RIGHT_TO_LEFT = auto()
    BOTTOM_TO_TOP = auto()


@unique
class Alignment(Enum):
    """
    Alignment of items in a Layout along the main and cross axis.
    """
    START = auto()  # items stacked from the start, no additional spacing
    END = auto()  # items stacked from the end, no additional spacing
    CENTER = auto()  # items centered, no additional spacing
    SPACE_BETWEEN = auto()  # equal spacing between items, no spacing between items and border
    SPACE_EQUAL = auto()  # equal spacing between the items and the border
    SPACE_AROUND = auto()  # single spacing between items and border, double spacing between items


@unique
class Wrap(Enum):
    """
    How a Layout wraps.
    """
    NO_WRAP = auto()  # no wrap
    WRAP = auto()  # wraps towards the lower-right corner
    REVERSE = auto()  # wraps towards the upper-left corner


# FLEXBOX ##############################################################################################################

class LtFlexbox:
    """
    The FlexLayout class offers a way to arrange items in one or multiple rows or columns.
    It behaves similar to the CSS Flex Box Layout.

    Claim
    =====
    The FlexLayout calculates its implicit Claim differently, depending on whether it is wrapping or not.
    A non-wrapping FlexLayout will simply add all of its child Claim's either vertically or horizontally.
    A wrapping FlexLayout on the other hand, will *always* return the default Claim (0 min/preferred, inf max).
    This is because a wrapping FlexLayout will most likely be used to arrange elements within an explicitly defined
    area that is not determined its child items themselves.
    By using the default Claim, it allows its parent to determine its size explicitly and arrange its children within
    the allotted size.
    """

    @staticmethod
    def create(args: Value) -> RowHandle:
        return core.get_app().get_table(core.TableIndex.LAYOUTS).add_row(
            layout_index=LayoutIndex.FLEXBOX,
            args=args,
        )

    @classmethod
    def layout(cls, self: Layout, grant: Size2f) -> LayoutComposition:
        return FlexboxImpl(self, grant)()
        # for node_view in self.get_nodes():
        #     node_grant: Size2f = Size2f(
        #         width=max(node_view.claim.horizontal.min, min(node_view.claim.horizontal.max, grant.width)),
        #         height=max(node_view.claim.vertical.min, min(node_view.claim.vertical.max, grant.height)),
        #     )
        #     assert node_view.name not in compositions
        #     compositions[node_view.name] = NodeComposition(
        #         xform=M3f(e=x_offset, f=10),
        #         grant=node_grant,
        #         opacity=node_view.opacity,
        #     )
        #     x_offset += node_grant.width + float(self.get_argument('margin'))


class FlexboxImpl:
    def __init__(self, layout: Layout, grant: Size2f):
        self._layout: Layout = layout
        self._grant: Size2f = grant
        self._compositions: Dict[str, NodeComposition] = {}
        self._claim: Claim = Claim()

        # Direction in which the FlexLayout is stacked.
        self._direction: Direction = Direction.LEFT_TO_RIGHT
        direction_arg: Optional[Value] = self._layout.get_argument("direction")
        if direction_arg is not None:
            self._direction = Direction(int(direction_arg))

        # Alignment of items in the main direction.
        self._main_alignment: Alignment = Alignment.START
        main_alignment_arg: Optional[Value] = self._layout.get_argument("alignment")
        if main_alignment_arg is not None:
            self._main_alignment = Alignment(int(main_alignment_arg))

        # Alignment of items in the cross direction.
        self._cross_alignment: Alignment = Alignment.START
        cross_alignment_arg: Optional[Value] = self._layout.get_argument("cross_alignment")
        if cross_alignment_arg is not None:
            self._cross_alignment = Alignment(int(cross_alignment_arg))
            if self._cross_alignment in (Alignment.SPACE_BETWEEN, Alignment.SPACE_AROUND, Alignment.SPACE_EQUAL):
                self._cross_alignment = Alignment.CENTER

        # Cross alignment of the individual stacks if the Layout wraps.
        self._stack_alignment: Alignment = Alignment.START
        stack_alignment_arg: Optional[Value] = self._layout.get_argument("stack_alignment")
        if stack_alignment_arg is not None:
            self._stack_alignment = Alignment(int(stack_alignment_arg))

        # How items in the Layout are wrapped.
        self._wrap: Wrap = Wrap.NO_WRAP
        wrap_arg: Optional[Value] = self._layout.get_argument("stack_alignment")
        if wrap_arg is not None:
            self._wrap = Wrap(int(wrap_arg))

        # Padding around the Layout's borders.
        self._padding: Padding = Padding(0, 0, 0, 0)
        padding_arg: Optional[Value] = self._layout.get_argument("padding")
        if padding_arg is not None:
            self._padding = Padding(*(max(0., float(padding_arg[i])) for i in range(4)))

        # Spacing between items in the Layout in the main direction.
        self._spacing: float = 0.
        spacing_arg: Optional[Value] = self._layout.get_argument("spacing")
        if spacing_arg is not None:
            self._spacing = float(spacing_arg)  # we allow negative spacing

        # Spacing between stacks, if this Layout wraps.
        self._cross_spacing: float = 0.
        cross_spacing_arg: Optional[Value] = self._layout.get_argument("cross_spacing")
        if cross_spacing_arg is not None:
            self._cross_spacing = float(cross_spacing_arg)  # we allow negative spacing

        # initialize the offset of the first item in the layout
        self._offset: FlexSize = FlexSize(0, 0)
        if self._direction == Direction.LEFT_TO_RIGHT:
            self._offset.main = self._padding.left
            self._offset.cross = self._padding.bottom if self._wrap == Wrap.REVERSE else self._padding.top
        elif self._direction == Direction.RIGHT_TO_LEFT:
            self._offset.main = self._padding.right
            self._offset.cross = self._padding.bottom if self._wrap == Wrap.REVERSE else self._padding.top
        elif self._direction == Direction.TOP_TO_BOTTOM:
            self._offset.main = self._padding.top
            self._offset.cross = self._padding.right if self._wrap == Wrap.REVERSE else self._padding.left
        elif self._direction == Direction.BOTTOM_TO_TOP:
            self._offset.main = self._padding.bottom
            self._offset.cross = self._padding.right if self._wrap == Wrap.REVERSE else self._padding.left

    def __call__(self) -> LayoutComposition:
        content_aabr: Aabrf = Aabrf()
        if self._wrap == Wrap.NO_WRAP:
            content_aabr = self._layout_stack(list(self._layout.get_nodes()), self._offset)
        else:
            # _layout_wrapping();
            pass

        return LayoutComposition(
            nodes=NodeCompositions(self._compositions),
            order=self._layout.get_order(),
            aabr=content_aabr,
            claim=self._claim,
        )
    #
    # def _layout_stack(self, nodes, offset: FlexSize) -> Aabrf:
    #     stack_aabr: Aabrf = Aabrf.zero()
    #     if len(nodes) == 0:
    #         return stack_aabr
    #
    #     is_positive: bool = self._direction in (Direction.LEFT_TO_RIGHT, Direction.TOP_TO_BOTTOM)
    #     is_horizontal: bool = self._direction in (Direction.LEFT_TO_RIGHT, Direction.RIGHT_TO_LEFT)
    #     available_size: FlexSize = FlexSize(
    #         max(0., self._grant.width - self.padding.left - self.padding.right - (
    #             self.spacing * (len(nodes) - 1) if is_horizontal else 0.)),
    #         max(0., self._grant.height - self.padding.top - self.padding.bottom - (
    #             0. if is_horizontal else self.spacing * (len(nodes) - 1))),
    #     )
    #
    #     compositions: Dict[str, NodeComposition] = {}


# HELPER FUNCTIONS #####################################################################################################

def distribute_surplus(surplus: float, items: List[LayoutAdapter]) -> float:
    """
    Distributes the given surplus along the given LayoutAdapters and returns the remaining surplus.
    Note that the `items` argument is an OUT argument.
    The Adapters start out with `result` set to the minimal applicable size.
    If result < preferred -> the item has a "deficit".
    If preferred < upper_bound -> the item has "leeway".
    """
    # do nothing if there is no surplus to distribute
    if surplus <= 0:
        return 0.

    # do nothing if there are no items to distribute to
    if len(items) == 0:
        return surplus

    # sum up the total deficit and scale factor
    total_deficit: float = 0.  # sum of all item deficits
    total_leeway: float = 0.  # sum of all item leeway
    total_deficit_scale_factor: float = 0.  # sum of all scale factors for items with deficit
    total_leeway_scale_factor: float = 0.  # sum of all scale factors for items with leeway
    for item in items:
        item_deficit: float = item.preferred - item.result
        if item_deficit > 0:
            total_deficit += item_deficit
            total_deficit_scale_factor += item.scaling
        item_leeway: float = item.maximum - item.preferred
        if item_leeway > 0:
            total_leeway += item_leeway
            total_leeway_scale_factor += item.scaling
        surplus -= item.result  # subtract the minimum space of the item from the surplus

    # if the minimum size already exhausts the surplus, there is nothing more we can do
    if surplus <= 0:
        return 0.

    # if the deficit is larger than the surplus, we need to distribute the space so that each item can
    # grow from its minimal towards its preferred size, even if it will not get there
    if total_deficit >= surplus:
        deficit_per_scale_factor: float = surplus / total_deficit_scale_factor
        for item in items:
            item.result = min(item.preferred, item.result + item.scaling * deficit_per_scale_factor)
        return 0.
    surplus -= total_deficit

    # if there is enough surplus available to fit the entire leeway, just assign each item its upper
    # bound and return the remainder
    if total_leeway <= surplus:
        for item in items:
            item.result = item.maximum
        return surplus - total_leeway

    # at this point we know that there is enough space available to satisfy all of the items' preferred size
    # but not enough space to fit their maximum size
    leeway_per_scale_factor: float = surplus / total_leeway_scale_factor
    for item in items:
        item.result = min(item.maximum, item.preferred + item.scaling * leeway_per_scale_factor)
    return 0.

from __future__ import annotations

from typing import Dict, Optional, TypeVar, List, Tuple
from enum import Enum, auto, unique

from pycnotf import Size2f, Aabrf, V2f, M3f
from pynotf.data import Value, RowHandle, Claim
import pynotf.core as core

from pynotf.data.claim import Stretch as Stretch
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


class FlexSize:
    """
    Helper struct denoting two sizes, but since the layout can be rotated either way we use main and cross instead of
    width and height.
    """
    __slots__ = ('main', 'cross')

    def __init__(self, main: float, cross: float):
        self.main: float = main
        self.cross: float = cross


@unique
class Direction(Enum):
    """
    Direction in which Nodes in a Layout can be stacked.
    """
    LEFT_TO_RIGHT = auto()
    TOP_TO_BOTTOM = auto()
    RIGHT_TO_LEFT = auto()
    BOTTOM_TO_TOP = auto()


@unique
class Alignment(Enum):
    """
    Alignment of Nodes in a Layout along the main and cross axis.
    """
    START = auto()  # nodes stacked from the start, no additional spacing
    END = auto()  # nodes stacked from the end, no additional spacing
    CENTER = auto()  # nodes centered, no additional spacing
    SPACE_BETWEEN = auto()  # equal spacing between nodes, no spacing between nodes and border
    SPACE_EQUAL = auto()  # equal spacing between the nodes and the border
    SPACE_AROUND = auto()  # single spacing between nodes and border, double spacing between nodes


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
    The FlexLayout class offers a way to arrange nodes in one or multiple rows or columns.
    It behaves similar to the CSS Flex Box Layout.

    Claim
    =====
    The FlexLayout calculates its implicit Claim differently, depending on whether it is wrapping or not.
    A non-wrapping FlexLayout will simply add all of its child Claim's either vertically or horizontally.
    A wrapping FlexLayout on the other hand, will *always* return the default Claim (0 min/preferred, inf max).
    This is because a wrapping FlexLayout will most likely be used to arrange elements within an explicitly defined
    area that is not determined its child nodes themselves.
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

        self._claim: Claim = Claim()

        # Direction in which the FlexLayout is stacked.
        self._direction: Direction = Direction.LEFT_TO_RIGHT
        direction_arg: Optional[Value] = self._layout.get_argument("direction")
        if direction_arg is not None:
            self._direction = Direction(int(direction_arg))

        # Alignment of nodes in the main direction.
        self._main_alignment: Alignment = Alignment.START
        main_alignment_arg: Optional[Value] = self._layout.get_argument("alignment")
        if main_alignment_arg is not None:
            self._main_alignment = Alignment(int(main_alignment_arg))

        # Alignment of nodes in the cross direction.
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

        # How nodes in the Layout are wrapped.
        self._wrap: Wrap = Wrap.NO_WRAP
        wrap_arg: Optional[Value] = self._layout.get_argument("wrap")
        if wrap_arg is not None:
            self._wrap = Wrap(int(wrap_arg))

        # Padding around the Layout's borders.
        self._padding: Padding = Padding(0, 0, 0, 0)
        padding_arg: Optional[Value] = self._layout.get_argument("padding")
        if padding_arg is not None:
            self._padding = Padding(*(max(0., float(padding_arg[i])) for i in range(4)))

        # Spacing between nodes in the Layout in the main direction.
        self._spacing: float = 0.
        spacing_arg: Optional[Value] = self._layout.get_argument("spacing")
        if spacing_arg is not None:
            self._spacing = float(spacing_arg)  # we allow negative spacing

        # Spacing between stacks, if this Layout wraps.
        self._cross_spacing: float = 0.
        cross_spacing_arg: Optional[Value] = self._layout.get_argument("cross_spacing")
        if cross_spacing_arg is not None:
            self._cross_spacing = float(cross_spacing_arg)  # we allow negative spacing

    def __call__(self) -> LayoutComposition:

        compositions: Dict[str, NodeComposition] = {}

        # if self._wrap == Wrap.NO_WRAP:
        available_space: float = (
            self._grant.width if self._direction in (Direction.LEFT_TO_RIGHT, Direction.RIGHT_TO_LEFT)
            else self._grant.height)
        content_aabr: Aabrf = layout_stack(
            nodes=self._layout.iter_nodes(),
            grant=self._grant,
            offset=get_start_offset(
                available_main=available_space,
                main_direction=self._direction,
                padding=self._padding,
                wrap=self._wrap,
            ),
            direction=self._direction,
            padding=self._padding,
            spacing=self._spacing,
            main_alignment=self._main_alignment,
            cross_alignment=self._cross_alignment,
            compositions=compositions,
        )

        return LayoutComposition(
            nodes=NodeCompositions(compositions),
            order=self._layout.get_order(),
            aabr=content_aabr,
            claim=self._claim,  # TODO: calculate claim
        )


# HELPER FUNCTIONS #####################################################################################################

def get_start_offset(available_main: float, main_direction: Direction, padding: Padding, wrap: Wrap) -> FlexSize:
    """
    Initialize the start offset of the first nodes in the layout.
    :param available_main: Available space along the main axis, the cross axis is unbounded.
    :param main_direction: Direction of the main axis.
    :param padding: Fixed padding around the edges of the layout.
    :param wrap: Flexbox wrap behavior.
    """
    # initialize the offset of the first nodes in the layout
    offset: FlexSize = FlexSize(0, 0)
    if main_direction == Direction.LEFT_TO_RIGHT:
        offset.main = padding.left
        offset.cross = padding.bottom if wrap == Wrap.REVERSE else padding.top
    elif main_direction == Direction.RIGHT_TO_LEFT:
        offset.main = max(0., available_main - padding.right)
        offset.cross = padding.bottom if wrap == Wrap.REVERSE else padding.top
    elif main_direction == Direction.TOP_TO_BOTTOM:
        offset.main = padding.top
        offset.cross = padding.right if wrap == Wrap.REVERSE else padding.left
    elif main_direction == Direction.BOTTOM_TO_TOP:
        offset.main = max(0., available_main - padding.bottom)
        offset.cross = padding.right if wrap == Wrap.REVERSE else padding.left
    return offset


def calculate_main_alignment(alignment: Alignment, surplus: float, spacing: float, node_count: int) \
        -> Tuple[float, float]:
    """
    Determine the offset at the start of the main alignment as well as the space between each node.
    :param alignment: What kind of main Alignment to calculate.
    :param surplus: Amount of space left to distribute after all nodes got theirs.
    :param spacing: Minimal space between nodes.
    :param node_count: Number if nodes in the main alignment.
    :return: The first returned value is the absolute start offset along the main axis.
             The second returned value is the absolute spacing between nodes.
    """
    start_offset: float = 0.
    if alignment == Alignment.START:
        pass
    elif alignment == Alignment.END:
        start_offset = surplus
    elif alignment == Alignment.CENTER:
        start_offset = surplus / 2.
    elif alignment == Alignment.SPACE_BETWEEN:
        spacing += surplus / (node_count - 1) if node_count > 1 else 0.
    elif alignment == Alignment.SPACE_AROUND:
        start_offset = surplus / (node_count * 2)
        spacing += start_offset * 2.
    elif alignment == Alignment.SPACE_EQUAL:
        start_offset = surplus / (node_count + 1)
        spacing += start_offset
    return start_offset, spacing


def calculate_cross_alignment(alignment: Alignment, surplus: float, is_cross_positive: bool) -> float:
    """
    Determine the offset at the start of the cross alignment.
    Unlike the main alignment, this does not calculate the spacing between rows because the cross axis is unbounded and
    the spacing therefore fixed.
    :param alignment: What kind of cross Alignment to calculate.
    :param surplus: Amount of space left to distribute after all nodes got theirs.
    :param is_cross_positive: Whether or not the cross axis grows into a positive direction.
    """
    if alignment == Alignment.START:
        return 0. if is_cross_positive else surplus
    elif alignment == Alignment.END:
        return surplus if is_cross_positive else 0.
    else:  # all others
        return surplus / 2.


def distribute_surplus(surplus: float, nodes: List[Stretch]) -> float:
    """
    Distributes the given surplus along the given LayoutAdapters and returns the remaining surplus.
    Note that the `nodes` argument is an OUT argument.
    The Adapters start out with `result` set to the minimal applicable size.
    If result < preferred -> the node has a "deficit".
    If preferred < upper_bound -> the node has "leeway".
    """
    # do nothing if there is no surplus to distribute
    if surplus <= 0:
        return 0.

    # do nothing if there are no nodes to distribute to
    if len(nodes) == 0:
        return surplus

    """
    The Flexbox version 2 C++ code has a few lines to adjust the maximum size of an node based on its Claim's ratio:
        const std::pair<float, float> width_to_height = claim.get_width_to_height();
        if (width_to_height.first > 0.f) {
            if (is_horizontal()) {
                adapters[i].upper_bound = min(stretch.get_max(), available.cross * width_to_height.second);
            } else {
                adapters[i].upper_bound = min(stretch.get_max(), available.main / width_to_height.first);
            }
        }
        else {
            adapters[i].upper_bound = stretch.get_max();
        }
    Maybe we could store the adjusted max in the `_result` field until replacing it with the actual result...
    """

    class Batch:
        __slots__ = ('total_deficit', 'total_leeway', 'total_deficit_scale_factor',
                     'total_leeway_scale_factor', 'stretches')

        def __init__(self):
            self.total_deficit: float = 0.  # sum of all node deficits
            self.total_leeway: float = 0.  # sum of all node leeway
            self.total_deficit_scale_factor: float = 0.  # sum of all scale factors for nodes with deficit
            self.total_leeway_scale_factor: float = 0.  # sum of all scale factors for nodes with leeway
            self.stretches: List[Stretch] = []

    # precompute batches
    batches: Dict[int, Batch] = {}
    for node in nodes:
        batch: Batch = batches.setdefault(node.priority, Batch())
        batch.stretches.append(node)
        node_deficit: float = node.preferred - node.min
        if node_deficit > 0:
            batch.total_deficit += node_deficit
            batch.total_deficit_scale_factor += node.scale_factor
        node_leeway: float = node.max - node.preferred
        if node_leeway > 0:
            batch.total_leeway += node_leeway
            batch.total_leeway_scale_factor += node.scale_factor
        node.grant = node.min  # start out by setting the result to the minimum
        surplus -= node.min  # subtract the minimum space of the node from the surplus

    # if the minimum size already exhausts the surplus, there is nothing more we can do
    if surplus <= 0:
        return 0.

    # go through each batch by priority
    for batch in (batches[priority] for priority in sorted(batches.keys())):
        # C++ implementation idea: have a fast path for cases where all nodes have THE SAME priority (as will happen in
        # most cases) and a fallback ONLY if you detect that there are multiple priorities.
        # One idea here would be that we store a single int that is initialized the the priority of the first node (or
        # simply the default priority = zero). As long as all nodes have the same priority, we don't store them into a
        # separate "batch" at all. Only if the first node has a different priority we take all values that we have
        # summed up so far, including the position of the first node with a different priority and switch over to the
        # "slow" function. The slow function can skip the first n nodes (how ever many there are without a difference in
        # priority) and move all of them into their own batch.
        # A more involved, but probably (?) more efficient version allows one node with a higher priority and one node
        # with a lower priority than zero before branching off into the slow version.
        # If any of this actually improves performance is up for the profiler to decide. This Python version is
        # basically the "slow" path by default ... because it is Python.

        # if the deficit is larger than the surplus, we need to distribute the space so that each stretch can
        # grow from its minimal towards its preferred size, even if it will not get there
        if batch.total_deficit >= surplus:
            deficit_per_scale_factor: float = surplus / batch.total_deficit_scale_factor
            for stretch in batch.stretches:
                stretch.grant = min(stretch.preferred, stretch.min + stretch.scale_factor * deficit_per_scale_factor)
            return 0.
        surplus -= batch.total_deficit

        # if there is enough surplus available to fit the entire leeway, just assign each stretch its upper
        # bound and return the remainder
        if batch.total_leeway <= surplus:
            for stretch in batch.stretches:
                stretch.grant = stretch.max
            surplus -= batch.total_leeway
            continue

        # at this point we know that there is enough space available to satisfy all of the stretch's preferred size
        # but not enough space to fit their maximum size
        leeway_per_scale_factor: float = surplus / batch.total_leeway_scale_factor
        for stretch in batch.stretches:
            stretch.grant = min(stretch.max, stretch.preferred + stretch.scale_factor * leeway_per_scale_factor)
        return 0.

    # return the remaining surplus
    return surplus


def layout_stack(nodes: Layout.NodeIterator, grant: Size2f, offset: FlexSize, direction: Direction,
                 padding: Padding, spacing: float, main_alignment: Alignment, cross_alignment: Alignment,
                 compositions: Dict[str, NodeComposition]) -> Aabrf:
    # do nothing if there are no nodes to stack
    node_count: int = len(nodes)
    if node_count == 0:
        return Aabrf.zero()

    # calculate the available space
    is_horizontal: bool = direction in (Direction.LEFT_TO_RIGHT, Direction.RIGHT_TO_LEFT)
    available: FlexSize = FlexSize(
        max(0., grant.width - (padding.left + padding.right) - spacing * (node_count - 1 if is_horizontal else 0.)),
        max(0., grant.height - (padding.top + padding.bottom) - (0. if is_horizontal else spacing) * (node_count - 1)),
    )

    # distribute the available space among the nodes in the stack (modifies the nodes' "result" values in their claims)
    surplus: float = distribute_surplus(
        available.main if is_horizontal else available.cross,
        [node.claim.horizontal if is_horizontal else node.claim.vertical for node in nodes],
    )
    # TODO: note that this does not set the other stretch result. When removing the `grant` Interop, we have to set the
    #  cross stretch result manually at the end of this method

    # determine values for alignment along the main axis
    is_positive: bool = direction in (Direction.LEFT_TO_RIGHT, Direction.TOP_TO_BOTTOM)
    start_offset, spacing = calculate_main_alignment(main_alignment, surplus, spacing, node_count)
    offset.main += start_offset * (1 if is_positive else -1)

    # apply the layout
    stack_aabr: Aabrf = Aabrf.wrongest()
    for node in nodes:
        # const std::pair<float, float> ratio = child->get_claim().get_width_to_height();
        if is_horizontal:
            node_size: Size2f = Size2f(
                width=node.claim.horizontal.grant,
                height=max(
                    node.claim.vertical.min,
                    min(node.claim.vertical.max, available.cross)),  # ... node.claim.horizontal.result / ratio.first),
            )
            node_pos: V2f = V2f(
                x=offset.main - (0. if is_positive else node_size.width),
                y=offset.cross + calculate_cross_alignment(
                    cross_alignment, available.cross - node_size.height, is_cross_positive=True),
            )
            node.claim.vertical.grant = node_size.height
        else:  # vertical
            node_size: Size2f = Size2f(
                width=max(
                    node.claim.horizontal.min,
                    min(node.claim.horizontal.max, available.main)),  # ... node.claim.vertical.result * ratio.second),
                height=node.claim.vertical.grant,
            )
            node_pos: V2f = V2f(
                x=offset.cross + calculate_cross_alignment(
                    cross_alignment, available.cross - node_size.width, is_cross_positive=True),
                y=offset.main - (0. if is_positive else node_size.height),
            )
            node.claim.horizontal.grant = node_size.width

        # store the node composition
        assert node.name not in compositions
        compositions[node.name] = NodeComposition(
            xform=M3f.translation(node_pos),
            grant=node_size,
            opacity=1.,
        )

        # grow the combined aabr
        stack_aabr.unite(Aabrf(node_pos, node_size))

        # ... and advance the offset
        offset.main += (((node.claim.horizontal.grant if is_horizontal else node.claim.vertical.grant) + spacing)
                        * (1 if is_positive else -1))

    return stack_aabr

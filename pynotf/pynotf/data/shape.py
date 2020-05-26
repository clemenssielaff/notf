from __future__ import annotations

from typing import List, Optional, Iterable

from pycnotf import V2f, CubicBezier2f, Aabrf, Polygon2f, approximate
from pynotf.extra import chunks


class ShapeBuilder:
    def __init__(self, start: V2f):
        self._is_closed: bool = False
        self._points: List[V2f] = [start]
        self._indices: List[int] = []

    def get_points(self) -> List[V2f]:
        return self._points

    def get_indices(self) -> List[int]:
        return self._indices

    def is_valid(self) -> bool:
        if len(self._indices) == 0:
            return False
        if len(self._indices) % 4 != 0:
            return False
        for index in self._indices:
            if index >= len(self._points):
                return False
        return True

    def add_segment(self, end: V2f, ctrl1: Optional[V2f] = None, ctrl2: Optional[V2f] = None) -> None:
        assert not self._is_closed

        # start
        index: int = len(self._points) - 1
        self._indices.append(index)

        # ctrl 1
        if ctrl1 is not None and not ctrl1.is_approx(self._points[index]):
            self._points.append(ctrl1)
            index += 1
        self._indices.append(index)

        # ctrl 2
        if ctrl2 is not None:
            if not ctrl2.is_approx(self._points[index]):
                self._points.append(ctrl2)
                index += 1
            self._indices.append(index)

        # end
        if not end.is_approx(self._points[index]):
            self._points.append(end)
            index += 1
        if ctrl2 is None:
            self._indices.append(index)
        self._indices.append(index)

    def close(self) -> V2f:
        if not self._is_closed:
            # close the shape with a straight line, if it is still open
            if not self._points[0].is_approx(self._points[-1]):
                self.add_segment(self._points[-1], self._points[0])
            self._is_closed = True
        return self._points[0]  # return start point, useful for parsing multiple relative paths


class Shape:
    def __init__(self, builder: Optional[ShapeBuilder] = None):
        assert builder is None or builder.is_valid()

        self._vertices: List[V2f] = [] if builder is None else builder.get_points()
        self._indices: List[int] = [] if builder is None else builder.get_indices()
        self._aabr: Aabrf = Aabrf.wrongest() if builder is None else self._calculate_bounding_box()

    def __repr__(self):
        coords: str = ', '.join(f'({v.x}, {v.y})' for v in (self._vertices[i] for i in self._indices))
        return f'Shape({coords})'

    def is_valid(self) -> bool:
        return self._aabr.is_valid()

    def get_vertex(self, index: int) -> V2f:
        return self._vertices[self._indices[index]]

    def iter_outline(self, linearize_straights: bool = True) -> Iterable[CubicBezier2f]:
        """
        :param linearize_straights: If true, all bezier segments where all points fall onto a straight line between start
                                    and end are replaced with visually equivalent beziers with linear interpolation
                                    speed. This is advantageous for approximation of the bezier with a line.
        """
        for start_index, ctrl1_index, ctrl2_index, end_index in chunks(self._indices, 4):
            start_vertex: V2f = self._vertices[start_index]
            end_vertex: V2f = self._vertices[end_index]
            if linearize_straights and end_index == start_index + 1:
                delta = end_vertex - start_vertex
                yield CubicBezier2f(
                    start_vertex, start_vertex + delta * (1. / 3.), start_vertex + delta * (2. / 3.), end_vertex)
            else:
                yield CubicBezier2f(
                    start_vertex, self._vertices[ctrl1_index], self._vertices[ctrl2_index], end_vertex)

    def contains(self, point: V2f) -> bool:
        # if the point is not in the aabr, it cannot be inside the shape
        if not self._aabr.contains(point):
            return False

        # TODO: improve Shape.contains
        # The idea here is that we avoid to test against the bezier as much as possible, and instead try to confirm or
        # rule out a hit by looking at the polygon cage exclusively.
        # The hypothesis is that given any shape, you can select a subset of vertices of the polygon cage that make up
        # the "outer" hull of the shape and another set that makes up the "inner" hull.
        # The outer hull is convex, whereas the inner one ... is also convex? I have to investigate that a bit.
        # Also I think not all vertices are part of the inner or outer hull... hmm.
        # Then, iterate over all quartets of vertices and perform a standard point-in-polygon test, after determining
        # whether the next vertex is part of the outer or inner hull. If the winding of the outer hull matches the
        # winding of the inner hull, we know that the point is either decisively outside or inside with regards to the
        # investigated segment. Only if the two differ must we test the point against the bezier itself. In this case,
        # it might be easiest to use the basic flattening approach with regular t-intervals, because even though it is
        # not optimal in many cases, we will most likely only have to do it once (if at all) and throw away the result
        # immediately anyway. Since it is so easy to calculate that it might offset the added calculation for producing
        # an approximation with fewer lines.
        # The reason why I don't do this right now is that this is somewhat experimental and I want to keep up my dev
        # speed.
        # On the flip side - do not forget to write a blog article about this, if this turns out to work!
        # 'cause it's cool!

        # for now, flatten the entire shape and use point-in-polygon
        vertices: List[V2f] = []
        for bezier in self.iter_outline():
            vertices.extend(approximate(bezier).get_vertices())
        return Polygon2f(vertices).contains(point)

    def _calculate_bounding_box(self) -> Aabrf:
        min_: V2f = V2f.highest()
        max_: V2f = V2f.lowest()
        for point in self._vertices:
            min_.set_min(point)
            max_.set_max(point)
        return Aabrf(min_, max_)

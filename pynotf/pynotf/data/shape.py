from __future__ import annotations

from typing import List, Optional

from pycnotf import V2f, CubicBezier2f, Aabrf, Segment2f, Polygon2f


class ShapeBuilder:
    def __init__(self):
        self._points: List[V2f] = []
        self._indices: List[int] = []

    @property
    def points(self) -> List[V2f]:
        return self._points

    @property
    def indices(self) -> List[int]:
        return self._indices

    def add_segment(self, start: V2f, end: V2f, ctrl1: Optional[V2f] = None, ctrl2: Optional[V2f] = None) -> None:
        # start
        index: int = len(self._points)
        if index > 0 and start.is_approx(self._points[-1]):
            index -= 1  # re-use the current end point, if the new start falls on it
        else:
            self._points.append(start)
        self._indices.append(index)

        # ctrl 1
        if ctrl1 and not ctrl1.is_approx(self._points[index]):
            self._points.append(ctrl1)
            index += 1
        self._indices.append(index)

        # ctrl 2
        if ctrl2 and not ctrl2.is_approx(self._points[index]):
            self._points.append(ctrl2)
            index += 1
        self._indices.append(index)

        # end
        if not end.is_approx(self._points[index]):
            self._points.append(ctrl2)
            index += 1
        self._indices.append(index)


class Shape:
    def __init__(self, builder: ShapeBuilder):
        self._vertices: List[V2f] = builder.points
        self._indices: List[int] = builder.indices
        self._aabr: Aabrf = self._calculate_bounding_box()

    def contains(self, point: V2f) -> bool:
        # first test: point in aabr
        if not self._aabr.contains(point):
            return False

        # second challenge: point in polygon hull
        if not Polygon2f.contains_(self._vertices, point):
            return False

    def _calculate_bounding_box(self) -> Aabrf:
        min_: V2f = V2f.highest()
        max_: V2f = V2f.lowest()
        for point in self._vertices:
            min_.minimize_with(point)
            max_.maximize_with(point)
        return Aabrf(min_, max_)


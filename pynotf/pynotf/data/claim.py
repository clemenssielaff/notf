from __future__ import annotations

from math import inf
from typing import Optional, Union

ALMOST_ZERO: float = 0.00001


########################################################################################################################


class Stretch:
    def __init__(self, preferred: float = 0., min_: Optional[float] = None, max_: Optional[float] = None,
                 scale_factor: float = 1., priority: int = 0):
        self._preferred: float = max(preferred, 0.)
        self._min: float = max(0., min(min_, self._preferred)) if min_ else self._preferred
        self._max: float = max(max_, self._preferred) if max_ else inf
        self._scale_factor: float = max(0., scale_factor)
        self._priority: int = priority

    @property
    def min(self) -> float:
        """
        0 <= min <= preferred
        """
        return self._min

    @min.setter
    def min(self, value: float) -> None:
        self._min = max(0., value)
        self._preferred = max(self._min, self._preferred)
        self._max = max(self._min, self._max)

    @property
    def preferred(self) -> float:
        """
        min <= preferred <= max
        """
        return self._preferred

    @preferred.setter
    def preferred(self, value: float) -> None:
        self._preferred = max(0., value)
        self._min = min(self._min, self._preferred)
        self._max = max(self._max, self._preferred)

    @property
    def max(self) -> float:
        """
        preferred <= max < INFINITY
        """
        return self._max

    @max.setter
    def max(self, value: float) -> None:
        self._max = max(0., value)
        self._min = min(self._min, self._max)
        self._preferred = min(self._preferred, self._max)

    @property
    def scale_factor(self) -> float:
        """
        0 <= factor < INFINITY
        """
        return self._scale_factor

    @scale_factor.setter
    def scale_factor(self, value: float) -> None:
        self._scale_factor = max(ALMOST_ZERO, value)

    @property
    def priority(self) -> int:
        """
        Unbound signed.
        """
        return self._priority

    @priority.setter
    def priority(self, value: int) -> None:
        self._priority = value

    def is_fixed(self) -> bool:
        return (self._max - self._min) < ALMOST_ZERO

    def set_fixed(self, value: float) -> None:
        self._min = self._preferred = self._max = max(0., value)

    def offset(self, value: float) -> None:
        """
        Add an offset to the min/preferred/max values.
        """
        self._min = max(0., self._min + value)
        self._preferred = max(0., self._preferred + value)
        self._max = max(0., self._max + value)

    def maxed(self, other: Stretch) -> None:
        """
        In-place max with another Stretch.
        """
        self._preferred = max(self._preferred, other._preferred)
        self._min = max(self._min, other._min)
        self._max = max(self._max, other._max)
        self._scale_factor = max(self._scale_factor, other._scale_factor)
        self._priority = max(self._priority, other._priority)

    def __iadd__(self, other: Stretch) -> Stretch:
        """
        In-place max with another Stretch.
        """
        self._preferred += other._preferred
        self._min += other._min
        self._max = other._max
        self._scale_factor = max(self._scale_factor, other._scale_factor)
        self._priority = max(self._priority, other._priority)
        return self


########################################################################################################################


class Claim:
    Stretch = Stretch

    def __init__(self, width: Union[Stretch, float] = 0., height: Union[Stretch, float] = 0.):
        if isinstance(width, float):
            self.horizontal: Stretch = Stretch(width)
        else:
            self.horizontal: Stretch = width

        if isinstance(height, float):
            self.vertical: Stretch = Stretch(height)
        else:
            self.vertical: Stretch = height

    def set_fixed(self, width: float, height: Optional[float] = None) -> None:
        self.horizontal.set_fixed(width)
        self.vertical.set_fixed(height or width)

    def add_horizontal(self, other: Claim) -> None:
        self.horizontal += other.horizontal
        self.vertical.maxed(other.vertical)

    def add_vertical(self, other: Claim) -> None:
        self.horizontal.maxed(other.horizontal)
        self.vertical += other.vertical

    def maxed(self, other: Claim) -> None:
        self.horizontal.maxed(other.horizontal)
        self.vertical.maxed(other.vertical)

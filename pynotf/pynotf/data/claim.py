from __future__ import annotations

from sys import float_info
from math import inf
from typing import Optional

from .value import Value


# STRETCH ##############################################################################################################

class Stretch:
    """
    The Stretch contains a field for the result value as well, even though it does not affect the layout.
    If we did not do it, we'd have to allocate space for the results every time and do a bit of housekeeping on top to
    relate a layout result to a specific Node.
    Of course, the idea was that the Claim was something defined by the user, whereas the grant was only defined by the
    Layout. But the user already does not have direct access to the Claim anyway to guarantee constraints.

    C++ implementation note:
    It would be nice to use uint16 for min, pref, max and float16 for scale factor, maybe int16 for priority (although
    honestly, int8 should do) and uint16 for the result ... bam: one Stretch takes up the space of only three doubles.
    """
    __slots__ = ('_preferred', '_min', '_max', '_scale_factor', '_priority', '_grant')

    SCHEMA: Value.Schema = Value.Schema.create(dict(preferred=0, min=0, max=inf, scale_factor=1, priority=0))

    def __init__(self, preferred: float = 0., minimum: float = 0., maximum: float = inf,
                 scale_factor: float = 1., priority: int = 0):
        self._preferred: float = 0.
        self._min: float = 0.
        self._max: float = 0.
        self._scale_factor: float = 0.
        self._priority: int = priority
        self._grant: float = 0.

        # order matters here
        self.min = minimum
        self.max = maximum
        self.preferred = preferred
        self.scale_factor = scale_factor

    @classmethod
    def from_value(cls, value: Value) -> Stretch:
        if value.get_schema() != cls.SCHEMA:
            raise ValueError(f'Cannot build a Claim.Stretch from an incompatible Value Schema:\n'
                             f'{value.get_schema()!s}.\n'
                             f'A valid Claim Value has the Schema:\n'
                             f'{cls.SCHEMA!s}')
        return Stretch(
            preferred=float(value[0]),
            minimum=float(value[1]),
            maximum=float(value[2]),
            scale_factor=float(value[3]),
            priority=int(value[4]),
        )

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
    def min(self) -> float:
        """
        0 <= min <= preferred
        """
        return self._min

    @min.setter
    def min(self, value: float) -> None:
        self._min = max(0., value)
        self._preferred = max(self._preferred, self._min)
        self._max = max(self._max, self._min)

    @property
    def max(self) -> float:
        """
        preferred <= max < INFINITY
        """
        return self._max

    @max.setter
    def max(self, value: float) -> None:
        self._max = max(0., value)
        self._preferred = min(self._preferred, self._max)
        self._min = min(self._min, self._max)

    @property
    def scale_factor(self) -> float:
        """
        0 <= factor < INFINITY
        """
        return self._scale_factor

    @scale_factor.setter
    def scale_factor(self, value: float) -> None:
        self._scale_factor = max(float_info.epsilon, value)

    @property
    def priority(self) -> int:
        """
        Unbound signed.
        """
        return self._priority

    @priority.setter
    def priority(self, value: int) -> None:
        self._priority = value

    @property
    def grant(self) -> float:
        """
        The "grant" member is invisible to the user! it is only read by Nodes and written by Layouts.
        min <= grant <= max
        """
        return self._grant

    @grant.setter
    def grant(self, value: float) -> None:
        self._grant = max(self._min, min(self._max, value))

    def is_fixed(self) -> bool:
        return (self.max - self.min) < float_info.epsilon

    def set_fixed(self, value: float) -> None:
        value = max(0., value)
        self._preferred = value
        self._min = value
        self._max = value

    def add(self, other: Stretch) -> None:
        """
        Adds the other Stretch to this one.
        """
        self._preferred += other.preferred
        self._min += other.min
        self._max += other.max
        self._scale_factor = max(self._scale_factor, other.scale_factor)
        self._priority = max(self._priority, other.priority)

    def offset(self, value: float) -> None:
        """
        Add an offset to the min/preferred/max values.
        """
        self._preferred = max(0., self._preferred + value)
        self._min = max(0., self._min + value)
        self._max = max(0., self._max + value)

    def maxed(self, other: Stretch) -> None:
        """
        Max this with another Stretch.
        """
        self._preferred = max(self._preferred, other.preferred)
        self._min = max(self._min, other.min)
        self._max = max(self._max, other.max)
        self._scale_factor = max(self._scale_factor, other.scale_factor)
        self._priority = max(self._priority, other.priority)


########################################################################################################################

class Claim:
    Stretch = Stretch
    SCHEMA: Value.Schema = Value.Schema.create(dict(
        horizontal=dict(preferred=0, min=0, max=inf, scale_factor=1, priority=0),
        vertical=dict(preferred=0, min=0, max=inf, scale_factor=1, priority=0)))

    def __init__(self):
        """
        Default, empty constructor.
        """
        self._horizontal: Stretch = Stretch()
        self._vertical: Stretch = Stretch()
        # TODO: Claim is missing the ratio

    @classmethod
    def from_value(cls, value: Value) -> Claim:
        if value.get_schema() != cls.SCHEMA:
            raise ValueError(f'Cannot build a Claim from an incompatible Value Schema:\n{value.get_schema()!s}.\n'
                             f'A valid Claim Value has the Schema:\n{cls.SCHEMA!s}')
        result: Claim = Claim()
        result._horizontal = Stretch.from_value(value[0])
        result._vertical = Stretch.from_value(value[1])
        return result

    def to_value(self) -> Value:
        return Value(
            horizontal=dict(
                preferred=self._horizontal.preferred,
                min=self._horizontal.min,
                max=self._horizontal.max,
                scale_factor=self._horizontal.scale_factor,
                priority=self._horizontal.priority),
            vertical=dict(
                preferred=self._vertical.preferred,
                min=self._vertical.min,
                max=self._vertical.max,
                scale_factor=self._vertical.scale_factor,
                priority=self._vertical.priority),
        )

    @property
    def horizontal(self) -> Stretch:
        return self._horizontal

    @property
    def vertical(self) -> Stretch:
        return self._vertical

    def set_fixed(self, width: float, height: Optional[float] = None) -> None:
        self._horizontal.set_fixed(width)
        self._vertical.set_fixed(height if height is not None else width)

    def add_horizontal(self, other: Claim) -> None:
        self._horizontal.add(other.horizontal)
        self._vertical.maxed(other.vertical)

    def add_vertical(self, other: Claim) -> None:
        self._horizontal.maxed(other.horizontal)
        self._vertical.add(other.vertical)

    def maxed(self, other: Claim) -> None:
        self._horizontal.maxed(other.horizontal)
        self._vertical.maxed(other.vertical)

from __future__ import annotations

from math import inf
from typing import Optional, Union, NamedTuple

from .value import Value, mutate_value, multimutate_value

# CONSTANTS ############################################################################################################

ALMOST_ZERO: float = 0.00001
SKETCH_INDEX_PRF: int = 0
SKETCH_INDEX_MIN: int = 1
SKETCH_INDEX_MAX: int = 2
SKETCH_INDEX_SCL: int = 3
SKETCH_INDEX_PRT: int = 4
CLAIM_INDEX_HORIZONTAL: int = 0
CLAIM_INDEX_VERTICAL: int = 1
CLAIM_DEFAULT: Value = Value(dict(
    horizontal=dict(preferred=0, min=0, max=inf, scale_factor=1, priority=0),
    vertical=dict(preferred=0, min=0, max=inf, scale_factor=1, priority=0)))
CLAIM_SCHEMA: Value.Schema = CLAIM_DEFAULT.get_schema()


# STRETCH ##############################################################################################################

class _Stretch:

    def __init__(self, claim: Claim, index: int):
        assert index in (CLAIM_INDEX_HORIZONTAL, CLAIM_INDEX_VERTICAL)
        self._claim: Claim = claim
        self._index: int = index  # in C++ we could have two types for horizontal/vertical and the index as a template

    @property
    def preferred(self) -> float:
        """
        min <= preferred <= max
        """
        return float(self._claim._value[self._index][SKETCH_INDEX_PRF])

    @preferred.setter
    def preferred(self, value: float) -> None:
        value = max(0., value)
        self._claim._value = multimutate_value(
            self._claim._value,
            ((self._index, SKETCH_INDEX_PRF), value),
            ((self._index, SKETCH_INDEX_MIN), min(value, self.min)),
            ((self._index, SKETCH_INDEX_MAX), max(value, self.max)),
        )

    @property
    def min(self) -> float:
        """
        0 <= min <= preferred
        """
        return float(self._claim._value[self._index][SKETCH_INDEX_MIN])

    @min.setter
    def min(self, value: float) -> None:
        value = max(0., value)
        self._claim._value = multimutate_value(
            self._claim._value,
            ((self._index, SKETCH_INDEX_PRF), max(value, self.preferred)),
            ((self._index, SKETCH_INDEX_MIN), value),
            ((self._index, SKETCH_INDEX_MAX), max(value, self.max)),
        )

    @property
    def max(self) -> float:
        """
        preferred <= max < INFINITY
        """
        return float(self._claim._value[self._index][SKETCH_INDEX_MAX])

    @max.setter
    def max(self, value: float) -> None:
        value = max(0., value)
        self._claim._value = multimutate_value(
            self._claim._value,
            ((self._index, SKETCH_INDEX_PRF), min(value, self.preferred)),
            ((self._index, SKETCH_INDEX_MIN), min(value, self.min)),
            ((self._index, SKETCH_INDEX_MAX), value),
        )

    @property
    def scale_factor(self) -> float:
        """
        0 <= factor < INFINITY
        """
        return float(self._claim._value[self._index][SKETCH_INDEX_SCL])

    @scale_factor.setter
    def scale_factor(self, value: float) -> None:
        self._claim._value = mutate_value(self._claim._value, (self._index, SKETCH_INDEX_SCL), max(ALMOST_ZERO, value))

    @property
    def priority(self) -> int:
        """
        Unbound signed.
        """
        return int(self._claim._value[self._index][SKETCH_INDEX_PRT])

    @priority.setter
    def priority(self, value: int) -> None:
        self._claim._value = mutate_value(self._claim._value, (self._index, SKETCH_INDEX_PRT), value)

    def is_fixed(self) -> bool:
        return (self.max - self.min) < ALMOST_ZERO

    def set_fixed(self, value: float) -> None:
        value = max(0., value)
        self._claim._value = multimutate_value(
            self._claim._value,
            ((self._index, SKETCH_INDEX_PRF), value),
            ((self._index, SKETCH_INDEX_MIN), value),
            ((self._index, SKETCH_INDEX_MAX), value),
        )

    def add(self, other: _Stretch) -> None:
        """
        Adds the other Stretch to this one.
        """
        self._claim._value = multimutate_value(
            self._claim._value,
            ((self._index, SKETCH_INDEX_PRF), self.preferred + other.preferred),
            ((self._index, SKETCH_INDEX_MIN), self.min + other.min),
            ((self._index, SKETCH_INDEX_MAX), self.max + other.max),
            ((self._index, SKETCH_INDEX_SCL), max(self.scale_factor, other.scale_factor)),
            ((self._index, SKETCH_INDEX_PRT), max(self.priority, other.priority)),
        )

    def offset(self, value: float) -> None:
        """
        Add an offset to the min/preferred/max values.
        """
        self._claim._value = multimutate_value(
            self._claim._value,
            ((self._index, SKETCH_INDEX_PRF), max(0., self.preferred + value)),
            ((self._index, SKETCH_INDEX_MIN), max(0., self.min + value)),
            ((self._index, SKETCH_INDEX_MAX), max(0., self.max + value)),
        )

    def maxed(self, other: _Stretch) -> None:
        """
        Max this with another Stretch.
        """
        self._claim._value = multimutate_value(
            self._claim._value,
            ((self._index, SKETCH_INDEX_PRF), max(self.preferred, other.preferred)),
            ((self._index, SKETCH_INDEX_MIN), max(self.min, other.min)),
            ((self._index, SKETCH_INDEX_MAX), max(self.max, other.max)),
            ((self._index, SKETCH_INDEX_SCL), max(self.scale_factor, other.scale_factor)),
            ((self._index, SKETCH_INDEX_PRT), max(self.priority, other.priority)),
        )


########################################################################################################################

class StretchDescription(NamedTuple):
    preferred: float = 0.
    min: Optional[float] = None
    max: Optional[float] = None
    scale_factor: float = 1.
    priority: int = 0

    def validate(self) -> StretchDescription:
        preferred: float = max(self.preferred, 0.)
        return StretchDescription(
            preferred=preferred,
            min=preferred if self.min is None else max(0., min(self.min, preferred)),
            max=inf if self.max is None else max(self.max, preferred),
            scale_factor=max(ALMOST_ZERO, self.scale_factor),
            priority=self.priority
        )


class Claim:
    Stretch = StretchDescription

    def __init__(self, horizontal: Optional[Union[float, StretchDescription, Value]] = None,
                 vertical: Optional[Union[float, StretchDescription]] = None):
        self._value: Value

        # default
        if horizontal is None:
            assert vertical is None
            self._value = CLAIM_DEFAULT

        # use an existing Value for the Claim
        elif isinstance(horizontal, Value):
            assert horizontal.get_schema() == CLAIM_SCHEMA
            self._value = horizontal

        # ... or construct a new one based on the user's description
        else:
            # horizontal description
            if isinstance(horizontal, float):
                horizontal = StretchDescription(horizontal)
            assert isinstance(horizontal, StretchDescription)
            horizontal = horizontal.validate()

            # vertical description
            if vertical is None:
                vertical = horizontal
            else:
                if isinstance(vertical, float):
                    vertical = StretchDescription(vertical)
                assert isinstance(vertical, StretchDescription)
                vertical = vertical.validate()

            # value
            self._value: Value = Value(dict(
                horizontal=dict(
                    preferred=horizontal.preferred,
                    min=horizontal.min,
                    max=horizontal.max,
                    scale_factor=horizontal.scale_factor,
                    priority=horizontal.priority
                ),
                vertical=dict(
                    preferred=vertical.preferred,
                    min=vertical.min,
                    max=vertical.max,
                    scale_factor=vertical.scale_factor,
                    priority=vertical.priority
                )
            ))

    def get_value(self) -> Value:
        """
        The Claim as Value.
        """
        return self._value

    @property
    def horizontal(self) -> _Stretch:
        return _Stretch(self, CLAIM_INDEX_HORIZONTAL)

    @property
    def vertical(self) -> _Stretch:
        return _Stretch(self, CLAIM_INDEX_VERTICAL)

    def set_fixed(self, width: float, height: Optional[float] = None) -> None:
        self.horizontal.set_fixed(width)
        self.vertical.set_fixed(height if height is not None else width)

    def add_horizontal(self, other: Claim) -> None:
        self.horizontal.add(other.horizontal)
        self.vertical.maxed(other.vertical)

    def add_vertical(self, other: Claim) -> None:
        self.horizontal.maxed(other.horizontal)
        self.vertical.add(other.vertical)

    def maxed(self, other: Claim) -> None:
        self.horizontal.maxed(other.horizontal)
        self.vertical.maxed(other.vertical)

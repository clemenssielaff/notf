from __future__ import annotations

from enum import IntEnum, auto, unique

from pyrsistent._checked_types import CheckedPVector as ConstList

from pycnotf import M3f
from pynotf.data import Value

__all__ = ('IndexEnum', 'ValueList', 'm3f_from_value', 'TableIndex')


# UTILS ################################################################################################################

class IndexEnum(IntEnum):
    """
    Enum class whose auto indices start with zero.
    """

    def _generate_next_value_(self, _1, index: int, _2) -> int:
        return index


class ValueList(ConstList):
    """
    An immutable list of Values.
    """
    __type__ = Value


def m3f_from_value(value: Value) -> M3f:
    """
    Constructs a Matrix3f from a Value.
    """
    return M3f(*(float(e) for e in value))


# DATA #################################################################################################################

@unique
class TableIndex(IndexEnum):
    OPERATORS = auto()
    NODES = auto()
    LAYOUTS = auto()

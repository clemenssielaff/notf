from __future__ import annotations

from enum import IntEnum, auto, unique
from typing import Any, NamedTuple, Optional, Dict
from types import CodeType

from pyrsistent._checked_types import CheckedPVector as ConstList

from pynotf.data import Value


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


# DATA #################################################################################################################

@unique
class TableIndex(IndexEnum):
    OPERATORS = auto()
    NODES = auto()
    LAYOUTS = auto()


class Xform(NamedTuple):
    """
    The matrix elements are laid out as follows:
      [a c e]                          [sx kx tx]
      [b d f]   which corresponds to   [ky sy ty]
      [0 0 1]                          [ 0  0  1]
    """
    a: float = 1
    b: float = 0
    c: float = 0
    d: float = 1
    e: float = 0
    f: float = 0


class Expression:
    def __init__(self, source: str):
        self._source: str = source
        self._code: CodeType = compile(self._source, '<string>', mode='eval')  # might raise a SyntaxError

    def execute(self, scope: Optional[Dict[str, Any]] = None) -> Any:
        return eval(self._code, {}, scope or {})

    def __repr__(self) -> str:
        return f'Expression("{self._source}")'

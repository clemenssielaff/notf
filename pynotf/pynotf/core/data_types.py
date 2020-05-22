from __future__ import annotations

from enum import IntEnum, unique
from typing import Tuple, Any, List, NamedTuple, Optional, Dict
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
class EmitterStatus(IntEnum):
    """
    Emitters start out IDLE and change to EMITTING whenever they are emitting a ValueSignal to connected Receivers.
    If an Emitter tries to emit but is already in EMITTING state, we know that we have caught a cyclic dependency.
    To emit a FailureSignal or a CompletionSignal, the Emitter will switch to the respective FAILING or COMPLETING
    state. Once it has finished its `_fail` or `_complete` methods, it will permanently change its state to
    COMPLETED or FAILED and will not emit anything again.

        --> IDLE <-> EMITTING
              |
              +--> FAILING --> FAILED
              |
              +--> COMPLETING --> COMPLETE
    """
    EMITTING = 0  # 0-2 matches the corresponding OperatorVtableIndex
    FAILING = 1
    COMPLETING = 2
    IDLE = 3  # follow-up status is active (EMITTING, FAILING, COMPLETING) + 3
    FAILED = 4
    COMPLETED = 5

    def is_active(self) -> bool:
        """
        There are 3 active and 3 passive states:
            * IDLE, FAILED and COMPLETED are passive
            * EMITTING, FAILING and COMPLETING are active
        """
        return self < EmitterStatus.IDLE

    def is_completed(self) -> bool:
        """
        Every status except IDLE and EMITTING can be considered "completed".
        """
        return not (self == EmitterStatus.IDLE or self == EmitterStatus.EMITTING)


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

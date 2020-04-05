from __future__ import annotations

from typing import NamedTuple
from enum import Enum, IntEnum, unique, auto

from pynotf.data import RowHandle


class Error(NamedTuple):
    """
    I am going to try to use these hand-crafted Error objects in place of Exceptions wherever possible.
    """
    emitter: RowHandle  # the Emitter that caught the error
    kind: Kind  # kind of error caught in the circuit
    message: str  # error message

    class Kind(Enum):
        NO_DAG = 1  # Exception raised when a cyclic dependency was detected in the graph.
        WRONG_VALUE_SCHEMA = 2
        USER_CODE_EXCEPTION = 3

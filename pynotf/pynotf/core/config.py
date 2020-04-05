from enum import IntEnum, auto, unique


@unique
class TableIndex(IntEnum):
    """
    All Tables of the Application are stored in a single array that makes up the "Application State".
    RowHandles contain the index of the Table they are referring to, so we need a central truth of what Table has what
    index in the Application.
    """
    EMITTERS = auto()
    OPERATORS = auto()
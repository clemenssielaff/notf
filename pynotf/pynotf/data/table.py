from __future__ import annotations
from typing import List, Type, Dict, Tuple, Any, TypeVar, Union, Callable, Optional

from pyrsistent import CheckedPVector as ConstList, CheckedPMap as ConstMap, PRecord as ConstNamedTuple, field
from pyrsistent.typing import CheckedPVector as ConstListT

from pynotf.extra import color_background


# BASIC DATA ###########################################################################################################

class RowHandle:
    """
    An immutable Handle to a specific row in a specific Table.
    The default constructed Handle is invalid.
    """

    _TABLE_SIZE: int = 8  # in bits
    _INDEX_SIZE: int = 24  # in bits
    _GENERATION_SIZE: int = 32  # in bits

    def __init__(self, table: int = 0, index: int = 0, generation: int = 0):
        assert self._TABLE_SIZE + self._INDEX_SIZE + self._GENERATION_SIZE == 64
        assert 0 <= table < 2 ** self._TABLE_SIZE
        assert 0 <= index < 2 ** self._INDEX_SIZE
        assert 0 <= generation < 2 ** self._GENERATION_SIZE

        self._data: int = (table << self._INDEX_SIZE + self._GENERATION_SIZE
                           | index << self._GENERATION_SIZE
                           | generation)

    @property
    def table(self) -> int:
        return self._data >> self._INDEX_SIZE + self._GENERATION_SIZE

    @property
    def index(self) -> int:
        return (self._data >> self._GENERATION_SIZE) & ((2 ** self._INDEX_SIZE) - 1)

    @property
    def generation(self) -> int:
        return self._data & ((2 ** self._GENERATION_SIZE) - 1)

    def is_valid(self) -> bool:
        """
        The only way for a Handle to be identifiable as invalid (without comparing it against a table) is a generation
        value of zero.
        """
        return self.generation != 0

    def __bool__(self) -> bool:
        return self.is_valid()


class HandleError(Exception):
    """
    Exception raised when an invalid RowHandle is used.
    """
    pass


class RowHandleList(ConstList):
    """
    An immutable list of RowHandles.
    """
    __type__ = RowHandle


class RowHandleMap(ConstMap):
    """
    An immutable map of named RowHandles.
    """
    __key_type__ = str
    __value_type__ = RowHandle


class TableRow(ConstNamedTuple):
    """
    Every table has a built-in column `_gen`, which allows the re-use of a table row without running into problems
    where a RowHandle would reference a row that has since been re-used.
    """
    _gen = field(type=int, mandatory=True)


AnyTable = ConstListT[TableRow]
T = TypeVar('T')


# FUNCTIONS ############################################################################################################

def is_row_active(table: AnyTable, row_index: int) -> bool:
    return row_index < len(table) and table[row_index]._gen > 0


def is_handle_valid(table: AnyTable, handle: RowHandle) -> bool:
    return 0 <= handle.index < len(table) and handle.generation == table[handle.index]._gen


def get_handle_error_reason(table: AnyTable, handle: RowHandle) -> str:
    if not (0 <= handle.index < len(table)):
        return f'Invalid RowHandle index {handle.index} for a table with a highest index of {len(table) - 1}'
    else:
        assert handle.generation != table[handle.index]._gen
        return f'Invalid RowHandle generation {handle.generation} ' \
               f'for a row with a current generation value of {table[handle.index]._gen}'


def add_table_row(table: AnyTable, table_id: int, row: TableRow, free_list: List[int]) -> Tuple[AnyTable, RowHandle]:
    """
    Adds a new row to a table.
    Either re-uses a expired row from the free list or appends a new row.
    :param table: Table to mutate.
    :param table_id: Numeric id of the table, required to construct a valid RowHandle.
    :param row: Row to add to the table. Must match the table's type.
    :param free_list: List of free indices in the table. Faster than searching for negative generation values.
    :return: Mutated table, Handle to the new row.
    """
    if free_list:
        # use any free index
        row_index: int = free_list.pop()
        assert row_index < len(table)

        if table[row_index]._gen == -(2 ** 31):
            # integer overflow!
            generation: int = 1
        else:
            # increment the row's generation
            generation: int = (-table[row_index]._gen) + 1
        assert generation > 0

        # mutate the table
        new_table: AnyTable = table.set(row_index, row.set(_gen=generation))

    else:  # free list is empty
        # create a new row
        row_index: int = len(table)
        generation: int = 1

        # mutate the table
        new_table: AnyTable = table.append(row.set(_gen=generation))

    # create and return a Handle to the new row
    return new_table, RowHandle(table_id, row_index, generation)


def remove_table_row(table: AnyTable, index: int, free_list: List[int]) -> AnyTable:
    """
    Removes the row with the given index from the table.
    :param table: Table to mutate.
    :param index: Index of the row to remove, must be 0 <= index < len(table)
    :param free_list: List of free row indices in the table.
    :return: Mutated table.
    """
    assert index < len(table)
    assert index not in free_list

    # mark the row as removed by negating the generation value
    generation: int = -table[index]._gen  # an int > 0 can always be matched to an int < 0 in Two's complement notation
    assert generation < 0

    # add the removed row to the free list
    free_list.append(index)

    # mutate the table
    return table.set(index, table[index].set(_gen=generation))


def mutate_data(data: T, path: List[Union[int, str]], func: Callable[[T], T]) -> T:
    """
    Mutates an immutable data structure by applying a function to an element identified by the path.
    :param data: Immutable data structure to update.
    :param path: Path of indices/keys within the data to the element to mutate.
    :param func: Mutating function to apply to the element.
    :return: The updated immutable data structure.
    """
    if len(path) == 0:
        return func(data)
    else:
        step: Union[int, str] = path.pop(0)
        return data.set(step, mutate_data(data[step], path, func))


def generate_table_type(description: Dict[str, Type]) -> Type:
    """
    Creates a Table type, an immutable named tuple with a typed row for each entry.
    :param description: Row name -> Row type (row names are converted to lowercase).
    """
    # create the row type
    row_type: Type = type('_Row', (TableRow,), dict())
    for row_name, row_value_type in description.items():
        assert isinstance(row_name, str)
        row_name = row_name.lower()
        row_type._precord_fields[row_name] = field(type=row_value_type, mandatory=True)
        row_type._precord_mandatory_fields.add(row_name)

    # create the table type
    table_type: Type = type('_Table', (ConstList,), dict())
    table_type.__type__ = row_type
    table_type._checked_types = (row_type,)
    table_type.Row = row_type  # for convenient access

    return table_type


# API ##################################################################################################################

class Storage:

    def __init__(self, *tables: Dict[str, Type]):
        self._table_types: List[Type] = [generate_table_type(table) for table in tables]
        self._data: ConstListT[AnyTable] = ConstList(type_() for type_ in self._table_types)
        self._tables: List[Table] = [Table(self, table_id, type_) for table_id, type_ in enumerate(self._table_types)]

    def __getitem__(self, index: int) -> Table:
        return self._tables[index]

    def __len__(self) -> int:
        return len(self._tables)

    def get_data(self):
        return self._data

    def set_data(self, data):
        self._data = data
        for table in self._tables:
            table._refresh_free_list()


class Table:
    class Accessor:
        def __init__(self, table: Table, data, row_index: int):
            self._table: Table = table
            self._data = data
            self._row_index: int = row_index

        def __getitem__(self, name: str) -> Any:
            return self._data[self._row_index][name]

        def __setitem__(self, name: str, value: Any) -> None:
            storage = self._table._storage
            storage._data = mutate_data(storage._data, [self._table._id, self._row_index],
                                        lambda row: row.set(name, value))

    def __init__(self, storage: Storage, table_id: int, table_type: Type):
        self._storage: Storage = storage
        self._id: int = table_id
        self._free_list: List[int] = []
        self._row_type: Type[TableRow] = table_type.Row

    @property
    def _table_data(self):
        return self._storage._data[self._id]

    def _refresh_free_list(self):
        self._free_list = [index for index in range(len(self._table_data)) if self._table_data[index]._gen < 0]

    def get_handle(self, row_index: int) -> RowHandle:
        table = self._table_data
        if row_index < len(table):
            return RowHandle(self._id, row_index, table[row_index]['_gen'])
        else:
            return RowHandle(self._id, row_index)  # invalid handle

    def is_row_active(self, row_index: int) -> bool:
        return is_row_active(self._table_data, row_index)

    def is_handle_valid(self, row_handle: RowHandle) -> bool:
        return row_handle.table == self._id and is_handle_valid(self._table_data, row_handle)

    def check_handle(self, row_handle: RowHandle) -> Optional[str]:
        table = self._table_data
        if row_handle.table != self._id:
            return f'Cannot use Handle for table {row_handle.table} with table {self._id}'
        return None if is_handle_valid(table, row_handle) else get_handle_error_reason(table, row_handle)

    def add_row(self, **kwargs) -> RowHandle:
        new_table, row = add_table_row(self._table_data, self._id, self._row_type(_gen=0, **kwargs), self._free_list)
        assert row.is_valid()
        self._storage._data = self._storage._data.set(self._id, new_table)
        return row

    def remove_row(self, row: Union[int, RowHandle]) -> None:
        table = self._table_data
        if isinstance(row, RowHandle):
            if row.table != self._id:
                raise HandleError(f'Cannot use Handle for table {row.table} with table {self._id}')
            if not is_handle_valid(table, row):
                raise HandleError(get_handle_error_reason(table, row))
            row_index: int = row.index
        else:
            row_index: int = row
        self._storage._data = self._storage._data.set(self._id, remove_table_row(table, row_index, self._free_list))

    def keys(self) -> List[str]:
        return list(self._row_type._precord_fields.keys())[1:]

    def __getitem__(self, row: Union[int, RowHandle]) -> Accessor:
        """
        The "int" overload should not be part of the public API as it allows unchecked modification of the data.
        :param row: Index or Handle of the row to return.
        :return: An Accessor to the requested row.
        :raises: HandleError.
        """
        table = self._table_data
        if isinstance(row, RowHandle):
            if row.table != self._id:
                raise HandleError(f'Cannot use Handle for table {row.table} with table {self._id}')
            if not is_handle_valid(table, row):
                raise HandleError(get_handle_error_reason(table, row))
            row_index: int = row.index
        else:
            row_index: int = row
        return self.Accessor(self, self._table_data, row_index)

    def __len__(self) -> int:
        return len(self._table_data)

    def __str__(self) -> str:
        titles: str = ", ".join(self.keys())
        return f'Table("{titles}") with {len(self._table_data)} rows.'

    def to_string(self) -> str:
        table = self._table_data
        titles: List[str] = list(self._row_type._precord_fields.keys())
        rows: List[List[str, ...]] = [
            [f' {number} ', *(f' {row[i]} ' for i in titles)] for number, row in enumerate(table)]

        row_widths: List[int] = [len(str(len(rows))) + 2, *(len(title) + 2 for title in titles)]
        for row in rows:
            for index in range(len(row)):
                row_widths[index] = max(row_widths[index], len(row[index]))

        total_width = sum(row_widths) + len(row_widths) - 1
        lines: List[str] = [
            '─' * total_width,  # top border
            f'Table #{self._id}'.center(total_width),  # table id
            '═' * total_width,  # name separator
            '│'.join(title.capitalize().center(width) for title, width in zip([' # ', *titles], row_widths)),  # titles
            '┼'.join(('─' * width) for width in row_widths),  # title separator
        ]
        for number, row in enumerate(rows):  # entries
            line: str = '│'.join(entry.ljust(width) for entry, width in zip(row, row_widths))
            if table[number]._gen < 0:
                lines.append(color_background(line, (33, 23, 23)))  # deleted row
            else:
                lines.append(line)
        lines.append('─' * total_width)  # bottom border
        lines = [
            f'┌{lines[0]}┐',  # top corners
            *(f'│{line}│' for line in lines[1:-1]),  # left and right border
            f'└{lines[-1]}┘',  # bottom corners
        ]
        return '\n'.join(lines)

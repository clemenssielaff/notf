from __future__ import annotations

from enum import IntEnum, unique
from typing import List, Type, Dict, Tuple, Any, TypeVar, Union, Callable
from logging import warning

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

    def is_null(self) -> bool:
        """
        Handles are a bit like pointers. We know that the nullptr is always invalid, but if a pointer is not null, we
        still don't know if it points to valid memory or not.
        The only way for a Handle to be identifiable as "null" is a generation value of zero.
        """
        return self.generation == 0

    def __eq__(self, other: RowHandle) -> bool:
        if not isinstance(other, RowHandle):
            return NotImplemented
        return self._data == other._data

    def __bool__(self) -> bool:
        return not self.is_null()


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


TableData = ConstListT[TableRow]
T = TypeVar('T')


# FUNCTIONS ############################################################################################################

def add_table_row(table: TableData, table_id: int, row: TableRow, free_list: List[int]) -> Tuple[TableData, RowHandle]:
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
            warning(f'Generation overflow in table {table_id}, row {row_index}')
            generation: int = 1
        else:
            # increment the row's generation
            generation: int = (-table[row_index]._gen) + 1
        assert generation > 0

        # mutate the table
        new_table: TableData = table.set(row_index, row.set(_gen=generation))

    else:  # free list is empty
        # create a new row
        row_index: int = len(table)
        generation: int = 1

        # mutate the table
        new_table: TableData = table.append(row.set(_gen=generation))

    # create and return a Handle to the new row
    return new_table, RowHandle(table_id, row_index, generation)


def remove_table_row(table: TableData, index: int, free_list: List[int]) -> TableData:
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


def generate_table_type(name: str, description: Dict[str, Type]) -> Type:
    """
    Creates a Table type, an immutable named tuple with a typed row for each entry.
    :param name: Name of the Table type (all lowercase).
    :param description: Row name -> Row type (row names are converted to lowercase).
    """
    # type names are capitalized
    name = name.capitalize()

    # create the row type
    row_type: Type = type(f'{name}Row', (TableRow,), dict())
    for row_name, row_value_type in description.items():
        assert isinstance(row_name, str)
        row_name = row_name.lower()
        row_type._precord_fields[row_name] = field(type=row_value_type, mandatory=True)
        row_type._precord_mandatory_fields.add(row_name)

    # create the table type
    table_type: Type = type(name, (ConstList,), dict())
    table_type.__type__ = row_type
    table_type._checked_types = (row_type,)
    table_type.Row = row_type  # for convenient access

    return table_type


# API ##################################################################################################################

class HandleError(Exception):
    """
    Exception raised when an invalid RowHandle is used.
    """

    @unique
    class Reason(IntEnum):
        NO_ERROR = 0
        TABLE_MISMATCH = 1
        INVALID_INDEX = 2
        GENERATION_MISMATCH = 3


class Storage:
    """
    An array of Tables.
    Contains the root of the immutable Application data.
    """

    def __init__(self, **tables: Dict[str, Type]):
        """
        Constructor.
        :param tables: Named Table description in order.
        """
        # normalize names
        description = {name.lower(): table_description for name, table_description in tables.items()}
        if len(tables) != len(description):
            raise NameError('Tables in Storage may not have the same (case insensitive) name')

        self._table_types: List[Type] = [generate_table_type(name, table) for name, table in description.items()]
        self._data: ConstListT[TableData] = ConstList(type_() for type_ in self._table_types)
        self._tables: List[Table] = [Table(self, table_id, type_) for table_id, type_ in enumerate(self._table_types)]

    def __getitem__(self, table_id: int) -> Table:
        """
        Returns a Table by its id.
        :param table_id: Id of the requested Table
        :return: The requested Table.
        :raise IndexError: If the table id does not match a Table in Storage.
        """
        return self._tables[table_id]

    def __len__(self) -> int:
        """
        The number of Tables in Storage.
        """
        return len(self._tables)

    def get_data(self) -> ConstListT[TableData]:
        """
        The complete Application data.
        """
        return self._data

    def set_data(self, data: ConstListT[TableData]) -> None:
        """
        Updates the complete Application data.
        Note that this method does not check the given data for validity, I assume you know what you are doing.
        We would get this for free with the type checking in C++; in Python I cannot be bothered to implement it.
        :param data: New Application data.
        """
        self._data = data
        for table in self._tables:
            table._refresh_free_list()


class Table:
    """
    Wrapper around a single Table in Storage.
    Allows the interaction with a Table as if it was mutable.
    """

    class Accessor:
        """
        Access to a single row in the Table.
        Object returned when you access a Table via the [] operator.
        Encapsulated both read and write operations, making them appear as if you were working on regular, mutable data.
        """

        def __init__(self, table: Table, data: TableData, row_index: int):
            """
            Constructor.
            :param table: The accessed Table.
            :param data: The current table data.
            :param row_index: Index of the accessed row.
            """
            # Unlike the Table, the Accessor _does_ store the table data itself, because we expect it to be short-lived.
            # An r-value, essentially. There should be no way to modify the data from somewhere else while it is being
            # accessed by this Accessor.
            # This of course assumes that the Storage is only accessed on a single thread.
            self._table: Table = table
            self._data: TableData = data
            self._row_index: int = row_index

        def __getitem__(self, column: str) -> Any:
            """
            Return the contents of a cell in the table by column name.
            :param column: Name of the column to read.
            :return: Contents of the cell identified by the row of the Accessor and the given column.
            :raise KeyError: If the column is not part of the Table.
            """
            return self._data[self._row_index][column]

        def __setitem__(self, column: str, value: Any) -> None:
            """
            Modifies the row, and continues the cascade of modification up to the Storage.
            :param column: Name of the column to update.
            :param value: New value of the cell.
            :raise KeyError: If the column is not part of the Table.
            """
            self._table._storage._data = mutate_data(
                self._table._storage._data, [self._table.get_id(), self._row_index], lambda row: row.set(column, value))

    def __init__(self, storage: Storage, table_id: int, table_type: Type):
        """
        Constructor.
        :param storage: Storage managing the data of this Table. 
        :param table_id: The numeric id of this Table.
        :param table_type: The type used to store the Table's data.
        """
        self._storage: Storage = storage
        self._id: int = table_id
        self._table_type: Type = table_type
        self._free_list: List[int] = []

    def __getitem__(self, handle: RowHandle) -> Accessor:
        """
        The "int" overload should not be part of the public API as it allows unchecked modification of the data.
        :param handle: Handle of the row to access.
        :return: An Accessor to the requested row.
        :raises HandleError: If the given Handle is invalid.
        """
        table_data: TableData = self._get_table_data()
        self._assert_handle(handle, table_data)
        return self.Accessor(self, table_data, handle.index)

    def __len__(self) -> int:
        """
        The current number of rows in this Table.
        """
        return len(self._get_table_data())

    def __str__(self) -> str:
        """
        Human-readable short description of this Table for debugging.
        """
        return f'Table "{self.get_name()}" ({len(self)} rows)'

    def get_id(self) -> int:
        """
        Numeric id of this Table in Storage.
        Can be used to request this Table from Storage.
        """
        return self._id

    def get_name(self) -> str:
        """
        Human-readable name of this Table for debugging.
        """
        return self._table_type.__name__

    def get_columns(self) -> List[str]:
        """
        The names of all (public) columns of this Table.
        Note that this excludes the generation value.
        """
        return list(self._table_type.Row._precord_fields.keys())[1:]

    def is_handle_valid(self, handle: RowHandle) -> bool:
        """
        Tests if a given Handle can be used to access this Table.
        :param handle: Handle to test.
        :return: True iff the Handle can be used to access this Table.
        """
        return not self._get_handle_error(handle, self._get_table_data())

    def add_row(self, **kwargs) -> RowHandle:
        """
        Adds a new row to this Table.
        :param kwargs: Per-column arguments used to construct the new Row.
        :return: A Handle to the new Row.
        """
        modified_table, row_handle = add_table_row(
            self._get_table_data(), self._id, self._table_type.Row(_gen=0, **kwargs), self._free_list)
        assert not row_handle.is_null()
        self._storage._data = self._storage._data.set(self._id, modified_table)
        return row_handle

    def remove_row(self, handle: RowHandle) -> None:
        """
        Removes an existing row from this Table.
        :param handle: Handle of the row to remove.
        :raise HandleError: If the given Handle is invalid.
        """
        table_data: TableData = self._get_table_data()
        self._assert_handle(handle, table_data)
        self._storage._data = self._storage._data.set(
            self._id, remove_table_row(table_data, handle.index, self._free_list))

    def to_string(self) -> str:
        """
        Returns a nicely formatted UTF-8 table representation of this Table and its contents.
        """
        table = self._get_table_data()
        titles: List[str] = list(self._table_type.Row._precord_fields.keys())
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

    def _get_table_data(self) -> TableData:
        """
        The table data cannot be stored, its current value has to be read from the Storage as it could have changed.
        """
        return self._storage._data[self._id]

    def _refresh_free_list(self):
        """
        Refreshes the "Free List" of this Table after its data has been updated by the Storage.
        """
        table_data: TableData = self._get_table_data()
        self._free_list = [index for index in range(len(table_data)) if table_data[index]._gen < 0]

    def _get_handle_error(self, row_handle: RowHandle, table_data: TableData) -> HandleError.Reason:
        """
        Internal test whether a given handle is valid for this Table and its current data.
        :param row_handle: Handle to test.
        :param table_data: The current data of this Table. Could be requested from the Storage, but since since this a
            private method we can be certain that the caller has means to do that outside of this function. This way we
            save a lookup in the Storage.
        :return: A HandleError.Reason. Is NO_ERROR if the handle is valid.
        """
        if row_handle.table != self._id:
            return HandleError.Reason.TABLE_MISMATCH
        elif not (0 <= row_handle.index < len(table_data)):
            return HandleError.Reason.INVALID_INDEX
        elif row_handle.generation != table_data[row_handle.index]._gen:
            return HandleError.Reason.GENERATION_MISMATCH
        else:
            return HandleError.Reason.NO_ERROR

    def _assert_handle(self, handle: RowHandle, table_data: TableData) -> None:
        """
        This does nothing but throw an exception if the given Handle is invalid.
        :param handle: Handle to validate.
        :param table_data: Current data of this Table.
        """
        error: HandleError.Reason = self._get_handle_error(handle, table_data)
        if error == HandleError.Reason.NO_ERROR:
            return
        elif error == HandleError.Reason.TABLE_MISMATCH:
            raise HandleError(f'Cannot use a Handle for Table {handle.table} '
                              f'to access Table {self._id} ({self.get_name()})')
        elif error == HandleError.Reason.INVALID_INDEX:
            raise HandleError(f'Invalid Handle index {handle.index} '
                              f'for a Table with a highest index of {len(table_data) - 1}')
        else:
            assert error == HandleError.Reason.GENERATION_MISMATCH
            raise HandleError(f'Invalid Handle generation {handle.generation} '
                              f'for a row with a current generation value of {table_data[handle.index]._gen}')

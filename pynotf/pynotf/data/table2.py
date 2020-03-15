from __future__ import annotations
from typing import List, Optional, Type, Dict, Tuple, Any, TypeVar, Union, Callable

from pyrsistent import CheckedPVector as ConstList, PRecord as ConstNamedTuple, field
from pyrsistent.typing import CheckedPVector as ConstListT

T = TypeVar('T')


class RowHandle(ConstNamedTuple):
    """
    A reference to a specific row in a Table.
    """
    index = field(type=int, mandatory=True, invariant=lambda x: (x >= 0, 'Row index must be >= 0'))
    _gen = field(type=int, mandatory=True, invariant=lambda x: (x > 0, 'Generation must be > 0'))


class TableRow(ConstNamedTuple):
    """
    Every table has a built-in column `_gen`, which allows the re-use of a table row without running into problems
    where a RowHandle would reference a row that has since been re-used.
    """
    _gen = field(type=int, mandatory=True)


AnyTable = ConstListT[TableRow]


def add_table_row(table: AnyTable, row: TableRow, free_list: List[int], refcounts: Optional[List[int]] = None) \
        -> Tuple[AnyTable, RowHandle]:
    """
    Adds a new row to a table.
    Either re-uses a expired row from the free list or appends a new row.
    :param table: Table to mutate.
    :param row: Row to add to the table. Must match the table's type.
    :param free_list: List of free indices in the table. Faster than searching for negative generation values.
    :param refcounts: Optional reference counts for each row in the table.
    :return: Mutated table, Handle to the new row.
    """
    assert refcounts is None or len(table) == len(refcounts)

    if free_list:
        # use any free index
        index: int = free_list.pop()
        assert index < len(table)

        # increment the row's generation
        generation: int = (-table[index].generation) + 1
        assert generation > 0

        # initialize the refcount of the new row
        if refcounts is not None:
            assert refcounts[index] == 0
            refcounts[index] = 1

        # mutate the table
        new_table: AnyTable = table.set(index, row.set(_gen=generation))

    else:  # free list is empty
        # create a new row
        index: int = len(table)
        generation: int = 1

        # initialize the refcount of the new row
        if refcounts is not None:
            refcounts.append(1)

        # mutate the table
        new_table: AnyTable = table.append(row.set(_gen=generation))

    # create and return a Handle to the new row
    return new_table, RowHandle(index=index, _gen=generation)


def remove_table_row(table: AnyTable, index: int, free_list: List[int],
                     refcounts: Optional[List[int]] = None) -> AnyTable:
    """
    Removes the row with the given index from the table.
    :param table: Table to mutate.
    :param index: Index of the row to remove, must be 0 <= index < len(table)
    :param free_list: List of free row indices in the table.
    :param refcounts: Optional reference counts for each row in the table.
    :return: Mutated table.
    """
    assert index < len(table)
    assert index not in free_list
    assert refcounts is None or (len(table) == len(refcounts) and refcounts[index] == 0)

    # mark the row as removed by negating the generation value
    generation: int = -table[index]._gen
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
    :param name: Name of the Table type (is capitalized).
    :param description: Row name -> Row type (row names are converted to lowercase).
    """
    # types are capitalized
    assert isinstance(name, str)
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


def generate_storage_type(tables: Dict[str, Type]) -> Type:
    # generate tables class, containing all the tables
    storage_type: Type = type('StorageData', (ConstNamedTuple,), dict())
    for table_name, table_type in tables.items():
        storage_type._precord_fields[table_name] = field(type=table_type, mandatory=True)
        storage_type._precord_mandatory_fields.add(table_name)
        setattr(storage_type, table_name, table_type)  # for convenient access

    return storage_type


def color_text(text: str, rgb: Tuple[int, int, int]) -> str:
    return f'\033[38;2;{rgb[0]};{rgb[1]};{rgb[2]}m{text}\033[0m'


def color_background(text: str, rgb: Tuple[int, int, int]) -> str:
    return f'\033[48;2;{rgb[0]};{rgb[1]};{rgb[2]}m{text}\033[0m'


########################################################################################################################

class Storage:

    def __init__(self, description: Dict[str, Dict[str, Type]]):
        description = {name.lower(): table_description for name, table_description in description.items()}

        if len(description) != len(set(description.keys())):
            raise NameError('No duplicate names (case insensitive) allowed')

        self._table_types: Dict[str, Type] = {
            name: generate_table_type(name, table) for name, table in description.items()}
        self._data_type: Type = generate_storage_type(self._table_types)
        self._data = self._data_type(**{name: type_() for name, type_ in self._table_types.items()})
        self._tables: Dict[str, Table] = {name: Table(self, name, type_) for name, type_ in self._table_types.items()}

    def __getitem__(self, name: str) -> Table:
        return self._tables[name]

    def get_data(self):
        return self._data

    def set_data(self, data):
        assert isinstance(data, self._data_type)
        self._data = data
        for table in self._tables.values():
            table._refresh_free_list()


class Table:
    class Accessor:
        def __init__(self, table: Table, data, index: int):
            self._table: Table = table
            self._data = data
            self._index: int = index

        def __getitem__(self, name: str) -> Any:
            return self._data[self._index][name]

        def __setitem__(self, name: str, value: Any) -> None:
            storage = self._table._storage
            storage._data = mutate_data(storage._data, [self._table._name, self._index],
                                        lambda row: row.set(name, value))

    def __init__(self, storage: Storage, name: str, table_type: Type):
        self._storage: Storage = storage
        self._name: str = name
        self._row_type: Type[TableRow] = table_type.Row
        self._free_list: List[int] = []

    @property
    def _table(self):
        return self._storage._data[self._name]

    def _refresh_free_list(self):
        # TODO: refresh free list
        pass

    def add_row(self, **kwargs) -> RowHandle:
        updated_table, new_row = add_table_row(self._table, self._row_type(_gen=0, **kwargs), self._free_list)
        assert new_row._gen != 0
        self._storage._data = self._storage._data.set(self._name, updated_table)
        return new_row

    def remove_row(self, index: int) -> None:
        self._storage._data = self._storage._data.set(
            self._name, remove_table_row(self._table, index, self._free_list))

    def __getitem__(self, index: int) -> Accessor:
        return self.Accessor(self, self._table, index)

    def __len__(self) -> int:
        return len(self._table)

    def __str__(self) -> str:
        table = self._table
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
            self._name.upper().center(total_width),  # table name
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


###########################################

def main():
    storage = Storage(dict(
        widgets=dict(
            name=str,
        ),
        properties=dict(
            name=str,
            value=int,
        )
    ))
    widgets = storage['widgets']
    widgets.add_row(name="derbe")
    widgets.add_row(name="okay")
    widgets.add_row(name="not so great")
    widgets.add_row(name="in the house")
    print(storage['widgets'])

    widgets[1]["name"] = 'Very okay'
    data_1 = storage.get_data()
    print(storage['widgets'])

    widgets.remove_row(2)
    print(storage['widgets'])

    # storage.set_data(data_1)
    # print(storage['widgets'])


if __name__ == '__main__':
    main()

from typing import Type, List

from pyrsistent import CheckedPVector as ConstList, PRecord as ConstNamedTuple, field


class Handle(ConstNamedTuple):
    """
    A reference to a specific row in a Table.
    """
    index = field(type=int, mandatory=True, invariant=lambda x: (x >= 0, 'Row index must be >= 0'))
    generation = field(type=int, mandatory=True, invariant=lambda x: (x > 0, 'Generation must be > 0'))


def create_table(column_type: Type[ConstNamedTuple]) -> ConstList:
    """
    Creates a new table instance containing columns of the given type.
    :param column_type: Type to store in the table's columns.
    :return: New empty table.
    """
    class Table(ConstList):
        class Row(ConstNamedTuple):
            generation = field(type=int, mandatory=True)
            row = field(type=column_type, mandatory=True)

        __type__ = Row

    return Table()


def add_row(self, row: ConstNamedTuple) -> Handle:
    if len(self._free_list) == 0:
        # create a new row
        index: int = len(self._rows)
        generation: int = 1

        # mutate the table
        self._rows = self._rows.append(self._rows.Row(generation=generation, row=row))
    else:
        # use any free index
        index: int = self._free_list.pop()
        assert index < len(self._rows)

        # increment the row's generation
        generation: int = (-self._rows[index].generation) + 1
        assert generation > 0

        # mutate the table
        self._rows = self._rows.set(index, self._rows.Row(generation=generation, row=row))

    # create and return a Handle to the new row
    return self.Handle(index=index, generation=generation)

def update_row(self, index: int, row: ConstNamedTuple):
    assert index < len(self._rows)
    assert self._rows[index].generation > 0
    assert index not in self._free_list

    # mutate the table
    self._rows = self._rows.set(index, self._rows.Row(generation=self._rows[index].generation, row=row))

def remove_row(self, index: int):
    assert index < len(self._rows)
    assert index not in self._free_list

    # mark the row as removed by negating the generation value
    generation: int = -self._rows[index].generation
    assert generation < 0

    # add the removed row to the free list
    self._free_list.append(index)

    # mutate the table
    self._rows = self._rows.set(index, self._rows.Row(generation=generation, row=self._rows[index].row))

def is_handle_valid(self, handle: Handle) -> bool:
    if not (0 <= handle.index < len(self._rows)):
        return False
    if handle.generation <= 0:
        return False
    if handle.generation != self._rows[handle.index].generation:
        return False
    return True


#######################################3

# self._free_list = [index for index in range(len(self._rows)) if self._rows[index].generation < 0]

class WidgetTable(Table):
    class Row(ConstNamedTuple):
        name = field(type=str, mandatory=True)

    def __init__(self):
        super().__init__(self._create_rows(self.Row))


def main():
    widget_table: WidgetTable = WidgetTable()
    snapshot1 = widget_table.save()
    widget_table.add_row(WidgetTable.Row(name="Widget1"))
    widget_table.add_row(WidgetTable.Row(name="Widget2"))
    snapshot2 = widget_table.save()
    widget_table.add_row(WidgetTable.Row(name="Widget3"))
    snapshot3 = widget_table.save()
    widget_table.remove_row(1)
    snapshot4 = widget_table.save()
    widget_table.update_row(0, WidgetTable.Row(name="Replacement1"))
    snapshot5 = widget_table.save()

    print(snapshot1)
    print(snapshot2)
    print(snapshot3)
    print(snapshot4)
    print(snapshot5)


main()

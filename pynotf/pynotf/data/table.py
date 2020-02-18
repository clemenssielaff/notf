from typing import Type, List, Tuple, Optional, Any

from pyrsistent import CheckedPVector as ConstList, CheckedPMap as ConstMap, PRecord as ConstNamedTuple, field


class Handle(ConstNamedTuple):
    """
    A reference to a specific row in a Table.
    """
    index = field(type=int, mandatory=True, invariant=lambda x: (x >= 0, 'Row index must be >= 0'))
    generation = field(type=int, mandatory=True, invariant=lambda x: (x > 0, 'Generation must be > 0'))


class Handles(ConstList):
    """
    An immutable list of Handles.
    """
    __type__ = Handle


class HandleMap(ConstMap):
    """
    An immutable map of named Handles.
    """
    __key_type__ = str
    __value_type__ = Handle


def create_table_type(column_type: Type[ConstNamedTuple]):
    """
    Creates a new table type containing columns of the given type.
    Every table has a built-in column `generation`, which allows the re-use of a table row without running into problems
    where a Handle would reference a row that has since been re-used.
    :param column_type: Type to store in the table's columns.
    :return: New table type.
    """

    class Table(ConstList):
        class Row(ConstNamedTuple):
            generation = field(type=int, mandatory=True)
            data = field(type=column_type, mandatory=True)

        __type__ = Row

    return Table


def add_row(table, data, free_list: List[int], refcounts: Optional[List[int]] = None) -> Tuple[Any, Handle]:
    """
    Adds a new row to a table.
    Either re-uses a expired row from the free list or appends a new row.
    :param table: Table to modify.
    :param data: Data to add to the table, must be of the Table's `Row` type.
    :param free_list: List of free indices in the table. Faster than searching for negative generation values.
    :param refcounts: Optional reference counts for each row in the table.
    :return: Modified table, Handle to the new row.
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
        new_table = table.set(index, table.Row(generation=generation, data=data))

    else:  # free list is empty
        # create a new row
        index: int = len(table)
        generation: int = 1

        # initialize the refcount of the new row
        if refcounts is not None:
            refcounts.append(1)

        # mutate the table
        new_table = table.append(table.Row(generation=generation, data=data))

    # create and return a Handle to the new row
    return new_table, Handle(index=index, generation=generation)


def update_row(table, index: int, data, free_list: List[int], refcounts: Optional[List[int]] = None) -> Any:
    assert index < len(table)
    assert index not in free_list
    assert refcounts is None or (len(table) == len(refcounts) and refcounts[index] > 0)

    # mutate the table
    generation: int = table[index].generation
    assert generation > 0
    return table.set(index, table.Row(generation=generation, data=data))


def remove_row(table, index: int, free_list: List[int], refcounts: Optional[List[int]] = None) -> Any:
    assert index < len(table)
    assert index not in free_list
    assert refcounts is None or (len(table) == len(refcounts) and refcounts[index] == 0)

    # mark the row as removed by negating the generation value
    generation: int = -table[index].generation
    assert generation < 0

    # add the removed row to the free list
    free_list.append(index)

    # mutate the table
    return table.set(index, table.Row(generation=generation, data=table[index].data))


def increase_refcount(table, index: int, refcounts: List[int]) -> int:
    assert index < len(table)
    assert len(table) == len(refcounts) and refcounts[index] > 0

    new_refcount: int = refcounts[index] + 1
    refcounts[index] = new_refcount
    return new_refcount


def decrease_refcount(table, index: int, refcounts: List[int]) -> int:
    assert index < len(table)
    assert len(table) == len(refcounts) and refcounts[index] > 0

    new_refcount: int = refcounts[index] - 1
    refcounts[index] = new_refcount
    return new_refcount


def is_handle_valid(table, handle: Handle) -> bool:
    if not (0 <= handle.index < len(table)):
        return False
    if handle.generation <= 0:
        return False
    if handle.generation != table[handle.index].generation:
        return False
    return True


## DATA ################################################################################################################

class WidgetRow(ConstNamedTuple):
    name = field(type=str, mandatory=True)
    parent = field(type=Handle, mandatory=True)
    children = field(type=HandleMap, mandatory=True)
    properties = field(type=HandleMap, mandatory=True)


class PropertyRow(ConstNamedTuple):
    name = field(type=str, mandatory=True)
    value = field(type=int, mandatory=True)


class ApplicationData(ConstNamedTuple):
    widgets = field(type=create_table_type(WidgetRow), mandatory=True)
    properties = field(type=create_table_type(PropertyRow), mandatory=True)


## HANDLER #############################################################################################################


class Widget:
    def __init__(self, handle: Handle):
        self._handle: Handle = handle

    @property
    def handle(self) -> Handle:
        return self._handle


class Application:
    def __init__(self):
        WidgetTable = create_table_type(WidgetRow)
        self._data: ApplicationData = ApplicationData(
            widgets=WidgetTable([WidgetTable.Row(
                generation=1,
                data=WidgetRow(
                    name='[root]',
                    parent=Handle(index=0, generation=1),
                    children=HandleMap(),
                    properties=HandleMap(),
                ))]),
            properties=create_table_type(PropertyRow)(),
        )

        self._widget_free_list: List[int] = []
        self._property_free_list: List[int] = []

    def get_state(self) -> ApplicationData:
        return self._data

    def is_valid(self, obj: Any) -> bool:
        if isinstance(obj, Widget):
            return is_handle_valid(self._data.widgets, obj.handle)
        else:
            assert False

    def add_widget(self, name: str, parent: Optional[Handle] = None) -> Handle:
        if parent is None:  # parent defaults to root if not set
            parent = Handle(index=0, generation=1)

        assert is_handle_valid(self._data.widgets, parent)
        assert name not in self._data.widgets[parent.index].data.children

        widget_table, new_widget = add_row(self._data.widgets,
                                           WidgetRow(name=name, parent=parent, children=HandleMap(),
                                                     properties=HandleMap()),
                                           self._widget_free_list)

        # TODO: update widget cell function?
        self._data = self._data.set(
            widgets=widget_table.set(
                parent.index, widget_table.Row(
                    generation=parent.generation,
                    data=widget_table[parent.index].data.set(
                        children=widget_table[parent.index].data.children.set(name, new_widget)))))

        return new_widget

    def remove_widget(self, widget: Handle):
        # find widget
        # recursively remove all children first
        # remove widget
        pass

def main():
    app: Application = Application()
    state1 = app.get_state()

    widget1 = app.add_widget("widget1")
    state2 = app.get_state()

    widget2 = app.add_widget('widget2', widget1)
    state3 = app.get_state()

    print(state1)
    print(state2)
    print(state3)


main()

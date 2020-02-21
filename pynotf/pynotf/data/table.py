from __future__ import annotations
from typing import List, Tuple, Optional, Union, TypeVar, Callable
from json import dumps as pretty_print

from pyrsistent import CheckedPVector as ConstList, CheckedPMap as ConstMap, PRecord as ConstNamedTuple, field
from pyrsistent.typing import CheckedPVector as ConstListT

T = TypeVar('T')


class RowHandle(ConstNamedTuple):
    """
    A reference to a specific row in a Table.
    """
    index = field(type=int, mandatory=True, invariant=lambda x: (x >= 0, 'Row index must be >= 0'))
    generation = field(type=int, mandatory=True, invariant=lambda x: (x > 0, 'Generation must be > 0'))


class RowHandles(ConstList):
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


class BuiltinColumns(ConstNamedTuple):
    """
    Every table has a built-in column `generation`, which allows the re-use of a table row without running into problems
    where a RowHandle would reference a row that has since been re-used.
    """
    generation = field(type=int, mandatory=True)


Table = ConstListT[BuiltinColumns]


def add_row(table: Table, row: BuiltinColumns, free_list: List[int], refcounts: Optional[List[int]] = None) \
        -> Tuple[Table, RowHandle]:
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
        new_table: Table = table.set(index, row.set(generation=generation))

    else:  # free list is empty
        # create a new row
        index: int = len(table)
        generation: int = 1

        # initialize the refcount of the new row
        if refcounts is not None:
            refcounts.append(1)

        # mutate the table
        new_table: Table = table.append(row.set(generation=generation))

    # create and return a Handle to the new row
    return new_table, RowHandle(index=index, generation=generation)


def remove_row(table: Table, index: int, free_list: List[int], refcounts: Optional[List[int]] = None) -> Table:
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
    generation: int = -table[index].generation
    assert generation < 0

    # add the removed row to the free list
    free_list.append(index)

    # mutate the table
    return table.set(index, table[index].set(generation=generation))


def mutate(data: T, path: List[Union[int, str]], func: Callable[[T], T]) -> T:
    if len(path) == 0:
        return func(data)
    step: Union[int, str] = path.pop(0)
    return data.set(step, mutate(data[step], path, func))


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


def is_handle_valid(table, handle: RowHandle) -> bool:
    if not (0 <= handle.index < len(table)):
        return False
    if handle.generation <= 0:
        return False
    if handle.generation != table[handle.index].generation:
        return False
    return True


## DATA ################################################################################################################

class WidgetRow(BuiltinColumns):
    name = field(type=str, mandatory=True)
    parent = field(type=RowHandle, mandatory=True)
    children = field(type=RowHandleMap, mandatory=True)
    properties = field(type=RowHandleMap, mandatory=True)


class PropertyRow(BuiltinColumns):
    name = field(type=str, mandatory=True)
    value = field(type=int, mandatory=True)


class WidgetTable(ConstList):
    __type__ = WidgetRow


class PropertyTable(ConstList):
    __type__ = PropertyRow


class ApplicationData(ConstNamedTuple):
    widgets = field(type=WidgetTable, mandatory=True)
    properties = field(type=PropertyTable, mandatory=True)


## APPLICATION #########################################################################################################


class Application:
    def __init__(self):
        self._data: ApplicationData = ApplicationData(
            widgets=WidgetTable([WidgetRow(
                generation=1,
                name='<root>',
                parent=RowHandle(index=0, generation=1),
                children=RowHandleMap(),
                properties=RowHandleMap(),
            )]),
            properties=PropertyTable(),
        )

        self._widget_free_list: List[int] = []
        self._property_free_list: List[int] = []

    def get_state(self) -> ApplicationData:
        return self._data

    @staticmethod
    def is_widget_root(widget: RowHandle) -> bool:
        return widget.index == 0

    def get_widget_name(self, widget: RowHandle) -> str:
        assert is_handle_valid(self._data.widgets, widget)
        return self._data.widgets[widget.index].name

    def get_widget_path(self, widget: RowHandle) -> str:
        names: List[str] = []
        while widget.index != 0:
            names.append(self.get_widget_name(widget))
            widget = self._data.widgets[widget.index].parent
        return f"/{'/'.join(reversed(names))}"

    def add_widget(self, name: str, parent: Optional[RowHandle] = None) -> RowHandle:
        if parent is None:  # parent defaults to root if not set
            parent = RowHandle(index=0, generation=1)

        assert is_handle_valid(self._data.widgets, parent)
        assert name not in self._data.widgets[parent.index].children

        widget_table, new_widget = add_row(
            self._data.widgets,
            WidgetRow(generation=0, name=name, parent=parent, children=RowHandleMap(), properties=RowHandleMap()),
            self._widget_free_list)
        assert new_widget.generation != 0

        self._data = self._data.set(
            widgets=mutate(widget_table, [parent.index, "children"], lambda x: x.set(name, new_widget)))

        return new_widget

    def remove_widget(self, widget: RowHandle):
        assert not self.is_widget_root(widget)  # cannot remove the root widget
        assert is_handle_valid(self._data.widgets, widget)

        def remove_recursive(_widget: RowHandle):
            assert is_handle_valid(self._data.widgets, _widget)

            # remove the widget
            self._data.set(widgets=remove_row(self._data.widgets, _widget.index, self._widget_free_list))

            # recursively remove all children first, depth-first
            child_widgets: List[RowHandle] = list(self._data.widgets[_widget.index].children.values())
            for child_widget in child_widgets:
                remove_recursive(child_widget)

        # update the parent's children map
        parent: RowHandle = self._data.widgets[widget.index].parent
        assert is_handle_valid(self._data.widgets, parent)
        self._data = mutate(self._data, ["widgets", parent.index, "children"],
                            lambda x: x.remove(self.get_widget_name(widget)))

        # remove the widget itself and all children recursively
        remove_recursive(widget)

    def print_widgets(self, root: RowHandle = RowHandle(index=0, generation=1)):
        def recurse(next_widget: RowHandle):
            return {
                self._data.widgets[child_widget.index].name: recurse(child_widget) for child_widget in
                self._data.widgets[next_widget.index].children.values()
            }

        print(pretty_print({self._data.widgets[root.index].name: recurse(root)}, indent=2))

    # TODO: print table


def main():
    app: Application = Application()
    w1 = app.add_widget("w1")
    w1_w1 = app.add_widget('w1', w1)
    app.add_widget('w2', w1)  # w1_w2
    w1_w3 = app.add_widget('w3', w1)
    app.add_widget('w1', w1_w1)  # w1_w1_w1
    app.add_widget('w2', w1_w1)  # w1_w1_w2
    app.add_widget('w1', w1_w3)  # w1_w3_w1
    w1_w3_w2 = app.add_widget('w2', w1_w3)  # w1_w3_w2

    app._data = mutate(app._data, ["widgets", w1_w3_w2.index], lambda x: x.set(name="fantastic"))

    app.print_widgets()
    app.remove_widget(w1_w3)
    app.print_widgets()
    app.remove_widget(w1)
    app.print_widgets()


main()

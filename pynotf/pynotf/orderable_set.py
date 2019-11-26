from typing import TypeVar, Union, Iterable, Generic, Optional, List

T = TypeVar('T')


class OrderableSet(Generic[T]):
    """
    For Emitters and Node children, we need a list of unique elements that can be ordered.
    In C++ we would use friend classes to only allow Nodes and Emitters to add and remove entries from the list, here we
    just note in the comments that subclasses may only change the order, not the number or identity of elements.
    """

    def __init__(self, *args: T):
        """
        Constructor.
        :param args: Initial contents of the list.
        """
        self._data: List[T] = list(*args)

    def __len__(self) -> int:
        """
        :return: Number of elements in the list.
        """
        return len(self._data)

    def __contains__(self, item: T) -> bool:
        """
        :param item: Item to check for.
        :return: Whether the requested item is contained in the list.
        """
        return item in self._data

    def __getitem__(self, index: Union[int, slice]) -> T:
        """
        [] access operator.
        :param index: Index (-slice) to return.
        :raise IndexError: If the index is illegal.
        :return: Element/s at the requested index/indices.
        """
        return self._data[index]

    def __iter__(self) -> Iterable[T]:
        """
        :return: Iterator through the list.
        """
        yield from self._data

    def __reversed__(self) -> Iterable[T]:
        """
        :return Reverse-iterator through the list:
        """
        return reversed(self._data)

    def index(self, item: T, start: Optional[int] = None, end: Optional[int] = None) -> int:
        """
        :param item: Item to look for.
        :param start: Start index to start limit the search space to the left.
        :param end: End index to limit the search space to the right.
        :raise ValueError: If no entry equal to item was found in the inspected subsequence.
        :return: Return zero-based index in the list of the first entry that is equal to item.
        """
        return self._data.index(item,
                                start if start is not None else 0,
                                end if end is not None else len(self._data))

    def count(self, item: T) -> int:
        """
        :param item: Item to look for.
        :return: The number of entries in the list that are equal to item.
        """
        return 1 if item in self._data else 0

    def append(self, item: T):
        """
        :param item: Item to append to the list if it is not already present in it.
        """
        if item not in self._data:
            self._data.append(item)

    def remove(self, item: T):
        """
        :param item: Item to remove if it is present in the list.
        """
        if item in self._data:
            self._data.remove(item)

    def clear(self):
        """
        Removes all entries from the list.
        """
        self._data.clear()

    def move_to_front(self, item: T):
        """
        :param item: Moves this item to the front of the list.
        :raise ValueError: If no such item is in the list.
        """
        self._rotate(0, self.index(item) + 1, 1)

    def move_to_back(self, item: T):
        """
        :param item: Moves this item to the back of the list.
        :raise ValueError: If no such item is in the list.
        """
        self._rotate(self.index(item), len(self._data), -1)

    def move_in_front_of(self, item: T, other: T):
        """
        :param item: Moves this item right in front of the other.
        :param other: Other item to move in front of.
        :raise ValueError: If either item is not in the list.
        """
        item_index: int = self.index(item)
        other_index: int = self.index(other)
        if item_index == other_index:
            return
        elif item_index > other_index:
            self._rotate(other_index, item_index + 1, 1)
        else:
            self._rotate(item_index, other_index, -1)

    def move_behind_of(self, item: T, other: T):
        """
        :param item: Moves this item right behind the other.
        :param other: Other item to move behind.
        :raise ValueError: If either item is not in the list.
        """
        item_index: int = self.index(item)
        other_index: int = self.index(other)
        if item_index == other_index:
            return
        elif item_index > other_index:
            self._rotate(other_index + 1, item_index + 1, 1)
        else:
            self._rotate(item_index, other_index + 1, -1)

    def _rotate(self, start: int, end: int, steps: int):
        """
        Rotates the given sequence to the right.
        Example:
            my_list = [0, 1, 2, 3, 4, 5, 6]
            _rotate(my_list, 2, 5, 1)  # returns [0, 1, 4, 2, 3, 5, 6]
        :param start:       First index of the rotated subsequence.
        :param end:         One-past the last index of the rotated subsequence.
        :param steps:       Number of steps to rotate.
        :return:            The modified list.
        """
        pivot: int = (end if steps >= 0 else start) - steps
        self._data = self._data[0:start] + self._data[pivot:end] + self._data[start:pivot] + self._data[end:]

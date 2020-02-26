from __future__ import annotations

import sys
from enum import IntEnum, auto
from typing import Union, List, Dict, Any, Sequence, Optional, Tuple, Iterable, AbstractSet

from pyrsistent import pvector as make_const_list, pmap as make_const_map
from pyrsistent.typing import PVector as ConstList, PMap as ConstMap

########################################################################################################################

Denotable = Union[
    float,  # numbers are always stored as floats
    str,  # strings are UTF-8 encoded strings
    Optional[List['Denotable']],  # list contain an unknown number of denotable data, all of the same Schema
    Union[Tuple['Denotable', ...], Dict[str, 'Denotable']],  # records contain a named or unnamed tuple
    'Value',  # Values can be used to define other Values
]
"""
Python data that conforms to the restrictions imposed by the Value type.
The only way to get a `Denotable` object is to pass "raw" Python data to `check_denotable`. If the input data is
malformed in any way, this function will raise an exception.
"""


def check_denotable(obj: Any, allow_empty_list: bool) -> Denotable:
    """
    Tries to convert any given Python object into a Denotable.
    Denotable are still pure Python objects, but they are known to be capable of being stored in a Value.
    :param obj: Object to convert.
    :param allow_empty_list: Empty lists are allowed when modifying a Value, but not during construction.
    :return: The given obj as Denotable.
    :raise ValueError: If the object cannot be denoted by a Value.
    """
    # already a Value
    if isinstance(obj, Value):
        return obj

    # ground
    elif isinstance(obj, (str, float)):
        return obj

    # trivially convertible
    elif isinstance(obj, int):
        return float(obj)

    # list
    elif isinstance(obj, list):
        denotable: List[Denotable] = [check_denotable(x, allow_empty_list) for x in obj]

        if len(denotable) > 0:
            reference_denotable: Denotable = denotable[0]
            reference_schema: Schema = Schema(reference_denotable)
            for i in range(1, len(denotable)):
                if Schema(denotable[i]) != reference_schema:
                    raise ValueError("All items in a Value.List must have the same Schema")

            if Kind.from_denotable(reference_denotable) == Kind.RECORD:
                def get_keys(rep: Denotable) -> Optional[AbstractSet[str]]:
                    if isinstance(rep, dict):
                        return rep.keys()
                    elif isinstance(rep, tuple):
                        return None
                    else:
                        assert isinstance(rep, Value)
                        result: Optional[AbstractSet[str]] = rep.get_keys()
                        if result is not None:
                            result = set(result)  # to allow == comparison with dict.keys()
                        return result

                reference_keys: Optional[AbstractSet[str]] = get_keys(reference_denotable)
                for i in range(1, len(denotable)):
                    keys: Optional[AbstractSet[str]] = get_keys(denotable[i])
                    if keys != reference_keys:
                        raise ValueError("All Records in a Value.List must have the same Dictionary")

            return denotable

    # named record
    elif isinstance(obj, dict):
        if len(obj) == 0:
            raise ValueError("Records cannot be empty")

        denotable: Dict[str, Denotable] = {}
        for key, value in obj.items():
            if not isinstance(key, str):
                raise ValueError("All keys of a Record must be of Value.Kind String")
            denotable[key] = check_denotable(value, allow_empty_list)
        return denotable

    # unnamed record
    elif isinstance(obj, tuple):
        if len(obj) == 0:
            raise ValueError("Records cannot be empty")

        return tuple(check_denotable(value, allow_empty_list) for value in obj)

    # empty list
    if obj is None or (isinstance(obj, list) and len(obj) == 0):
        if allow_empty_list:
            return None
        else:
            raise ValueError("Lists cannot be empty during Value definition")

    # incompatible type
    raise ValueError("Cannot construct Denotable from a {}".format(type(obj).__name__))


########################################################################################################################

class Kind(IntEnum):
    """
    Enumeration of the four denotable Value Kinds.
    To differentiate between integers in a Value.Schema that represent forward offsets (see Schema), and enum values
    that denote the Kind of an Element, we only use the highest denotable integers for the enum.
    """
    LIST = sys.maxsize - 3
    RECORD = auto()
    NUMBER = auto()
    STRING = auto()

    @staticmethod
    def is_valid(value: int) -> bool:
        """
        :return: True iff `value` is a valid Kind enum value.
        """
        return value >= Kind.LIST

    @staticmethod
    def is_ground(value: int) -> bool:
        """
        :return: Whether `value` denotes one of the ground types: Number and String
        """
        return value >= Kind.NUMBER

    @staticmethod
    def from_denotable(denotable: Optional[Denotable]) -> Kind:
        """
        :return: The Kind of the given denotable Python data.
        """
        if isinstance(denotable, Value):
            return denotable.get_kind()
        elif isinstance(denotable, str):
            return Kind.STRING
        elif isinstance(denotable, float):
            return Kind.NUMBER
        elif isinstance(denotable, list):
            return Kind.LIST
        else:
            assert isinstance(denotable, (dict, tuple))
            return Kind.RECORD


########################################################################################################################

class Schema(tuple, Sequence[int]):
    """
    A Schema describes how the denotable of a Value instance is laid out in memory.
    It is a simple tuple of integers, each integer either identifying a ground Kind (Number or String) or a forward
    offset to a container Schema (List or Record).
    """

    @classmethod
    def from_slice(cls, base: Schema, start: int, end: int) -> Schema:
        return super().__new__(Schema, base[start: end])

    @classmethod
    def explicit(cls, *args: Tuple[int, ...]) -> Schema:
        return super().__new__(Schema, args)

    def __new__(cls, obj: Optional[Any] = None) -> Schema:
        """
        Recursive, breadth-first assembly of a Schema for the given Denotable.
        :returns: Schema describing how a Value containing the given Denotable would be laid out.
        """
        if obj is None:
            # The empty Schema
            return super().__new__(Schema)

        if isinstance(obj, Value):
            # Value Schemas can simply be copied
            obj.get_schema()

        schema: List[int] = []
        denotable: Denotable = check_denotable(obj, allow_empty_list=False)
        kind: Kind = Kind.from_denotable(denotable)

        if kind == Kind.NUMBER:
            # Numbers take up a single word in a Schema, the NUMBER Kind.
            schema.append(int(Kind.NUMBER))

        elif kind == Kind.STRING:
            # Strings take up a single word in a Schema, the STRING Kind.
            schema.append(Kind.STRING)

        elif kind == Kind.LIST:
            # A List Schema is a pair [LIST Kind, child Schema].
            # All Values contained in a List must have the same Schema.
            # An example Schema of a List is:
            #
            # ||    1.     ||     2.    | ... ||
            # || List Type || ChildType | ... ||
            # ||- header --||---- child ------||
            #
            assert len(denotable) > 0
            schema.append(int(Kind.LIST))
            schema.extend(Schema(denotable[0]))

        else:
            assert kind == Kind.RECORD
            # Records take up at least 3 words in the buffer - the RECORD kind identifier, the number of entries in the
            # Record (must be at least 1) followed by the child entries.
            # An example Schema of a Record containing a Number, a List, a String and another Record can be visualized
            # as follows:
            #
            # ||    1.    |    2.    ||    3.    |    4.    |    5.    |    6.    ||   7.     | ... |    14.   | ... ||
            # || Rec Type | Rec Size || Nr. Type |  +3 (=7) | Str Type | +8 (=14) || ListType | ... | Rec Type | ... ||
            # ||------ header -------||------------------ body -------------------||------------ children -----------||
            #
            # The Record Schema itself is split into three parts:
            # 1. The header just contains the RECORD Kind and number of child entries.
            # 2. The body contains a single word for each child. Strings and Number Schemas are only a single word long,
            #    therefore they can be written straight into the body of the Schema.
            #    List and Record Schemas are longer than one word and are appended at the end of the body. The body
            #    itself contains only the forward offset to the child Schema in question.
            #    In the special case where the Record contains only one non-ground child and it is the last child in the
            #    Record, we can remove the last entry in the body and move the single child up one word.
            # 3. Child Lists and Records in order of their appearance in the body.
            assert len(denotable) > 0
            schema.append(int(Kind.RECORD))
            schema.append(len(denotable))  # number of children
            child_position = len(schema)
            schema.extend([0] * len(denotable))  # reserve space for the body

            if isinstance(denotable, dict):
                children: Iterable[Denotable] = denotable.values()
            else:
                assert isinstance(denotable, tuple)
                children: Iterable[Denotable] = denotable

            for child in children:
                # only store the Kind for ground types
                if Kind.is_ground(Kind.from_denotable(child)):
                    schema[child_position] = int(Kind.from_denotable(child))
                # lists and records store a forward offset and append themselves to the end of the schema
                else:
                    offset = len(schema) - child_position
                    assert offset > 0
                    # If the offset is 1, we only have a single non-ground child in the entire record and it is at the
                    # very end. In this case, we can simply put the sub-schema of the child inline and save a word.
                    if offset == 1:
                        schema.pop()
                    else:
                        schema[child_position] = offset  # forward offset
                    schema.extend(Schema(child))
                child_position += 1

        return super().__new__(Schema, schema)

    def is_empty(self) -> bool:
        """
        Checks if this is the empty Schema.
        """
        return len(self) == 0

    def as_list(self) -> Schema:
        """
        Creates a Schema that is a list containing elements of this Schema.
        """
        return super().__new__(Schema, [int(Kind.LIST), *self])

    def __str__(self) -> str:
        """
        :return: A human-readable string representation of this Schema.
        """
        result: str = ""
        next_word_is_record_size: bool = False
        for word, index in zip(self, range(len(self))):
            result += "{:>3}: ".format(index)
            if word == Kind.NUMBER:
                result += "Number"
            elif word == Kind.STRING:
                result += "String"
            elif word == Kind.LIST:
                result += "List"
            elif word == Kind.RECORD:
                result += "Record"
                next_word_is_record_size = True
            else:
                if next_word_is_record_size:
                    result += " ↳ Size: {}".format(word)
                    next_word_is_record_size = False
                else:
                    result += "→ {}".format(index + word)
            result += "\n"
        return result


def get_subschema_end(schema: Schema, start_index: int) -> int:
    """
    For the start index of a Subschema in the given Schema, return the index one past the end.
    :param schema: Schema containing the subschema.
    :param start_index: Start index of a subschema. Must contain a valid Kind and be inside the Schema.
    :return: Index in the schema one past the end of the subschema starting at `start_index`.
    """
    assert start_index < len(schema)
    assert Kind.is_valid(schema[start_index])
    kind: Kind = Kind(schema[start_index])

    if Kind.is_ground(kind):
        return start_index + 1

    elif kind == Kind.LIST:
        return get_subschema_end(schema, start_index + 1)

    else:
        assert kind == Kind.RECORD
        assert start_index + 1 < len(schema)
        child_count: int = schema[start_index + 1]
        assert child_count > 0

        # we need to find the end index of the last, non-ground child
        for child_index in (start_index + 2 + child for child in reversed(range(child_count))):
            assert child_index < len(schema)
            if not Kind.is_valid(schema[child_index]):
                return get_subschema_end(schema, child_index + schema[child_index])

        # if all children are ground, the end index is one past the body
        return start_index + 2 + child_count


########################################################################################################################

Data = Union[
    None,  # denotes an empty list, immutable by design
    float,  # numbers are ground and immutable by design
    int,  # the size of a list is stored as an unsigned integer which are immutable by design
    str,  # strings are ground and immutable because of their Python implementation
    ConstList['Data'],  # references to child Data are immutable through the use of ConstList
]
"""
The immutable data stored inside a Value.
"""


def create_data_from_schema(schema: Schema) -> Data:
    """
    Creates a default-initialized, immutable Data object conforming to the given Schema.
    :param schema: Schema describing the layout of the Data object to produce.
    :return: The Schema's default Data.
    """
    if schema.is_empty():
        raise ValueError("Cannot instantiate a Value from the empty Schema")

    def create_data(index: int = 0) -> Data:
        assert index < len(schema)

        # number
        if schema[index] == Kind.NUMBER:
            return 0.

        # string
        elif schema[index] == Kind.STRING:
            return ""

        # list
        elif schema[index] == Kind.LIST:
            return None  # empty list

        # record
        assert schema[index] == Kind.RECORD
        child_indices: List[int] = []

        # if the Schema contains an offset at the child index, resolve it
        assert index + 1 < len(schema)
        record_size: int = schema[index + 1]
        for child_index in (index + 2 + child for child in range(record_size)):
            assert child_index < len(schema)
            if not not Kind.is_valid(schema[child_index]):
                child_index += schema[child_index]
                assert child_index < len(schema)
            child_indices.append(child_index)

        # recursively create child data
        return make_const_list(create_data(child_index) for child_index in child_indices)

    return create_data()


def create_data_from_denotable(denotable: Denotable) -> Data:
    if denotable is None:
        return None

    elif isinstance(denotable, Value):
        return denotable._data

    kind: Kind = Kind.from_denotable(denotable)

    if Kind.is_ground(kind):
        return denotable

    elif kind == Kind.LIST:
        list_size: int = len(denotable)
        if list_size > 0:
            return make_const_list((list_size, *(create_data_from_denotable(child) for child in denotable)))
        else:
            return None  # empty list

    assert kind == Kind.RECORD
    if isinstance(denotable, tuple):
        return make_const_list(create_data_from_denotable(child) for child in denotable)

    assert isinstance(denotable, dict)
    return make_const_list(create_data_from_denotable(child) for child in denotable.values())


########################################################################################################################

Dictionary = ConstMap[str, Tuple[int, Optional['Dictionary']]]
"""
A Dictionary can be attached to a Value to access Record entries by name.
The Record keys are not part of the data as they are not mutable, much like a Schema.
The Dictionary is also ignored when two Schemas are compared, meaning a Record {x=float, y=float, z=float} will be 
compatible to {r=float, g=float, b=float}.
"""


def create_dictionary(denotable: Denotable) -> Optional[Dictionary]:
    """
    Recursive, depth-first assembly of a Dictionary.
    :param denotable: Denotable data to build the Dictionary from.
    :return: The Dictionary corresponding to the given denotable or None if it would be empty.
    """

    def parse_next(next_denotable: Denotable) -> Optional[Dictionary]:
        if next_denotable is None:
            return None

        elif isinstance(next_denotable, Value):
            return next_denotable._dictionary

        kind: Kind = Kind.from_denotable(next_denotable)

        if Kind.is_ground(kind):
            return None

        elif kind == Kind.LIST:
            return parse_next(next_denotable[0])

        else:
            assert kind == Kind.RECORD
            return make_const_map({str(name): (index, parse_next(child))
                                   for index, (name, child) in enumerate(next_denotable.items())})

    return parse_next(denotable)


########################################################################################################################


class Value:

    def __init__(self, obj: Any = None):
        self._schema: Schema = Schema()
        self._data: Data = None
        self._dictionary: Optional[Dictionary] = None

        # default (empty) initialization
        if obj is None:
            return

        # copy initialization
        if isinstance(obj, Value):
            self._schema = obj._schema
            self._data = obj._data
            self._dictionary = obj._dictionary

        # initialization from Schema
        elif isinstance(obj, Schema):
            self._schema = obj
            self._data = create_data_from_schema(self._schema)
            self._dictionary = None

        # value initialization
        else:
            denotable: Denotable = check_denotable(obj, allow_empty_list=False)
            self._schema = Schema(denotable)
            self._data = create_data_from_denotable(denotable)
            self._dictionary = create_dictionary(denotable)

        assert self._is_consistent()

    @classmethod
    def _create(cls, schema: Schema, data: Data, dictionary: Optional[Dictionary]) -> Value:
        value: Value = Value()
        value._schema = schema
        value._data = data
        value._dictionary = dictionary
        assert value._is_consistent()
        return value

    def get_schema(self) -> Schema:
        """
        The Schema of this Value.
        """
        return self._schema

    def get_kind(self) -> Kind:
        """
        The Kind of this Value.
        """
        assert Kind.is_valid(self._schema[0])
        return Kind(self._schema[0])

    def get_keys(self) -> AbstractSet[str]:
        """
        Returns the known keys if this is a named record.
        Otherwise returns an empty set.
        """
        if self._dictionary:
            return self._dictionary.keys()
        else:
            return set()

    def __eq__(self, other: Any) -> bool:
        """
        Equality test.
        """
        return self._data == other._data and self._schema == other._schema

    def __ne__(self, other) -> bool:
        """
        Inequality tests.
        """
        return not self.__eq__(other)

    def __len__(self) -> int:
        """
        The number of child Elements if the current Element is a List or Map, or zero otherwise.
        """
        kind: Kind = self.get_kind()

        if kind == Kind.LIST:
            if self._data is None:
                return 0
            else:
                list_size: int = self._data[0]
                assert list_size == len(self._data) - 1  # -1 for the first integer that is the size of the list
                return list_size

        elif kind == Kind.RECORD:
            assert self._data is not None
            assert len(self._schema) > 1
            return self._schema[1]

        else:
            return 0  # only lists and records

    def _get_item_by_index(self, index: int) -> Value:
        assert isinstance(self._data, ConstList)

        # this method must only be called on list and record values
        kind: Kind = self.get_kind()
        assert kind == Kind.LIST or kind == Kind.RECORD

        # support negative indices from the back
        size: int = len(self._data)
        if index < 0:
            index = size - index
        if kind == kind.RECORD:
            assert size == self._schema[1]

        # make sure that the index is valid
        if not (0 <= index < size):
            raise KeyError(f'Cannot get Element at index {size - index} from a '
                           f'{"List" if kind == Kind.LIST else "Record"} of size {size}')

        # lists store the child schema size at index one
        if kind == Kind.LIST:
            start: int = 1
            end: int = get_subschema_end(self._schema, start)
            schema: Schema = Schema.from_slice(self._schema, start, end)

        # while records might require a lookup
        else:
            child_entry: int = self._schema[2 + index]
            if Kind.is_ground(child_entry):
                schema: Schema = Schema.explicit((child_entry,))
            else:
                start: int = 2 + index + child_entry
                end: int = get_subschema_end(self._schema, start)
                schema: Schema = Schema.from_slice(self._schema, start, end)

        return Value._create(schema, self._data[index], self._dictionary)

    def _get_item_by_name(self, name: str) -> Value:
        """

        :param name:
        :return:
        """
        assert isinstance(self._data, ConstList)

        if self._dictionary is None:
            kind: Kind = self.get_kind()
            if kind == Kind.RECORD:
                raise KeyError(f'This Record Value has only unnamed entries, use an index to access them')
            else:
                raise KeyError(f'Cannot request children of a {kind.name.capitalize()} Value by name')
        if name not in self._dictionary:
            available_keys: str = '", "'.join(self._dictionary.keys())  # since this is a Record, there is at least one
            raise KeyError(f'Unknown key "{name}" in Record. Available keys are: "{available_keys}"')

        assert len(self._data) == self._schema[1]
        index: int = self._dictionary[name][0]
        assert 0 <= index < len(self._data)

        child_entry: int = self._schema[2 + index]
        if Kind.is_ground(child_entry):
            schema: Schema = Schema.explicit((child_entry,))
        else:
            start: int = 2 + index + child_entry
            end: int = get_subschema_end(self._schema, start)
            schema: Schema = Schema.from_slice(self._schema, start, end)

        return Value._create(schema, self._data[index], self._dictionary[name][1])

    def __getitem__(self, key: Union[str, int]) -> Value:
        kind: Kind = self.get_kind()

        if kind == Kind.LIST:
            if not isinstance(key, int):
                raise KeyError(f'Lists must be accessed using an index, not {key}')
            return self._get_item_by_index(key)

        if kind == Kind.RECORD:
            if isinstance(key, int):
                return self._get_item_by_index(key)
            elif isinstance(key, str):
                return self._get_item_by_name(key)
            else:
                raise KeyError(f'Records must be accessed using an index or a string, not {key}')

        else:
            raise KeyError(f'Cannot use operator[] with a {kind.name.capitalize()} Value')

    # noinspection PyMethodMayBeStatic
    def _is_consistent(self) -> bool:
        return True  # TODO: Value._is_consistent


########################################################################################################################

def main():
    test_object = {
        "coords": [
            {
                "x": 0,
                "someName": "SUCCESS",
                "number_list": [1],
            },
            {
                "x": 2,
                "someName": "Hello world",
                "number_list": [2, 3],
            }
        ],
        "name": "Mr. Okay",
        "my_map": {
            "key": "string",
            "list_in_the_middle": ["I'm in the middle"],
            "a number": 847,
        },
        "pos": 32.2,
        "number_list": [2, 23.1, -347],
        "nested_list": [
            ["a", "b"],
            ["c", "d"],
        ],
    }
    raw_dictionary = {
        "coords": {
            "x": None,
            "someName": None,
            "number_list": None,
        },
        "name": None,
        "my_map": {
            "key": None,
            "list_in_the_middle": None,
            "a number": None,
        },
        "pos": None,
        "number_list": None,
        "nested_list": None,
    }

    test_denotable = check_denotable(test_object, allow_empty_list=False)
    # dictionary: Optional[Dictionary] = create_dictionary(test_denotable)
    # print(raw_dictionary.keys() == set(dictionary.keys()))
    schema: Schema = Schema(test_denotable)
    print(schema)
    start = 15
    end = get_subschema_end(schema, start)
    print(f'End index of element starting at {start}: {end}')
    print(Schema.from_slice(schema, start, end))
    pass


if __name__ == '__main__':
    main()

from __future__ import annotations

import sys
from enum import IntEnum, auto
from typing import Union, List, Dict, Any, Sequence, Optional, Tuple, Iterable, AbstractSet

from pyrsistent import pvector as make_const_list, pmap as make_const_map
from pyrsistent.typing import PVector as ConstList, PMap as ConstMap

########################################################################################################################

Representable = Union[
    float,  # numbers are always stored as floats
    str,  # strings are UTF-8 encoded strings
    Optional[List['Representable']],  # list contain an unknown number of representable data, all of the same Schema
    Union[Tuple['Representable', ...], Dict[str, 'Representable']],  # records contain a named or unnamed tuple
    'Value',  # Values can be used to define other Values
]
"""
Python data that conforms to the restrictions imposed by the Value type.
The only way to get a `Representable` object is to pass "raw" Python data to `check_representable`. If the input data is
malformed in any way, this function will raise an exception.
"""


def check_representable(obj: Any, allow_empty_list: bool) -> Representable:
    """
    Tries to convert any given Python object into a Representable.
    Representable are still pure Python objects, but they are known to be capable of being stored in a Value.
    :param obj: Object to convert.
    :param allow_empty_list: Empty lists are allowed when modifying a Value, but not during construction.
    :return: The given obj as Representable.
    :raise ValueError: If the object cannot be represented as an Element.
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
        representable: List[Representable] = [check_representable(x, allow_empty_list) for x in obj]

        if len(representable) > 0:
            reference_representable: Representable = representable[0]
            reference_schema: Schema = Schema(reference_representable)
            for i in range(1, len(representable)):
                if Schema(representable[i]) != reference_schema:
                    raise ValueError("All items in a Value.List must have the same Schema")

            if Kind.from_representable(reference_representable) == Kind.RECORD:
                def get_keys(rep: Representable) -> Optional[AbstractSet[str]]:
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

                reference_keys: Optional[AbstractSet[str]] = get_keys(reference_representable)
                for i in range(1, len(representable)):
                    keys: Optional[AbstractSet[str]] = get_keys(representable[i])
                    if keys != reference_keys:
                        raise ValueError("All Records in a Value.List must have the same Dictionary")

            return representable

    # named record
    elif isinstance(obj, dict):
        if len(obj) == 0:
            raise ValueError("Records cannot be empty")

        representable: Dict[str, Representable] = {}
        for key, value in obj.items():
            if not isinstance(key, str):
                raise ValueError("All keys of a Record must be of Value.Kind String")
            representable[key] = check_representable(value, allow_empty_list)
        return representable

    # unnamed record
    elif isinstance(obj, tuple):
        if len(obj) == 0:
            raise ValueError("Records cannot be empty")

        return tuple(check_representable(value, allow_empty_list) for value in obj)

    # empty list
    if obj is None or (isinstance(obj, list) and len(obj) == 0):
        if allow_empty_list:
            return None
        else:
            raise ValueError("Lists cannot be empty during Value definition")

    # incompatible type
    raise ValueError("Cannot construct Representable from a {}".format(type(obj).__name__))


########################################################################################################################

class Kind(IntEnum):
    """
    Enumeration of the four representable Kinds and the empty Kind.
    To differentiate between integers in a Value.Schema that represent forward offsets (see Schema), and enum values
    that denote the Kind of an Element, we only use the highest representable integers for the enum.
    The EMPTY Kind never part of a Schema, it is only used as a sentinel to denote that a Schema is empty.
    """
    EMPTY = 0
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
        :return: Whether `value` represents one of the ground types: Number and String
        """
        return value >= Kind.NUMBER

    @staticmethod
    def from_representable(representable: Optional[Representable]) -> Kind:
        """
        :return: The Kind of the given representable Python data.
        """
        if representable is None:
            return Kind.NONE
        elif isinstance(representable, Value):
            return representable.get_kind()
        elif isinstance(representable, str):
            return Kind.STRING
        elif isinstance(representable, float):
            return Kind.NUMBER
        elif isinstance(representable, list):
            return Kind.LIST
        else:
            assert isinstance(representable, (dict, tuple))
            return Kind.RECORD


########################################################################################################################

class Schema(tuple, Sequence[int]):
    """
    A Schema describes how the representable of a Value instance is laid out in memory.
    It is a simple tuple of integers, each integer either identifying a ground Kind (Number or String) or a forward
    offset to a container Schema (List or Record).
    """

    def __new__(cls, obj: Optional[Any] = None) -> Schema:
        """
        Recursive, breadth-first assembly of a Schema for the given Representable.
        :returns: Schema describing how a Value containing the given Representable would be laid out.
        """
        if isinstance(obj, Value):
            # Value Schemas can simply be copied
            obj.get_schema()

        schema: List[int] = []
        representable: Representable = check_representable(obj, allow_empty_list=False)
        kind: Kind = Kind.from_representable(representable)

        if kind == Kind.EMPTY:
            # The Empty Schema is empty.
            pass

        elif kind == Kind.NUMBER:
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
            assert len(representable) > 0
            schema.append(int(Kind.LIST))
            schema.extend(Schema(representable[0]))

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
            assert len(representable) > 0
            schema.append(int(Kind.RECORD))
            schema.append(len(representable))  # number of children
            child_position = len(schema)
            schema.extend([0] * len(representable))  # reserve space for the body

            if isinstance(representable, dict):
                children: Iterable[Representable] = representable.values()
            else:
                assert isinstance(representable, tuple)
                children: Iterable[Representable] = representable

            for child in children:
                # only store the Kind for ground types
                if Kind.is_ground(Kind.from_representable(child)):
                    schema[child_position] = int(Kind.from_representable(child))
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
    None,  # the empty value, immutable by design
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
        return None

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
            return ConstList()

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


########################################################################################################################

Dictionary = ConstMap[str, Optional['Dictionary']]
"""
A Dictionary can be attached to a Value to access Record entries by name.
The Record keys are not part of the data as they are not mutable, much like a Schema.
The Dictionary is also ignored when two Schemas are compared, meaning a Record {x=float, y=float, z=float} will be 
compatible to {r=float, g=float, b=float}.
"""


def create_dictionary(representable: Representable) -> Optional[Dictionary]:
    """
    Recursive, depth-first assembly of a Dictionary.
    :param representable: Representable data to build the Dictionary from.
    :return: The Dictionary corresponding to the given representable or None if it were empty.
    """

    def parse_next(next_representable: Representable) -> Optional[Dictionary]:
        if next_representable is None:
            return None

        elif isinstance(next_representable, Value):
            return next_representable.get_dictionary()

        kind: Kind = Kind.from_representable(next_representable)

        if Kind.is_ground(kind):
            return None

        elif kind == Kind.LIST:
            return parse_next(next_representable[0])

        else:
            assert kind == Kind.RECORD
            return make_const_map({str(name): parse_next(child) for (name, child) in next_representable.items()})

    return parse_next(representable)


########################################################################################################################


class Value:
    def __init__(self):
        self._schema: Schema = Schema()
        self._dictionary: Optional[Dictionary] = None

    def get_schema(self) -> Schema:
        return self._schema

    def get_dictionary(self) -> Optional[Dictionary]:
        return self._dictionary

    def get_kind(self) -> Kind:
        if self._schema.is_empty():
            return Kind.EMPTY
        else:
            assert Kind.is_valid(self._schema[0])
            return Kind(self._schema[0])

    def get_keys(self) -> Optional[AbstractSet[str]]:
        if self._dictionary:
            return self._dictionary.keys()
        return None


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

    test_representable = check_representable(test_object, allow_empty_list=False)
    dictionary: Optional[Dictionary] = create_dictionary(test_representable)
    print(raw_dictionary.keys() == set(dictionary.keys()))
    # schema: Schema = Schema(test_object)
    # print(schema)
    # print(f'End index of element starting at 0: {get_subschema_end(schema, 0)}')
    # print(create_dictionary({}))
    pass


if __name__ == '__main__':
    main()

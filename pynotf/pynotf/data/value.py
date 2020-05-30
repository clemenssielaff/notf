from __future__ import annotations

import sys
from enum import IntEnum, auto
from typing import Union, List, Dict, Any, Sequence, Optional, Tuple, Iterable, Callable, overload
from math import trunc, ceil, floor, pow
from operator import floordiv, mod, truediv
from json import dumps as write_json, loads as read_json

from pyrsistent import pvector as make_const_list, PVector as ConstList, PRecord as ConstNamedTuple, field as c_field
from pyrsistent.typing import PVector as ConstListT

from pynotf.data import Bonsai

# DENOTABLE ############################################################################################################

# Python data that conforms to the restrictions imposed by the Value type.
# The only way to get a `Denotable` object is to give "raw" Python data to `check_denotable`.
# If the input data is malformed in any way, this function will raise an exception.
Denotable = Union[
    None,  # the only data held by a None Value, cannot be nested
    float,  # numbers are always stored as floats
    str,  # strings are UTF-8 encoded strings
    List['Denotable'],  # list contain an unknown number of denotable data, all of the same Schema
    Union[Tuple['Denotable', ...], Dict[str, 'Denotable']],  # records contain a named or unnamed tuple
    'Value',  # Values can be used to define other Values
]


def check_number(obj: Any) -> Optional[float]:
    converter: Optional[Callable[[], float]] = getattr(obj, "__float__", None)
    if converter is None:
        return None
    else:
        return float(converter())


def create_denotable(obj: Any, allow_empty_list: bool = False, list_can_be_record: bool = False) -> Denotable:
    """
    Tries to convert any given Python object into a Denotable.
    Denotable are still pure Python objects, but they are known to be capable of being stored in a Value.
    :param obj: Object to convert.
    :param allow_empty_list: Empty lists are allowed when modifying a Value, but not during construction.
    :param list_can_be_record: When deserializing JSON, we cannot differentiate between lists and unnamed records.
        While the list type is built in, unnamed records are identified by lists starting with a single <null> element.
        Usually, this would not be allowed.
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
    elif isinstance(obj, (int,)):
        return float(obj)

    # none
    elif obj is None:
        # there is no code path to create a None Value at the root,
        # therefore it must be encountered within a list or record
        raise ValueError("If present, None must be the only data in a Value")

    # list
    elif isinstance(obj, list):
        # when converting to JSON, there is no type differentiation between lists and unnamed records
        # an unnamed record is identified by having the first element in the list set to <null>
        if list_can_be_record and len(obj) > 0 and obj[0] is None:
            return tuple(create_denotable(value, allow_empty_list, list_can_be_record) for value in obj[1:])

        denotable: List[Denotable] = [create_denotable(x, allow_empty_list, list_can_be_record) for x in obj]
        if len(denotable) == 0:
            if allow_empty_list:
                return []
            else:
                raise ValueError("Lists cannot be empty during Value definition")

        reference_denotable: Denotable = denotable[0]
        reference_schema: Schema = Schema(reference_denotable)
        for i in range(1, len(denotable)):
            if Schema(denotable[i]) != reference_schema:
                raise ValueError("All items in a Value.List must have the same Schema")

        if Kind.from_denotable(reference_denotable) == Kind.RECORD:
            def get_keys(rep: Denotable) -> Optional[List[str]]:
                if isinstance(rep, dict):
                    return list(rep.keys())
                elif isinstance(rep, tuple):
                    return None
                else:
                    assert isinstance(rep, Value)
                    return rep.get_keys()

            reference_keys: Optional[List[str]] = get_keys(reference_denotable)
            for i in range(1, len(denotable)):
                if get_keys(denotable[i]) != reference_keys:
                    raise ValueError("All Records in a Value.List must have the same Dictionary")

        return denotable

    # schema is a tuple, but we treat it as a list...
    elif isinstance(obj, Schema):
        if len(obj) == 0:  # TODO: validate Schema
            raise ValueError("Schema cannot be empty")
        return [float(word) for word in obj]

    # named record
    elif isinstance(obj, dict):
        if len(obj) == 0:
            raise ValueError("Records cannot be empty")

        denotable: Dict[str, Denotable] = {}
        for key, value in obj.items():
            if not isinstance(key, str):
                raise ValueError("All keys of a Record must be of Value.Kind String")
            denotable[key] = create_denotable(value, allow_empty_list, list_can_be_record)
        return denotable

    # unnamed record
    elif isinstance(obj, tuple):
        if len(obj) == 0:
            raise ValueError("Records cannot be empty")

        return tuple(create_denotable(value, allow_empty_list, list_can_be_record) for value in obj)

    # incompatible type
    raise ValueError("Cannot construct Denotable from a {}".format(type(obj).__name__))


# KIND #################################################################################################################

class Kind(IntEnum):
    """
    Enumeration of the five denotable Value Kinds.
    To differentiate between integers in a Value.Schema that represent forward offsets (see Schema), and enum values
    that denote the Kind of an Element, we only use the highest denotable integers for the enum.
    """
    NONE = 0
    NUMBER = 1
    STRING = sys.maxsize - 2
    LIST = auto()
    RECORD = auto()

    # TODO: investigate whether we "really" need an explicit NONE type
    #  Having zero in a Schema reserved for 'None' is a waste, because zero can never be a valid offset. So it is free
    #  to use for a type that can be stored INSIDE a Value. Whereas 'None' implies that the Value is empty.

    # TODO: you can ever get a 1 offset either, because the element in that special case is inlined.

    # TODO: CONTINUE HERE
    #  * maybe use 1 as the NUMBER Kind, since it is ground and 1 word long
    #  * one way to keep the explicit NONE type _and_ don't waste any more numbers on an explicit VALUE kind would be to
    #       use 0 as the VALUE Kind, but if a Schema _only_ has a single Value, then it is NONE as that arrangement
    #       would otherwise be invalid anyway

    @staticmethod
    def is_valid(value: int) -> bool:
        """
        :return: True iff `value` is a valid Kind enum value.
        """
        return not (Kind.NUMBER < value < Kind.STRING)

    @staticmethod
    def is_offset(value: int) -> bool:
        """
        :return: True iff `value` is not a valid Kind enum value but an offset in a Schema.
        """
        return not Kind.is_valid(value)

    @staticmethod
    def is_ground(value: int) -> bool:
        """
        :return: Whether `value` denotes one of the ground types: Number and String
        """
        return value == Kind.NUMBER or value == Kind.STRING

    @staticmethod
    def is_none(value: int) -> bool:
        """
        :return: Whether `value` is the NONE Kind.
        """
        return value == Kind.NONE

    @staticmethod
    def from_denotable(denotable: Optional[Denotable]) -> Kind:
        """
        :return: The Kind of the given denotable Python data.
        """
        assert denotable is not None
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


# SCHEMA ###############################################################################################################

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
    def from_value(cls, value: Value) -> Optional[Schema]:
        """
        Creates a Schema from a list of numbers stored in a Value.
        :param value: Value storing the Schema.
        :return: The new Schema or None on error.
        """
        if value.get_kind() != Value.Kind.LIST or Value.Kind(value.get_schema()[1]) != Value.Kind.NUMBER:
            return None
        return super().__new__(Schema, (int(value[i]) for i in range(len(value))))  # TODO: validate Schema

    def __new__(cls, obj: Optional[Any] = None) -> Schema:
        """
        Recursive, breadth-first assembly of a Schema for the given Denotable.
        :returns: Schema describing how a Value containing the given Denotable would be laid out.
        """
        if obj is None:
            # The None Schema is still one word long, so you can store it in a Value.
            # Empty lists cannot be used to create a Value, so you'd have to jump through hoops to store a None Schema
            # in a Value.
            return super().__new__(Schema, (0,))

        if isinstance(obj, Value):
            # Value Schemas can simply be copied
            return obj.get_schema()

        schema: List[int] = []
        denotable: Denotable = create_denotable(obj)
        kind: Kind = Kind.from_denotable(denotable)
        assert kind != Kind.NONE

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

    def is_none(self) -> bool:
        """
        Checks if this is the None Schema.
        """
        assert len(self) > 0
        if self[0] == Kind.NONE:
            assert len(self) == 1
            return True
        else:
            return False

    def as_list(self) -> Schema:
        """
        Creates a Schema that is a list containing elements of this Schema.
        You cannot create a corresponding list Schema to a None Schema, if this Schema is None the returned Schema
        will also be None.
        """
        if self.is_none():
            return self
        else:
            return super().__new__(Schema, [int(Kind.LIST), *self])

    def __str__(self) -> str:
        """
        :return: A human-readable string representation of this Schema.
        """
        result: str = ""
        next_word_is_record_size: bool = False
        for word, index in zip(self, range(len(self))):
            result += "{:>3}: ".format(index)
            if next_word_is_record_size:
                result += " ↳ Size: {}".format(word)
                next_word_is_record_size = False
            elif word == Kind.NUMBER:
                result += "Number"
            elif word == Kind.STRING:
                result += "String"
            elif word == Kind.LIST:
                result += "List"
            elif word == Kind.RECORD:
                result += "Record"
                next_word_is_record_size = True
            else:
                result += "→ {}".format(index + word)  # offset
            result += "\n"
        return result


def get_subschema_start(schema: Schema, iterator: int, index: int) -> int:
    """
    Given a Schema, the position of an iterator within the Schema, pointing to the start of a Record subschema,
    and the index of a child in the Record, returns the position of the child subschema.
    :param schema: Schema containing the subschema.
    :param iterator: Index in the Schema pointing to the parent Record subschema.
    :param index: Index of the child subschema to find.
    :return: Index in the Schema pointing to the child subschema.
    """
    assert iterator < len(schema)
    assert Kind(schema[iterator]) == Kind.RECORD
    child_location: int = iterator + 2 + index
    child_entry: int = schema[child_location]

    # the child Schema can either be in the body of the Record Schema if it is ground, or require a lookup
    # mote that you can have a non-offset, non-ground child entry if the last child is the only one non-ground
    return child_location + (child_entry if Kind.is_offset(child_entry) else 0)


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
            if not Kind.is_ground(schema[child_index]):
                if Kind.is_offset(schema[child_index]):
                    return get_subschema_end(schema, child_index + schema[child_index])
                else:
                    return get_subschema_end(schema, child_index)

        # if all children are ground, the end index is one past the body
        return start_index + 2 + child_count


# DATA #################################################################################################################

Data = Union[
    None,  # the only data stored in a None Value
    float,  # numbers are ground and immutable by design
    int,  # the size of a list is stored as an unsigned integer which are immutable by design
    str,  # strings are ground and immutable because of their Python implementation
    ConstListT['Data'],  # references to child Data are immutable through the use of ConstList
]
"""
The immutable data stored inside a Value.
In Python, an empty list is stored like a regular list in the Value, meaning the Value contains a list object with one
zero, which is the size of the list. In C++, we could store a nullptr (or equivalent) instead of going to the heap for
what is basically a None value. 
"""


def create_data_from_schema(schema: Schema) -> Data:
    """
    Creates a default-initialized, immutable Data object conforming to the given Schema.
    :param schema: Schema describing the type of the Data object to produce.
    :return: The Schema's default Data.
    """
    if schema.is_none():
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
            return make_const_list((0,))  # empty list

        # record
        assert schema[index] == Kind.RECORD
        child_indices: List[int] = []

        # if the Schema contains an offset at the child index, resolve it
        assert index + 1 < len(schema)
        record_size: int = schema[index + 1]
        for child_index in (index + 2 + child for child in range(record_size)):
            assert child_index < len(schema)
            if Kind.is_offset(schema[child_index]):
                child_index += schema[child_index]
                assert child_index < len(schema)
            child_indices.append(child_index)

        # recursively create child data
        return make_const_list(create_data(child_index) for child_index in child_indices)

    return create_data()


def create_data_from_denotable(denotable: Denotable) -> Data:
    assert denotable is not None

    if isinstance(denotable, Value):
        return denotable._data

    kind: Kind = Kind.from_denotable(denotable)

    if Kind.is_ground(kind):
        return denotable

    elif kind == Kind.LIST:
        list_size: int = len(denotable)
        if list_size > 0:
            return make_const_list((list_size, *(create_data_from_denotable(child) for child in denotable)))
        else:
            return make_const_list((0,))  # empty list

    assert kind == Kind.RECORD
    if isinstance(denotable, tuple):
        return make_const_list(create_data_from_denotable(child) for child in denotable)

    assert isinstance(denotable, dict)
    return make_const_list(create_data_from_denotable(child) for child in denotable.values())


# DICTIONARY ###########################################################################################################

class Dictionary(ConstNamedTuple):
    """
    A Dictionary can be attached to a Value to access Record entries by name.
    The Record keys are not part of the data as they are not mutable, much like a Schema.
    The Dictionary is also ignored when two Schemas are compared, meaning a Record {x=float, y=float, z=float} will be
    compatible to {r=float, g=float, b=float}.
    The child Dictionaries need to be addressable by name and index, therefore we have a map [str -> index] and a vector
    that contain the actual child Dictionaries.
    Unnamed records still have a Dictionary, but with an empty name field.
    """
    names = c_field(type=Bonsai, mandatory=True)
    children = c_field(type=ConstList, mandatory=True)


def create_dictionary(denotable: Denotable) -> Optional[Dictionary]:
    """
    Recursive, depth-first assembly of a Dictionary.
    :param denotable: Denotable data to build the Dictionary from.
    :return: The Dictionary corresponding to the given denotable or None if it would be empty.
    """

    def parse_next(next_denotable: Denotable) -> Optional[Dictionary]:
        if isinstance(next_denotable, Value):
            return next_denotable._dictionary

        kind: Kind = Kind.from_denotable(next_denotable)

        if Kind.is_none(kind) or Kind.is_ground(kind):
            return None

        elif kind == Kind.LIST:
            assert len(next_denotable) > 0
            return parse_next(next_denotable[0])

        else:
            assert kind == Kind.RECORD
            if isinstance(next_denotable, tuple):
                return Dictionary(
                    names=Bonsai(),
                    children=make_const_list([parse_next(child) for child in next_denotable]),
                )
            else:
                assert isinstance(next_denotable, dict)
                return Dictionary(
                    names=Bonsai(list(next_denotable.keys())),
                    children=make_const_list([parse_next(child) for child in next_denotable.values()]),
                )

    return parse_next(denotable)


# VALUE ################################################################################################################

class Value:
    Kind = Kind
    Schema = Schema

    def __init__(self, *args, **kwargs):
        if args:
            assert not kwargs
            if len(args) == 1:
                obj: Any = args[0]
            else:
                obj: Tuple[Any, ...] = args
        elif kwargs:
            obj: Dict[str, Any] = kwargs
        else:
            obj: None = None

        self._schema: Schema = Schema()
        self._data: Data = None
        self._dictionary: Optional[Dictionary] = None

        # default (None) initialization
        if obj is None:
            return

        # copy initialization
        if isinstance(obj, Value):
            assert len(kwargs) == 0
            self._schema = obj._schema
            self._data = obj._data
            self._dictionary = obj._dictionary

        # initialization from Schema
        elif isinstance(obj, Schema):
            assert len(kwargs) == 0
            self._schema = obj
            self._data = create_data_from_schema(self._schema)
            self._dictionary = None

        # initialization from denotable
        else:
            denotable: Denotable = create_denotable(obj)
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

    def is_number(self) -> bool:
        return isinstance(self._data, float)

    def is_string(self) -> bool:
        return isinstance(self._data, str)

    def is_none(self) -> bool:
        return self._data is None

    def get_schema(self) -> Schema:
        """
        The Schema of this Value.
        """
        return self._schema

    def get_kind(self) -> Kind:
        """
        The Kind of this Value.
        """
        assert len(self._schema) > 0
        return Kind(self._schema[0])

    def get_keys(self) -> List[str]:
        """
        Returns the known keys.
        The returned list is empty if this is not a named record.
        """
        if self._dictionary is None:
            return []
        else:
            return self._dictionary.names.keys()

    def as_json(self) -> str:
        """
        Serializes this Value to a JSON formatted string.
        Unnamed records are stored as JSON objects with keys "0", "1", "2" ...
        Upon deserialization, a JSON object with integer keys starting at "0" and counting up (in order) for every
        key:value pair will be turned back into an unnamed record.
        This opens the possibility of someone wanting to store a named record with integer names, which could not be
        serialized.
        """

        def recursion(data: Any, iterator: int, dictionary: Optional[Dictionary]) -> str:
            assert iterator < len(self._schema)
            kind: Kind = Kind(self._schema[iterator])
            if kind == Kind.NUMBER:
                if trunc(data) == data:
                    # int
                    return write_json(int(data))
                else:
                    # float
                    return write_json(data)
            elif kind == Kind.STRING:
                # string
                return f'"{data}"'
            elif kind == Kind.LIST:
                # list
                iterator += 1
                return '[{}]'.format(', '.join(recursion(child_data, iterator, dictionary) for child_data in data[1:]))
            else:
                assert kind == Kind.RECORD
                assert (iterator + 1) < len(self._schema)
                child_count: int = self._schema[iterator + 1]
                if dictionary is None or dictionary.names.is_empty():
                    # unnamed record
                    assert dictionary is None or isinstance(dictionary, Dictionary)  # for pycharm
                    # unnamed records have <null> as their first element to differentiate them from lists
                    return '[null, {}]'.format(', '.join(
                        recursion(data[child_index], get_subschema_start(self._schema, iterator, child_index),
                                  None if dictionary is None else dictionary.children[child_index])
                        for child_index in range(child_count)))
                else:
                    # named record
                    keys: List[str] = dictionary.names.keys()
                    assert len(keys) == child_count
                    children: List[Tuple[str, str]] = []
                    for child_index in range(child_count):
                        child_name: str = keys[child_index]
                        child_value: str = recursion(
                            data[child_index],
                            get_subschema_start(self._schema, iterator, child_index),
                            dictionary.children[dictionary.names[child_name]])
                        children.append((child_name, child_value))
                    return '{{{}}}'.format(', '.join('"{}": {}'.format(key, value) for (key, value) in children))

        if self._schema.is_none():
            return write_json(None)
        else:
            return recursion(self._data, 0, self._dictionary)

    @classmethod
    def from_json(cls, json_string: str, reference: Optional[Value] = None) -> Value:
        """
        Deserializes a Value from a JSON formatted string.
        :param json_string: JSON formatted string.
        :param reference: Optional Value that must match the Schema of the serialized Value.
            Is required if the JSON contains empty lists.
        :raise JSONDecodeError: If the JSON could not be decoded.
        :raise ValueError: If the JSON object cannot be denoted by a Value.
        :return: The deserialized Value.
        """
        json_object: Any = read_json(json_string)
        if json_object is None:
            return Value()
        denotable: Denotable = create_denotable(
            json_object, list_can_be_record=True, allow_empty_list=reference is not None)
        return Value._create(
            reference._schema if reference is not None else Schema(denotable),
            create_data_from_denotable(denotable),
            reference._dictionary if reference is not None else create_dictionary(denotable))

    def __repr__(self) -> str:
        """
        Unlike __str__, this function produces a string meant for debugging.
        """

        def recursion(data: Any, iterator: int, dictionary: Optional[Dictionary]) -> str:
            assert iterator < len(self._schema)
            kind: Kind = Kind(self._schema[iterator])
            if kind == Kind.NUMBER:
                if trunc(data) == data:
                    # int
                    return f'{int(data)}'
                else:
                    # float
                    return f'{data}'
            elif kind == Kind.STRING:
                # string
                return f'"{data}"'
            elif kind == Kind.LIST:
                # list
                iterator += 1
                return '[{}]'.format(', '.join(recursion(child_data, iterator, dictionary) for child_data in data[1:]))
            else:
                assert kind == Kind.RECORD
                assert (iterator + 1) < len(self._schema)
                child_count: int = self._schema[iterator + 1]
                if dictionary is None or dictionary.names.is_empty():
                    # unnamed record
                    assert dictionary is None or isinstance(dictionary, Dictionary)  # for pycharm
                    return '{{{}}}'.format(', '.join(
                        recursion(data[child_index], get_subschema_start(self._schema, iterator, child_index),
                                  None if dictionary is None else dictionary.children[child_index])
                        for child_index in range(child_count)))
                else:
                    # named record
                    keys: List[str] = dictionary.names.keys()
                    assert len(keys) == child_count
                    children: List[Tuple[str, str]] = []
                    for child_index in range(child_count):
                        child_name: str = keys[child_index]
                        child_value: str = recursion(
                            data[child_index],
                            get_subschema_start(self._schema, iterator, child_index),
                            dictionary.children[dictionary.names[child_name]])
                        children.append((child_name, child_value))
                    return '{{{}}}'.format(', '.join('{}: {}'.format(key, value) for (key, value) in children))

        if self._schema.is_none():
            return f'Value(None)'
        else:
            return f'Value({recursion(self._data, 0, self._dictionary)})'

    def __eq__(self, other: Any) -> bool:
        """
        Equality test.
        """
        if isinstance(other, Value):
            return self._data == other._data and self._schema == other._schema
        elif isinstance(other, (int, float)):
            return self.get_kind() == Value.Kind.NUMBER and self._data == float(other)
        elif isinstance(other, str):
            return self.get_kind() == Value.Kind.STRING and self._data == other
        return False

    def __ne__(self, other: Any) -> bool:
        """
        Inequality tests.
        """
        return not self.__eq__(other)

    def __lt__(self, other: Any) -> bool:
        other_number: Optional[float] = check_number(other)
        if other_number is None:
            return NotImplemented
        else:
            return float(self) < other_number

    def __le__(self, other: Any) -> bool:
        other_number: Optional[float] = check_number(other)
        if other_number is None:
            return NotImplemented
        else:
            return float(self) <= other_number

    def __gt__(self, other: Any) -> bool:
        other_number: Optional[float] = check_number(other)
        if other_number is None:
            return NotImplemented
        else:
            return float(self) > other_number

    def __ge__(self, other: Any) -> bool:
        other_number: Optional[float] = check_number(other)
        if other_number is None:
            return NotImplemented
        else:
            return float(self) >= other_number

    def __float__(self) -> float:
        if self.is_number():
            return self._data
        else:
            raise TypeError("Value is not a number")

    def __int__(self) -> int:
        if self.is_number():
            return int(self._data)
        else:
            raise TypeError("Value is not a number")

    def __str__(self) -> str:
        if self.is_string():
            return self._data
        else:
            raise TypeError("Value is not a string")

    def __len__(self) -> int:
        """
        The number of child Elements if the current Element is a List or Map, or zero otherwise.
        """
        kind: Kind = self.get_kind()

        if kind == Kind.LIST:
            assert len(self._data) > 0
            list_size: int = self._data[0]
            assert list_size == len(self._data) - 1  # -1 for the first integer that is the size of the list
            return list_size

        elif kind == Kind.RECORD:
            assert len(self._schema) > 1
            return self._schema[1]

        else:
            return 0  # only lists and records

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

    def __neg__(self) -> Value:
        return Value._create(self._schema, -float(self), self._dictionary)

    def __pos__(self) -> Value:
        return Value._create(self._schema, +float(self), self._dictionary)

    def __abs__(self) -> Value:
        return Value._create(self._schema, abs(float(self)), self._dictionary)

    def __trunc__(self) -> Value:
        return Value._create(self._schema, float(trunc(float(self))), self._dictionary)

    def __floor__(self) -> Value:
        return Value._create(self._schema, float(floor(float(self))), self._dictionary)

    def __ceil__(self) -> Value:
        return Value._create(self._schema, float(ceil(float(self))), self._dictionary)

    def __round__(self, digits: Optional[int] = None) -> Value:
        return Value._create(self._schema, float(round(float(self), digits)), self._dictionary)

    def __truediv__(self, other: Any) -> Value:
        other_number: Optional[float] = check_number(other)
        if other_number is None:
            return NotImplemented
        else:
            return Value._create(self._schema, float(truediv(float(self), other)), self._dictionary)

    def __rtruediv__(self, other: Any) -> float:
        return truediv(other, float(self))

    def __floordiv__(self, other: Any) -> Value:
        other_number: Optional[float] = check_number(other)
        if other_number is None:
            return NotImplemented
        else:
            return Value._create(self._schema, float(floordiv(float(self), other)), self._dictionary)

    def __rfloordiv__(self, other: Any) -> int:
        return floordiv(other, float(self))

    def __add__(self, other: Any) -> Value:
        other_number: Optional[float] = check_number(other)
        if other_number is None:
            return NotImplemented
        else:
            return Value._create(self._schema, float(self) + other_number, self._dictionary)

    def __radd__(self, other: Any) -> float:
        other_number: Optional[float] = check_number(other)
        if other_number is None:
            return NotImplemented
        else:
            return other_number + float(self)

    def __sub__(self, other: Any) -> Value:
        other_number: Optional[float] = check_number(other)
        if other_number is None:
            return NotImplemented
        else:
            return Value._create(self._schema, float(self) - other_number, self._dictionary)

    def __rsub__(self, other: Any) -> float:
        other_number: Optional[float] = check_number(other)
        if other_number is None:
            return NotImplemented
        else:
            return other_number - float(self)

    def __mul__(self, other: Any) -> Value:
        other_number: Optional[float] = check_number(other)
        if other_number is None:
            return NotImplemented
        else:
            return Value._create(self._schema, float(self) * other_number, self._dictionary)

    def __rmul__(self, other: Any) -> float:
        other_number: Optional[float] = check_number(other)
        if other_number is None:
            return NotImplemented
        else:
            return other_number * float(self)

    def __mod__(self, other: Any) -> Value:
        other_number: Optional[float] = check_number(other)
        if other_number is None:
            return NotImplemented
        else:
            return Value._create(self._schema, float(mod(float(self), other)), self._dictionary)

    def __rmod__(self, other: Any) -> float:
        return mod(other, float(self))

    def __pow__(self, exponent: Any) -> Value:
        other_number: Optional[float] = check_number(exponent)
        if other_number is None:
            return NotImplemented
        else:
            return Value._create(self._schema, pow(float(self), exponent), self._dictionary)

    def __rpow__(self, base: Any) -> float:
        return pow(base, float(self))

    def _get_item_by_index(self, index: int) -> Value:
        assert isinstance(self._data, ConstList)

        # this method must only be called on list and record values
        kind: Kind = self.get_kind()
        assert kind == Kind.LIST or kind == Kind.RECORD

        # lists have one additional entry at the beginning where they store the size of the list
        size: int = len(self._data)
        if kind == kind.LIST:
            assert len(self._data) > 0  # at least the size of the list must be stored in it
            size -= 1
        else:
            assert size == self._schema[1]

        # support negative indices from the back
        if index < 0:
            index = size + index

        # make sure that the index is valid
        if not (0 <= index < size):
            raise KeyError(f'Cannot get Element at index {size - index} from a '
                           f'{"List" if kind == Kind.LIST else "Record"} of size {size}')

        if kind == Kind.LIST:
            # the child Schema starts at index 1
            start: int = 1
            end: int = get_subschema_end(self._schema, start)
            schema: Schema = Schema.from_slice(self._schema, start, end)

            # children in list data start at index 1
            assert len(self._data) > index + 1
            data: Data = self._data[index + 1]

            # since this is not a record, we do not have to change the dictionary
            dictionary: Optional[Dictionary] = self._dictionary

        else:
            start_index: int = get_subschema_start(self._schema, 0, index)
            end_index: int = get_subschema_end(self._schema, start_index)
            schema: Schema = Schema.from_slice(self._schema, start_index, end_index)

            # children in record data start at index 0
            data: Data = self._data[index]

            # since this is a record, we need to advance to the child's dictionary
            dictionary: Dictionary = self._dictionary.children[index]

        return Value._create(schema, data, dictionary)

    def _get_item_by_name(self, name: str) -> Value:
        """

        :param name:
        :return:
        """
        assert isinstance(self._data, ConstList)
        assert self.get_kind() == Kind.RECORD
        assert self._dictionary is not None

        if self._dictionary.names.is_empty():
            raise KeyError(f'This Record Value has only unnamed entries, use an index to access them')

        if name not in self._dictionary.names:
            available_keys: str = '", "'.join(self._dictionary.names.keys())  # this is a Record, there is at least one
            raise KeyError(f'Unknown key "{name}" in Record. Available keys are: "{available_keys}"')

        assert len(self._data) == self._schema[1]
        index: int = self._dictionary.names[name]
        assert 0 <= index < len(self._data)

        start_index: int = get_subschema_start(self._schema, 0, index)
        end_index: int = get_subschema_end(self._schema, start_index)
        schema: Schema = Schema.from_slice(self._schema, start_index, end_index)

        return Value._create(schema, self._data[index], self._dictionary.children[index])

    # noinspection PyMethodMayBeStatic
    def _is_consistent(self) -> bool:
        """
        Checks if the Schema, the Data and the Dictionaries of this Value are consistent.
        This can safely be assumed when you create a Value from a Denotable or Schema, but not when assembling the
        individual members through Value.create.
        Especially in the case where you have a given Schema and a Data object that might contain empty lists, you
        cannot simply get the Schema from the Data to compare it.
        """
        return True  # TODO: Value._is_consistent


# MUTATION #############################################################################################################

def mutate_data(current_data: Data, new_data: Any, schema: Schema, schema_itr: int) -> Tuple[Data, bool]:
    """
    The last step when mutating Data.
    Does not change the `current_data` field.
    :param current_data: The currently iterated Data (sub)-buffer prior to mutation.
    :param new_data: The new Data to place at the end of the `path`. Must match the existing Data's Schema.
    :param schema: Schema of the Value providing the Data buffer.
    :param schema_itr: Position of the iteration in the Schema.
    :returns: (Potentially) New Data and a flag indicating whether or not it is different than `current_data`.
    """
    # get the kind of data expected at the current location in the Schema
    assert schema_itr < len(schema)
    kind: Kind = Kind(schema[schema_itr])
    assert Kind.is_valid(kind)

    # ensure that the new data is denotable
    denotable: Denotable = create_denotable(new_data, allow_empty_list=True)

    # set to empty list
    if isinstance(denotable, list) and len(denotable) == 0:
        if kind == Kind.LIST:
            if len(current_data) == 0:
                return current_data, False
            else:
                return create_data_from_denotable(denotable), True
        else:
            raise TypeError(f'Type mismatch, cannot set a Value of kind {kind.name.capitalize()} '
                            f'to the empty list')

    # check if the data's Schema matches the child Value's
    data_schema: Schema = Schema(denotable)  # cannot create a Schema for an empty list
    current_schema: Schema = Schema.from_slice(schema, schema_itr, get_subschema_end(schema, schema_itr))
    if data_schema != current_schema:
        raise TypeError(f'Cannot mutate a Value of kind {kind.name.capitalize()} to "{denotable}"')

    # return the original Data if it and the new one are equal
    result_data: Data = create_data_from_denotable(denotable)
    if result_data == current_data:
        return current_data, False
    else:
        return result_data, True


def mutate_recursive(current_data: Data, new_data: Any, schema: Value.Schema, schema_itr: int,
                     dict_itr: Optional[Dictionary], path: Sequence[Union[int, str], ...]) -> Tuple[Data, bool]:
    """
    Mutates a Value Data and returns the resulting Data with a flag that indicates whether the Data has been changed.
    This is separate from the `mutate_value` function to allow the data to be transient.
    :param current_data: The currently iterated Data (sub)-buffer prior to mutation.
    :param new_data: The new Data to place at the end of the `path`. Must match the existing Data's Schema.
    :param schema: Schema of the Value providing the Data buffer.
    :param schema_itr: Position of the iteration in the Schema.
    :param dict_itr: The current Dictionary associated with the Data buffer.
    :param path: Remaining path to where the `new_data` is to be placed.
    :returns: The (potentially) mutated Data and a flag indicating whether or not it has changed.
    """
    # last step
    if len(path) == 0:
        return mutate_data(current_data, new_data, schema, schema_itr)

    # get the kind of data expected at the current location in the Schema
    assert schema_itr < len(schema)
    kind: Kind = Kind(schema[schema_itr])
    assert Kind.is_valid(kind)

    # cannot continue recursion past a ground value
    if Kind.is_ground(kind):
        raise IndexError(f'Unsupported operator[] for Value of kind {kind.name.capitalize()}')
    assert kind in (Kind.LIST, Kind.RECORD)

    # pop the index of the next child from the path
    index: Union[int, str] = path[0]
    if isinstance(index, str):
        if dict_itr is None:
            assert kind == Kind.LIST
            raise KeyError(f'Cannot access List by name "{index}"')
        elif dict_itr.names.is_empty():
            raise KeyError(f'Cannot access unnamed Record by name "{index}"')
        elif index not in dict_itr.names:
            available_keys: str = '", "'.join(dict_itr.names.keys())
            raise KeyError(f'Unknown key "{index}" in Record. Available keys are: "{available_keys}"')
        index = dict_itr.names[index]

    else:
        assert isinstance(index, int)

        # support negative indices from the back
        if index < 0:
            size: int = len(current_data)
            if kind == kind.LIST:
                assert len(current_data) > 0  # at least the size of the list must be stored in it
                size -= 1
            else:
                assert schema_itr + 1 < len(schema)
                assert size == schema[schema_itr + 1]
            index = size + index

        # list items start at index 1
        if kind == kind.LIST:
            index += 1

    assert isinstance(index, int) and (0 <= index < len(current_data))

    # advance the schema iterator
    if kind == Kind.LIST:
        schema_itr += 1
    else:
        schema_itr = get_subschema_start(schema, schema_itr, index)

    # advance the dictionary iterator
    if kind == Kind.RECORD:
        dict_itr = dict_itr.children[index]

    # continue the recursion
    result_data, was_updated = mutate_recursive(current_data[index], new_data, schema, schema_itr, dict_itr, path[1:])
    if was_updated:
        return current_data.set(index, result_data), True
    else:
        return current_data, False


@overload
def mutate_value(value: Value, new_data: Any) -> Value: ...


@overload
def mutate_value(value: Value, key: str, new_data: Any) -> Value: ...


@overload
def mutate_value(value: Value, index: int, new_data: Any) -> Value: ...


@overload
def mutate_value(value: Value, path: Sequence[Union[int, str]], new_data: Any) -> Value: ...


def mutate_value(value: Value, *args) -> Value:
    """
    Mutates a Value, meaning create a new Value through a single modification of an existing one.
    For changing multiple parts of a Value's data at once, see `multimutate_value`.
    If the new Value would the equal to the old one, the old one is returned to avoid the creation of a new instance.

    This function allows several overloads:

        1. mutate_value(value, new_data)
        2. mutate_value(value, key, new_data)
        3. mutate_value(value, index, new_data)
        4. mutate_value(value, path, new_data)

    1. Only works for ground types, where the new_data matches the Schema of the existing data.
    2 and 3. Works for shallow record lookup or lists.
    4. Works for all cases, but requires the specification of a path, a (potentially empty) sequence of key or indices
        the part to change.

    IN C++, value["key][4] could return an accessor which would then mutate the Value on assignment. The accessor
    retains a reference to the root Value, so that `set_value` can modify it, instead of the last one in the Path.
    In C++ we can have an implicit conversion between a Value.Accessor and a Value and use r-value overloads to ensure
    that the user does not accidentally store a Value.Accessor instead of a Value, but in Python we cannot.

    :param value: Value providing the Data buffer to mutate.
    """
    if len(args) == 0:  # noop
        return value

    elif len(args) == 1:
        if value.is_none():
            if args[0] is None:
                return value  # setting None to None returns None
            else:
                raise ValueError("Cannot modify the None Value.")
        new_data, was_modified = mutate_data(value._data, args[0], value.get_schema(), 0)
        if was_modified:
            return Value._create(value._schema, new_data, value._dictionary)
        else:
            return value

    else:
        assert len(args) == 2
        if value.is_none():
            raise ValueError("Cannot modify the None Value.")
        new_data: Data
        was_modified: bool
        if isinstance(args[0], (int, str)):
            new_data, was_modified = mutate_recursive(
                value._data, args[1], value.get_schema(), 0, value._dictionary, (args[0],))
        else:
            assert isinstance(args[0], (list, tuple))
            new_data, was_modified = mutate_recursive(
                value._data, args[1], value.get_schema(), 0, value._dictionary, args[0])
        if was_modified:
            return Value._create(value._schema, new_data, value._dictionary)
        else:
            return value


def multimutate_value(value: Value, *changes: Tuple[Union[int, str, Sequence[Union[int, str]]], Any]) -> Value:
    """
    Performs multiple mutations on the given value.
    :param value: Value providing the Data buffer to mutate.
    :param changes: Path / New Data pair to describe a change in the Value's Data buffer.
    """
    if value.is_none():
        raise ValueError("Cannot modify the None Value.")

    was_at_all_modified: bool = False
    was_modified: bool
    new_data: Data = value._data
    for change in changes:
        assert len(change) == 2
        if isinstance(change[0], (int, str)):
            new_data, was_modified = mutate_recursive(
                new_data, change[1], value.get_schema(), 0, value._dictionary, (change[0],))
        else:
            assert isinstance(change[0], (list, tuple))
            new_data, was_modified = mutate_recursive(
                new_data, change[1], value.get_schema(), 0, value._dictionary, change[0])
        was_at_all_modified |= was_modified

    if was_at_all_modified:
        return Value._create(value._schema, new_data, value._dictionary)
    else:
        return value

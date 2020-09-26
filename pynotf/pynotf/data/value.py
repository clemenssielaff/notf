from __future__ import annotations

from enum import IntEnum
from typing import Union, List, Dict, Any, Sequence, Optional, Tuple, Iterable, Callable, overload
from math import trunc, ceil, floor, pow
from operator import floordiv, mod, truediv
from json import dumps as write_json, loads as read_json

from pyrsistent import pvector as make_const_list, PVector as ConstList, PRecord as ConstNamedTuple, field as c_field
from pyrsistent.typing import PVector as ConstListT

from pynotf.data import Bonsai, Z85

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
    """
    Checks if the given number can be converted to a float (the only number type that can be stored in a Value).
    """
    converter: Optional[Callable[[], float]] = getattr(obj, "__float__", None)
    if converter is None:
        return None
    else:
        return float(converter())


def create_denotable(obj: Any) -> Denotable:
    """
    Tries to convert any given Python object into a Denotable.
    Denotable are still pure Python objects, but they are known to be capable of being stored in a Value.
    :param obj: Object to convert.
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
        denotable: List[Denotable] = [create_denotable(x) for x in obj]
        if len(denotable) == 0:
            return []

        # ensure that all children have the same schema but allow empty list children
        for idx, reference in enumerate(denotable):
            if isinstance(reference, list) and len(reference) == 0:
                continue
            reference_schema: Schema = schema_from_denotable(reference)
            for other_child in denotable[idx + 1:]:
                if isinstance(other_child, list) and len(other_child) == 0:
                    continue
                elif not is_denotable_valid_for_schema(other_child, reference_schema):
                    raise ValueError("All items in a Value.List must have the same Schema")
            break
        # ... unless all of the children are empty lists
        else:
            raise ValueError("Cannot define a List Schema with only empty child lists")

        if Kind.from_denotable(reference) == Kind.RECORD:
            def get_keys(rep: Denotable) -> Optional[List[str]]:
                if isinstance(rep, dict):
                    return list(rep.keys())
                else:
                    assert isinstance(rep, tuple)
                    return None

            reference_keys: Optional[List[str]] = get_keys(reference)
            for i in range(1, len(denotable)):
                if get_keys(denotable[i]) != reference_keys:
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
            denotable[key] = create_denotable(value)
        return denotable

    # schema (can be empty)
    elif isinstance(obj, Schema):
        return [float(word) for word in obj]

    # unnamed record
    elif isinstance(obj, tuple):
        if len(obj) == 0:
            raise ValueError("Records cannot be empty")

        return tuple(create_denotable(value) for value in obj)

    # incompatible type
    raise ValueError("Cannot construct Denotable from {}".format(type(obj).__name__))


# KIND #################################################################################################################

class Kind(IntEnum):
    """
    Enumeration of the denotable Value Kinds.

    At the start of the value range of a Schema word (uint8), we can use zero and one as identifiers because they can
    never be valid offsets. A zero offset makes no sense and a one offset is a special case that is "inlined".
    At the end of the range, we use the highest available integers to allow the space in between to be used as valid
    offsets.
    """
    RECORD = 0
    LIST = 1
    NUMBER = 253
    STRING = 254
    VALUE = 255

    _LEFT = LIST
    _RIGHT = NUMBER

    # NONE is not a Kind, we just need an id for it. If we encounter it inside a Schema it is treated as an offset.
    NONE = _LEFT + 1
    assert _LEFT < NONE < _RIGHT

    @staticmethod
    def is_valid(value: int) -> bool:
        """
        :return: True iff `value` is a valid Kind enum value.
        """
        return value >= 0 and not Kind.is_offset(value)

    @staticmethod
    def is_offset(value: int) -> bool:
        """
        :return: True iff `value` is not a valid Kind enum value but an offset in a Schema.
        """
        return Kind._LEFT < value < Kind._RIGHT

    @staticmethod
    def is_ground(value: int) -> bool:
        """
        :return: Whether the integer denotes one of the ground Kinds: Number, String and Value.
        """
        return value >= Kind._RIGHT

    @staticmethod
    def from_denotable(denotable: Optional[Denotable]) -> Kind:
        """
        :return: The Kind of the given denotable Python data.
        """
        assert denotable is not None
        if isinstance(denotable, Value):
            return Kind.VALUE
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

    @staticmethod
    def from_slice(base: Schema, start: int, end: int) -> Schema:
        return Schema(base[start: end])

    @staticmethod
    def from_ground_kind(kind: Kind) -> Schema:
        assert Kind.is_ground(kind)
        return Schema((int(kind),))

    @staticmethod
    def from_value(value: Value) -> Optional[Schema]:
        """
        Creates a Schema from a list of numbers stored in a Value.
        :param value: Value storing the Schema.
        :return: The new Schema or None on error.
        """
        if value.get_kind() != Value.Kind.LIST or Value.Kind(value.get_schema()[1]) != Value.Kind.NUMBER:
            return None
        if len(value) == 0:
            return Schema()
        schema: Schema = Schema(int(value[i]) for i in range(len(value)))
        return schema if Schema.validate(schema) else None

    @staticmethod
    def validate(_: Schema) -> bool:
        return True  # TODO: validate Schema

    @staticmethod
    def create(obj: Any) -> Schema:
        """
        Helper function to create Schemas from anything encountered in the wild.
        :raise ValueError: If you cannot create a Value.Schema from the given object.
        :returns: Schema describing how a Value containing the given Denotable would be laid out.
        """
        if obj is None:
            return Schema()
        return schema_from_denotable(create_denotable(obj))

    @staticmethod
    def any() -> Schema:
        """
        Create the "Any" Schema.
        """
        return Schema((int(Kind.VALUE),))

    def is_none(self) -> bool:
        """
        Checks if this is the None Schema.
        """
        return len(self) == 0

    def is_any(self) -> bool:
        """
        Checks if this is the Any Schema.
        """
        return len(self) == 1 and self[0] == int(Kind.VALUE)

    def is_number(self) -> bool:
        """
        Checks if this Schema represents a single number.
        """
        return len(self) == 1 and self[0] == int(Kind.NUMBER)

    def is_string(self) -> bool:
        """
        Checks if this Schema represents a single string.
        """
        return len(self) == 1 and self[0] == int(Kind.STRING)

    def is_list(self) -> bool:
        """
        Checks if this Schema represents a single list.
        """
        return not self.is_none() and self[0] == int(Kind.LIST)

    def is_record(self) -> bool:
        """
        Checks if this Schema represents a single record.
        """
        return not self.is_none() and self[0] == int(Kind.RECORD)

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

    def as_z85(self) -> bytes:
        """
        Encodes this Schema to Z85.
        The Z85 output is left-padded with NONE bytes because they can never be the first word.
        """
        raw_bytes: bytes = bytes(self)
        if len(raw_bytes) == 0:
            return raw_bytes
        padding: int = 3 - ((len(raw_bytes) - 1) % 4)  # pad to a multiple of 4
        return Z85.encode(bytes([Kind.NONE] * padding) + raw_bytes)

    @staticmethod
    def from_z85(z85_bytes: bytes) -> Schema:
        """
        Decodes a Schema from a Z85 encoded byte stream.
        """
        return Schema(Z85.decode(z85_bytes).lstrip(bytes([Kind.NONE])))

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
            elif word == Kind.RECORD:
                result += "Record"
                next_word_is_record_size = True
            elif word == Kind.LIST:
                result += "List"
            elif word == Kind.NUMBER:
                result += "Number"
            elif word == Kind.STRING:
                result += "String"
            elif word == Kind.VALUE:
                result += "Value"
            else:  # offset
                result += "→ {}".format(index + word)
            result += "\n"
        return result


def schema_from_denotable(denotable: Denotable) -> Schema:
    """
    Recursive, breadth-first assembly of a Schema for the given Denotable.
    :returns: Schema describing how a Value containing the given Denotable would be laid out.
    :raise ValueError: If you cannot create a Value.Schema from the given object.
    """
    assert denotable is not None

    schema: List[int] = []
    kind: Kind = Kind.from_denotable(denotable)

    if isinstance(denotable, Value):
        # Values take up a single word in a Schema, the VALUE Kind.
        schema.append(int(Kind.VALUE))

    elif kind == Kind.NUMBER:
        # Numbers take up a single word in a Schema, the NUMBER Kind.
        schema.append(int(Kind.NUMBER))

    elif kind == Kind.STRING:
        # Strings take up a single word in a Schema, the STRING Kind.
        schema.append(int(Kind.STRING))

    elif kind == Kind.LIST:
        # A List Schema is a pair [LIST Kind, child Schema].
        # All Values contained in a List must have the same Schema.
        # An example Schema of a List is:
        #
        # ||    1.     ||     2.    | ... ||
        # || List Type || ChildType | ... ||
        # ||- header --||---- child ------||
        #
        schema.append(int(Kind.LIST))
        for child in denotable:
            if isinstance(child, list) and len(child) == 0:
                continue
            schema.extend(schema_from_denotable(child))
            break
        else:
            raise ValueError("Lists cannot be empty for Schema definition")

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

                # The forward offset must be less than the first Kind identifier on the right of the word range.
                # I expect this to hold for the foreseeable future, because you can be very expressive with Schemas
                # of small sizes.
                # However, there is a plan B that we could investigate should large Schemas ever become a problem.
                # The idea is that instead of storing the raw forward offset, in the body of the record, you use
                # the previous offsets to accumulate the total. In the example above, word 4 would keep its offset
                # of 3 because it is the first offset. However, word 6 would have an offset of only 5 instead of
                # 8, because it would implicitly add the 3 from word 4. And while that does not look like a lot,
                # imagine that the second type in the record is a deeply nested record type that is 100 words long.
                # Then word 6 would need to point to word 108 and would require an offset of 102 with the current,
                # and 99 with the proposed method. ... Still not great. But a hypothetical third offset in the body
                # would only need to store the forward offset forward, maybe another 3 or 4. And not 105 or 106.
                # This is were the beauty is, the only way to run out of space is if you have a single element that
                # is huge, not if you have many ones that are small. ... And in the worst case, you could always use
                # a nested Value for those.
                #
                # The reason why we don't do it though is that you need to iterate through the entire body up to the
                # word that you want, at least if it is an offset. Right now, you can jump to the word, read the
                # offset and jump on to the child in question with no iteration required.
                # Of course, iterating over tightly packed bytes is hardly a problem, but why do it if we do not see
                # a need for it at the moment or the foreseeable future?
                else:
                    assert offset < Kind._RIGHT
                    schema[child_position] = offset  # forward offset
                schema.extend(schema_from_denotable(child))
            child_position += 1

    return Schema(schema)


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


def is_denotable_valid_for_schema(denotable: Denotable, schema: Schema) -> bool:
    """
    If a Denotable contains an empty list, it is not possible to derive a Schema to compare against an existing one.
    This function checks if a Denotable would be valid data for a Value with the given Schema without requiring the
    Denotable to contain no empty lists.
    """
    if schema.is_none():
        return denotable is None

    def recursion(schema_itr: int, child: Denotable) -> bool:
        kind: Kind = Kind(schema[schema_itr])

        # decision about ground types are easy
        if kind == Kind.NUMBER:
            return isinstance(child, float)
        elif kind == Kind.STRING:
            return isinstance(child, str)
        elif kind == Kind.VALUE:
            return isinstance(child, Value)

        # lists
        elif kind == Kind.LIST:
            if not isinstance(child, list):
                return False
            if len(child) == 0:  # empty lists always match
                return True
            # we know the first item is representable for all, as the child is a denotable
            return recursion(schema_itr + 1, child[0])

        # records
        else:
            assert kind == Kind.RECORD
            if not isinstance(child, (dict, tuple)):
                return False
            if len(child) != schema[schema_itr + 1]:  # not the same number of entries
                return False
            for index, grandchild in enumerate(child if isinstance(child, tuple) else child.values()):
                if not recursion(get_subschema_start(schema, schema_itr, index), grandchild):
                    return False
            return True

    return recursion(0, denotable)


# DATA #################################################################################################################

Data = Union[
    None,  # the only data stored in a None Value
    float,  # numbers are ground and immutable by design
    int,  # the size of a list is stored as an unsigned integer which are immutable by design
    str,  # strings are ground and immutable because of their Python implementation
    ConstListT['Data'],  # references to child Data are immutable through the use of ConstList
    'Value',  # Values are immutable by induction
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

        # value
        elif schema[index] == Kind.VALUE:
            return Value()

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


def data_from_denotable(denotable: Denotable) -> Data:
    assert denotable is not None

    if isinstance(denotable, Value):
        return denotable

    kind: Kind = Kind.from_denotable(denotable)

    if Kind.is_ground(kind):
        return denotable

    elif kind == Kind.LIST:
        list_size: int = len(denotable)
        if list_size > 0:
            return make_const_list((list_size, *(data_from_denotable(child) for child in denotable)))
        else:
            return make_const_list((0,))  # empty list

    assert kind == Kind.RECORD
    if isinstance(denotable, tuple):
        return make_const_list(data_from_denotable(child) for child in denotable)

    assert isinstance(denotable, dict)
    return make_const_list(data_from_denotable(child) for child in denotable.values())


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
        kind: Kind = Kind.from_denotable(next_denotable)

        if Kind.is_ground(kind):
            return None

        elif kind == Kind.LIST:
            if len(next_denotable) == 0:
                return None
            else:
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

        # initialization from Schema
        if isinstance(obj, Schema):
            assert len(kwargs) == 0
            self._schema = obj
            self._data = create_data_from_schema(self._schema)
            self._dictionary = None

        # initialization from denotable
        else:
            denotable: Denotable = create_denotable(obj)
            self._schema = schema_from_denotable(denotable)
            self._data = data_from_denotable(denotable)
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

    def is_any(self) -> bool:
        return isinstance(self._data, Value)

    def get_schema(self) -> Schema:
        """
        The Schema of this Value.
        """
        return self._schema

    def get_kind(self) -> Kind:
        """
        The Kind of this Value.
        """
        if len(self._schema) == 0:
            return Kind.NONE
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
        """

        def recursion(data: Any, schema: Schema, schema_itr: int, dictionary: Optional[Dictionary]) -> Any:
            assert schema_itr < len(schema)
            kind: Kind = Kind(schema[schema_itr])
            if kind == Kind.NUMBER:
                if trunc(data) == data:
                    # int
                    return int(data)
                else:
                    # float
                    assert isinstance(data, float)
                    return data
            elif kind == Kind.STRING:
                # string
                assert isinstance(data, str)
                return data
            elif kind == Kind.VALUE:
                # value
                assert isinstance(data, Value)
                return dict(
                    schema=data._schema.as_z85().decode(),
                    data=None if data._schema.is_none() else recursion(data._data, data._schema, 0, data._dictionary),
                )
            elif kind == Kind.LIST:
                # list
                assert isinstance(data, ConstList) and len(data) > 0 and data[0] == len(data) - 1
                schema_itr += 1
                return [recursion(child_data, schema, schema_itr, dictionary) for child_data in data[1:]]
            else:
                assert kind == Kind.RECORD
                assert (schema_itr + 1) < len(schema)
                child_count: int = schema[schema_itr + 1]
                if dictionary is None or dictionary.names.is_empty():
                    # unnamed record
                    assert dictionary is None or isinstance(dictionary, Dictionary)  # for pycharm
                    return [recursion(data[child_index], schema,
                                      get_subschema_start(schema, schema_itr, child_index),
                                      None if dictionary is None else dictionary.children[child_index])
                            for child_index in range(child_count)]
                else:
                    # named record
                    keys: List[str] = dictionary.names.keys()
                    assert len(keys) == child_count
                    return {
                        child_name: recursion(
                            data[child_index], schema,
                            get_subschema_start(schema, schema_itr, child_index),
                            dictionary.children[dictionary.names[child_name]])
                        for child_index, child_name in ((index, keys[index]) for index in range(child_count))
                    }

        return write_json(dict(
            head=dict(
                type="notf-value",
                version=1,
                binary='z85<',  # small endian Z85 encoding
            ),
            body=recursion(self, Schema.from_ground_kind(Kind.VALUE), 0, None),
        ))

    @classmethod
    def from_json(cls, json_string: str) -> Value:
        """
        Deserializes a Value from a JSON formatted string.
        :raise JSONDecodeError: If the JSON could not be decoded.
        :raise Z85.DecodeError: If a z85 encoded Schema could not be decoded.
        :raise ValueError: If the JSON object cannot be denoted by a Value.
        :return: The deserialized Value.
        """
        json_object: Any = read_json(json_string)

        # check head
        if 'head' not in json_object:
            raise ValueError(f'JSON encoded Value is missing the `head` tag')
        head: Dict[str, Any] = json_object['head']

        # check type
        if 'type' not in head:
            raise ValueError(f'JSON encoded Value head is missing the `type` tag')
        type_id: str = head['type']
        if type_id != 'notf-value':
            raise ValueError(f'JSON encoded type "{type_id}" cannot be decoded into a Value (must be "notf-value")')

        # check binary
        if 'binary' not in head:
            raise ValueError(f'JSON encoded Value head is missing the `binary` tag')
        binary_id: str = head['binary']
        if binary_id != 'z85<':
            raise ValueError(f'JSON encoded Values require binary format "z85<" (small-endian Z85), not "{binary_id}"')

        # check body
        if 'body' not in json_object:
            raise ValueError("JSON encoded Value is missing the `body` tag")
        body: Dict[str, Any] = json_object['body']

        def value_recursion(value_dict: Dict[str, Any]) -> Value:

            # schema
            if 'schema' not in value_dict:
                raise ValueError("JSON encoded Value is missing the `schema` tag")
            encoded_schema: Any = value_dict['schema']
            if not isinstance(encoded_schema, str):
                raise ValueError("JSON encoded Value schema is not a string")
            result_schema: Schema = Schema.from_z85(encoded_schema.encode())

            # data
            if 'data' not in value_dict:
                raise ValueError("JSON encoded Value is missing the `data` tag")
            data_root: Any = value_dict['data']
            if data_root is None:
                return Value()

            def data_recursion(raw_data: Any, schema: Schema, schema_itr: int) -> Tuple[Data, Optional[Dictionary]]:
                kind: Kind = Kind(schema[schema_itr])
                if kind == Kind.NUMBER:
                    assert isinstance(raw_data, (float, int))
                    return float(raw_data), None
                elif kind == Kind.STRING:
                    assert isinstance(raw_data, str)
                    return raw_data, None
                elif kind == Kind.VALUE:
                    assert isinstance(raw_data, dict)
                    return value_recursion(raw_data), None
                elif kind == Kind.LIST:
                    assert isinstance(raw_data, list)
                    schema_itr += 1
                    child_data: List[Any] = [len(raw_data)]
                    dictionary: Optional[Dictionary] = None
                    for raw_child_data in raw_data:
                        data, dictionary = data_recursion(raw_child_data, schema, schema_itr)
                        child_data.append(data)
                    return make_const_list(child_data), dictionary
                else:
                    assert kind == Kind.RECORD
                    assert schema_itr < len(schema) + 1
                    assert schema[schema_itr + 1] == len(raw_data)
                    if isinstance(raw_data, list):
                        # unnamed record
                        child_data: List[Any] = []
                        child_dictionaries: List[Any] = []
                        for child_index, raw_child_data in enumerate(raw_data):
                            data, dictionary = data_recursion(
                                raw_child_data, schema, get_subschema_start(schema, schema_itr, child_index))
                            child_data.append(data)
                            child_dictionaries.append(dictionary)
                        return make_const_list(child_data), Dictionary(
                            names=Bonsai(), children=make_const_list(child_dictionaries))
                    else:
                        # named record
                        assert isinstance(raw_data, dict)
                        child_data: List[Any] = []
                        child_dictionaries: Dict = {}
                        for child_index, (child_name, raw_child_data) in enumerate(raw_data.items()):
                            data, dictionary = data_recursion(
                                raw_child_data, schema, get_subschema_start(schema, schema_itr, child_index))
                            child_data.append(data)
                            child_dictionaries[child_name] = dictionary
                        return make_const_list(child_data), Dictionary(
                            names=Bonsai(list(child_dictionaries.keys())),
                            children=make_const_list(child_dictionaries.values()))

            result_data, result_dictionary = data_recursion(data_root, result_schema, 0)
            return Value._create(result_schema, result_data, result_dictionary)

        return value_recursion(body)

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
            elif kind == Kind.VALUE:
                # value
                return repr(data)
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
            return int(round(float(self._data)))
        else:
            raise TypeError("Value is not a number")

    def __str__(self) -> str:
        if self.is_string():
            return self._data
        else:
            raise TypeError("Value is not a string")

    def __bool__(self) -> bool:
        kind: Kind = self.get_kind()
        if kind == Kind.NONE:
            return False
        elif kind == Kind.RECORD:
            return True  # records cannot be empty
        else:
            return bool(self._data)

    def __hash__(self):
        return hash((self._schema, self._data, self._dictionary))

    def __len__(self) -> int:
        """
        The number of child Elements if the current Element is a List or Map, or zero otherwise.
        """
        kind: Kind = self.get_kind()

        if kind == Kind.LIST:
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

    def __iter__(self) -> Iterable[Value]:
        if self.get_kind() not in (Kind.LIST, Kind.RECORD):
            return
        for index in range(len(self)):
            yield self[index]

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
            # C++ transition note:
            # I wonder if it is really the best idea to raise Exceptions here, when returning an Optional would
            # suffice as well.

        if kind == Kind.LIST:
            # children in list data start at index 1 (index 0 is the size of the list)
            assert len(self._data) > index + 1
            data: Data = self._data[index + 1]

            # if the child is a Value, return that instead of creating a new one
            if self._schema[1] == Kind.VALUE:
                assert isinstance(data, Value)
                return data

            # extract the child schema
            start: int = 1
            end: int = get_subschema_end(self._schema, start)
            schema: Schema = Schema.from_slice(self._schema, start, end)

            # since this is not a record, we do not have to change the dictionary
            dictionary: Optional[Dictionary] = self._dictionary

        else:
            assert kind == Kind.RECORD

            # children in record data start at index 0
            data: Data = self._data[index]

            # if the child is a Value, return that instead of creating a new one
            start_index: int = get_subschema_start(self._schema, 0, index)
            if self._schema[start_index] == Kind.VALUE:
                assert isinstance(data, Value)
                return data

            # extract the child schema
            end_index: int = get_subschema_end(self._schema, start_index)
            schema: Schema = Schema.from_slice(self._schema, start_index, end_index)

            # since this is a record, we need to advance to the child's dictionary
            dictionary: Dictionary = self._dictionary.children[index]

        # create a new value to wrap the child
        return Value._create(schema, data, dictionary)

    def _get_item_by_name(self, name: str) -> Value:
        assert isinstance(self._data, ConstList)
        assert self.get_kind() == Kind.RECORD
        assert self._dictionary is not None

        if self._dictionary.names.is_empty():
            raise KeyError(f'This Record Value has only unnamed entries, use an index to access them')

        if name not in self._dictionary.names:
            available_keys: str = '", "'.join(self._dictionary.names.keys())  # this is a Record, there is at least one
            raise KeyError(f'Unknown key "{name}" in Record. Available keys are: "{available_keys}"')

        # look up the index of the child by name
        assert len(self._data) == self._schema[1]
        index: int = self._dictionary.names[name]
        assert 0 <= index < len(self._data)

        # if the index points to a Value child, return that instead of creating a new one
        start_index: int = get_subschema_start(self._schema, 0, index)
        if self._schema[start_index] == Kind.VALUE:
            assert isinstance(self._data[index], Value)
            return self._data[index]

        # extract the child schema
        end_index: int = get_subschema_end(self._schema, start_index)
        schema: Schema = Schema.from_slice(self._schema, start_index, end_index)

        # create a new value to wrap the child
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

    # if the new data is a Value with the same Schema as the current data, extract the new data instead of attempting
    # to set the complete Value as data
    current_schema: Schema = Schema.from_slice(schema, schema_itr, get_subschema_end(schema, schema_itr))
    if isinstance(new_data, Value) and new_data.get_schema() == current_schema:
        return new_data._data, new_data._data != current_data  # note that this still creates a new value... :/

    # ensure that the new data is denotable
    denotable: Denotable = create_denotable(new_data)

    # check if the data's Schema matches the child Value's
    if not is_denotable_valid_for_schema(denotable, current_schema):
        raise TypeError(f'Cannot mutate a Value of kind {kind.name.capitalize()} to "{denotable!r}"')

    # return the original Data if it and the new one are equal
    result_data: Data = data_from_denotable(denotable)
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
def get_mutated_value(value: Value, new_data: Any) -> Value: ...


@overload
def get_mutated_value(value: Value, key: str, new_data: Any) -> Value: ...


@overload
def get_mutated_value(value: Value, index: int, new_data: Any) -> Value: ...


@overload
def get_mutated_value(value: Value, path: Sequence[Union[int, str]], new_data: Any) -> Value: ...


def get_mutated_value(value: Value, *args) -> Value:
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

    If you try to set a Value A to another Value B which has the same Schema as A, this function will set A to the data
    contained _within_ B, not B itself.

    :param value: Value providing the Data buffer to mutate.
    """
    if len(args) == 1:
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

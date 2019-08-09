import sys
from enum import IntEnum, auto
from copy import copy
from typing import NewType, List, Dict, Union, Optional, Sequence, Any

Value = NewType('Value', Union[float, str, List["Value"], Dict[str, "Value"]])
"""
The `Value` kind is a variant of the four data kinds that can be stored in a StructuredBuffer.
It defines a single value including all of its child Values (if the value is a List or a Map)
"""


def check_value(raw: Any) -> Value:
    """
    Tries to convert any given "raw" value into a StructuredBuffer Value.
    :param raw: Raw value to convert.
    :return: The given raw value as a StructuredBuffer Value.
    :raise ValueError: If the raw value cannot be represented as a StructuredBuffer Value.
    """
    if isinstance(raw, (str, float)):
        # native Value types
        return raw

    if isinstance(raw, int):
        # types trivially convertible to a Value kind
        return float(raw)

    if isinstance(raw, list):
        # lists
        if len(raw) == 0:
            raise ValueError("Lists cannot be empty")

        values: List[Value] = [check_value(x) for x in raw]
        reference_value: Value = values[0]
        reference_schema: Schema = Schema(reference_value)
        for i in range(1, len(values)):
            if Schema(values[i]) != reference_schema:
                raise ValueError("List elements must all have the same schema")

        if Kind.from_value(reference_value) == Kind.MAP:
            for i in range(1, len(values)):
                if values[i].keys() != reference_value.keys():
                    raise ValueError("Map elements in a List must all have the same keys")

        return values

    if isinstance(raw, dict):
        # map
        if len(raw) == 0:
            raise ValueError("Maps cannot be empty")

        values: Dict[str, Value] = {}
        for key, value in raw.items():
            if not isinstance(key, str):
                raise ValueError("All keys of a Map must be of kind string")
            values[key] = check_value(value)
        return values

    raise ValueError("Cannot construct a StructuredBuffer Value from a {}".format(type(raw).__name__))


class Kind(IntEnum):
    """
    Enumeration of the four Value kinds.
    To differentiate between integers in a Schema that represent forward offsets (see Schema), we only use the 4 highest
    representable integers for these enum values.
    """
    _FIRST_KIND = sys.maxsize - 3
    LIST = _FIRST_KIND
    MAP = auto()
    _FIRST_GROUND = auto()
    NUMBER = _FIRST_GROUND
    STRING = auto()

    @staticmethod
    def is_ground(value: int) -> bool:
        """
        :return: Checks whether `value` is one of the ground types: Number and String
        """
        return value >= Kind._FIRST_GROUND

    @staticmethod
    def is_kind(value: int) -> bool:
        """
        Checks if the `value` is a valid Kind enum.
        """
        return value >= Kind._FIRST_KIND

    @staticmethod
    def is_offset(value: int) -> bool:
        """
        :return: True iff `value` would be considered a forward offset in a Schema.
        """
        return not Kind.is_kind(value)

    @staticmethod
    def from_value(value: Value) -> 'Kind':
        """
        The kind of the given Value.
        """
        if isinstance(value, str):
            return Kind.STRING
        elif isinstance(value, float):
            return Kind.NUMBER
        elif isinstance(value, list):
            return Kind.LIST
        else:
            assert isinstance(value, dict)
            return Kind.MAP


class Schema(tuple, Sequence[int]):
    """
    A Schema describes how data of a StructuredBuffer instance is laid out in memory.
    It is a simple list of integers, each integer either identifying a ground value kind (like a number or string) or a
    forward offset to a container value (like a list or map).
    """

    def __new__(cls, value: Value):
        """
        Recursive, breadth-first assembly of a Schema for a given Value.
        :returns: Schema describing how a StructuredBuffer containing the given Value would be laid out.
        """
        kind: Kind = Kind.from_value(value)
        schema: List[int] = []

        if kind == Kind.NUMBER:
            # Numbers take up a single word in a Schema, the NUMBER kind identifier
            schema.append(int(Kind.NUMBER))

        elif kind == Kind.STRING:
            # Strings take up a single word in a Schema, the STRING kind identifier
            schema.append(Kind.STRING)

        elif kind == Kind.LIST:
            # A List is a pair [List Type ID, child value] and its size in words is the size of the child value + 1.
            # Lists can only contain a single kind of value and in case that value is a Map, all Maps in the List
            # are required to have the same subschema.
            # An example Schema of a list is:
            #
            # ||    1.     ||     2.    | ... ||
            # || List Type || ChildType | ... ||
            # ||- header --||---- child ------||
            #
            assert len(value) > 0
            schema.append(int(Kind.LIST))
            schema.extend(Schema(value[0]))

        else:
            assert kind == Kind.MAP
            # Maps take up at least 3 words in the buffer - the MAP kind identifier, the number of elements in the map
            # (must be at least 1) followed by the child elements.
            # An example Schema of a map containing a number, a list, a string and another map can be visualized as
            # follows:
            #
            # ||    1.    |    2.    ||    3.    |    4.    |    5.    |    6.    ||   7.     | ... |    14.   | ... ||
            # || Map Type | Map Size || Nr. Type |  +3 (=7) | Str Type | +8 (=14) || ListType | ... | Map Type | ... ||
            # ||------ header -------||------------------ body -------------------||------------ children -----------||
            #
            # The map itself is split into three parts:
            # 1. The header just contains the Map Type ID and number of child elements.
            # 2. The body contains a single word for each child. Strings and Number Schemas are only a single word long,
            #    therefore they can be written straight into the body of the map.
            #    List and Map Schemas are longer than one word and are appended at the end of the body. The body itself
            #    contains only the forward offset to the child value in question.
            # 3. Child Lists and Maps in order of their appearance in the body.
            assert len(value) > 0
            schema.append(int(Kind.MAP))  # this is a map
            schema.append(len(value))  # number of items in the map
            child_position = len(schema)
            schema.extend([0] * len(value))  # reserve space for pointers to the child data
            for child in value.values():
                # numbers and strings are stored inline
                if Kind.is_ground(Kind.from_value(child)):
                    schema[child_position] = int(Kind.from_value(child))
                # lists and maps store a pointer and append themselves to the end of the schema
                else:
                    schema[child_position] = len(schema) - child_position  # store the offset
                    schema.extend(Schema(child))
                child_position += 1

        return super().__new__(Schema, schema)

    def __str__(self) -> str:
        """
        :return: A string representation of this Schema.
        """
        result: str = ""
        next_number_is_map_size = False
        for x, index in zip(self, range(len(self))):
            result += "{:>3}: ".format(index)
            if x == Kind.NUMBER:
                result += "Number"
            elif x == Kind.STRING:
                result += "String"
            elif x == Kind.LIST:
                result += "List"
            elif x == Kind.MAP:
                result += "Map"
                next_number_is_map_size = True
            else:
                if next_number_is_map_size:
                    result += " ↳ Size: {}".format(x)
                    next_number_is_map_size = False
                else:
                    result += "→ {}".format(index + x)
            result += "\n"
        return result


Buffer = List[Union[float, int, str, List['Buffer']]]
"""
The Buffer is a list of values that make up the data of a StructuredBuffer.
Note that buffers also contain integers as list/map sizes and offsets.
"""


def produce_buffer(source: Union[Schema, Value]) -> Buffer:
    """
    Builds a Buffer instance from either a Schema or a Value.
    :param source: Schema or Value from which to produce the Buffer.
    :return: The Buffer, initialized to the capabilities of the source.
    :raise ValueError: If `source` is neither a Schema, nor a type that can be converted into a StructuredBuffer Value.
    """

    def build_buffer_from_schema(buffer: Buffer, index: int = 0) -> Buffer:
        """
        If `source` is a Schema: Creates a zero-initialized Buffer laid out according to the given Schema.
        """
        assert len(source) > index
        if source[index] == Kind.NUMBER:
            buffer.append(0)

        elif source[index] == Kind.STRING:
            buffer.append("")

        elif source[index] == Kind.LIST:
            buffer.append([])

        else:
            assert source[index] == Kind.MAP
            assert len(source) > index + 1
            map_size = source[index + 1]
            for child_index in range(map_size):
                child_schema_index = index + 2 + child_index
                assert len(source) > child_schema_index
                # if the child index is not a number or a string, it is a pointer and needs to be resolved
                if Kind.is_offset(source[child_schema_index]):
                    child_schema_index += source[child_schema_index]
                    assert len(source) > child_schema_index
                build_buffer_from_schema(buffer, child_schema_index)

        return buffer

    def build_buffer_from_value(value: Value) -> Union[float, str, list]:
        """
        If `source` is a Value: Creates a value-initialized Buffer laid out according to the given value's Schema.
        """
        kind: Kind = Kind.from_value(value)

        if Kind.is_ground(kind):
            return value

        elif kind == Kind.LIST:
            result = [len(value)]  # size of the list
            result.extend(build_buffer_from_value(child) for child in value)
            return result

        else:
            assert kind == Kind.MAP
            return [build_buffer_from_value(child) for child in value.values()]

    # choose which of the two nested functions to use
    if isinstance(source, Schema):
        return build_buffer_from_schema([])
    else:
        root_value = build_buffer_from_value(check_value(source))
        return root_value if isinstance(root_value, list) else [root_value]


Dictionary = Dict[str, Optional['Dictionary']]
"""
A Dictionary can be attached to a StructuredBuffer to access map values by name.
The map keys are not part of the data as they are not mutable, much like a Schema.
Unlike a Schema however, a Dictionary is not mandatory for a StructuredBuffer, as some might not contain maps.
The Dictionary is also ignored when two Schemas are compared, meaning a map {x=float, y=float, z=float} will be 
compatible to {r=float, g=float, b=float}.
"""


def produce_dictionary(value: Value) -> Optional[Dictionary]:
    """
    Recursive, depth-first assembly of a Dictionary.
    :param value: Value to represent in the Dictionary.
    :return: The Dictionary corresponding to the given value. Is None if the value does not contain a Map.
    """

    def parse_next_value(next_value: Value):
        kind: Kind = Kind.from_value(next_value)

        if Kind.is_ground(kind):
            return None

        elif kind == Kind.LIST:
            return produce_dictionary(next_value[0])

        else:
            assert kind == Kind.MAP
            return {name: parse_next_value(child) for (name, child) in next_value.items()}

    return parse_next_value(value)


class Accessor:
    """
    Base class for both Reader and Writer accessors to StructuredBuffers.
    """

    def __init__(self, structured_buffer: 'StructuredBuffer'):
        """
        :param structured_buffer:   StructuredBuffer instance being accessed.
        """
        self._structured_buffer = structured_buffer
        self._schema_itr: int = 0
        # since the schema is constant, we store the structured buffer itself instead of the schema as well as the
        # offset, which also has the advantage that the structured buffer is needed for write operations anyway.
        # In order to access the schema, we use the private `_schema` property

        self._buffer: Buffer = structured_buffer._buffer
        self._buffer_itr: int = 0
        # the currently iterated buffer as well as the offset to the current value within the buffer

        self._dictionary: Optional[Dictionary] = structured_buffer._dictionary
        # dictionaries do not need an extra iterator since it is enough to point to the dictionary in question

    @property
    def _schema(self) -> Schema:
        """
        Private access to the constant schema of the iterated StructuredBuffer, just for readability.
        """
        return self._structured_buffer.schema

    @property
    def kind(self) -> Kind:
        """
        Kind of the currently accessed Value.
        In C++, this would be a protected method that is only public on Reader.
        """
        assert Kind.is_kind(self._schema[self._schema_itr])
        return Kind(self._schema[self._schema_itr])

    def __len__(self) -> int:
        """
        The number of child Values if the current Value is a List or Map, or zero otherwise.
        In C++, this would be a protected method that is only public on Reader.
        """
        if self.kind == Kind.LIST:
            assert len(self._buffer) > 0
            list_size = self._buffer[0]
            assert list_size == len(self._buffer) - 1  # -1 for the first integer that is the size of the list
            return list_size

        elif self.kind == Kind.MAP:
            assert len(self._schema) > self._schema_itr + 1
            map_size = self._schema[self._schema_itr + 1]
            assert map_size == len(self._buffer)
            return map_size

        else:
            return 0  # Number and Strings do not have child elements

    def __getitem__(self, key: Union[str, int]) -> Union['Accessor', 'Reader', 'Writer']:
        """
        Getting an item from an Accessor with the [] operator returns a new, deeper Accessor into the StructuredBuffer.
        In C++ we would differentiate between r-value and l-value overloads, the r-value one would modify itself while
        the l-value overload would produce a new Accessor as the Python implementation does.
        :param key: Index / Name of the value to access.
        :return: A new Accessor to the requested item.
        :raise KeyError: If the key does not identify an index in the current List / a key in the current Map.
        """
        result: Accessor = copy(self)  # shallow copy

        if result.kind == Kind.LIST:
            if not isinstance(key, int):
                raise KeyError("Lists must be accessed using an index, not {}".format(key))

            # support negative indices from the back of the list
            list_size: int = len(self)
            original_key: int = key
            if key < 0:
                key = list_size - key

            # make sure that there are enough elements in the list
            if not (0 <= key < list_size):
                raise KeyError(
                    "Cannot get value at index {} from a List of size {}".format(original_key, list_size))

            # advance the schema index
            result._schema_itr += 1

            # advance the iterator within the current buffer
            if Kind.is_ground(result._schema[result._schema_itr]):
                result._buffer_itr = key + 1
            # ... or advance to the next buffer if the value is not ground
            else:
                result._buffer_itr = 0
                result._buffer = result._buffer[key + 1]

        elif result.kind == Kind.MAP:
            if not isinstance(key, str):
                raise KeyError("Maps must be accessed using a string, not {}".format(key))

            if result._dictionary is None or key not in result._dictionary:
                raise KeyError("Unknown key \"{}\"".format(key))

            # find the index of the child in the map
            child_index = list(result._dictionary.keys()).index(key)
            assert child_index < len(result)

            # advance the schema index
            result._schema_itr = result._schema_itr + 2 + child_index
            if Kind.is_offset(result._schema[result._schema_itr]):
                # resolve pointer to list or map
                result._schema_itr += result._schema[result._schema_itr]

            # advance to the next buffer if the value is not ground
            if Kind.is_ground(result._schema[result._schema_itr]):
                result._buffer_itr = child_index
            else:
                result._buffer = result._buffer[child_index]
                result._buffer_itr = 0

            # advance the dictionary
            result._dictionary = result._dictionary[key]

        else:
            raise KeyError("Cannot use operator[] on a {} Value".format(result.kind.name))

        assert result._schema_itr < len(result._schema)
        return result


class Reader(Accessor):

    def keys(self) -> Optional[List[str]]:
        """
        If the current value is a Map, returns the available keys. If not, returns None.
        """
        if self.kind != Kind.MAP:
            return None
        return list(self._dictionary.keys())

    def as_number(self) -> float:
        """
        Returns the current value as a Number.
        :raise ValueError: If the value is not a number.
        """
        if self.kind != Kind.NUMBER:
            raise ValueError("Element is not a Number")
        result = self._buffer[self._buffer_itr]
        assert isinstance(result, float)
        return result

    def as_string(self) -> str:
        """
        Returns the current value as a String.
        :raise ValueError: If the value is not a string.
        """
        if self.kind != Kind.STRING:
            raise ValueError("Element is not a String")
        result = self._buffer[self._buffer_itr]
        assert isinstance(result, str)
        return result


class Writer(Accessor):
    """
    Writing to a StructuredBuffer creates a new StructuredBuffer referencing a new root buffer.
    The new root buffer contains as many references to the existing child-buffers, that are immutable, but in each
    level there can be (at most) one updated reference to a new buffer. An example:

    Let's say, we have a simple tree of buffers:

            A
         +--+--+
         |     |
         B     C

    In order to change C to C', we need to update C and A:

            A'
         +--+--+
         |     |
         B     C'

    Later on, when you update B to B`, we update B and A` again and you get:

            A"
         +--+--+
         |     |
         B'    C'
    """

    def __init__(self, structured_buffer: 'StructuredBuffer'):
        """
        :param structured_buffer:   StructuredBuffer instance to modify.
        """
        super().__init__(structured_buffer)
        self._path = [structured_buffer._buffer]
        # in order to update the buffer tree from root to the modified element, we need to store the path containing
        # each buffer that was passed on the way

    def __getitem__(self, key: Union[str, int]) -> 'Writer':
        """
        Getting an item from an Accessor with the [] operator returns a new, deeper Accessor into the StructuredBuffer.
        :param key: Index / Name of the value to access.
        :return: A new Accessor to the requested item.
        :raise KeyError: If the key does not identify an index in the current List / a key in the current Map.
        """
        writer: Writer = super().__getitem__(key)

        # if the item resides in a new buffer, add it to the path of buffers to modify
        if writer._path[-1] is not writer._buffer:
            writer._path.append(writer._buffer)

        return writer

    def set(self, raw: Any) -> 'StructuredBuffer':
        """
        Creates a new StructuredBuffer with the minimal number of new sub-buffers.

        :param raw: "Raw" new value to set.
        :raise ValueError: If `raw` cannot be converted to a Value.
                           If the value does not match the currently accessed Value Kind.
        """
        # make sure the given raw value can be stored in the buffer
        value: Value = check_value(raw)
        kind: Kind = Kind.from_value(value)
        schema: Schema = Schema(value)

        # make sure that the given value is compatible with the current
        if kind != self.kind:
            raise ValueError("Cannot change a value of kind {} to one of kind {}".format(self.kind.name, value))
        else:
            slice_end = self._schema_itr + len(schema)
            if len(self._schema) < slice_end or schema != self._schema[self._schema_itr: slice_end]:
                raise ValueError("Cannot assign a value with an incompatible Schema")

        # create a new buffer for the updated value
        if Kind.is_ground(self._schema[self._schema_itr]):

            # if the new value is equal to the old one, we don't need to do anything
            if value == self._buffer[self._buffer_itr]:
                return self._structured_buffer

            new_buffer = copy(self._buffer)  # shallow copy
            new_buffer[self._buffer_itr] = value

        # ... or replace the entire subbuffer, if it is a map or a list
        else:
            new_buffer = produce_buffer(value)

            # if the new buffer is equal to the old one, we don't need to do anything
            if new_buffer == self._buffer:
                return self._structured_buffer

        # create the new buffer tree from the leaf up, made up of updated copies for those on the path straight copies
        # for all others
        assert (self._path[-1] == self._buffer)
        while len(self._path) > 1:
            old_buffer = self._path.pop()
            old_parent_buffer = self._path[-1]
            new_buffer = [(x if x != old_buffer else new_buffer) for x in old_parent_buffer]

        # create a new StructuredBuffer instance that references the updated buffer tree
        result: StructuredBuffer = copy(self._structured_buffer)
        result._buffer = new_buffer
        return result


class StructuredBuffer:
    """
    The StructuredBuffer contains a Buffer from which to read the data, a Schema describing the layout of the Buffer
    and an optional Dictionary to access Map elements by name.
    """

    Kind = Kind
    Schema = Schema
    Dictionary = Dictionary

    def __init__(self, source: Any):
        """
        :param source: Anything that can be used to create a new StructuredBuffer.
        :raise ValueError: If `source` cannot be used to initialize a StructuredBuffer.
        """
        # null-initialize from schema
        if isinstance(source, Schema):
            schema: Schema = source
            buffer: Buffer = produce_buffer(schema)
            dictionary = None

        # value-initialize from value
        else:
            value: Value = check_value(source)
            schema: Schema = Schema(value)
            buffer: Buffer = produce_buffer(value)
            dictionary: Optional[Dictionary] = produce_dictionary(value)

        # Schema of the buffer, is constant
        assert schema is not None
        assert len(schema) > 0
        self._schema: Schema = schema

        # Buffer storing the data of this StructuredBuffer.
        assert buffer is not None
        assert len(buffer) > 0
        self._buffer: Buffer = buffer

        # A Dictionary referring to the topmost map in the Schema. Can be None.
        self._dictionary: Optional[Dictionary] = dictionary

    @property
    def schema(self) -> Schema:
        """
        Schema of the buffer, is constant
        """
        return self._schema

    @property
    def kind(self) -> Kind:
        """
        The Kind of the root Value.
        """
        assert Kind.is_kind(self._schema[0])
        return Kind(self._schema[0])

    def keys(self) -> Optional[List[str]]:
        """
        If the root value of this StructuredBuffer is a Map, returns the available keys. If not, returns None.
        """
        if self.kind != Kind.MAP:
            return None
        return list(self._dictionary.keys())       

    def __getitem__(self, key: Union[str, int]) -> Reader:
        """
        Read access to this StructuredBuffer instance via the [] operator.
        """
        return Reader(self)[key]

    def modify(self) -> Writer:
        """
        Write access to this StructuredBuffer instance.
        """
        return Writer(self)

import sys
from enum import IntEnum, auto
from copy import copy
from itertools import chain
from typing import NewType, List, Dict, Union, Optional, Sequence, Any

Element = NewType('Element', Union[float, str, List["Element"], Dict[str, "Element"]])
"""
The `Element` kind is a variant of the four data kinds that can be stored in a Value.
It defines a single Element including all of its child Elements (if the parent Element is a List or a Map)
"""


def check_element(raw: Any, allow_empty_lists: bool) -> Element:
    """
    Tries to convert any given "raw" value into an Element.
    :param raw: Raw value to convert.
    :param allow_empty_lists: Empty lists are allowed when modifying a Value, but not during construction.
    :return: The given raw value as an Element.
    :raise ValueError: If the raw value cannot be represented as an Element.
    """
    if isinstance(raw, (str, float)):
        # native Element types
        return raw

    elif isinstance(raw, int):
        # types trivially convertible to a Element kind
        return float(raw)

    elif isinstance(raw, list):
        # lists
        elements: List[Element] = [check_element(x, allow_empty_lists) for x in raw]

        if len(elements) > 0:
            reference_element: Element = elements[0]
            reference_schema: Schema = Schema(reference_element)
            for i in range(1, len(elements)):
                if Schema(elements[i]) != reference_schema:
                    raise ValueError("List entries must all have the same schema")

            if Kind.from_element(reference_element) == Kind.MAP:
                for i in range(1, len(elements)):
                    if elements[i].keys() != reference_element.keys():
                        raise ValueError("Map entries in a List must all have the same keys")
        elif not allow_empty_lists:
            raise ValueError("Lists cannot be empty when creating a Value")

        return elements

    elif isinstance(raw, dict):
        # map
        if len(raw) == 0:
            raise ValueError("Maps cannot be empty")

        elements: Dict[str, Element] = {}
        for key, element in raw.items():
            if not isinstance(key, str):
                raise ValueError("All keys of a Map must be of kind string")
            elements[key] = check_element(element, allow_empty_lists)
        return elements

    raise ValueError("Cannot construct a Value.Element from a {}".format(type(raw).__name__))


class Kind(IntEnum):
    """
    Enumeration of the four Element kinds and None.
    To differentiate between integers in a Schema that represent forward offsets (see Schema) and enum values that
    denote the Kind of an Element, we only use the highest representable integers for the enum.
    The NONE kind never mentioned as part of a Schema, it is only returned if the Schema is empty.
    """
    NONE = 0
    LIST = sys.maxsize - 3
    MAP = auto()
    NUMBER = auto()
    STRING = auto()

    @staticmethod
    def is_kind(value: int) -> bool:
        """
        Checks if the `value` is a valid Kind enum.
        """
        return value >= Kind.LIST

    @staticmethod
    def is_offset(value: int) -> bool:
        """
        :return: True iff `value` would be considered a forward offset in a Schema.
        """
        return not Kind.is_kind(value)

    @staticmethod
    def is_ground(value: int) -> bool:
        """
        :return: Checks whether `value` represents one of the ground types: Number and String
        """
        return value >= Kind.NUMBER

    @staticmethod
    def from_element(element: Optional[Element]) -> 'Kind':
        """
        The kind of the given Element.
        """
        if element is None:
            return Kind.NONE
        elif isinstance(element, str):
            return Kind.STRING
        elif isinstance(element, (int, float)):
            return Kind.NUMBER
        elif isinstance(element, list):
            return Kind.LIST
        else:
            assert isinstance(element, dict)
            return Kind.MAP


class Schema(tuple, Sequence[int]):
    """
    A Schema describes how data of a Value instance is laid out in memory.
    It is a simple list of integers, each integer either identifying a ground Element kind (like a number or string) or
    a forward offset to a container Element (like a list or map).
    """

    def __new__(cls, element: Optional[Element] = None):
        """
        Recursive, breadth-first assembly of a Schema for a given Element.
        :returns: Schema describing how a Value containing the given Element would be laid out.
        """
        kind: Kind = Kind.from_element(element)
        schema: List[int] = []

        if kind == Kind.NONE:
            # The None Schema is empty.
            pass

        elif kind == Kind.NUMBER:
            # Numbers take up a single word in a Schema, the NUMBER kind identifier
            schema.append(int(Kind.NUMBER))

        elif kind == Kind.STRING:
            # Strings take up a single word in a Schema, the STRING kind identifier
            schema.append(Kind.STRING)

        elif kind == Kind.LIST:
            # A List is a pair [List Type ID, child Element] and its size in words is the size of the child Element + 1.
            # Lists can only contain a single kind of Element and in case that Element is a Map, all Maps in the List
            # are required to have the same subschema.
            # An example Schema of a list is:
            #
            # ||    1.     ||     2.    | ... ||
            # || List Type || ChildType | ... ||
            # ||- header --||---- child ------||
            #
            assert len(element) > 0
            schema.append(int(Kind.LIST))
            schema.extend(Schema(element[0]))

        else:
            assert kind == Kind.MAP
            # Maps take up at least 3 words in the buffer - the MAP kind identifier, the number of entries in the map
            # (must be at least 1) followed by the child entries.
            # An example Schema of a map containing a number, a list, a string and another map can be visualized as
            # follows:
            #
            # ||    1.    |    2.    ||    3.    |    4.    |    5.    |    6.    ||   7.     | ... |    14.   | ... ||
            # || Map Type | Map Size || Nr. Type |  +3 (=7) | Str Type | +8 (=14) || ListType | ... | Map Type | ... ||
            # ||------ header -------||------------------ body -------------------||------------ children -----------||
            #
            # The map itself is split into three parts:
            # 1. The header just contains the Map Type ID and number of child entries.
            # 2. The body contains a single word for each child. Strings and Number Schemas are only a single word long,
            #    therefore they can be written straight into the body of the map.
            #    List and Map Schemas are longer than one word and are appended at the end of the body. The body itself
            #    contains only the forward offset to the child Element in question.
            #    In the special case where the map only contains a single non-ground Element at the very end, we can
            #    remove the last entry in the body and move the single child up one word.
            # 3. Child Lists and Maps in order of their appearance in the body.
            assert len(element) > 0
            schema.append(int(Kind.MAP))  # this is a map
            schema.append(len(element))  # number of items in the map
            child_position = len(schema)
            schema.extend([0] * len(element))  # reserve space for the body
            for child in element.values():
                # numbers and strings are stored inline
                if Kind.is_ground(Kind.from_element(child)):
                    schema[child_position] = int(Kind.from_element(child))
                # lists and maps store a pointer and append themselves to the end of the schema
                else:
                    offset = len(schema) - child_position
                    # if the offset is 1, we only have a single non-ground Element in the map and it is at the very end
                    # in this case, we can simply put the sub-schema of the child inline and save a pointer
                    if offset > 1:
                        schema[child_position] = offset  # store the offset
                    else:
                        schema.pop(-1)
                    schema.extend(Schema(child))
                child_position += 1

        return super().__new__(Schema, schema)

    def is_empty(self) -> bool:
        """
        Checks if this is the empty Schema.
        """
        return len(self) == 0

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

    def as_list(self) -> 'Schema':
        """
        Creates a Schema that is a list containing elements of this Schema.
        """
        return super().__new__(Schema, [int(Kind.LIST), *self])


Buffer = List[Union[float, int, str, List['Buffer']]]
"""
The Buffer is a list of Element that make up the data of a Value.
Note that buffers also contain integers as list/map sizes and offsets.
"""


def produce_buffer(source: Union[Schema, Element]) -> Buffer:
    """
    Builds a Buffer instance from either a Schema or an Element.
    :param source: Schema or Element from which to produce the Buffer.
    :return: The Buffer, initialized to the capabilities of the source.
    :raise ValueError: If `source` is neither a Schema, nor a type that can be converted into an Element.
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

    def build_buffer_from_element(element: Element) -> Union[float, str, list]:
        """
        If `source` is an Element: Creates a value-initialized Buffer laid out according to the given Element's Schema.
        """
        kind: Kind = Kind.from_element(element)

        if Kind.is_ground(kind):
            return element

        elif kind == Kind.LIST:
            result = [len(element)]  # size of the list
            result.extend(build_buffer_from_element(child) for child in element)
            return result

        else:
            assert kind == Kind.MAP
            return [build_buffer_from_element(child) for child in element.values()]

    # choose which of the two nested functions to use
    if isinstance(source, Schema):
        if source.is_empty():
            return []
        else:
            return build_buffer_from_schema([])
    else:
        root_element = build_buffer_from_element(source)
        return root_element if isinstance(root_element, list) else [root_element]


Dictionary = Dict[str, Optional['Dictionary']]
"""
A Dictionary can be attached to a Value to access Map entries by name.
The map keys are not part of the data as they are not mutable, much like a Schema.
Unlike a Schema however, a Dictionary is not mandatory for a Value, as some might not contain maps.
The Dictionary is also ignored when two Schemas are compared, meaning a map {x=float, y=float, z=float} will be 
compatible to {r=float, g=float, b=float}.
"""


def produce_dictionary(element: Element) -> Optional[Dictionary]:
    """
    Recursive, depth-first assembly of a Dictionary.
    :param element: Element to represent in the Dictionary.
    :return: The Dictionary corresponding to the given Element. Is None if the Element does not contain a Map.
    """

    def parse_next_element(next_element: Element):
        kind: Kind = Kind.from_element(next_element)

        if Kind.is_ground(kind):
            return None

        elif kind == Kind.LIST:
            return produce_dictionary(next_element[0])

        else:
            assert kind == Kind.MAP
            return {name: parse_next_element(child) for (name, child) in next_element.items()}

    return parse_next_element(element)


class Accessor:
    """
    Base class for both Reader and Writer accessors to a Value.
    """

    def __init__(self, value: 'Value'):
        """
        :param value:   Value instance being accessed.
        """
        self._value = value
        self._schema_itr: int = 0
        # since the schema is constant, we store the Value itself instead of the schema as well as the
        # offset, which also has the advantage that the value's buffer is needed for write operations anyway.
        # In order to access the schema, we use the private `_schema` property

        self._buffer: Buffer = value._buffer
        self._buffer_itr: int = 0
        # the currently iterated buffer as well as the offset to the current Element in the buffer

        self._dictionary: Optional[Dictionary] = value._dictionary
        # dictionaries do not need an extra iterator since it is enough to point to the dictionary in question

    @property
    def _schema(self) -> Schema:
        """
        Private access to the constant schema of the iterated Value, just for readability.
        """
        return self._value.schema

    @property
    def kind(self) -> Kind:
        """
        Kind of the currently accessed Element.
        In C++, this would be a protected method that is only public on Reader.
        """
        if self._schema.is_empty():
            return Kind.NONE
        else:
            assert Kind.is_kind(self._schema[self._schema_itr])
            return Kind(self._schema[self._schema_itr])

    def __len__(self) -> int:
        """
        The number of child Elements if the current Element is a List or Map, or zero otherwise.
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
            return 0  # Number and Strings do not have child entries

    def __getitem__(self, key: Union[str, int]) -> Union['Accessor', 'Reader', 'Writer']:
        """
        Getting an item from an Accessor with the [] operator returns a new, deeper Accessor into the Value.
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

            # make sure that there are enough entries in the list
            if not (0 <= key < list_size):
                raise KeyError(
                    "Cannot get Element at index {} from a List of size {}".format(original_key, list_size))

            # advance the schema index
            result._schema_itr += 1

            # advance the iterator within the current buffer
            if Kind.is_ground(result._schema[result._schema_itr]):
                result._buffer_itr = key + 1
            # ... or advance to the next buffer if the Element is not ground
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

            # advance to the next buffer if the Element is not ground
            if Kind.is_ground(result._schema[result._schema_itr]):
                result._buffer_itr = child_index
            else:
                result._buffer = result._buffer[child_index]
                result._buffer_itr = 0

            # advance the dictionary
            result._dictionary = result._dictionary[key]

        else:
            raise KeyError("Cannot use operator[] on a {} Element".format(result.kind.name))

        assert result._schema_itr < len(result._schema)
        return result


class Reader(Accessor):

    def keys(self) -> Optional[List[str]]:
        """
        If the current Element is a Map, returns the available keys. If not, returns None.
        """
        if self.kind != Kind.MAP:
            return None
        return list(self._dictionary.keys())

    def as_number(self) -> float:
        """
        Returns the current Element as a Number.
        :raise ValueError: If the Element is not a number.
        """
        if self.kind != Kind.NUMBER:
            raise ValueError("Element is not a Number")
        result = self._buffer[self._buffer_itr]
        assert isinstance(result, float)
        return result

    def as_string(self) -> str:
        """
        Returns the current Element as a String.
        :raise ValueError: If the Element is not a string.
        """
        if self.kind != Kind.STRING:
            raise ValueError("Element is not a String")
        result = self._buffer[self._buffer_itr]
        assert isinstance(result, str)
        return result


class Writer(Accessor):
    """
    Writing to a Value creates a new Value referencing a new root buffer.
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

    def __init__(self, value: 'Value'):
        """
        :param value:   Value instance to modify.
        """
        super().__init__(value)
        self._path = [value._buffer]
        # in order to update the buffer tree from root to the modified element, we need to store the path containing
        # each buffer that was passed on the way

    def __getitem__(self, key: Union[str, int]) -> 'Writer':
        """
        Getting an item from an Accessor with the [] operator returns a new, deeper Accessor into the Value.
        :param key: Index / Name of the Element to access.
        :return: A new Accessor to the requested item.
        :raise KeyError: If the key does not identify an index in the current List / a key in the current Map.
        """
        writer: Writer = super().__getitem__(key)

        # if the item resides in a new buffer, add it to the path of buffers to modify
        if writer._path[-1] is not writer._buffer:
            writer._path.append(writer._buffer)

        return writer

    def set(self, raw: Any) -> 'Value':
        """
        Creates a new Value with the minimal number of new sub-buffers.

        :param raw: "Raw" new value to set.
        :raise ValueError: If `raw` cannot be converted to an Element.
                           If the Element does not match the currently accessed Element Kind.
        """
        # make sure the given raw value can be stored in the buffer
        element: Element = check_element(raw, allow_empty_lists=True)
        kind: Kind = Kind.from_element(element)

        # make sure that the given Element is compatible with the current
        if kind != self.kind:
            raise ValueError("Cannot change an Element of kind {} to one of kind {}".format(self.kind.name, kind.name))
        else:
            if kind == Kind.MAP and tuple(raw.keys()) != tuple(self._dictionary.keys()):
                raise ValueError("Cannot assign to a Map with incompatible keys (name and order)")
            if kind == Kind.LIST and len(element) == 0:
                pass  # we cannot deduce the Schema from an empty list
            else:
                schema: Schema = Schema(element)
                slice_end = self._schema_itr + len(schema)
                if len(self._schema) < slice_end or schema != self._schema[self._schema_itr: slice_end]:
                    raise ValueError("Cannot assign an Element with an incompatible Schema")

        # create a new buffer for the updated Element
        if Kind.is_ground(self._schema[self._schema_itr]):

            # if the new Element is equal to the old one, we don't need to do anything
            if element == self._buffer[self._buffer_itr]:
                return self._value

            new_buffer = copy(self._buffer)  # shallow copy
            new_buffer[self._buffer_itr] = element

        # ... or replace the entire subbuffer, if it is a map or a list
        else:
            new_buffer = produce_buffer(element)

            # if the new buffer is equal to the old one, we don't need to do anything
            if new_buffer == self._buffer:
                return self._value

        # create the new buffer tree from the leaf up, made up of updated copies for those on the path straight copies
        # for all others
        assert (self._path[-1] == self._buffer)
        while len(self._path) > 1:
            old_buffer = self._path.pop()
            old_parent_buffer = self._path[-1]
            new_buffer = [(x if x != old_buffer else new_buffer) for x in old_parent_buffer]

        # create a new Value instance that references the updated buffer tree
        result: Value = copy(self._value)
        result._buffer = new_buffer
        return result


class Value:
    """
    The Value contains a Buffer from which to read the data, a Schema describing the layout of the Buffer, and
    an optional Dictionary to access Map entries by name.
    Apart from a Value that contains one of the 4 Element types, you can also have an empty (None) value that
    does not contain anything. It is not possible to have a None value nested inside another Value.
    """

    Kind = Kind
    Schema = Schema
    Dictionary = Dictionary

    def __init__(self, source: Any = None):
        """
        :param source: Anything that can be used to create a new Value.
        :raise ValueError: If `source` cannot be used to initialize a Value.
        """
        # explict None
        if source is None:
            schema: Schema = Schema()
            buffer: Buffer = []
            dictionary = None

        # null-initialize from schema
        elif isinstance(source, Schema):
            schema: Schema = source
            buffer: Buffer = produce_buffer(schema)
            dictionary = None

        # initialize from list of values
        elif isinstance(source, (tuple, list)) and len(source) > 0 and isinstance(source[0], Value):
            # check the schema
            reference_schema: Schema = source[0].schema
            for other_value in source[1:]:
                if not other_value.schema == reference_schema:
                    raise ValueError("Cannot construct a list from Values with different Schemas")

            # the schema of this value is a list of all others
            schema: Schema = reference_schema.as_list()
            if Kind.is_ground(reference_schema[0]):
                # if the reference value kind is ground, we can collapse the individual buffers into one
                buffer: Buffer = [len(source), *chain(*[value._buffer for value in source])]
            else:
                # if the reference value kind is not ground, we store each buffer in the new one
                buffer: Buffer = [len(source), *[value._buffer for value in source]]
            dictionary = None

        # TODO: initialize from dict of values?

        # value-initialize from Element
        else:
            element: Element = check_element(source, allow_empty_lists=False)
            schema: Schema = Schema(element)
            buffer: Buffer = produce_buffer(element)
            dictionary: Optional[Dictionary] = produce_dictionary(element)

        # Schema of the buffer, is constant
        assert schema is not None
        self._schema: Schema = schema

        # Buffer storing the data of this Value.
        assert buffer is not None
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
        The Kind of the root Element.
        """
        if self._schema.is_empty():
            return Kind.NONE
        else:
            assert Kind.is_kind(self._schema[0])
            return Kind(self._schema[0])

    def keys(self) -> Optional[List[str]]:
        """
        If the root Element of this Value is a Map, returns the available keys. If not, returns None.
        """
        if self.kind != Kind.MAP:
            return None
        return list(self._dictionary.keys())

    def __getitem__(self, key: Union[str, int]) -> Reader:
        """
        Read access to this Value instance via the [] operator.
        """
        return Reader(self)[key]

    def __eq__(self, other: Any) -> bool:
        """
        Equality test.
        """
        if not isinstance(other, self.__class__):
            return False
        return self._schema == other._schema and self._buffer == other._buffer and self._dictionary == other._dictionary

    def __ne__(self, other) -> bool:
        """
        Inequality tests.
        """
        return not self.__eq__(other)

    def as_number(self) -> float:
        """
        Returns the Element as a Number.
        :raise ValueError: If the Element is not a number.
        """
        return Reader(self).as_number()

    def as_string(self) -> str:
        """
        Returns the Element as a String.
        :raise ValueError: If the Element is not a string.
        """
        return Reader(self).as_string()

    def is_none(self) -> bool:
        """
        Checks if this is a None value.
        """
        return self._schema.is_empty()

    def modified(self) -> Writer:  # TODO: I had to look up what "modified" means here, maybe rename it to "write"?
        """
        Write access to this Value instance.
        """
        return Writer(self)


"""
Additional Thoughts
===================

Better support for growing lists
--------------------------------
One use-case for lists that I can think of is storing an ever-growing log of something, a chat log for example. Or maybe
a history of sensor readings or whatever. In that case, it will be very wasteful to copy a list every time it is 
changed. For example if you have a list "2ab" and want to add a "c" at the end, you'll have to copy the entire list 
before you can modify it to "3abc". The problem here is that we store the size of the list at the beginning.

Instead, the size of the list should be stored in the buffer looking into the list (alternatively, you could also store
the beginning and end of the list). This way, you can have two Values referencing the same underlying list, but offering
different views. Using this approach we could even use different start- and end-points in the same list...

The big problem here is that we'd now have a single Element type that uses two words instead of one. 
Possible ramifications:
    - not a problem, we know that the Element is a list and a fixed number of words per Element was an accident more 
      than a requirement anyway
    - use 64 bit words, but split them into two 32 bit pointers. This way we still have double precision for numbers,
      but we would limit the addressable memory to whatever 32 bits can provide. Also, we'd waste 32 bits for each
      String and Map Element...
    - not a problem because the pointer stored in the word of a list is not a pointer to the start of the list itself,
      but a pointer to the shared state, that manages the underlying memory (similar to a shared_ptr). See the chapter
      on memory management below.

The same does of course also apply to strings -- where of course the size of a string is not equivalent to the number
of words since UTF8 allows variable sizes for each code point... Maybe storing the start- and end-point is in fact the
better alternative here (like iterators)?


Custom memory management
------------------------
We could go all the way down to memory allocation in order to optimize working with immutable values. Here are my 
thoughts so far:

There should be a single value cache / -manager that knows of all the values and has weak references to them. It should
work like an unordered_map, every value is hashable and the hash is used as a key to a weak reference to that value.
If we enforce that two values that share the same content are always consolidated into one (the later one is dropped in
favor of the earlier one), value comparison is a simple pointer comparison.

Orthogonal to the value cache, there is the memory manager that manages individual chunks of memory. A chunk is 
a part of memory that can be shared by multiple values, not all of them need to refer to the same slice of the chunk.
For example:

    ||  00  01  02  03  ...   0f  10  11  12  ...   c3  c4  | ...   d9 || 
                 <------  A  ------>                        |
                 <------  B  -------------->                |
                               <---------  C  ----------->  |

Here A, B and C all reference the same chunk of size c4 (196), meaning the chunk will only be removed after all three
references have all gone out of scope, even though some of them refer to different values.

Note the vertical line after c4. Everything left to that line is memory that is either in use (02 ... c4) or has been
used once, but is no longer referenced by any value (00 ... 01), which we call unreachable. We don't keep track of
unreachable memory, even though it is effectively lost until the chunk itself can be cleared. That is a reason to keep
chunks small.

On the other hand, Lists that are expected to grow might well profit from larger chunks. In the example, we can have a
list that starts out small (A) and then grows (B). Both A and B are views on the same list, even though they are
different Values. If you have a chat log or something else that only grows larger, it would be very useful to have a
large chunk allocated for that log alone, with different Values referring to different slices but without any copying
when another message is added. 

We could approach this problem by allowing chunks of varying sizes, where the manager keeps track of the start and end
points of each chunk.
With fixed size chunks however, the manager could reserve a giant space up front and use that to allocate all possible 
chunks at once. They would be easy to manage because you could use a simple list of free chunk indices + the index of 
the lowest yet unused chunk to find out where you can create a new chunk, and the size of a chunk * index in order to 
get its location in memory. The downside is that the number of chunks is limited right from the start.
Then again, this seems to be a non-issue when you're using virtual memory:
https://www.gamasutra.com/blogs/NiklasGray/20171107/309071/Virtual_Memory_Tricks.php
Maybe it would be a good idea to have two separate pools of memory: one for fixed size Values (everything but Lists) and
one for Lists only, with large gaps in between.

Chunks must also have a generation value, which informs a weak reference that the chunk it points to might be in use, 
but it was reclaimed since the weak reference was created.
For details see: https://gamasutra.com/blogs/NiklasGray/20190724/347232/Data_Structures_Part_1_Bulk_Data.php

Values (Lists and Maps to be precise) point not directly into memory, but to a shared block of data appropriate for 
their type. Lists for example should store both the offset and the size (or the last/one past last element) and maybe
even a growing vector of chunks, if a list happens to span multiple?
"""

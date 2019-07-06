import sys
from enum import IntEnum, unique, auto
from typing import NewType, List, Dict, Union, Optional, Sequence

Buffer = List[Union[float, int, str, List['Buffer']]]


class Schema(list, Sequence[int]):
    """
    A simple list of integers.
    """

    def __str__(self) -> str:
        """
        :return: A string representation of this Schema.
        """
        result: str = ""
        next_number_is_map_size = False
        for x, index in zip(self, range(len(self))):
            result += "{:>3}: ".format(index)
            if x == Element.Type.NUMBER:
                result += "Number"
            elif x == Element.Type.STRING:
                result += "String"
            elif x == Element.Type.LIST:
                result += "List"
            elif x == Element.Type.MAP:
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

    @property
    def data(self) -> List[int]:
        """
        Access to the Schema as a list of integers (useful for printing the raw data)
        """
        return [x for x in self]

    @staticmethod
    def is_ground(value: int) -> bool:
        """
        :return: Checks whether `value` is one of the ground types: Number and String
        """
        return value in (Element.Type.NUMBER, Element.Type.STRING)

    @staticmethod
    def is_pointer(value: int) -> bool:
        """
        :return: True iff `value` would be considered a pointer in a Schema.
        """
        return value not in (Element.Type.NUMBER, Element.Type.STRING, Element.Type.LIST, Element.Type.MAP)


class Element:
    """
    Elements define a single element in a StructuredMemory including all of its child elements (if it is a List or Map).
    From them, we can derive the Schema (which is a simple list of integers that define how data is laid out in
    memory), the Buffer (which are blocks of memory containing the data, but without additional information on the
    type of data) and the Dictionary (which contains lookup names for Map elements that are not part of the Schema).
    """

    Value = NewType('Value', Union[float, str, List["Element"], Dict[str, "Element"]])

    @unique
    class Type(IntEnum):
        NUMBER = sys.maxsize - 3
        STRING = auto()
        LIST = auto()
        MAP = auto()

    def __init__(self, element_type: Type, value: Value):
        """
        Constructor.
        :param element_type:    Element type.
        :param value:           Value stored in this Element, must be of the right type.
        """
        self.type: Element.Type = element_type
        self.value: Element.Value = value
        self.schema: Schema = self._produce_schema()

    def _produce_schema(self) -> Schema:
        """
        Recursive, breadth-first assembly of the Schema.
        :returns: Schema describing how the element buffer is laid out.
        """
        schema = Schema()

        if self.type == Element.Type.NUMBER:
            # Numbers take up a single word in a Schema, the NUMBER type identifier
            schema.append(int(Element.Type.NUMBER))

        elif self.type == Element.Type.STRING:
            # Strings take up a single word in a Schema, the STRING type identifier
            schema.append(Element.Type.STRING)

        elif self.type == Element.Type.LIST:
            # Lists take up at least two words in the buffer - the LIST type identifier, immediately followed by their
            # child type in place. Lists can only contain a single type of element and in case that element is a Map,
            # all Maps in the List are required to have the same subschema. A List is basically a pair
            # <List Type ID, child element> and its size is the size of the child element + 1.
            # An example Schema of a list containing a Map would be:
            #
            # ||    1.    ||    2.    | ... ||
            # || ListType || ListType | ... ||
            # ||- header -||---- child -----||
            #
            assert len(self.value) > 0
            schema.append(int(Element.Type.LIST))
            schema.extend(self.value[0].schema)

        elif self.type == Element.Type.MAP:
            # Maps take up at least 3 words in the buffer - the MAP type identifier, the number of elements in the map
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
            #    contains only the forward offset to the child element in question.
            # 3. Child Lists and Maps in order of their appearance in the body.
            assert len(self.value) > 0
            schema.append(int(Element.Type.MAP))  # this is a map
            schema.append(len(self.value))  # number of items in the map
            child_position = len(schema)
            schema.extend([0] * len(self.value))  # reserve space for pointers to the child data
            for child_element in self.value.values():
                # numbers and strings are stored inline
                if child_element.type in (Element.Type.NUMBER, Element.Type.STRING):
                    schema[child_position] = int(child_element.type)
                # lists and maps store a pointer and append themselves to the end of the schema
                else:
                    schema[child_position] = len(schema) - child_position  # store the offset
                    schema.extend(child_element.schema)
                child_position += 1

        return schema


class Number(Element):
    def __init__(self, value: float = 0.):
        super().__init__(Element.Type.NUMBER, float(value))


class String(Element):
    def __init__(self, value: str = ""):
        super().__init__(Element.Type.STRING, value)


class List(Element):
    def __init__(self, *values: Element):
        if len(values) == 0:
            raise ValueError("List element cannot be empty")

        reference_value = values[0]
        for i in range(1, len(values)):
            if values[i].schema != reference_value.schema:
                raise ValueError("List elements must all have the same schema")

        if reference_value.type == Element.Type.MAP:
            for i in range(1, len(values)):
                if values[i].value.keys() != reference_value.value.keys():
                    raise ValueError("Maps in a list must all have the same keys")

        super().__init__(Element.Type.LIST, values)


class Map(Element):
    def __init__(self, **values: Element):
        if len(values) == 0:
            raise ValueError("Map element cannot be empty")
        super().__init__(Element.Type.MAP, values)


class StructuredBuffer:
    Dictionary = Dict[str, Optional['Dictionary']]

    class Accessor:
        def __init__(self, structured_buffer: 'StructuredBuffer'):
            """
            :param structured_buffer:   StructuredBuffer instance being accessed.
            """
            self._structured_buffer = structured_buffer
            self._schema_itr: int = 0
            self._buffer: Buffer = structured_buffer.buffer
            self._buffer_itr: int = 0
            self._dictionary_itr: Optional[StructuredBuffer.Dictionary] = structured_buffer.dictionary

        @property
        def type(self) -> Element.Type:
            """
            Element type currently accessed.
            """
            return Element.Type(self._schema[self._schema_itr])

        @property
        def _schema(self) -> Schema:
            """
            Schema of the StructuredBuffer begin accessed.
            """
            return self._structured_buffer.schema

        def get_list_size(self) -> int:
            """
            The size of the List is the first word in the List buffer.
            :return:    Number of elements in the currently traversed List.
            """
            assert self.type == Element.Type.LIST
            assert len(self._buffer) > 0
            return self._buffer[0]

        def get_map_size(self) -> int:
            """
            The size of the Map is the second word in the Map Schema.
            :return:    Number of elements in the currently traversed Map.
            """
            assert self.type == Element.Type.MAP
            assert len(self._schema) > self._schema_itr + 1
            return self._schema[self._schema_itr + 1]

        def __getitem__(self, key: (str, int)) -> 'StructuredBuffer.Accessor':
            if self.type == Element.Type.LIST:
                if not isinstance(key, int):
                    raise IndexError("Lists must be accessed using an index, not {}".format(key))

                # make sure that there are enough elements in the list
                list_size = self.get_list_size()
                if key >= list_size:
                    raise IndexError(
                        "Cannot get element at index {} from a List of size {}".format(key, list_size))

                # advance the schema index
                self._schema_itr = self._schema_itr + 1

                # advance to the next buffer if the value is not ground
                if Schema.is_ground(self._schema[self._schema_itr]):
                    self._buffer_itr = key + 1
                else:
                    self._buffer = self._buffer[key + 1]
                    self._buffer_itr = 0

            elif self.type == Element.Type.MAP:
                if not isinstance(key, str):
                    raise KeyError("Maps must be accessed using a string, not {}".format(key))

                if self._dictionary_itr is None or key not in self._dictionary_itr:
                    raise KeyError("Unknown key \"{}\"".format(key))

                # find the index of the child in the map
                child_index = list(self._dictionary_itr.keys()).index(key)
                map_size = self.get_map_size()
                assert child_index < map_size

                # advance the schema index
                self._schema_itr = self._schema_itr + 2 + child_index
                if Schema.is_pointer(self._schema[self._schema_itr]):
                    # resolve pointer to list or map
                    self._schema_itr += self._schema[self._schema_itr]

                # advance to the next buffer if the value is not ground
                if Schema.is_ground(self._schema[self._schema_itr]):
                    self._buffer_itr = child_index
                else:
                    self._buffer = self._buffer[child_index]
                    self._buffer_itr = 0

                # advance the dictionary
                self._dictionary_itr = self._dictionary_itr[key]

            else:
                raise ValueError("Cannot use operator[] on a {} element".format(
                    "Number" if self.type == Element.Type.NUMBER else "String"))

            assert self._schema_itr < len(self._schema)
            return self

    class Reader(Accessor):

        def as_number(self) -> float:
            """
            Returns the current element as a Number.
            """
            if self.type != Element.Type.NUMBER:
                raise ValueError("Element is not a Number")
            result = self._buffer[self._buffer_itr]
            assert isinstance(result, float)
            return result

        def as_string(self) -> str:
            """
            Returns the current element as a String.
            """
            if self.type != Element.Type.STRING:
                raise ValueError("Element is not a String")
            result = self._buffer[self._buffer_itr]
            assert isinstance(result, str)
            return result

    class Writer(Accessor):

        def __init__(self, structured_buffer: 'StructuredBuffer'):
            """
            :param structured_buffer:   StructuredBuffer instance being accessed.
            """
            super().__init__(structured_buffer)
            self._path = [structured_buffer.buffer]

        def __getitem__(self, key: (str, int)) -> 'StructuredBuffer.Writer':
            super().__getitem__(key)
            if self._path[-1] is not self._buffer:
                self._path.append(self._buffer)
            return self

        def set(self, value: Union[float, int, str, Element]):
            """
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

            :param value:   New value to set.
            """
            from copy import copy

            # make sure that the given value is of the correct type
            if isinstance(value, (float, int)) and self.type != Element.Type.NUMBER:
                raise ValueError("Cannot change a value of type NUMBER to {}".format(self.type.name, value))
            elif isinstance(value, str) and self.type != Element.Type.STRING:
                raise ValueError("Cannot change a value of type STRING to {}".format(self.type.name, value))
            elif isinstance(value, Element):
                slice_end = self._schema_itr + len(value.schema)
                if len(self._schema) <= slice_end or value.schema != self._schema[self._schema_itr: slice_end]:
                    raise ValueError("Incompatible Schemas")

            # if the new value is equal to the old one, we don't need to do anything
            if value == self._buffer[self._buffer_itr]:
                return

            # create a new buffer for the updated value
            if Schema.is_ground(self._schema[self._schema_itr]):
                new_buffer = copy(self._buffer)
                if self.type == Element.Type.NUMBER:
                    new_buffer[self._buffer_itr] = float(value)
                elif self.type == Element.Type.STRING:
                    new_buffer[self._buffer_itr] = value
            else:
                new_buffer = StructuredBuffer._produce_buffer_from_element(value)

            # create a copy of all buffers that are changed
            assert (self._path[-1] == self._buffer)
            while len(self._path) > 1:
                old_buffer = self._path.pop()
                old_parent_buffer = self._path[-1]
                new_buffer = [(x if x != old_buffer else new_buffer) for x in old_parent_buffer]
            self._structured_buffer.buffer = new_buffer

    def __init__(self, schema: Schema, buffer: Buffer, dictionary: Optional[Dictionary]):
        """
        :param schema:      Schema of the Buffer. Is constant.
        :param buffer:      Buffer from which to read and into which to write data.
        :param dictionary:  A Dictionary referring to the topmost map in the Schema. Can be None.
        """
        self.schema: Schema = schema
        self.buffer: Buffer = buffer if isinstance(buffer, list) else [buffer]
        self.dictionary: Optional[StructuredBuffer.Dictionary] = dictionary

    @classmethod
    def create(cls, source: [Schema, Element, 'StructuredBuffer']):
        """
        Factory function.
        :param source: Anything that can be used to create a new StructuredBuffer.
        :return: A new StructuredBuffer instance.
        """
        # null-initialize from schema
        if isinstance(source, Schema):
            return StructuredBuffer(source, cls._produce_buffer_from_schema(source), None)

        # value-initialize from element
        elif isinstance(source, Element):
            return StructuredBuffer(source.schema, cls._produce_buffer_from_element(source),
                                    cls.produce_dictionary(source))

        # copy-initialize from other StructuredBuffer
        elif isinstance(source, StructuredBuffer):
            return StructuredBuffer(source.schema, source.buffer, source.dictionary)

        # error
        else:
            raise ValueError("Cannot create a StructuredBuffer from a {}".format(type(source).__name__))

    @classmethod
    def produce_dictionary(cls, element: Element) -> Optional[Dictionary]:
        """
        Recursive, depth-first assembly of the Dictionary.
        :param element: Element whose name (if it has one) to write into the dictionary.
        :return: Dictionary or None if this is a leaf Element.
        """
        if element.type in (Element.Type.NUMBER, Element.Type.STRING):
            return None

        elif element.type == Element.Type.LIST:
            return cls.produce_dictionary(element.value[0])

        elif element.type == Element.Type.MAP:
            return {name: cls.produce_dictionary(child_element) for (name, child_element) in element.value.items()}

    @classmethod
    def produce_buffer(cls, source: Union[Element, Schema]) -> Buffer:
        """
        The internal methods produce either a list (if the root element is a List or a Map) or a value (if the root
        element is a Number or a String). We always need a list as the buffer element, so if the return value is not
        already one, we wrap it into a list.
        :param source: Thing from which to produce the buffer
        :return: Root element of the buffer in a list.
        """
        if isinstance(source, Element):
            root = cls._produce_buffer_from_element(source)
        elif isinstance(source, Schema):
            root = cls._produce_buffer_from_schema(source)
        else:
            raise ValueError("Cannot produce a buffer using a {}".format(type(source).__name__))
        return root if isinstance(root, list) else [root]

    @classmethod
    def _produce_buffer_from_element(cls, element: Element) -> Union[float, str, Buffer]:
        """
        Recursive, depth-first assembly of the Buffer.
        :param element: Element to write into the buffer
        :return: A Buffer representing the given Element.
        """
        if element.type == Element.Type.NUMBER:
            return element.value

        elif element.type == Element.Type.STRING:
            return element.value

        elif element.type == Element.Type.LIST:
            result = [len(element.value)]
            result.extend(cls._produce_buffer_from_element(child) for child in element.value)
            return result

        elif element.type == Element.Type.MAP:
            return [cls._produce_buffer_from_element(child) for child in element.value.values()]

    @classmethod
    def _produce_buffer_from_schema(cls, schema: Schema) -> Union[float, str, Buffer]:
        """
        Zero-initialized assembly of a Buffer.
        :param schema: Schema from which to create the Buffer.
        :return: Empty buffer laid out according to the given Schema.
        """

        def _produce_subbuffer(buffer: Buffer, index: int = 0) -> Buffer:
            assert len(schema) > index
            if schema[index] == Element.Type.NUMBER:
                buffer.append(0)

            elif schema[index] == Element.Type.STRING:
                buffer.append("")

            elif schema[index] == Element.Type.LIST:
                buffer.append([])

            elif schema[index] == Element.Type.MAP:
                assert len(schema) > index + 1
                map_size = schema[index + 1]
                for child_index in range(map_size):
                    child_schema_index = index + 2 + child_index
                    assert len(schema) > child_schema_index
                    # if the child index is not a number or a string, it is a pointer and needs to be resolved
                    if Schema.is_pointer(schema[child_schema_index]):
                        child_schema_index += schema[child_schema_index]
                        assert len(schema) > child_schema_index
                    _produce_subbuffer(buffer, child_schema_index)

            return buffer

        return _produce_subbuffer([])

    def read(self) -> 'StructuredBuffer.Reader':
        """
        Read access to this StructuredBuffer instance.
        """
        return StructuredBuffer.Reader(self)

    def write(self) -> 'StructuredBuffer.Writer':
        """
        Write access to this StructuredBuffer instance.
        """
        return StructuredBuffer.Writer(self)

    def __hash__(self) -> int:
        """
        The hash of a StructuredBuffer only considers values in the buffer, not the Schema or Dictionary.
        """
        return hash(tuple(self.buffer))

import sys
from enum import IntEnum, unique, auto
from typing import NewType, List, Dict, Union, Optional

Dictionary = Dict[str, Optional['Dictionary']]
Buffer = Union[float, str, List['Buffer']]
Schema = NewType('Schema', List[int])


@unique
class Type(IntEnum):
    NUMBER = sys.maxsize - 3
    STRING = auto()
    LIST = auto()
    MAP = auto()


class Element:
    """
    StructuredMemory Elements define a StructuredMemory.
    From them, we can derive the Schema (which is a simple list of integers that define how data is laid out in
    memory), the Buffer (which are blocks of memory containing the data, but without additional information on the
    type of data) and the Accessor (which requires both a Schema and a Buffer to read/write data).
    """

    def __init__(self, element_type: Type, value):
        self._type = element_type
        self._value = value
        self._buffer: Buffer = self._produce_buffer(self)
        self._schema: Schema = Schema([])
        self._dictionary: Dictionary = self._produce_dictionary(self)

        self._produce_schema(self, self._schema)

    @property
    def data_type(self):
        return self._type

    @property
    def value(self) -> Union[float, str, List["Element"], Dict[str, "Element"]]:
        return self._value

    @property
    def schema(self) -> Schema:
        return self._schema

    @property
    def buffer(self) -> Buffer:
        return self._buffer

    @property
    def dictionary(self) -> Dictionary:
        return self._dictionary

    @classmethod
    def _produce_schema(cls, element: 'Element', schema: Schema):
        """
        Recursive, breadth-first assembly of the Schema.
        :param element: Element to add to the Schema.
        :param schema: [out] Schema to modify.
        """
        if element.data_type == Type.NUMBER:
            schema.append(int(Type.NUMBER))

        elif element.data_type == Type.STRING:
            schema.append(int(Type.STRING))

        elif element.data_type == Type.LIST:
            schema.append(int(Type.LIST))
            cls._produce_schema(element.value[0], schema)

        elif element.data_type == Type.MAP:
            schema.append(int(Type.MAP))  # this is a map
            schema.append(len(element.value))  # number of items in the map
            child_position = len(schema)
            schema.extend([0] * len(element.value))  # reserve space for pointers to the child data
            for child_element in element.value.values():

                if child_element.data_type in (Type.NUMBER, Type.STRING):  # numbers and strings are stored inline
                    schema[child_position] = int(child_element.data_type)

                else:  # lists and maps store a pointer and append themselves to the end of the schema
                    schema[child_position] = len(schema)
                    cls._produce_schema(child_element, schema)
                child_position += 1

    @classmethod
    def _produce_buffer(cls, element: 'Element') -> Buffer:
        """
        Recursive, depth-first assembly of the Buffer.
        :param element: Element to write into the buffer
        :return: A Buffer representing the given Element.
        """
        if element.data_type == Type.NUMBER:
            return element.value

        elif element.data_type == Type.STRING:
            return element.value

        elif element.data_type == Type.LIST:
            return [cls._produce_buffer(child) for child in element.value]

        elif element.data_type == Type.MAP:
            return [cls._produce_buffer(child) for child in element.value.values()]

    @classmethod
    def _produce_dictionary(cls, element: 'Element') -> Optional[Dictionary]:
        """
        Recursive, depth-first assembly of the Dictionary.
        :param element: Element whose name (if it has one) to write into the dictionary.
        :return: Dictionary or None if this is a leaf Element.
        """
        if element.data_type in (Type.NUMBER, Type.STRING):
            return None

        elif element.data_type == Type.LIST:
            return cls._produce_dictionary(element.value[0])

        elif element.data_type == Type.MAP:
            return {name: cls._produce_dictionary(child_element) for (name, child_element) in element.value.items()}


class Number(Element):
    def __init__(self, value: float):
        super().__init__(Type.NUMBER, value)


class String(Element):
    def __init__(self, value: str):
        super().__init__(Type.STRING, value)


class List(Element):
    def __init__(self, *values: Element):
        if len(values) == 0:
            raise ValueError("List element cannot be empty")

        reference_value = values[0]
        for i in range(1, len(values)):
            if values[i].schema != reference_value.schema:
                raise ValueError("List elements must all have the same schema")

        if reference_value.data_type == Type.MAP:
            for i in range(1, len(values)):
                if values[i].value.keys() != reference_value.value.keys():
                    raise ValueError("Maps in a list must all have the same keys")

        super().__init__(Type.LIST, values)


class Map(Element):
    def __init__(self, **values: Element):
        if len(values) == 0:
            raise ValueError("Map element cannot be empty")
        super().__init__(Type.MAP, values)

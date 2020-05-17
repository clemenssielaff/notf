#!/usr/bin/env python3
from os import getcwd
from sys import path as PATH
from re import compile as compile_regex, Match
from typing import List, Callable, NamedTuple, Any, Iterable, Optional
from inspect import isclass, isbuiltin, ismethoddescriptor as ismethod, isdatadescriptor as isfield

# add the current working directory to the path, so you can find the module
PATH.append(getcwd())
import pycnotf

MODULE = pycnotf
INDENTATION_WIDTH: int = 4
DOCSTRING_QUOTE = '"'
CONSTRUCTOR_OVERLOAD_REGEX = compile_regex(r"\(self.*?\)")
FIELD_DOCSTRING_REGEX = compile_regex(r"\[(\w*)\] (.*)")


class FieldDescription(NamedTuple):
    name: str
    data_type: str
    docstring: str


class ClassDescription(NamedTuple):
    name: str
    constructors: List[str]
    fields: List[FieldDescription]
    methods: List[Callable]
    staticmethods: List[Callable]
    enums: List[str]


def indent(lines: Iterable[str], depth: int = 1) -> str:
    """
    Concatenates the given lines of text into a single string, each line indented by the given depth.
    """
    return f'\n{" " * (depth * INDENTATION_WIDTH)}'.join(('', *(line for line in lines if line)))


def get_public_names(obj) -> List[str]:
    """
    Get all public names from a module or class.
    """
    return [name for name in dir(obj) if not name.startswith('_')]


def parse_functions(module) -> List[Callable]:
    """
    Find all free functions in a module.
    """
    return [getattr(MODULE, name) for name in get_public_names(module) if isbuiltin(getattr(MODULE, name))]


def parse_constructors(cls) -> List[str]:
    """
    Returns a list of constructor overload signatures for a given class.
    """
    overloads: List[str] = []
    docstring: str = cls.__init__.__doc__
    match: Optional[Match] = CONSTRUCTOR_OVERLOAD_REGEX.search(docstring)
    assert match
    while match:
        overloads.append(match.group())
        match = CONSTRUCTOR_OVERLOAD_REGEX.search(docstring, match.span()[1])
    return overloads


def parse_fields(cls) -> List[FieldDescription]:
    """
    Returns a list of descriptions of all fields of a given class.
    """
    result: List[FieldDescription] = []
    for name in get_public_names(cls):
        field: Any = getattr(cls, name)
        if isfield(field):
            match: Optional[Match] = FIELD_DOCSTRING_REGEX.search(field.__doc__)
            if match is None:
                data_type = "Any"
                docstring = field.__doc__
            else:
                data_type, docstring = match.groups()
            result.append(FieldDescription(
                name=name,
                data_type=data_type,
                docstring=docstring,
            ))
    return result


def parse_enums(cls) -> List[str]:
    """
    Parses all class-level things that can be converted to int.
    I think, that condition is enough to catch enums and only enums..?
    """
    result: List[str] = []
    for name in get_public_names(cls):
        attribute = getattr(cls, name)
        try:
            enum_value: int = int(attribute)
        except TypeError:
            continue
        result.append(f'{name}: int = {enum_value}')
    return result


def parse_classes(module) -> List[ClassDescription]:
    """
    Find all classes in a module.
    """
    return [
        ClassDescription(
            name=cls.__name__,
            constructors=parse_constructors(cls),
            fields=parse_fields(cls),
            methods=[getattr(cls, mth) for mth in get_public_names(cls) if ismethod(getattr(cls, mth))],
            staticmethods=[getattr(cls, mth) for mth in get_public_names(cls) if isbuiltin(getattr(cls, mth))],
            enums=parse_enums(cls),
        ) for cls in (getattr(MODULE, name) for name in get_public_names(module) if isclass(getattr(MODULE, name)))
    ]


def get_function_signature(func: Callable, depth: int = 0, static: bool = False) -> str:
    """
    Infers the function signature and docstring (if available) from a C++ function/method exposed via pybind11.
    """
    signature, docstring = func.__doc__.split('\n', 1)
    return indent((
        '@staticmethod' if static else None,
        f'def {signature}:' + (' ...' if not docstring else '')
    ), depth) + (indent((
        DOCSTRING_QUOTE * 3,
        *[line for line in docstring.split('\n') if not line.strip() == ""],
        DOCSTRING_QUOTE * 3,
        '...'
    ), depth + 1) if docstring else '')


def get_property_signature(field: FieldDescription) -> str:
    return indent((
        '@property',
        f'def {field.name}(self) -> {field.data_type}:' + ("" if field.docstring else " ..."),
    )) + (indent((DOCSTRING_QUOTE * 3, field.docstring, DOCSTRING_QUOTE * 3, '...'), 2) if field.docstring else '')


def generate_stubs(module) -> str:
    """
    Generate all class and function stubs for a module.
    """
    result: List[str] = [
        "from __future__ import annotations",
    ]

    cls: ClassDescription
    for cls in parse_classes(module):
        result.append(f'\n\nclass {cls.name}:')
        if cls.enums:
            result.append(indent(cls.enums))
        for constructor in cls.constructors:
            result.append(indent((f'def __init__{constructor} -> None: ...',)))
        for field in cls.fields:
            result.append(get_property_signature(field))
        for method in cls.staticmethods:
            result.append(get_function_signature(method, depth=1, static=True))
        for method in cls.methods:
            result.append(get_function_signature(method, depth=1))

    for function in parse_functions(module):
        result.append(get_function_signature(function, depth=0))

    result.append("")  # newline at the end of file
    return '\n'.join(result)


def run_heuristic_fixes(string: str) -> str:
    """
    A hand-crafted list of dirty hacks in order to get the resulting Python file to be error-free.
    """
    # simple replacements (order matters)
    string = string.replace('pycnotf.', '')
    string = string.replace('notf::detail::Size2<float>', 'Size2f')
    string = string.replace('notf::detail::Size2<int>', 'Size2i')
    string = string.replace('notf::detail::Aabr<float>', 'Aabrf')
    string = string.replace('notf::detail::Segment<notf::detail::Vector2<float> >', 'Segment2f')
    string = string.replace('notf::detail::Vector2<float>', 'V2f')
    string = string.replace('notf::Orientation', 'Orientation')

    return string


def main():
    """
    Run this file with the current working directory set to a directory that contains the compiled Python extension to
    produce a new file `*.pyi` that contains information about the functions and classes contained in
    the extension. This information would usually be hidden from IDEs because the function signatures in the compiled
    extension are missing type hints and/or the IDE cannot introspect the extension (for whatever reason).

    For more information about Python pyi files (which are still valid Python code) see PEP 484:
    https://www.python.org/dev/peps/pep-0484/#stub-files

    It should be possible to do this with MyPy, but I could not get it to work in any reasonable amount of time.
    Additionally, while pybind11 does add the signature including type hints in the docstring, it does not expose the
    type hints on the Python objects themselves (AFAIK, this is currently done via the __annotations__ attribute). So it
    is doubtful that it would have worked correctly anyway.
    """
    with open(f'{MODULE.__name__}.pyi', 'w') as file:
        file.write(run_heuristic_fixes(generate_stubs(MODULE)))


if __name__ == '__main__':
    main()

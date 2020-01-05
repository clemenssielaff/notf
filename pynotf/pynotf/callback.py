from re import compile as compile_regex
from types import CodeType
from typing import Optional, List, Type, Dict, Callable, Any


########################################################################################################################

class Callback:
    """
    A Callback is a user-defined piece of code that can be slotted into different places in the notf architecture that
    allow a user to fully define the behavior of the UI without having to worry about the system's internals.

    All Callbacks are purely functional, even though some uses supply a modifiable piece of data, that allows the
    Callback to act as an instance method.

    For internal reasons, the user-defined Python code must adhere to the PEP8 convention of indentation by 4 spaces.
    """

    _function_name: str = "func"  # name of the wrapper function, does not matter but must be consistent
    _empty_line = compile_regex(r"^\s*?$")  # regex to find out whether a given string only contains whitespace
    _trailing_whitespace = compile_regex(r"\s*?$")  # regex to find trailing whitespace on a line

    def __init__(self, signature: Dict[str, Type], return_type: Optional[Type], source: str) -> None:
        """
        Constructor.
        Example:
            callback: Callback = Callback(dict(a=int, b=float), float, "return float(a + b)")
        :param signature: Signature of the Callback. Is used to define the internal function header.
        :param return_type: The Callback's return type. The source code must return a value of this type, otherwise the
                            Callback will raise a TypeError when called.
        :param source: Source string defining the body of the Callback's function. Must be valid Python code.
        """

        # store the return type to ensure that the callback returns the correct type
        self._signature: Dict[str, Type] = signature  # is ordered, is constant
        self._return_type: Optional[Type] = return_type  # is constant

        # wrap the given source in a function, that we we can call with the given signature
        self._source: str = '\n'.join((
            f"def {self._function_name}({self._get_parameter_list()}) -> {self._get_return_type()}:",
            self._normalize_body(source),
            ''))  # add an empty line at the end to close the function

        # create the Python function object by executing the source in a new environment
        # and then extracting the function by its name from the environment
        env = {}
        code: CodeType = compile(self._source, "<string>", "exec", optimize=1)
        exec(code, env)
        self._callback: Callable = env[self._function_name]

    def get_source(self) -> str:
        """
        The actual source of the Callback function (after normalization and with the Callback header).
        """
        return self._source

    def __call__(self, *args) -> Any:
        """
        Invokes the Callback function with the given arguments.
        :raise TypeError: If arguments of the wrong type were passed to the Callback, or the Callback returned a value
                          of a wrong type.
        :return: The result of the Callback.
        """
        # check the argument types
        are_arguments_valid: bool = len(args) == len(self._signature)
        if are_arguments_valid:
            for index, parameter_type in enumerate(self._signature.values()):
                if not isinstance(args[index], parameter_type):
                    are_arguments_valid = False
                    break
        if not are_arguments_valid:
            raise TypeError(
                f'Callback called with arguments of the wrong type!\nExpected ({self._get_parameter_list()}), '
                f'got: ({", ".join(f"{value}: {type(value).__name__}" for value in args)})'
            )

        # invoke the callback
        result: Any = self._callback(*args)

        # check the return type
        if self._return_type is None:
            is_result_valid: bool = result is None
        else:
            is_result_valid: bool = isinstance(result, self._return_type)
        if not is_result_valid:
            raise TypeError(
                f'Callback did not return the promised return type "{self._get_return_type()}", '
                f'but "{result.__class__.__name__}" instead')

        # return the valid result
        return result

    def _get_parameter_list(self) -> str:
        """
        A comma-separated list of argument name: argument type.
        """
        # get the name for each argument
        arguments: Dict[str, str] = {}
        for arg_name, arg_type in self._signature.items():
            type_name: Optional[str] = getattr(arg_type, "__name__")
            if type_name is None:
                type_name = str(arg_type).lstrip('typing')
            arguments[arg_name] = type_name

        # compile the comma-separated list in the return string
        return ', '.join(f"{arg_name}: {arg_type}" for arg_name, arg_type in arguments.items())

    def _get_return_type(self) -> str:
        """
        The name of the Callbacks' return type.
        """
        # if the return type has a name use that for type hints (None has no __name__ attribute)
        return_type: Optional[str] = getattr(self._return_type, "__name__")
        if return_type is None:
            return_type = 'NoneType'
        return return_type

    @classmethod
    def _normalize_body(cls, source: str) -> str:
        """
        Removes superfluous whitespace, adds an indent to make the source usable as a function body.
        :param source: User-defined source body.
        :return: Normalized source body.
        """
        lines: List[str] = source.split('\n')

        # find the first non-empty line
        start_index: int = 0
        for line in lines:
            if cls._empty_line.fullmatch(line) is None:
                break
            else:
                start_index += 1

        # find the last non-empty line
        end_index: int = len(lines)
        for line in reversed(lines):
            if cls._empty_line.fullmatch(line) is None:
                break
            else:
                end_index -= 1

        # return the normalized result
        return '\n'.join(
            (' ' * 4) +  # indent
            cls._trailing_whitespace.sub('', line)  # trim trailing whitespace from each line
            for line in lines[start_index: end_index]  # trim empty lines from the top and bottom
        )

from typing import Any, Optional, Dict
from types import CodeType

import pynotf.core as core
import pycnotf

__all__ = ('Expression', 'ConstNodeAccessor')


# EXPRESSION ###########################################################################################################

class Expression:
    """
    Empty expressions are allowed, they return None when executed.
    """

    DEFAULT_SCOPE = dict(
        V2f=pycnotf.V2f,
        M3f=pycnotf.M3f,
        Aabrf=pycnotf.Aabrf,
    )

    def __init__(self, source: str = ''):
        self._source: str = source
        self._is_expression: bool = True
        self._code: CodeType

        actual_source: str = self._source or 'None'  # an empty string does not compile
        try:
            self._code = compile(actual_source, '<string>', mode='eval')
        except SyntaxError:
            self._is_expression: bool = False
            self._code = compile(actual_source, '<string>', mode='exec')

    def execute(self, **scope) -> Any:
        scope.update(self.DEFAULT_SCOPE)
        if self._is_expression:
            return eval(self._code, {}, scope)  # returns the result of the expression
        else:
            return exec(self._code, {}, scope)  # returns None

    def __repr__(self) -> str:
        return f'Expression("{self._source}")'


# NODE ACCESSOR ########################################################################################################

class ConstNodeAccessor:
    """
    Constant access to a Node.
    """

    def __init__(self, node: core.Node):
        self._node: core.Node = node

    def __getattr__(self, name: str):
        if name == "window_xform":
            return (self._node.get_composition().xform
                    * core.m3f_from_value(self._node.get_interop('widget.xform').get_value()))
        elif name == "grant":
            return self._node.get_composition().grant
        raise NameError(f'Unknown NodeAccessor attribute "{name}"')

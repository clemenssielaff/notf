from typing import Optional, Tuple
from abc import ABCMeta, abstractmethod

from .value import Value
from .logic import Subscriber, Publisher


########################################################################################################################

class Operation(metaclass=ABCMeta):
    """
    Operations are Functors that can be part of a Operator.
    """

    @property
    @abstractmethod
    def input_schema(self) -> Value.Schema:
        """
        The Schema of the input value of the Operation.
        """
        pass

    @property
    @abstractmethod
    def output_schema(self) -> Value.Schema:
        """
        The Schema of the output value of the Operation (if there is one).
        """
        pass

    @abstractmethod
    def _perform(self, value: Value) -> Optional[Value]:
        """
        Operation implementation.
        :param value: Input value, conforms to the Schema specified by the `input_schema` property.
        :return: Either a new output value (conforming to the Schema specified by the `output_schema` property)
            or None.
        """
        pass

    def __call__(self, value: Value) -> Optional[Value]:
        """
        Perform the Operation on a given value.
        :param value: Input value, must conform to the Schema specified by the `input_schema` property.
        :return: Either a new output value (conforming to the Schema specified by the `output_schema` property)
            or None.
        :raise TypeError: If the input Value does not conform to this Operator's input Schema.
        """
        if value.schema != self.input_schema:
            raise TypeError("Value does not conform to the Operator's input Schema")
        result: Optional[Value] = self._perform(value)
        if result is not None:
            assert result.schema == self.output_schema
        return result


########################################################################################################################

class NoOp(Operation):
    """
    An Operation that passes the given Value onwards.
    Used to create valid but empty Operators.
    """

    def __init__(self, schema: Value.Schema):
        """
        Constructor.
        :param schema: Schema of the Value to pass through.
        """
        self._schema: Value.Schema = schema

    @property
    def input_schema(self) -> Value.Schema:
        return self._schema

    @property
    def output_schema(self) -> Value.Schema:
        return self._schema

    def _perform(self, value: Value) -> Value:
        """
        :param value: Input value.
        :return: Unchanged input value.
        """
        return value


########################################################################################################################

class Operator:
    """
    A Operator is a sequence of Operations that are applied to an input value.
    Not every input is guaranteed to produce an output as Operations are free to store or ignore input values.
    If an Operation returns a value, it is passed on to the next Operation. If any Operation does not return a value
    (returns None), the following Operations are not called and the Operator produces None. Otherwise, its `__call__`
    method returns the result of the last Operation.
    If an Operation throws an exception, it will propagate to the outer scope.
    """

    def __init__(self, *operations: Operation):
        """
        Constructor.
        :param operations: All Operations that this Operator performs in order. Must not be empty.
        :raise ValueError: If no Operations are passed.
        :raise TypeError: If two subsequent Operations have mismatched Schemas.
        """
        if len(operations) == 0:
            raise ValueError("Cannot create a Operator without a single Operation")

        for i in range(len(operations) - 1):
            if operations[i].output_schema != operations[i + 1].input_schema:
                raise TypeError(f"Operations {i} and {i + 1} have mismatched Value Schemas")

        self._operations: Tuple[Operation] = operations

    @property
    def input_schema(self) -> Value.Schema:
        """
        The Schema of the input Value of the Operator.
        """
        return self._operations[0].input_schema

    @property
    def output_schema(self) -> Value.Schema:
        """
        The Schema of the output Value of the Operator.
        """
        return self._operations[-1].output_schema

    def __call__(self, value: Value) -> Optional[Value]:
        """
        Performs the sequence of Operations on the given Value and returns it or None.
        Exceptions thrown by a Operation will propagate back to the caller.
        """
        for operation in self._operations:
            value = operation(value)
            if value is None:
                break
        return value


########################################################################################################################

class Transformer(Subscriber, Publisher):
    """
    A Transformer is Subscriber/Publisher that applies a sequence of Operations to an input value before publishing it.
    If an Operation throws an exception, the Transformer will fail as a whole.
    Operations are not able to complete the Transformer, other than through failure.
    """

    def __init__(self, *operations: Operation):
        """
        Constructor.
        :param operations: All Operations that this Transformer performs in order. Must not be empty.
        :raise ValueError: If no Operations are passed.
        :raise TypeError: If two subsequent Operations have mismatched Schemas.
        """
        self._operator: Operator = Operator(*operations)

        Subscriber.__init__(self, self._operator.input_schema)
        Publisher.__init__(self, self._operator.output_schema)

    def on_next(self, signal: Publisher.Signal, value: Value):
        """
        Abstract method called by any upstream Publisher.
        :param signal   The Signal associated with this call.
        :param value    Input Value conforming to the Operator's input Schema.
        """
        try:
            result = self._operator(value)
        except Exception as exception:
            self.error(exception)
        else:
            if result is not None:
                self.publish(result)

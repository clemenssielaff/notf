from typing import List, Optional, Union, Callable, TypeVar
from random import randint as random_int, choice as random_choice, shuffle

from pynotf.logic import Operator, Receiver, Emitter, Circuit, ValueSignal, FailureSignal, \
    CompletionSignal
from pynotf.value import Value

T = TypeVar('T')

number_schema: Value.Schema = Value(0).schema
"""
Schema consisting of a single number.
"""

string_schema: Value.Schema = Value("").schema
"""
Schema consisting of a single string.
"""


def random_schema(depth: int = 4, width: int = 3) -> Value.Schema:
    """
    Produces a random Value Schema.
    :param depth: Maximum depth of the Schema.
    :param width: Maximum number of elements in a map.
    """
    # ensure sane inputs
    depth = max(0, depth)
    width = max(1, width)

    def random_element(remaining_depth: int = 4):
        # if the schema can not get any deeper, return a leaf element
        # even if the schema is allowed to get deeper, there is a change that it won't
        if random_int(0, remaining_depth) == 0:  # will always be true if _depth == 0
            return random_choice((0, ""))

        # we are going deeper, which means that this level needs to be a list or a map
        if random_choice((True, False)):  # list
            return [random_element(remaining_depth - 1)]
        else:  # map
            return {
                "key{}".format(index): random_element(remaining_depth - 1) for index in range(random_int(1, width))
            }

    return Value.Schema(random_element(depth))


def random_shuffle(*args: T) -> List[T]:
    """
    Random shuffle of a list in-place.
    Is necessary because random.shuffle returns None and I want to be able to write:
        for x in random_shuffle(1, 2, 3, 4):
            print(x)  # returns 3, 4, 1, 2 for example
    """
    values = list(args)
    shuffle(values)
    return values


class Recorder(Receiver):
    """
    A Recorder is a special Receiver for test cases that simply records all signals that it receives.
    """

    def __init__(self, circuit: 'Circuit', element_id: Circuit.Element.ID, schema: Value.Schema):
        Receiver.__init__(self, schema)

        self._circuit: Circuit = circuit  # is constant
        self._element_id: Circuit.Element.ID = element_id  # is constant
        self.signals: List[Union[ValueSignal, FailureSignal, CompletionSignal]] = []

    @classmethod
    def record(cls, emitter: Emitter) -> 'Recorder':
        """
        Convenience method to create and connect a matching Recorder to a given Emitter.
        """
        recorder: Recorder = emitter.get_circuit().create_element(Recorder, emitter.get_output_schema())
        recorder.connect_to(make_handle(emitter))
        return recorder

    def get_id(self) -> Circuit.Element.ID:
        return self._element_id

    def get_circuit(self) -> 'Circuit':
        return self._circuit

    def _on_value(self, signal: ValueSignal):
        self.signals.append(signal)

    def _on_failure(self, signal: FailureSignal):
        self.signals.append(signal)

    def _on_completion(self, signal: CompletionSignal):
        self.signals.append(signal)

    def get_values(self) -> List[Value]:
        """
        All Values that the Recorder received so far.
        """
        result: List[Value] = []
        for signal in self.signals:
            if isinstance(signal, ValueSignal):
                result.append(signal.get_value())
        return result

    def get_failures(self) -> List[FailureSignal]:
        """
        All errors that the Recorder received so far.
        """
        result: List[FailureSignal] = []
        for signal in self.signals:
            if isinstance(signal, FailureSignal):
                result.append(signal)
        return result

    def get_completions(self) -> List[CompletionSignal]:
        """
        All completion that the Recorder received so far.
        """
        result: List[CompletionSignal] = []
        for signal in self.signals:
            if type(signal) == CompletionSignal:
                result.append(signal)
        return result


def make_handle(emitter: Emitter) -> Emitter.Handle:
    """
    Convenience function to create a Handle around a strong reference of an Emitter.
    :param emitter: Emitter to handle.
    :return:        Handle to the given Emitter.
    """
    return Emitter.Handle(emitter)


def create_emitter(circuit: Circuit, schema: Value.Schema, is_blockable: bool = False):
    """
    Creates an instance of an anonymous Emitter class that can emit values of the given schema.
    :param circuit:         Circuit to contain the Emitter.
    :param schema:          Schema of the Value emitted by this Emitter, defaults to empty.
    :param is_blockable:    Whether or not Signals from this Emitter can be blocked, defaults to False.
    :return: New instance of the Emitter class.
    """

    class AnonymousEmitter(Emitter):
        def __init__(self, circuit_: 'Circuit', element_id: Circuit.Element.ID):
            Emitter.__init__(self, schema, is_blockable)
            self._circuit: Circuit = circuit_  # is constant
            self._element_id: Circuit.Element.ID = element_id  # is constant

        def get_id(self) -> Circuit.Element.ID:
            return self._element_id

        def get_circuit(self) -> 'Circuit':
            return self._circuit

    return circuit.create_element(AnonymousEmitter)


def create_receiver(circuit: Circuit, schema: Value.Schema,
                    on_value: Optional[Callable] = None,
                    on_failure: Optional[Callable] = None,
                    on_completion: Optional[Callable] = None):
    """
    Creates an instance of an anonymous Receiver class that can receive values of the given schema.
    :param circuit:         Circuit to contain the Receiver.
    :param schema:          Schema of the values to receive.
    :param on_value:        (Optional) Callback for the `on_value` method.
    :param on_failure:      (Optional) Callback for the `on_failure` method.
    :param on_completion:   (Optional) Callback for the `on_completion` method.
    :return: New instance of the Receiver class.
    """

    class AnonymousReceiver(Receiver):
        def __init__(self, circuit_: 'Circuit', element_id: Circuit.Element.ID):
            Receiver.__init__(self, schema)
            self._circuit: Circuit = circuit_  # is constant
            self._element_id: Circuit.Element.ID = element_id  # is constant

        def get_id(self) -> Circuit.Element.ID:
            return self._element_id

        def get_circuit(self) -> 'Circuit':
            return self._circuit

        def _on_value(self, signal: ValueSignal):
            if on_value:
                on_value(signal)

        def _on_failure(self, signal: FailureSignal):
            if on_failure:
                on_failure(signal)

        def _on_completion(self, signal: CompletionSignal):
            if on_completion:
                on_completion(signal)

    return circuit.create_element(AnonymousReceiver)


def create_operation(schema: Value.Schema, operation: Callable, output_schema: Optional[Value.Schema] = None):
    """
    Creates an instance of an anonymous Operation class that can be used to create an Operator.
    :param schema:          Schema of the Values ingested by the Operation.
    :param operation:       Operation to perform on given Values.
    :param output_schema:   (Optional) Schema of the output Value if different from the input.
    :return:                New instance of the Operation class.
        """

    class AnonymousOperation(Operator.Operation):
        def get_input_schema(self) -> Value.Schema:
            return schema

        def get_output_schema(self) -> Value.Schema:
            return schema if output_schema is None else output_schema

        def __call__(self, value: Value) -> Optional[Value]:
            return operation(value)

    return AnonymousOperation()


def create_operator(circuit: Circuit,
                    schema: Value.Schema,
                    operation: Callable,
                    output_schema: Optional[Value.Schema] = None):
    """
    Creates an instance of an anonymous Operator class that can operate on values of the given schema.
    :param circuit:         Circuit to contain the Operator.
    :param schema:          Schema of the Values ingested by the Operation.
    :param operation:       Operation to perform on given Values.
    :param output_schema:   (Optional) Schema of the output Value if different from the input.
    :return:                New instance of the Operator class.
    """

    class AnonymousOperator(Operator):
        def __init__(self, circuit_: 'Circuit', element_id: Circuit.Element.ID):
            Operator.__init__(self, circuit_, element_id, create_operation(schema, operation, output_schema))

    return circuit.create_element(AnonymousOperator)

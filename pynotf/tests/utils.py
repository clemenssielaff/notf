from typing import List, Optional, Union, Callable
from random import randint as random_int, choice as random_choice

from pynotf.logic import Operator, Receiver, Emitter, Circuit, ValueSignal, FailureSignal, CompletionSignal
from pynotf.value import Value

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


class Recorder(Receiver):
    """
    A Recorder is a special Receiver for test cases that simply records all signals that it receives.
    """

    def __init__(self, circuit: Circuit, schema: Value.Schema):
        Receiver.__init__(self, circuit, schema)
        self.signals: List[Union[ValueSignal, FailureSignal, CompletionSignal]] = []

    @classmethod
    def record(cls, emitter: Emitter, circuit: Optional[Circuit] = None) -> 'Recorder':
        """
        Convenience method to create and connect a matching Recorder to a given Emitter.
        If the Emitter is an Operator, we will get the "circuit" argument from it.
        If it is a pure Emitter you have to specify the Circuit as well.
        """
        # find the circuit
        if isinstance(emitter, Operator):
            circuit = emitter._circuit
        else:
            assert circuit is not None

        # create and connect the recorder
        recorder = Recorder(circuit, emitter.get_output_schema())
        recorder.connect_to(Emitter.Handle(emitter))
        return recorder

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


def create_emitter(schema: Value.Schema, is_blockable: bool = False,
                   handle_error: Optional[Callable] = None,
                   sort_receivers: Optional[Callable] = None):
    """
    Creates an instance of an anonymous Emitter class that can emit values of the given schema.
    :param schema:          Schema of the Value emitted by this Emitter, defaults to empty.
    :param is_blockable:    Whether or not Signals from this Emitter can be blocked, defaults to False.
    :param handle_error:    (Optional) Callback for the `_handle_error` method.
    :param sort_receivers:  (Optional) Callback for the `_sort_receivers` method.
    :return: New instance of the Emitter class.
    """

    class AnonymousEmitter(Emitter):
        def __init__(self):
            Emitter.__init__(self, schema, is_blockable)

        def _handle_error(self, receiver_id: int, error: Exception):
            if handle_error:
                handle_error(receiver_id, error)

        def _sort_receivers(self, receivers: Emitter._OrderableReceivers):
            if sort_receivers:
                sort_receivers(receivers)

    return AnonymousEmitter()


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
        def __init__(self):
            Receiver.__init__(self, circuit, schema)

        def on_value(self, signal: ValueSignal):
            if on_value:
                on_value(signal)

        def on_failure(self, signal: FailureSignal):
            if on_failure:
                on_failure(signal)

        def on_completion(self, signal: CompletionSignal):
            if on_completion:
                on_completion(signal)

    return AnonymousReceiver()


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

    class AnonymousOperation(Operator.Operation):
        def get_input_schema(self) -> Value.Schema:
            return schema

        def get_output_schema(self) -> Value.Schema:
            return schema if output_schema is None else output_schema

        def __call__(self, value: Value) -> Optional[Value]:
            return operation(value)

    class AnonymousOperator(Operator):
        def __init__(self):
            Operator.__init__(self, circuit, AnonymousOperation())

    return AnonymousOperator()

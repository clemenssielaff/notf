from typing import List, ClassVar, Optional, Tuple, Union, Callable
import sys

from pynotf.logic import Operator, Receiver, Emitter, Circuit, ValueSignal, FailureSignal, CompletionSignal
from pynotf.value import Value

__all__ = ["AddAnotherReceiverReceiver", "AddConstantOperation", "ClampOperation", "DisconnectReceiver",
           "ErrorOperation", "GroupTwoOperation", "Nope",
           "NumberEmitter", "Recorder", "StringifyOperation", "basic_schemas", "count_live_receivers",
           "create_receiver", "number_schema", "string_schema", "create_emitter"]


# TODO: create random schemas

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

    def on_value(self, signal: ValueSignal):
        self.signals.append(signal)

    def on_failure(self, signal: FailureSignal):
        self.signals.append(signal)

    def on_completion(self, signal: CompletionSignal):
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

    def get_errors(self) -> List[Exception]:
        """
        All errors that the Recorder received so far.
        """
        result: List[Exception] = []
        for signal in self.signals:
            if isinstance(signal, FailureSignal):
                result.append(signal.get_error())
        return result

    def get_completions(self) -> List[int]:
        """
        All completion that the Recorder received so far.
        """
        result: List[int] = []
        for signal in self.signals:
            if isinstance(signal, CompletionSignal):
                result.append(signal.get_source())
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


#####################################################################################################

number_schema: Value.Schema = Value(0).schema
string_schema: Value.Schema = Value("").schema

basic_schemas = (number_schema, string_schema)
"""
The basic Value Schemas.
"""


class Nope:
    """
    A useless class that is never the correct type.
    """
    pass


def count_live_receivers(emitter: Emitter) -> int:
    return len([rec for rec in emitter._downstream if rec() is not None])


class NumberEmitter(Emitter):
    def __init__(self):
        Emitter.__init__(self, Value(0).schema)
        self.errors: List[Tuple[int, Exception]] = []

    def _handle_error(self, receiver_id: int, error: Exception):
        Emitter._handle_error(self, receiver_id, error)  # for coverage
        self.errors.append((receiver_id, error))


class AddAnotherReceiverReceiver(Receiver):
    """
    Whenever one of the three methods is called, this Receiver creates a new (Noop) Receiver and connects it to
    the given Emitter.
    """

    def __init__(self, circuit: Circuit, emitter: Emitter, modulus: int = sys.maxsize,
                 schema: Value.Schema = Value(0).schema):
        """
        :param circuit: Circuit containing the Receiver.
        :param emitter: Emitter to connect new Receivers to.
        :param modulus: With n = number of Receivers, every time n % modulus = 0, delete all Receivers.
        :param schema:  Schema of the Receiver.
        """
        super().__init__(circuit, schema)
        self._emitter: Emitter = emitter
        self._modulus: int = modulus
        self._receivers: List[NoopReceiver] = []  # to keep them alive

    def _add_another(self):
        if (len(self._receivers) + 1) % self._modulus == 0:
            self._receivers.clear()
        else:
            receiver: NoopReceiver = NoopReceiver(self._circuit, self.get_input_schema())
            self._receivers.append(receiver)
            receiver.connect_to(Emitter.Handle(self._emitter))

    def on_value(self, signal: ValueSignal):
        self._add_another()

    def on_failure(self, signal: FailureSignal):
        self._add_another()

    def on_completion(self, signal: CompletionSignal):
        self._add_another()


class DisconnectReceiver(Receiver):
    """
    A Receiver that disconnects whenever it gets a value.
    """

    def __init__(self, circuit: Circuit, emitter: Emitter, schema: Value.Schema = Value(0).schema):
        """
        :param emitter: Emitter to connect to.
        :param schema: Schema of the Receiver.
        """
        super().__init__(circuit, schema)
        self.emitter: Emitter = emitter
        self.connect_to(Emitter.Handle(self.emitter))

    def on_value(self, signal: ValueSignal):
        self.disconnect_from(self.emitter.get_id())


class AddConstantOperation(Operator.Operation):
    def __init__(self, addition: float):
        self._constant: float = addition

    def get_input_schema(self) -> Value.Schema:
        return number_schema

    def get_output_schema(self) -> Value.Schema:
        return number_schema

    def _perform(self, value: Value) -> Value:
        return value.modified().set(value.as_number() + self._constant)


class GroupTwoOperation(Operator.Operation):
    _output_prototype: ClassVar[Value] = Value({"x": 0, "y": 0})

    def __init__(self):
        self._last_value: Optional[float] = None

    def get_input_schema(self) -> Value.Schema:
        return number_schema

    def get_output_schema(self) -> Value.Schema:
        return self._output_prototype.schema

    def _perform(self, value: Value) -> Optional[Value]:
        if self._last_value is None:
            self._last_value = value.as_number()
        else:
            result = self._output_prototype.modified().set({"x": self._last_value, "y": value.as_number()})
            self._last_value = None
            return result


class ErrorOperation(Operator.Operation):
    """
    An Operation that raises a ValueError if a certain number is passed.
    """

    def __init__(self, err_on_number: float):
        self._err_on_number: float = err_on_number

    def get_input_schema(self) -> Value.Schema:
        return number_schema

    def get_output_schema(self) -> Value.Schema:
        return number_schema

    def _perform(self, value: Value) -> Optional[Value]:
        if value.as_number() == self._err_on_number:
            raise ValueError("The error condition has occurred")
        return value


class ClampOperation(Operator.Operation):
    """
    Clamps a numeric Value to a certain range.
    """

    def __init__(self, min_value: float, max_value: float):
        self._min: float = min_value
        self._max: float = max_value

    def get_input_schema(self) -> Value.Schema:
        return number_schema

    def get_output_schema(self) -> Value.Schema:
        return number_schema

    def _perform(self, value: Value) -> Value:
        return value.modified().set(max(self._min, min(self._max, value.as_number())))


class StringifyOperation(Operator.Operation):
    """
    Converts a numeric Value into a string representation.
    """

    def get_input_schema(self) -> Value.Schema:
        return number_schema

    def get_output_schema(self) -> Value.Schema:
        return string_schema

    def _perform(self, value: Value) -> Value:
        return Value(str(value.as_number()))

# class EmptyFact(Fact):
#     def __init__(self, executor: Executor):
#         Fact.__init__(self, executor)
#
#
# class NumberFact(Fact):
#     def __init__(self, executor: Executor):
#         Fact.__init__(self, executor, number_schema)

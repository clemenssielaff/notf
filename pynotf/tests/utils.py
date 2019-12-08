from typing import List, ClassVar, Optional, Tuple, Union
import sys

from pynotf.logic import Operator, Receiver, Emitter, Circuit, ValueSignal, FailureSignal, CompletionSignal
from pynotf.value import Value

__all__ = ["AddAnotherReceiverReceiver", "AddConstantOperation", "ClampOperation", "DisconnectReceiver",
           "ErrorOperation", "ExceptionOnCompleteReceiver", "GroupTwoOperation", "NoopReceiver", "Nope",
           "NumberEmitter", "Recorder", "StringifyOperation", "basic_schemas", "count_live_receivers", "create_emitter",
           "create_receiver", "number_schema", "record", "string_schema"]

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


def create_emitter(schema: Value.Schema):
    """
    Creates an instance of an anonymous Emitter class that can emit values of the given schema.
    :param schema: Schema of the values to emit.
    :return: New instance of the Emitter class.
    """

    class AnonymousEmitter(Emitter):
        def __init__(self):
            Emitter.__init__(self, schema)

    return AnonymousEmitter()


def create_receiver(schema: Value.Schema):
    """
    Creates an instance of an anonymous Receiver class that can receive values of the given schema.
    :param schema: Schema of the values to receive.
    :return: New instance of the Receiver class.
    """

    class AnonymousReceiver(Receiver):
        def __init__(self):
            Receiver.__init__(self, schema)
            self.callback = lambda signal: None

        def on_value(self, *args, **kwargs):
            self.callback(*args, **kwargs)

    return AnonymousReceiver()


def count_live_receivers(emitter: Emitter) -> int:
    return len([rec for rec in emitter._downstream if rec() is not None])


class NumberEmitter(Emitter):
    def __init__(self):
        Emitter.__init__(self, Value(0).schema)
        self.errors: List[Tuple[int, Exception]] = []

    def _handle_error(self, receiver_id: int, error: Exception):
        Emitter._handle_error(self, receiver_id, error)  # for coverage
        self.errors.append((receiver_id, error))


class NoopReceiver(Receiver):
    def __init__(self, circuit: Circuit, schema: Value.Schema = Value(0).schema):
        super().__init__(circuit, schema)

    def on_value(self, signal: ValueSignal):
        pass

    def on_failure(self, signal: FailureSignal):
        pass

    def on_completion(self, signal: CompletionSignal):
        pass


class ExceptionOnCompleteReceiver(Receiver):
    def __init__(self, circuit: Circuit, schema: Value.Schema = Value(0).schema):
        super().__init__(circuit, schema)

    def on_value(self, signal: ValueSignal):
        pass

    def on_failure(self, signal: FailureSignal):
        raise RuntimeError("Raising an Exception during on_failure")

    def on_completion(self, signal: CompletionSignal):
        raise RuntimeError("Raising an Exception during on_complete")


class Recorder(Receiver):
    def __init__(self, schema: Value.Schema):
        super().__init__(schema)
        self.values: List[Value] = []
        self.signals: List[Union[ValueSignal, FailureSignal, CompletionSignal]] = []
        self.completed: List[int] = []
        self.errors: List[Exception] = []

    def on_value(self, signal: ValueSignal):
        self.values.append(signal.get_value())
        self.signals.append(signal)

    def on_failure(self, signal: FailureSignal):
        Receiver.on_failure(self, signal)  # just for coverage
        self.errors.append(signal.get_error())

    def on_completion(self, signal: CompletionSignal):
        Receiver.on_completion(self, signal)  # just for coverage
        self.completed.append(signal.get_source())


def record(emitter: Emitter) -> Recorder:
    recorder = Recorder(emitter.get_output_schema())
    recorder.connect_to(Emitter.Handle(emitter))
    return recorder


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

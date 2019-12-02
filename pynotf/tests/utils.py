from typing import List, ClassVar, Optional, Tuple
import sys

from pynotf.logic import Switch, Receiver, Emitter, Signal, Circuit
from pynotf.value import Value

# from pynotf.scene import Fact

number_schema: Value.Schema = Value(0).schema
string_schema: Value.Schema = Value("").schema
list_schema: Value.Schema = Value([]).schema
dict_schema: Value.Schema = Value({}).schema

basic_schemas = (number_schema, string_schema, list_schema, dict_schema)
"""
The four basic Value Schemas.
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
    return len([rec for rec in emitter._receivers if rec() is not None])


class NumberEmitter(Emitter):
    def __init__(self):
        Emitter.__init__(self, Value(0).schema)
        self.exceptions: List[Tuple[Receiver, Exception]] = []

    def _handle_exception(self, receiver: 'Receiver', exception: Exception):
        Emitter._handle_exception(self, receiver, exception)  # for coverage
        self.exceptions.append((receiver, exception))


class NoopReceiver(Receiver):
    def __init__(self, schema: Value.Schema = Value(0).schema):
        super().__init__(schema)

    def on_value(self, signal: Emitter.Signal, value: Value):
        pass

    def on_error(self, signal: Emitter.Signal, exception: Exception):
        pass

    def on_complete(self, signal: Emitter.Signal):
        pass


class ExceptionOnCompleteReceiver(Receiver):
    def __init__(self, schema: Value.Schema = Value(0).schema):
        super().__init__(schema)

    def on_value(self, signal: Emitter.Signal, value: Value):
        pass

    def on_error(self, signal: Emitter.Signal, exception: Exception):
        raise RuntimeError("I also want to err")

    def on_complete(self, signal: Emitter.Signal):
        raise RuntimeError("I am now also complete")


class Recorder(Receiver):
    def __init__(self, schema: Value.Schema):
        super().__init__(schema)
        self.values: List[Value] = []
        self.signals: List[Emitter.Signal] = []
        self.completed: List[int] = []
        self.errors: List[Exception] = []

    def on_value(self, signal: Emitter.Signal, value: Value):
        self.values.append(value)
        self.signals.append(signal)

    def on_error(self, signal: Emitter.Signal, exception: Exception):
        Receiver.on_error(self, signal, exception)  # just for coverage
        self.errors.append(exception)

    def on_complete(self, signal: Emitter.Signal):
        Receiver.on_complete(self, signal)  # just for coverage
        self.completed.append(signal.source)


def record(emitter: [Emitter, Switch]) -> Recorder:
    recorder = Recorder(emitter.output_schema)
    recorder.connect_to(emitter)
    return recorder


class AddAnotherReceiverReceiver(Receiver):
    """
    Whenever one of the three methods is called, this Receiver creates a new (Noop) Receiver and connects it to
    the given Emitter.
    """

    def __init__(self, emitter: Emitter, modulus: int = sys.maxsize, schema: Value.Schema = Value(0).schema):
        """
        :param emitter: Emitter to connect new Receivers to.
        :param modulus: With n = number of Receivers, every time n % modulus = 0, delete all Receivers.
        :param schema: Schema of the Receiver.
        """
        super().__init__(schema)
        self._emitter: Emitter = emitter
        self._modulus: int = modulus
        self._receivers: List[NoopReceiver] = []  # to keep them alive

    def _add_another(self):
        if (len(self._receivers) + 1) % self._modulus == 0:
            self._receivers.clear()
        else:
            receiver: NoopReceiver = NoopReceiver(self.input_schema)
            self._receivers.append(receiver)
            receiver.connect_to(self._emitter)

    def on_value(self, signal: Emitter.Signal, value: Value):
        self._add_another()

    def on_error(self, signal: Emitter.Signal, exception: Exception):
        self._add_another()

    def on_complete(self, signal: Emitter.Signal):
        self._add_another()


class DisconnectReceiver(Receiver):
    """
    A Receiver that disconnects whenever it gets a value.
    """

    def __init__(self, emitter: Emitter, schema: Value.Schema = Value(0).schema):
        """
        :param emitter: Emitter to connect to.
        :param schema: Schema of the Receiver.
        """
        super().__init__(schema)
        self.emitter: Emitter = emitter
        self.connect_to(self.emitter)

    def on_value(self, signal: Emitter.Signal, value: Value):
        self.disconnect_from(self.emitter)


class AddConstantOperation(Switch.Operation):
    _schema: ClassVar[Value.Schema] = Value(0).schema

    def __init__(self, addition: float):
        self._constant = addition

    @property
    def input_schema(self) -> Value.Schema:
        return self._schema

    @property
    def output_schema(self) -> Value.Schema:
        return self._schema

    def _perform(self, value: Value) -> Value:
        return value.modified().set(value.as_number() + self._constant)


class GroupTwoOperation(Switch.Operation):
    _input_schema: ClassVar[Value.Schema] = Value(0).schema
    _output_prototype: ClassVar[Value] = Value({"x": 0, "y": 0})

    def __init__(self):
        self._last_value: Optional[float] = None

    @property
    def input_schema(self) -> Value.Schema:
        return self._input_schema

    @property
    def output_schema(self) -> Value.Schema:
        return self._output_prototype.schema

    def _perform(self, value: Value) -> Optional[Value]:
        if self._last_value is None:
            self._last_value = value.as_number()
        else:
            result = self._output_prototype.modified().set({"x": self._last_value, "y": value.as_number()})
            self._last_value = None
            return result


class ErrorOperation(Switch.Operation):
    """
    An Operation that raises a ValueError if a certain number is passed.
    """
    _schema: ClassVar[Value.Schema] = Value(0).schema

    def __init__(self, err_on_number: float):
        self._err_on_number: float = err_on_number

    @property
    def input_schema(self) -> Value.Schema:
        return self._schema

    @property
    def output_schema(self) -> Value.Schema:
        return self._schema

    def _perform(self, value: Value) -> Optional[Value]:
        if value.as_number() == self._err_on_number:
            raise ValueError("The error condition has occurred")
        return value


class ClampOperation(Switch.Operation):
    """
    Clamps a numeric Value to a certain range.
    """
    _schema: ClassVar[Value.Schema] = Value(0).schema

    def __init__(self, min_value: float, max_value: float):
        self._min: float = min_value
        self._max: float = max_value

    @property
    def input_schema(self) -> Value.Schema:
        return self._schema

    @property
    def output_schema(self) -> Value.Schema:
        return self._schema

    def _perform(self, value: Value) -> Value:
        return value.modified().set(max(self._min, min(self._max, value.as_number())))


class StringifyOperation(Switch.Operation):
    """
    Converts a numeric Value into a string representation.
    """

    @property
    def input_schema(self) -> Value.Schema:
        return Value(0).schema

    @property
    def output_schema(self) -> Value.Schema:
        return Value("").schema

    def _perform(self, value: Value) -> Value:
        return Value(str(value.as_number()))

# class EmptyFact(Fact):
#     def __init__(self, executor: Executor):
#         Fact.__init__(self, executor)
#
#
# class NumberFact(Fact):
#     def __init__(self, executor: Executor):
#         Fact.__init__(self, executor, Value(0).schema)

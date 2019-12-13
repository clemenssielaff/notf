import unittest
import logging
from typing import List, Optional, ClassVar
from random import randint as random_int

from pynotf.logic import Receiver, Emitter, Circuit, FailureSignal, CompletionSignal, ValueSignal, Operator, \
    CyclicDependencyError
from pynotf.value import Value

from tests.utils import number_schema, string_schema, Recorder, random_schema, create_emitter, create_receiver, \
    create_operator


########################################################################################################################
# TEST CASE
########################################################################################################################

class BaseTestCase(unittest.TestCase):
    """
    Base class for all test cases in this module.
    Contains a single Circuit and convenience functions to drive it.
    """

    def setUp(self):
        logging.disable(logging.CRITICAL)
        self.circuit: Circuit = Circuit()

    def tearDown(self):
        logging.disable(logging.NOTSET)

    def _handle_events(self) -> int:
        """
        Handles all Events that are queued in the Circuit, including those that might be generated as a result of the
        handling of earlier ones.
        :return: Number of handled events
        """
        event = self.circuit.await_event(timeout=0)
        counter: int = 0
        while event is not None:
            counter += 1
            self.circuit.handle_event(event)
            event = self.circuit.await_event(timeout=0)
        return counter

    def _apply_topology_changes(self) -> int:
        """
        Perform topology changes in the Circuit without consuming an event.
        :return: Number of topology changes.
        """
        count: int = len(self.circuit._topology_changes)
        if count > 0:
            self.circuit.handle_event(Circuit._NoopEvent())
        return count


class SimpleTestCase(BaseTestCase):
    """
    All test cases that just need a simple Circuit as setup.
    """

    def test_receiver_schema(self):
        """
        Tests whether the input Schema of an Receiver is set correctly
        """
        for schema in (random_schema() for _ in range(4)):
            receiver: Receiver = create_receiver(self.circuit, schema)
            self.assertEqual(receiver.get_input_schema(), schema)

    def test_emitter_schema(self):
        """
        Tests whether the output Schema of an Emitter is set correctly
        """
        for schema in (random_schema() for _ in range(4)):
            emitter: Emitter = Emitter(schema)
            self.assertEqual(emitter.get_output_schema(), schema)

    def test_emitter_handle(self):
        """
        Check if an Emitter handle behaves correctly.
        """
        emitter_handle: Optional[Emitter.Handle] = None
        for schema in (random_schema() for _ in range(4)):
            emitter: Emitter = Emitter(schema)
            emitter_handle = Emitter.Handle(emitter)
            self.assertTrue(emitter_handle.is_valid())
            self.assertEqual(emitter_handle.get_output_schema(), schema)
        del emitter
        self.assertIsNotNone(emitter_handle)
        self.assertFalse(emitter_handle.is_valid())
        self.assertIsNone(emitter_handle.get_output_schema())

    def test_emit_wrong_values(self):
        """
        Make sure you cannot emit anything that is not the correct Value type.
        """
        emitter = Emitter(number_schema)
        with self.assertRaises(TypeError):
            self.circuit.emit_value(emitter, None)
        with self.assertRaises(TypeError):
            self.circuit.emit_value(emitter, "")
        with self.assertRaises(TypeError):
            self.circuit.emit_value(emitter, Value([1, ]))
        with self.assertRaises(TypeError):
            self.circuit.emit_value(emitter, emitter)

    def test_connect_to_emitter_with_wrong_schema(self):
        """
        Tests that an appropriate exception is thrown should you try to connect a Receiver to an Emitter with an
        incompatible Value schema.
        """
        emitter = Emitter(number_schema)
        receiver = Receiver(self.circuit, string_schema)
        with self.assertRaises(TypeError):
            receiver.connect_to(Emitter.Handle(emitter))

    def test_connecting_to_completed_emitter(self):
        """
        Checks that connecting to a completed Emitter triggers a single completion Event only for the Receiver.
        """
        # set up an emitter and connect a recorder
        emitter = Emitter(number_schema)
        recorder1 = Recorder.record(emitter, self.circuit)
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertEqual(len(recorder1.signals), 0)

        # complete the emitter
        self.circuit.emit_completion(emitter)
        self.assertEqual(self._handle_events(), 1)

        # make sure that the completion was correctly propagated to existing receivers
        completions = recorder1.get_completions()
        self.assertEqual(len(recorder1.signals), 1)
        self.assertEqual(len(completions), 1)
        self.assertEqual(completions[0].get_source(), emitter.get_id())
        self.assertFalse(recorder1.has_upstream())

        # connect a new receiver
        recorder2 = Recorder.record(emitter, self.circuit)
        self.assertEqual(self._apply_topology_changes(), 1)  # to complete the connection
        self.assertEqual(self._handle_events(), 1)  # to handle the immediate completion call

        # make sure that the new receiver gets the same completion signal as the old one
        self.assertEqual(len(recorder2.signals), 1)
        self.assertEqual([signal.get_source() for signal in completions],
                         [signal.get_source() for signal in recorder2.get_completions()])
        self.assertFalse(recorder2.has_upstream())

        # also check that the old receiver did not receive another completion signal
        self.assertEqual(completions, recorder1.get_completions())

    def test_double_connection(self):
        """
        Create two connections between the same Emitter/Receiver pair.
        """
        # create an emitter without connecting any receivers
        emitter: Emitter = Emitter(number_schema)
        self.assertFalse(emitter.has_downstream())

        # connect a receiver
        recorder: Recorder = Recorder(self.circuit, number_schema)
        self.assertFalse(recorder.has_upstream())
        recorder.connect_to(Emitter.Handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertTrue(emitter.has_downstream())
        self.assertTrue(recorder.has_upstream())
        self.assertEqual(len(emitter._downstream), 1)
        self.assertEqual(len(recorder._upstream), 1)

        # a second connection between the emitter and receiver is silently ignored
        recorder.connect_to(Emitter.Handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertEqual(len(emitter._downstream), 1)
        self.assertEqual(len(recorder._upstream), 1)

        # just to make sure, emit a single value and see how many end up at the receiver
        self.circuit.emit_value(emitter, 46)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual([value.as_number() for value in recorder.get_values()], [46])

    def test_signal_source(self):
        """
        Checks that a Receiver is able to identify which upstream Emitter a Signal originated from.
        """
        emitter1: Emitter = Emitter(number_schema)
        emitter2: Emitter = Emitter(number_schema)
        recorder: Recorder = Recorder(self.circuit, number_schema)

        # connect the recorder to the two emitters and check that nothing else has happened
        recorder.connect_to(Emitter.Handle(emitter1))
        recorder.connect_to(Emitter.Handle(emitter2))
        self.assertEqual(self._apply_topology_changes(), 2)
        self.assertEqual(len(recorder.signals), 0)

        # emit a few values
        self.circuit.emit_value(emitter1, 0)
        self.circuit.emit_value(emitter2, 1)
        self.circuit.emit_value(emitter1, 2)
        self.circuit.emit_value(emitter1, 3)
        self.circuit.emit_value(emitter2, 4)
        self.assertEqual(self._handle_events(), 5)

        # make sure all signals were received and we know where they come from
        self.assertEqual([value.as_number() for value in recorder.get_values()], [0, 1, 2, 3, 4])
        e1: int = emitter1.get_id()
        e2: int = emitter2.get_id()
        self.assertEqual([signal.get_source() for signal in recorder.signals if isinstance(signal, ValueSignal)],
                         [e1, e2, e1, e1, e2])

    def test_exception_on_value(self):
        """
        Tests what happens if a Receiver throws an exception while processing a ValueSignal.
        """

        # special exception class to make sure that we are catching the right one
        class TestException(Exception):
            def __init__(self, value):
                super().__init__()
                self.value = value

        # callback function for the receiver to call when it receives a FailureSignal
        def throw_exception(signal: ValueSignal):
            raise TestException(signal.get_value())

        # create a custom receiver
        receiver: Receiver = create_receiver(self.circuit, number_schema, on_value=throw_exception)

        error_handler_called: bool = False

        # error handling callback for the emitter
        def error_handling(receiver_id: int, error: Exception):
            nonlocal error_handler_called
            self.assertEqual(receiver_id, receiver.get_id())
            self.assertIsInstance(error, TestException)
            self.assertEqual(error.value.as_number(), 437)
            error_handler_called = True

        # create a custom emitter and connect the receiver to it
        emitter: Emitter = create_emitter(number_schema, handle_error=error_handling)
        receiver.connect_to(Emitter.Handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 1)

        # emit a value
        self.circuit.emit_value(emitter, 437)
        self.assertEqual(self._handle_events(), 1)

        # ake sure that the error handler got called and everything is in order
        self.assertTrue(error_handler_called)

    def test_exception_on_failure(self):
        """
        Tests what happens if a Receiver throws an exception while processing a FailureSignal.
        """

        # special exception class to make sure that we are catching the right one
        class TestException(Exception):
            pass

        # callback function for the receiver to call when it receives a FailureSignal
        def throw_exception(signal: FailureSignal):
            raise signal.get_error()

        # create a custom receiver
        receiver: Receiver = create_receiver(self.circuit, number_schema, on_failure=throw_exception)

        error_handler_called: bool = False

        # error handling callback for the emitter
        def error_handling(receiver_id: int, error: Exception):
            nonlocal error_handler_called
            self.assertEqual(receiver_id, receiver.get_id())
            self.assertIsInstance(error, TestException)
            error_handler_called = True

        # create a custom emitter and connect the receiver to it
        emitter: Emitter = create_emitter(number_schema, handle_error=error_handling)
        receiver.connect_to(Emitter.Handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 1)

        # let the emitter fail
        self.circuit.emit_failure(emitter, TestException())
        self.assertEqual(self._handle_events(), 1)

        # ake sure that the error handler got called and everything is in order
        self.assertTrue(error_handler_called)

    def test_exception_on_complete(self):
        """
        Tests what happens if a Receiver throws an exception while processing a CompletionSignal.
        """

        # special exception class to make sure that we are catching the right one
        class TestException(Exception):
            pass

        # callback function for the receiver to call when it receives a FailureSignal
        def throw_exception(_: CompletionSignal):
            raise TestException()

        # create a custom receiver
        receiver: Receiver = create_receiver(self.circuit, number_schema, on_completion=throw_exception)

        error_handler_called: bool = False

        # error handling callback for the emitter
        def error_handling(receiver_id: int, error: Exception):
            nonlocal error_handler_called
            self.assertEqual(receiver_id, receiver.get_id())
            self.assertIsInstance(error, TestException)
            error_handler_called = True

        # create a custom emitter and connect the receiver to it
        emitter: Emitter = create_emitter(number_schema, handle_error=error_handling)
        receiver.connect_to(Emitter.Handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 1)

        # let the emitter fail
        self.circuit.emit_completion(emitter)
        self.assertEqual(self._handle_events(), 1)

        # ake sure that the error handler got called and everything is in order
        self.assertTrue(error_handler_called)

    def test_simple_operator(self):
        """
        0, 1, 2, 3 -> multiply by two -> 0, 2, 4, 6
        """
        # set up the Circuit
        emitter: Emitter = Emitter(number_schema)
        operator: Operator = create_operator(self.circuit, number_schema,
                                             lambda value: value.modified().set(value.as_number() * 2)
                                             )
        operator.connect_to(Emitter.Handle(emitter))
        recorder: Recorder = Recorder.record(operator)
        self.assertEqual(self._apply_topology_changes(), 2)

        # emit 0, 1, 2, 3
        for number in range(4):
            self.circuit.emit_value(emitter, number)
        self.assertEqual(self._handle_events(), 4)

        # check that the operator did what it promised
        self.assertEqual([v.as_number() for v in recorder.get_values()], [0, 2, 4, 6])

    def test_conversion_operator(self):
        """
        0, 1, 2, 3 -> stringify -> "0", "1", "2", "3"
        """
        # set up the Circuit
        emitter: Emitter = Emitter(number_schema)
        operator: Operator = create_operator(self.circuit, number_schema,
                                             lambda value: Value(str(value.as_number())),
                                             output_schema=string_schema)
        operator.connect_to(Emitter.Handle(emitter))
        recorder: Recorder = Recorder.record(operator)
        self.assertEqual(self._apply_topology_changes(), 2)

        # emit 0, 1, 2, 3
        for number in range(4):
            self.circuit.emit_value(emitter, number)
        self.assertEqual(self._handle_events(), 4)

        # check that the operator did what it promised
        self.assertEqual([v.as_string() for v in recorder.get_values()], ["0.0", "1.0", "2.0", "3.0"])

    def test_stateful_operator(self):
        """
        0, 1, 2, 3 -> group two -> (0, 1), (2, 3)
        """

        class GroupTwo(Operator.Operation):
            """
            Groups two subsequent numbers into a pair.
            """
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

        # set up the Circuit
        emitter: Emitter = Emitter(number_schema)
        operator: Operator = Operator(self.circuit, GroupTwo())
        operator.connect_to(Emitter.Handle(emitter))
        recorder: Recorder = Recorder.record(operator)
        self.assertEqual(self._apply_topology_changes(), 2)

        # emit 0, 1, 2, 3
        for number in range(4):
            self.circuit.emit_value(emitter, number)
        self.assertEqual(self._handle_events(), 4)

        # check that the operator did what it promised
        self.assertEqual(recorder.get_values(), [Value({"x": 0, "y": 1}), Value({"x": 2, "y": 3})])

    def test_failing_operation(self):
        """
        Tests an Operation that will fail given a certain input.
        """

        def raise_on(trigger: int):
            """
            Function factory that produces a function that fails if a certain input is given.
            :param trigger: Number at which the exception is thrown.
            """

            def anonymous_func(value: Value) -> Value:
                if value.as_number() == trigger:
                    raise RuntimeError()
                return value

            return anonymous_func

        # set up the circuit
        emitter: Emitter = Emitter(number_schema)
        operator: Operator = create_operator(self.circuit, number_schema, raise_on(4))
        operator.connect_to(Emitter.Handle(emitter))
        recorder: Recorder = Recorder.record(operator)
        self.assertEqual(self._apply_topology_changes(), 2)

        # try to emit all numbers [0, 10)
        for number in range(10):
            self.circuit.emit_value(emitter, number)
        self.assertEqual(self._handle_events(), 10)

        # even though we handled 10 events, the operator failed while handling input 4
        self.assertEqual([signal.get_source() for signal in recorder.get_failures()], [operator.get_id()])
        self.assertEqual([value.as_number() for value in recorder.get_values()], [0, 1, 2, 3])

    def test_bad_emitter(self):
        """
        Test for Emitters that try to emit Value types that they cannot.
        """
        emitter: Emitter = Emitter(number_schema)
        recorder: Recorder = Recorder.record(emitter, self.circuit)
        self.assertTrue(self._apply_topology_changes(), 1)

        # emit a value to make sure everything works
        emitter._emit(Value(98))
        self.assertEqual(len(recorder.signals), 1)
        self.assertEqual(recorder.signals[0].get_value().as_number(), 98)

        # emitting anything that can be casted to the right value type works as well
        emitter._emit(9436)
        self.assertEqual(len(recorder.signals), 2)
        self.assertEqual(recorder.signals[1].get_value().as_number(), 9436)

        # emitting wrong value types will throw an exception
        with self.assertRaises(TypeError):
            emitter._emit(Value("nope"))
        self.assertEqual(len(recorder.signals), 2)

        # same goes for anything that cannot be cast to the right value type
        with self.assertRaises(TypeError):
            emitter._emit(None)
        with self.assertRaises(TypeError):
            emitter._emit(TypeError())
        with self.assertRaises(TypeError):
            emitter._emit("string")
        with self.assertRaises(TypeError):
            emitter._emit("0.1")
        self.assertEqual(len(recorder.signals), 2)

    def test_bad_operator(self):
        """
        Test for Operators that don't deliver the Value types that they promise.
        """
        # set up the circuit
        emitter: Emitter = Emitter(number_schema)
        operator: Operator = create_operator(self.circuit, number_schema, lambda value: value, string_schema)
        operator.connect_to(Emitter.Handle(emitter))
        recorder: Recorder = Recorder.record(operator)
        self.assertEqual(self._apply_topology_changes(), 2)

        # the operator should fail during emission
        self.circuit.emit_value(emitter, 234)
        self.assertEqual(self._handle_events(), 1)
        self.assertTrue(operator.is_completed())

        # make sure that it has emitted its failure
        self.assertEqual([signal.get_source() for signal in recorder.get_failures()], [operator.get_id()])

    def test_expired_receiver_on_value(self):
        """
        While it is not possible to remove Receivers mid-event, they are removed at some point (during the event
        epilogue) and will not notify their upstream because it will either be destroyed with them, or notice that
        they are gone the next time any upstream Emitter tries to emit.
        """
        emitter: Emitter = Emitter(number_schema)
        rec1 = Recorder.record(emitter, self.circuit)
        rec2 = Recorder.record(emitter, self.circuit)
        rec3 = Recorder.record(emitter, self.circuit)
        rec4 = Recorder.record(emitter, self.circuit)
        rec5 = Recorder.record(emitter, self.circuit)
        self.assertEqual(self._apply_topology_changes(), 5)
        self.assertEqual(len(emitter._downstream), 5)

        self.circuit.emit_value(emitter, 23)
        self.assertEqual(self._handle_events(), 1)
        for rec in (rec1, rec2, rec3, rec4, rec5):
            self.assertEqual(len(rec.signals), 1)
            self.assertEqual(rec.get_values()[0].as_number(), 23)
        del rec

        del rec1  # delete first
        self.assertEqual(len(emitter._downstream), 5)  # rec1 has expired but hasn't been removed yet
        self.circuit.emit_value(emitter, 54)  # remove rec1
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(emitter._downstream), 4)

        del rec5  # delete last
        self.assertEqual(len(emitter._downstream), 4)  # rec5 has expired but hasn't been removed yet
        self.circuit.emit_value(emitter, 25)  # remove rec5
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(emitter._downstream), 3)

        del rec3  # delete center
        self.assertEqual(len(emitter._downstream), 3)  # rec3 has expired but hasn't been removed yet
        self.circuit.emit_value(emitter, -2)  # remove rec3
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(emitter._downstream), 2)

        # complete
        self.circuit.emit_completion(emitter)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(emitter._downstream), 0)

    def test_expired_receiver_on_complete(self):
        """
        See test_expired_receiver_on_value
        """
        emitter: Emitter = Emitter(number_schema)
        recorder: Recorder = Recorder(self.circuit, number_schema)
        recorder.connect_to(Emitter.Handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertEqual(len(emitter._downstream), 1)
        del recorder
        emitter._complete()
        self.assertEqual(len(emitter._downstream), 0)

    def test_expired_receiver_on_failure(self):
        """
        See test_expired_receiver_on_value
        """
        emitter: Emitter = Emitter(number_schema)
        recorder: Recorder = Recorder(self.circuit, number_schema)
        recorder.connect_to(Emitter.Handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertEqual(len(emitter._downstream), 1)
        del recorder
        emitter._fail(ValueError())
        self.assertEqual(len(emitter._downstream), 0)

    def test_signal_status(self):
        """
        Checks that the Signal status mechanism.
        """

        circuit: Circuit = self.circuit

        class Ignore(Recorder):
            """
            Records the value if it has not been accepted yet, but doesn't modify the Signal.
            """

            def __init__(self):
                Recorder.__init__(self, circuit, number_schema)

            def _on_value(self, signal: ValueSignal):
                if signal.is_blockable() and not signal.is_accepted():
                    Recorder._on_value(self, signal)

        class Accept(Recorder):
            """
            Always records the value, and accepts the Signal.
            """

            def __init__(self):
                Recorder.__init__(self, circuit, number_schema)

            def _on_value(self, signal: ValueSignal):
                Recorder._on_value(self, signal)
                if signal.is_blockable():
                    signal.accept()

        class Blocker(Recorder):
            """
            Always records the value, and blocks the Signal.
            """

            def __init__(self):
                Recorder.__init__(self, circuit, number_schema)

            def _on_value(self, signal: ValueSignal):
                Recorder._on_value(self, signal)
                signal.block()
                signal.block()  # again ... for coverage

        distributor: Emitter = Emitter(number_schema, is_blockable=True)
        ignore1: Ignore = Ignore()  # records all values
        accept1: Accept = Accept()  # records and accepts all values
        ignore2: Ignore = Ignore()  # should not record any since all are accepted
        accept2: Accept = Accept()  # should record the same as accept1
        block1: Blocker = Blocker()  # should record the same as accept1
        block2: Blocker = Blocker()  # should not record any values

        # order matters here as the Receivers are called in the order they connected
        distributor_handle: Emitter.Handle = Emitter.Handle(distributor)
        ignore1.connect_to(distributor_handle)
        accept1.connect_to(distributor_handle)
        ignore2.connect_to(distributor_handle)
        accept2.connect_to(distributor_handle)
        block1.connect_to(distributor_handle)
        block2.connect_to(distributor_handle)
        self.assertEqual(self._apply_topology_changes(), 6)

        for x in range(1, 5):
            self.circuit.emit_value(distributor, x)
        self.assertEqual(self._handle_events(), 4)

        self.assertEqual([value.as_number() for value in ignore1.get_values()], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in accept1.get_values()], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in ignore2.get_values()], [])
        self.assertEqual([value.as_number() for value in accept2.get_values()], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in block1.get_values()], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in block2.get_values()], [])

    def test_early_cyclic_dependency(self):
        """
        +------------------------------------------+
        |                    +--> Recorder1        |
        |                    |                     |
        +--> Noop1(Number) --+--> Noop2(Number) -->+
                             |
                             +--> Recorder2
                             |
                             +--> Noop3(Number) --> Recorder3
        """
        error_producer: Optional[int] = None

        def report_error(receiver_id: int, error: Exception):
            nonlocal error_producer
            self.assertIsInstance(error, CyclicDependencyError)
            error_producer = receiver_id

        # create the Circuit
        noop1: Operator = create_operator(self.circuit, number_schema, lambda value: value)
        recorder1: Recorder = Recorder.record(noop1)
        noop2: Operator = create_operator(self.circuit, number_schema, lambda value: value)
        noop2._handle_error = report_error  # monkey patch error handling method
        recorder2: Recorder = Recorder.record(noop1)
        noop3: Operator = create_operator(self.circuit, number_schema, lambda value: value)
        recorder3: Recorder = Recorder.record(noop3)
        noop2.connect_to(Emitter.Handle(noop1))
        noop3.connect_to(Emitter.Handle(noop1))
        noop1.connect_to(Emitter.Handle(noop2))
        self.assertEqual(self._apply_topology_changes(), 6)  # so far so good
        self.assertEqual(len(recorder1.signals), 0)
        self.assertEqual(len(recorder2.signals), 0)
        self.assertEqual(len(recorder3.signals), 0)
        self.assertIsNone(error_producer)

        # emit a value
        self.circuit.emit_value(noop1, 666)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(recorder1.signals), 1)  # the event will only be propagated until the cycle is closed
        self.assertEqual(len(recorder2.signals), 1)
        self.assertEqual(len(recorder3.signals), 1)
        self.assertEqual(error_producer, noop1.get_id())


class EmitterRecorderCircuit(BaseTestCase):
    """
                      +--> Recorder1(Number)
                      |
    Emitter(Number) ->+
                      |
                      +--> Recorder2(Number)
    """

    def setUp(self):
        super().setUp()

        # create the circuit
        self.emitter: Emitter = Emitter(number_schema)
        self.recorder1: Recorder = Recorder.record(self.emitter, self.circuit)
        self.recorder2: Recorder = Recorder.record(self.emitter, self.circuit)
        self._apply_topology_changes()

    def test_emitter_emission(self):
        """
        Emit a few number values and make sure that they are received.
        """
        # create a random list of random numbers
        numbers: List[int] = []
        for i in range(random_int(0, 8)):
            numbers.append(random_int(0, 100))

        # emit all numbers in order (generating events)
        for number in numbers:
            self.circuit.emit_value(self.emitter, number)
        self.assertEqual(self._handle_events(), len(numbers))

        # make sure that all values were received in order
        self.assertEqual([value.as_number() for value in self.recorder1.get_values()], numbers)
        self.assertEqual([value.as_number() for value in self.recorder2.get_values()], numbers)

    def test_emitter_failure(self):
        """
        Ensures that a failed emitter will call "on_failure" on all Receivers and then complete.
        Also that Receivers disconnect immediately.
        """
        # emit a few numbers
        for number in range(3):
            self.circuit.emit_value(self.emitter, number)
            self.assertEqual(self._handle_events(), 1)

        # make sure that all values were received in order
        received_values: List[Value] = self.recorder1.get_values()
        self.assertEqual([value.as_number() for value in received_values], [0, 1, 2])
        self.assertEqual(received_values, self.recorder2.get_values())

        # make sure that the receivers have not received an error yet
        self.assertEqual(len(self.recorder1.get_failures()), 0)
        self.assertEqual(len(self.recorder2.get_failures()), 0)

        # emit failure
        error = RuntimeError()
        self.circuit.emit_failure(self.emitter, error)
        self.assertEqual(self._handle_events(), 1)

        # ensure that the emitter is now completed
        self.assertTrue(self.emitter.is_completed())

        # ensure that the error has been received
        received_failures = self.recorder1.get_failures()
        self.assertEqual(len(received_failures), 1)
        self.assertEqual(received_failures[0].get_error(), error)
        self.assertEqual(received_failures, self.recorder2.get_failures())

        # emitting any further values will do nothing
        self.circuit.emit_value(self.emitter, 3)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(received_values, self.recorder1.get_values())
        self.assertEqual(received_values, self.recorder2.get_values())

        # with completing, the receiver has now disconnected from the emitter and the emitter from everything
        self.assertEqual(len(self.recorder1._upstream), 0)
        self.assertEqual(len(self.recorder2._upstream), 0)
        self.assertFalse(self.emitter.has_downstream())

    def test_emitter_completion(self):
        """
        Ensures that a completed emitter will call "on_completion" on all Receivers and then complete.
        Also that Receivers disconnect immediately.
        """
        # emit a few numbers
        for number in range(3):
            self.circuit.emit_value(self.emitter, number)
            self.assertEqual(self._handle_events(), 1)

        # make sure that all values were received in order
        received_values: List[Value] = self.recorder1.get_values()
        self.assertEqual([value.as_number() for value in received_values], [0, 1, 2])
        self.assertEqual(received_values, self.recorder2.get_values())

        # make sure that the receiver has not received an completions yet
        self.assertEqual(len(self.recorder1.get_completions()), 0)
        self.assertEqual(len(self.recorder2.get_completions()), 0)

        # emit completion
        self.circuit.emit_completion(self.emitter)
        self.assertEqual(self._handle_events(), 1)

        # ensure that the emitter is now completed
        self.assertTrue(self.emitter.is_completed())

        # ensure that the error has been received
        received_completions = self.recorder1.get_completions()
        self.assertEqual(len(received_completions), 1)
        self.assertEqual(received_completions[0].get_source(), self.emitter.get_id())
        self.assertEqual(received_completions, self.recorder2.get_completions())

        # emitting any further values will do nothing
        self.circuit.emit_value(self.emitter, 3)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(received_values, self.recorder1.get_values())
        self.assertEqual(received_values, self.recorder2.get_values())

        # with completing, the receiver has now disconnected from the emitter and the emitter from everything
        self.assertEqual(len(self.recorder1._upstream), 0)
        self.assertEqual(len(self.recorder2._upstream), 0)
        self.assertFalse(self.emitter.has_downstream())

    def test_disconnection(self):
        """
        Tests manual disconnection of Receivers from an upstream Emitter.
        """
        # make sure the recorders are fresh
        self.assertEqual(len(self.recorder1.signals), 0)
        self.assertEqual(len(self.recorder2.signals), 0)

        # emit a single value
        self.circuit.emit_value(self.emitter, 0)
        self.assertEqual(self._handle_events(), 1)

        # make sure that the value was received by all
        self.assertEqual([value.as_number() for value in self.recorder1.get_values()], [0])
        self.assertEqual([value.as_number() for value in self.recorder2.get_values()], [0])

        # disconnect a single receiver
        self.recorder2.disconnect_from(self.emitter.get_id())
        self.assertEqual(self._apply_topology_changes(), 1)
        self.circuit.emit_value(self.emitter, 1)
        self.assertEqual(self._handle_events(), 1)

        # make sure that only one receiver got the value
        self.assertEqual([value.as_number() for value in self.recorder1.get_values()], [0, 1])
        self.assertEqual([value.as_number() for value in self.recorder2.get_values()], [0])

        # disconnect the other receiver
        self.recorder1.disconnect_from(self.emitter.get_id())
        self.assertEqual(self._apply_topology_changes(), 1)
        self.circuit.emit_value(self.emitter, 2)
        self.assertEqual(self._handle_events(), 1)

        # this time, nobody should have gotten the value
        self.assertEqual([value.as_number() for value in self.recorder1.get_values()], [0, 1])
        self.assertEqual([value.as_number() for value in self.recorder2.get_values()], [0])


class NamedEmitterCircuit(BaseTestCase):
    """
    NamedEmitter(Number, "Bob") --> ...
    """

    def setUp(self):
        super().setUp()

        # create the circuit
        self.emitter: Emitter = self.circuit.create_emitter(number_schema, name="bob")

    def test_connection_mid_event(self):
        """
        Changes in Circuit topology are performed in the Event epilogue, which means that they might happen after user-
        defined code that was executed later.
        """
        operators: List[Operator] = []  # we need to keep the created receivers alive until the end of the test

        def create_another_operator(_: ValueSignal):
            operator: Operator = create_operator(self.circuit, number_schema, lambda value: value)
            operators.append(operator)
            emitter: Optional[Emitter.Handle] = operator.find_emitter('bob')
            self.assertIsNotNone(emitter)
            operator.connect_to(emitter)

        # create a custom receiver and connect it to the circuit's emitter
        receiver: Receiver = create_receiver(self.circuit, number_schema, on_value=create_another_operator)
        receiver.connect_to(Emitter.Handle(self.emitter))
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertEqual(len([rec for rec in self.emitter._downstream if rec() is not None]), 1)

        # emit a random number of value signals and make sure that there is an equal number of new operators
        event_count: int = random_int(1, 6)
        for number in range(event_count):
            self.circuit.emit_value(self.emitter, number)
        self.assertEqual(self._handle_events(), event_count)
        self.assertEqual(len([rec for rec in self.emitter._downstream if rec() is not None]), event_count + 1)

    def test_disconnection_mid_event(self):
        """
        Changes in Circuit topology are performed in the Event epilogue, which means that they might happen after user-
        defined code that was executed later.
        """

        class DisconnectOnValue(Receiver):
            def __init__(self, circuit: Circuit):
                Receiver.__init__(self, circuit, number_schema)

            def _on_value(self, signal: ValueSignal):  # virtual
                self._circuit.remove_connection(signal.get_source(), self)

        # we need to keep the created receivers alive until the end of the test
        receivers: List[Receiver] = []
        for index in range(3):
            # create receivers and monkey patch their on_value method
            receiver: Receiver = DisconnectOnValue(self.circuit)
            receivers.append(receiver)

            # connect each operator to the emitter
            emitter: Optional[Emitter.Handle] = receiver.find_emitter('bob')
            self.assertIsNotNone(emitter)
            receiver.connect_to(emitter)

        # make sure the emitter sees all of the receivers
        self.assertEqual(self._apply_topology_changes(), len(receivers))
        self.assertEqual(len([rec for rec in self.emitter._downstream if rec() is not None]), len(receivers))

        # emit a value and all operators should disconnect
        self.circuit.emit_value(self.emitter, 0)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len([rec for rec in self.emitter._downstream if rec() is not None]), 0)


if __name__ == '__main__':
    unittest.main()

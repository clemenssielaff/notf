import unittest
import logging
from typing import List, Optional, ClassVar
from random import randint

from pynotf.logic import Receiver, Emitter, Circuit, FailureSignal, CompletionSignal, ValueSignal, Operator
from pynotf.value import Value

from tests.utils import number_schema, string_schema, Recorder, random_schema, create_emitter, create_receiver, \
    create_operator

########################################################################################################################
# TEST CASE
########################################################################################################################

"""
Ownership
* create an emitter whose ownership is passed from one subscriber to another one and make sure it is deleted afterwards
* create an emitter with an outside reference who outlives multiple receiver popping in and out of existence
* create a Receiver and repeatedly connect and disconnect new emitters
* create a chain of emitter - switch - switch - receiver and make sure everything stays alive through the last strong reference only
    - with that setup, create a strong reference to a switch inside the chain and drop the old receiver to see if the rest survives

Signal

* create a receiver that used the signal ID to differentiate between emitters, two signal ids from the same emitter should match
* create an emitter with a list of receivers to play through the signal state:
    - unblockable signal, tries to get blocked or accepted along the way
    - blockable signal, is ignored, then checked, then accepted, then checked, then blocked

Exceptions
* create a receiver that throws an exception and let the emitter handle it
* create a custom exception handler (unsubscribe after 2nd exception) for an emitter class and exchange the default one for that at runtime

Switches and Operations

Circles
(assert that when a switch emits, all user-defined code has finished and that emission does not require access to the switch's state, add the "allow reentrancy" flag)
* create a circuit with an unapproved loop (error)
* create a circuit with an allowed loop
    - loop once
    - loop multiple times
    - be infinite (still an error)

Logic modification
* create a circuit that adds a new connection
    - before the current
    - after the current
* create a circuit that removes an active connection
    - before the current
    - after the current

Ordered Emission
* ensure new receivers are emitted to last (check example that produces 1 or -1)
* create emitter that sorts receivers based on some arbitrary value
"""


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
        for i in range(randint(0, 8)):
            numbers.append(randint(0, 100))

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

        # emitting any further values will result in an error
        self.circuit.emit_value(self.emitter, 3)
        with self.assertRaises(RuntimeError):
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

        # emitting any further values will result in an error
        self.circuit.emit_value(self.emitter, 3)
        with self.assertRaises(RuntimeError):
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

    #
    # class NamedOperatorCircuit(BaseTestCase):
    #     """
    #     Emitter(Number) --> Operator(Number, name="bob")
    #     """
    #
    #     def setUp(self):
    #         super().setUp()
    #
    #         # create the circuit
    #         self.emitter: Emitter = Emitter(number_schema)
    #         self.recorder: Recorder = Recorder(self.circuit, number_schema)
    #         self.named_operator:
    #         self.named_operator: Operator = create_operator(self.circuit, number_schema, )
    #         self.recorder1: Recorder = Recorder.record(self.emitter, self.circuit)
    #         self.recorder2: Recorder = Recorder.record(self.emitter, self.circuit)
    #
    #     def test_connection_mid_event(self):
    #         """
    #         Changes in Circuit topology are performed in the Event epilogue, which means that they might happen after user-
    #         defined code that was executed later.
    #         """
    #
    #
    #         class ConnectAnother(Operator.Operation):
    #             """
    #             Creats a new Operator that connects to the named Emitter
    #             """
    #             _output_prototype: ClassVar[Value] = Value({"x": 0, "y": 0})
    #
    #             def __init__(self):
    #                 self._last_value: Optional[float] = None
    #
    #             def get_input_schema(self) -> Value.Schema:
    #                 return number_schema
    #
    #             def get_output_schema(self) -> Value.Schema:
    #                 return self._output_prototype.schema
    #
    #             def _perform(self, value: Value) -> Optional[Value]:
    #                 if self._last_value is None:
    #                     self._last_value = value.as_number()
    #                 else:
    #                     result = self._output_prototype.modified().set({"x": self._last_value, "y": value.as_number()})
    #                     self._last_value = None
    #                     return result

    # def test_connection_during_emit(self):
    #     class ConnectOther(Recorder):
    #         """
    #         Connects another Recorder when receiving a value.
    #         """
    #
    #         def __init__(self, emitter: Emitter, other: Recorder):
    #             Recorder.__init__(self, Value(0).schema)
    #             self._emitter = emitter
    #             self._other = other
    #
    #         def on_value(self, signal: Emitter.Signal, value: Value):
    #             self._other.connect_to(self._emitter)
    #             Recorder.on_value(self, signal, value)
    #
    #     emt: Emitter = NumberEmitter()
    #     rec2 = Recorder(Value(0).schema)
    #     rec1 = ConnectOther(emt, rec2)
    #     rec1.connect_to(emt)
    #
    #     self.assertEqual(len(rec1.values), 0)
    #     self.assertEqual(len(rec2.values), 0)
    #
    #     self.assertEqual(len(emt._receivers), 1)
    #     emt.emit(45)
    #     self.assertEqual(len(emt._receivers), 2)
    #     self.assertEqual(len(rec1.values), 1)
    #     self.assertEqual(rec1.values[0].as_number(), 45)
    #     self.assertEqual(len(rec2.values), 0)
    #
    #     emt.emit(89)
    #     self.assertEqual(len(rec1.values), 2)
    #     self.assertEqual(rec1.values[1].as_number(), 89)
    #     self.assertEqual(len(rec2.values), 1)
    #     self.assertEqual(rec2.values[0].as_number(), 89)
    #
    def test_disconnection_mid_event(self):
        """
        Changes in Circuit topology are performed in the Event epilogue, which means that they might happen after user-
        defined code that was executed later.
        """
        pass  # TODO

    # def test_disconnect_during_emit(self):
    #     class DisconnectOther(Recorder):
    #         """
    #         Disconnects another Recorder when receiving a value.
    #         """
    #
    #         def __init__(self, emitter: Emitter, other: Recorder):
    #             Recorder.__init__(self, Value(0).schema)
    #             self._emitter = emitter
    #             self._other = other
    #
    #         def on_value(self, signal: Emitter.Signal, value: Value):
    #             self._other.disconnect_from(self._emitter)
    #             Recorder.on_value(self, signal, value)
    #
    #     emt: Emitter = NumberEmitter()
    #     rec2 = Recorder(Value(0).schema)
    #     rec1 = DisconnectOther(emt, rec2)
    #     rec1.connect_to(emt)
    #     rec2.connect_to(emt)
    #
    #     self.assertEqual(len(rec1.values), 0)
    #     self.assertEqual(len(rec2.values), 0)
    #
    #     self.assertEqual(len(emt._receivers), 2)
    #     emt.emit(45)
    #     self.assertEqual(len(emt._receivers), 1)
    #     self.assertEqual(len(rec1.values), 1)
    #     self.assertEqual(rec1.values[0].as_number(), 45)
    #     self.assertEqual(len(rec2.values), 1)
    #     self.assertEqual(rec2.values[0].as_number(), 45)
    #
    #     emt.emit(89)
    #     self.assertEqual(len(rec1.values), 2)
    #     self.assertEqual(rec1.values[1].as_number(), 89)
    #     self.assertEqual(len(rec2.values), 1)

    ######################

    # def test_receiver_lifetime(self):
    #     emitter: Emitter = NumberEmitter()
    #     rec1 = record(emitter)
    #     rec2 = record(emitter)
    #     rec3 = record(emitter)
    #     rec4 = record(emitter)
    #     rec5 = record(emitter)
    #     self.assertEqual(len(emitter._receivers), 5)
    #
    #     emitter.emit(23)
    #     for rec in (rec1, rec2, rec3, rec4, rec5):
    #         self.assertEqual(len(rec.values), 1)
    #         self.assertEqual(rec.values[0].as_number(), 23)
    #     del rec
    #
    #     del rec1  # delete first
    #     self.assertEqual(len(emitter._receivers), 5)  # rec1 has expired but hasn't been removed yet
    #     emitter.emit(54)  # remove rec1
    #     self.assertEqual(len(emitter._receivers), 4)
    #
    #     del rec5  # delete last
    #     self.assertEqual(len(emitter._receivers), 4)  # rec5 has expired but hasn't been removed yet
    #     emitter.emit(25)  # remove rec5
    #     self.assertEqual(len(emitter._receivers), 3)
    #
    #     del rec3  # delete center
    #     self.assertEqual(len(emitter._receivers), 3)  # rec3 has expired but hasn't been removed yet
    #     emitter.emit(-2)  # remove rec3
    #     self.assertEqual(len(emitter._receivers), 2)
    #
    #     emitter._complete()
    #     self.assertEqual(len(emitter._receivers), 0)
    #
    # def test_expired_receiver_on_complete(self):
    #     emitter: Emitter = NumberEmitter()
    #     rec1 = record(emitter)
    #     self.assertEqual(len(emitter._receivers), 1)
    #     del rec1
    #     emitter._complete()
    #     self.assertEqual(len(emitter._receivers), 0)
    #
    # def test_expired_receiver_on_failure(self):
    #     emitter: Emitter = NumberEmitter()
    #     rec1 = record(emitter)
    #     self.assertEqual(len(emitter._receivers), 1)
    #     del rec1
    #     emitter._error(ValueError())
    #     self.assertEqual(len(emitter._receivers), 0)
    #
    # def test_add_receivers(self):
    #     emitter: Emitter = NumberEmitter()
    #     receiver: AddAnotherReceiverReceiver = AddAnotherReceiverReceiver(emitter, modulus=3)
    #
    #     receiver.connect_to(emitter)
    #     self.assertEqual(count_live_receivers(emitter), 1)
    #     emitter.emit(1)
    #     self.assertEqual(count_live_receivers(emitter), 2)
    #     emitter.emit(2)
    #     self.assertEqual(count_live_receivers(emitter), 3)
    #     emitter.emit(3)  # activates modulus
    #     self.assertEqual(count_live_receivers(emitter), 1)
    #
    #     emitter.emit(4)
    #     emitter._complete()  # this too tries to add a Receiver, but is rejected
    #     self.assertEqual(count_live_receivers(emitter), 0)
    #
    # def test_remove_receivers(self):
    #     emitter: Emitter = NumberEmitter()
    #     rec1: Receiver = DisconnectReceiver(emitter)
    #
    #     self.assertEqual(count_live_receivers(emitter), 1)
    #     emitter.emit(0)
    #     self.assertEqual(count_live_receivers(emitter), 0)
    #
    #     rec2: Receiver = DisconnectReceiver(emitter)
    #     rec3: Receiver = DisconnectReceiver(emitter)
    #     self.assertEqual(count_live_receivers(emitter), 2)
    #     emitter.emit(1)
    #     self.assertEqual(count_live_receivers(emitter), 0)
    #

    # def test_signal_status(self):
    #     class Ignore(Recorder):
    #         """
    #         Records the value if it has not been accepted yet, but doesn't modify the Signal.
    #         """
    #
    #         def __init__(self):
    #             Recorder.__init__(self, Value(0).schema)
    #
    #         def on_value(self, signal: Emitter.Signal, value: Value):
    #             if signal.is_blockable() and not signal.is_accepted():
    #                 Recorder.on_value(self, signal, value)
    #
    #     class Accept(Recorder):
    #         """
    #         Always records the value, and accepts the Signal.
    #         """
    #
    #         def __init__(self):
    #             Recorder.__init__(self, Value(0).schema)
    #
    #         def on_value(self, signal: Emitter.Signal, value: Value):
    #             Recorder.on_value(self, signal, value)
    #             if signal.is_blockable():
    #                 signal.accept()
    #
    #     class Blocker(Recorder):
    #         """
    #         Always records the value, and blocks the Signal.
    #         """
    #
    #         def __init__(self):
    #             Recorder.__init__(self, Value(0).schema)
    #
    #         def on_value(self, signal: Emitter.Signal, value: Value):
    #             Recorder.on_value(self, signal, value)
    #             signal.block()
    #             signal.block()  # again ... for coverage
    #
    #     class Distributor(NumberEmitter):
    #         def _emit(self, receivers: List['Receiver'], value: Value):
    #             signal = Emitter.Signal(self, is_blockable=True)
    #             for receiver in receivers:
    #                 receiver.on_value(signal, value)
    #                 if signal.is_blocked():
    #                     return
    #
    #     distributor: Distributor = Distributor()
    #     ignore1: Ignore = Ignore()  # records all values
    #     accept1: Accept = Accept()  # records and accepts all values
    #     ignore2: Ignore = Ignore()  # should not record any since all are accepted
    #     accept2: Accept = Accept()  # should record the same as accept1
    #     block1: Blocker = Blocker()  # should record the same as accept1
    #     block2: Blocker = Blocker()  # should not record any values
    #
    #     # order matters here, as the Receivers are called in the order they connected
    #     ignore1.connect_to(distributor)
    #     accept1.connect_to(distributor)
    #     ignore2.connect_to(distributor)
    #     accept2.connect_to(distributor)
    #     block1.connect_to(distributor)
    #     block2.connect_to(distributor)
    #
    #     for x in range(1, 5):
    #         distributor.emit(x)
    #
    #     self.assertEqual([value.as_number() for value in ignore1.values], [1, 2, 3, 4])
    #     self.assertEqual([value.as_number() for value in accept1.values], [1, 2, 3, 4])
    #     self.assertEqual([value.as_number() for value in ignore2.values], [])
    #     self.assertEqual([value.as_number() for value in accept2.values], [1, 2, 3, 4])
    #     self.assertEqual([value.as_number() for value in block1.values], [1, 2, 3, 4])
    #     self.assertEqual([value.as_number() for value in block2.values], [])
    #


if __name__ == '__main__':
    unittest.main()

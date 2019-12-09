import unittest
import logging
from typing import List
from random import randint

from pynotf.logic import Receiver, Emitter, Circuit, FailureSignal, CompletionSignal, ValueSignal
from pynotf.value import Value

from tests.utils import *

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
* connect multiple receivers with different priorities
* create emitter that sorts receivers based on some arbitrary value

Exceptions
* have a Receiver throw an exception and let it be handled by the Emitter
* change the error handler of an emitter to drop the receiver after the second error
* change the error handler than change it back

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


class SimpleTestCase(BaseTestCase):
    """
    All test cases that just need a simple Circuit as setup.
    """

    def test_receiver_schema(self):
        """
        Tests whether the input Schema of an Receiver is set correctly
        """
        for schema in basic_schemas:
            receiver: Receiver = create_receiver(self.circuit, schema)
            self.assertEqual(receiver.get_input_schema(), schema)

    def test_emitter_schema(self):
        """
        Tests whether the output Schema of an Emitter is set correctly
        """
        for schema in basic_schemas:
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
        self.assertEqual(len(recorder1.signals), 0)

        # complete the emitter
        self.circuit.emit_completion(emitter)
        self.assertEqual(self._handle_events(), 1)

        # make sure that the completion was correctly propagated to existing receivers
        completions = recorder1.get_completions()
        self.assertEqual(len(recorder1.signals), 1)
        self.assertEqual(len(completions), 1)
        self.assertEqual(completions[0], emitter.get_id())
        self.assertFalse(recorder1.has_upstream())

        # connect a new receiver
        recorder2 = Recorder.record(emitter, self.circuit)
        self.assertEqual(self._handle_events(), 1)

        # make sure that the new receiver gets the same completion signal as the old one
        self.assertEqual(len(recorder2.signals), 1)
        self.assertEqual(completions, recorder2.get_completions())
        self.assertFalse(recorder2.has_upstream())

        # also check that the old receiver did not receive another completion signal
        self.assertEqual(completions, recorder1.get_completions())

    def test_double_connection(self):
        # create an emitter without connecting any receivers
        emitter: Emitter = Emitter(number_schema)
        self.assertFalse(emitter.has_downstream())

        # connect a receiver
        recorder: Recorder = Recorder(self.circuit, number_schema)
        self.assertFalse(recorder.has_upstream())
        recorder.connect_to(Emitter.Handle(emitter))
        self.assertEqual(self._handle_events(), 1)
        self.assertTrue(emitter.has_downstream())
        self.assertTrue(recorder.has_upstream())
        self.assertEqual(len(emitter._downstream), 1)
        self.assertEqual(len(recorder._upstream), 1)

        # a second connection between the emitter and receiver is silently ignored
        recorder.connect_to(Emitter.Handle(emitter))
        self.assertEqual(self._handle_events(), 1)
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
        self.assertEqual(self._handle_events(), 2)
        self.assertEqual(len(recorder.signals), 0)

        # emit a few values
        self.circuit.emit_value(emitter1, 0)
        self.circuit.emit_value(emitter2, 1)
        self.circuit.emit_value(emitter1, 2)
        self.circuit.emit_value(emitter1, 3)
        self.circuit.emit_value(emitter2, 4)

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
        self.assertEqual(self._handle_events(), 1)

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
        self.assertEqual(self._handle_events(), 1)

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
        self.assertEqual(self._handle_events(), 1)

        # let the emitter fail
        self.circuit.emit_completion(emitter)
        self.assertEqual(self._handle_events(), 1)

        # ake sure that the error handler got called and everything is in order
        self.assertTrue(error_handler_called)


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
        self.assertEqual(len(self.recorder1.get_errors()), 0)
        self.assertEqual(len(self.recorder2.get_errors()), 0)

        # emit failure
        error = RuntimeError()
        self.circuit.emit_failure(self.emitter, error)
        self.assertEqual(self._handle_events(), 1)

        # ensure that the emitter is now completed
        self.assertTrue(self.emitter.is_completed())

        # ensure that the error has been received
        received_errors = self.recorder1.get_errors()
        self.assertEqual(len(received_errors), 1)
        self.assertEqual(received_errors[0], error)
        self.assertEqual(received_errors, self.recorder2.get_errors())

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
        self.assertEqual(received_completions[0], self.emitter.get_id())
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
        self.circuit.emit_value(self.emitter, 1)
        self.assertEqual(self._handle_events(), 2)

        # make sure that only one receiver got the value
        self.assertEqual([value.as_number() for value in self.recorder1.get_values()], [0, 1])
        self.assertEqual([value.as_number() for value in self.recorder2.get_values()], [0])

        # disconnect the other receiver
        self.recorder2.disconnect_from(self.emitter.get_id())
        self.circuit.emit_value(self.emitter, 2)
        self.assertEqual(self._handle_events(), 2)

        # this time, nobody should have gotten the value
        self.assertEqual([value.as_number() for value in self.recorder1.get_values()], [0, 1])
        self.assertEqual([value.as_number() for value in self.recorder2.get_values()], [0])

    def test_disconnection_mid_event(self):
        """
        Changes in Circuit topology are performed in the Event epilogue, which means that they might happen after user-
        defined code that was executed later.
        """
        pass  # TODO

    ######################

    # def test_simple_switch(self):
    #     """
    #     0, 1, 2, 3 -> 7, 8, 9, 10 -> (7, 8), (9, 10) -> (7, 8), (9, 10)
    #     """
    #     emitter = NumberEmitter()
    #     switch = Operator(AddConstantOperation(7), GroupTwoOperation(),
    #                     Switch.NoOp(GroupTwoOperation().output_schema))
    #     switch.connect_to(emitter)
    #     recorder = record(switch)
    #
    #     for x in range(4):
    #         emitter.emit(Value(x))
    #
    #     expected = [(7, 8), (9, 10)]
    #     for index, value in enumerate(recorder.values):
    #         self.assertEqual(expected[index], (value["x"].as_number(), value["y"].as_number()))
    #
    # def test_switch_wrong_type(self):
    #     switch = Switch(Switch.NoOp(Value(0).schema))
    #     switch.on_value(Emitter.Signal(NumberEmitter()), Value("Not A Number"))
    #
    # def test_switch_error(self):
    #     emitter = NumberEmitter()
    #     switch = Switch(ErrorOperation(4))
    #     switch.connect_to(emitter)
    #     recorder = record(switch)
    #
    #     with self.assertRaises(TypeError):
    #         emitter.emit("Not A Number")
    #
    #     # it's not the ErrorOperation that fails, but trying to emit another value
    #     for x in range(10):
    #         emitter.emit(x)
    #     self.assertTrue(len(emitter.exceptions) > 0)
    #     self.assertTrue(switch.is_completed())
    #
    #     expected = [0, 1, 2, 3]
    #     self.assertEqual(len(recorder.values), len(expected))
    #     for i in range(len(expected)):
    #         self.assertEqual(expected[i], recorder.values[i].as_number())
    #
    # def test_no_empty_switch(self):
    #     with self.assertRaises(ValueError):
    #         _ = Switch()
    #
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
    #
    # def test_removal_during_emit(self):
    #     class RemoveOther(Recorder):
    #         """
    #         Removes another Recorder when receiving a value.
    #         """
    #
    #         def __init__(self, emitter: Emitter):
    #             Recorder.__init__(self, Value(0).schema)
    #             self._emitter = emitter
    #             self._other = Recorder(Value(0).schema)
    #             self._other.connect_to(self._emitter)
    #
    #         def on_value(self, signal: Emitter.Signal, value: Value):
    #             self._other = None
    #             Recorder.on_value(self, signal, value)
    #
    #     emt: Emitter = NumberEmitter()
    #     rec = RemoveOther(emt)
    #     rec.connect_to(emt)
    #
    #     self.assertEqual(len(emt._receivers), 2)
    #     emt.emit(30)
    #     self.assertEqual(len(emt._receivers), 2)  # although one of them should have expired by now
    #
    #     emt.emit(93)
    #     self.assertEqual(len(emt._receivers), 1)
    #
    # def test_sorting(self):
    #     class NamedReceiver(Receiver):
    #
    #         class Status(Enum):
    #             NOT_CALLED = 0
    #             FIRST = 1
    #             ACCEPTED = 2
    #
    #         def __init__(self, name: str):
    #             Receiver.__init__(self, Value(0).schema)
    #             self.name: str = name
    #             self.status = self.Status.NOT_CALLED
    #
    #         def on_value(self, signal: Emitter.Signal, value: Value):
    #             if not signal.is_accepted():
    #                 self.status = self.Status.FIRST
    #                 signal.accept()
    #             else:
    #                 self.status = self.Status.ACCEPTED
    #                 signal.block()
    #
    #     class SortByName(NumberEmitter):
    #
    #         def _emit(self, receivers: List['Receiver'], value: Value):
    #             # split the receivers into sortable and un-sortable
    #             named_recs = []
    #             other_recs = []
    #             for rec in receivers:
    #                 if isinstance(rec, NamedReceiver):
    #                     named_recs.append(rec)
    #                 else:
    #                     other_recs.append(rec)
    #             named_recs: List = sorted(named_recs, key=lambda x: x.name)
    #
    #             # re-apply the order back to the Emitter
    #             self._sort_receivers([receivers.index(x) for x in (named_recs + other_recs)])
    #
    #             # emit to the sorted Receivers first
    #             signal = Emitter.Signal(self, is_blockable=True)
    #             for named_rec in named_recs:
    #                 named_rec.on_value(signal, value)
    #                 if signal.is_blocked():
    #                     return
    #
    #             # unsorted Receivers should not be able to block the emitting process
    #             signal = Emitter.Signal(self, is_blockable=False)
    #             for other_rec in other_recs:
    #                 other_rec.on_value(signal, value)
    #
    #     a: NamedReceiver = NamedReceiver("a")
    #     b: NamedReceiver = NamedReceiver("b")
    #     c: NamedReceiver = NamedReceiver("c")
    #     d: NamedReceiver = NamedReceiver("d")
    #     e: Recorder = Recorder(Value(0).schema)
    #
    #     # connect in random order
    #     emt: Emitter = SortByName()
    #     for receiver in (e, c, b, a, d):
    #         receiver.connect_to(emt)
    #     del receiver
    #
    #     emt.emit(938)
    #     self.assertEqual(a.status, NamedReceiver.Status.FIRST)
    #     self.assertEqual(b.status, NamedReceiver.Status.ACCEPTED)
    #     self.assertEqual(c.status, NamedReceiver.Status.NOT_CALLED)
    #     self.assertEqual(d.status, NamedReceiver.Status.NOT_CALLED)
    #     self.assertEqual(len(e.values), 0)
    #     self.assertEqual(emt._receivers, [weak_ref(x) for x in [a, b, c, d, e]])
    #
    #     # delete all but b and reset
    #     del a
    #     b.status = NamedReceiver.Status.NOT_CALLED
    #     del c
    #     del d
    #
    #     emt.emit(34)
    #     self.assertEqual(b.status, NamedReceiver.Status.FIRST)
    #     self.assertEqual(len(e.values), 1)
    #
    # def test_apply_invalid_order(self):
    #     emt = NumberEmitter()
    #     _ = record(emt)
    #     _ = record(emt)
    #     _ = record(emt)
    #
    #     with self.assertRaises(RuntimeError):
    #         emt._sort_receivers([])  # empty
    #     with self.assertRaises(RuntimeError):
    #         emt._sort_receivers([0, 1])  # too few indices
    #     with self.assertRaises(RuntimeError):
    #         emt._sort_receivers([0, 1, 2, 3])  # too many indices
    #     with self.assertRaises(RuntimeError):
    #         emt._sort_receivers([0, 1, 1])  # duplicate indices
    #     with self.assertRaises(RuntimeError):
    #         emt._sort_receivers([1, 2, 3])  # wrong indices
    #     emt._sort_receivers([0, 1, 2])  # success


if __name__ == '__main__':
    unittest.main()

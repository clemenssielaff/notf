import unittest
import logging
import sys
from copy import copy
from typing import List, Optional, ClassVar, Dict, Any
from random import randint as random_int
from weakref import ref as weak_ref

from pynotf.logic import AbstractReceiver, AbstractEmitter, Circuit, FailureSignal, CompletionSignal, ValueSignal, \
    Operator, Element, Operation
from pynotf.value import Value

from tests.utils import number_schema, string_schema, Recorder, random_schema, create_emitter, create_receiver, \
    create_operator, random_shuffle, create_operation, make_handle


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
        self.errors: List[Circuit.Error] = []
        self.circuit.set_error_callback(lambda error: self.errors.append(error))

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

    def create_emitter(self, *args, **kwargs):
        return create_emitter(self.circuit, *args, **kwargs)

    def create_receiver(self, *args, **kwargs):
        return create_receiver(self.circuit, *args, **kwargs)

    def create_recorder(self, schema: Value.Schema):
        return self.circuit.create_element(Recorder, schema)


class SimpleTestCase(BaseTestCase):
    """
    All test cases that just need a simple Circuit as setup.
    """

    def test_receiver_schema(self):
        """
        Tests whether the input Schema of an Receiver is set correctly
        """
        for schema in (random_schema() for _ in range(4)):
            receiver: AbstractReceiver = self.create_receiver(schema)
            self.assertEqual(receiver.get_input_schema(), schema)

    def test_emitter_schema(self):
        """
        Tests whether the output Schema of an Emitter is set correctly
        """
        for schema in (random_schema() for _ in range(4)):
            emitter: AbstractEmitter = self.create_emitter(schema)
            self.assertEqual(emitter.get_output_schema(), schema)
            self.assertEqual(make_handle(emitter).get_output_schema(), schema)

    def test_emitter_handle(self):
        """
        Tests whether the output Schema of an Emitter is set correctly
        """
        handle: Optional[AbstractEmitter.Handle] = None
        for schema in (random_schema() for _ in range(4)):
            emitter: AbstractEmitter = self.create_emitter(schema)
            handle = make_handle(emitter)
            self.assertTrue(handle.is_valid())
            self.assertEqual(handle.get_id(), emitter.get_id())
            self.assertEqual(handle.get_output_schema(), schema)
        del emitter
        self.assertFalse(handle.is_valid())
        self.assertIsNone(handle.get_id())
        self.assertIsNone(handle.get_output_schema())

    def test_default_error_handling(self):
        """
        ... just for coverage, really
        """
        self.circuit.set_error_callback(None)
        error: Circuit.Error = Circuit.Error(weak_ref(self.create_receiver(number_schema)),
                                             Circuit.Error.Kind.NO_DAG,
                                             "test error")
        self.circuit.handle_error(error)

    def test_emit_wrong_values(self):
        """
        Make sure you cannot emit anything that is not the correct Value type.
        """
        emitter = self.create_emitter(number_schema)
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
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        receiver: AbstractReceiver = self.create_receiver(string_schema)

        self.assertEqual(len(self.errors), 0)
        receiver.connect_to(make_handle(emitter))
        self.assertEqual(self.errors[0].kind, Circuit.Error.Kind.WRONG_VALUE_SCHEMA)

    def test_repeat_connect_disconnect(self):
        """
        Create-connects and disconnects a number of times to make sure that all disconnections succeed.
        """
        recorder: Recorder = self.create_recorder(number_schema)

        upstream_count = random_int(3, 8)
        for _ in range(upstream_count):
            recorder.create_operator(create_operation(number_schema, lambda v: v))
        self.assertEqual(self._apply_topology_changes(), upstream_count)
        self.assertEqual(len(recorder._upstream), upstream_count)

        recorder.disconnect_upstream()
        self.assertEqual(self._apply_topology_changes(), upstream_count)
        self.assertEqual(len(recorder._upstream), 0)

    def test_operator_lifetime(self):
        """
        Creates a Receiver and two upstream noop operators to ensure that they are kept alive solely through their
        downstream connections.
        """
        recorder: Recorder = self.create_recorder(number_schema)
        weak_head: weak_ref = recorder \
            .create_operator(create_operation(number_schema, lambda v: v)) \
            .create_operator(create_operation(number_schema, lambda v: v))._element
        self.assertEqual(self._apply_topology_changes(), 2)

        # only the downstream has a strong reference to the head of the operator chain
        self.assertEqual(sys.getrefcount(weak_head()) - 1, 1)

        # we can still emit from it
        self.circuit.emit_value(weak_head(), 74)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(recorder.signals), 1)
        self.assertEqual(len(recorder.get_values()), 1)
        self.assertEqual(recorder.get_values()[0].as_number(), 74)

        # when we delete the bottom receiver, the whole chain is destroyed
        del recorder
        self.assertIsNone(weak_head())

    def test_connecting_to_completed_emitter(self):
        """
        Checks that connecting to a completed Emitter triggers a single completion Event only for the Receiver.
        """
        # set up an emitter and connect a recorder
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        recorder1: Recorder = Recorder.record(emitter)
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
        recorder2 = Recorder.record(emitter)
        self.assertEqual(self._apply_topology_changes(), 1)  # to complete the connection
        self.assertEqual(self._handle_events(), 1)  # to handle the immediate completion call

        # make sure that the new receiver gets the same completion signal as the old one
        self.assertEqual(len(recorder2.signals), 1)
        self.assertEqual([signal.get_source() for signal in completions],
                         [signal.get_source() for signal in recorder2.get_completions()])
        self.assertFalse(recorder2.has_upstream())

        # also check that the old receiver did not receive another completion signal
        self.assertEqual(completions, recorder1.get_completions())

    def test_get_operator_by_id(self):
        """
        Checks if a emitter found by ID is the same as the one returned on creation.
        """
        receiver: AbstractReceiver = self.create_receiver(number_schema)
        operator: AbstractEmitter.Handle = receiver.create_operator(create_operation(number_schema, lambda v: v))
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertEqual(receiver._find_emitter(operator.get_id())._element, operator._element)
        self.assertIsNone(receiver._find_emitter(587658765))

    def test_get_removed_operator_by_id(self):
        """
        Make sure that named emitters are no longer found when they have been removed.
        """
        receiver: AbstractReceiver = self.create_receiver(number_schema)
        operator: AbstractEmitter.Handle = receiver.create_operator(create_operation(number_schema, lambda v: v))
        operator_id: Element.ID = operator.get_id()
        self.assertEqual(self._apply_topology_changes(), 1)
        receiver.disconnect_upstream()
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertEqual(self.circuit._expired_elements, [operator._element()])
        self.circuit.cleanup()
        self.assertIsNone(receiver._find_emitter(operator_id))

    def test_shared_operator_lifetime(self):
        """
        Checks the lifetime of an operator that is connected to two downstream receivers.
        """
        # create two recorders, create-connect an operator from one and connect to it from the other
        recorder1: Recorder = self.create_recorder(number_schema)
        recorder2: Recorder = self.create_recorder(number_schema)
        operator: Operator.CreatorHandle = recorder1.create_operator(create_operation(number_schema, lambda v: v))
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        recorder2.connect_to(operator)
        operator.connect_to(make_handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 3)

        # emit a value to make sure everything is in order
        self.circuit.emit_value(emitter, 0)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(recorder1.signals), 1)
        self.assertEqual(recorder1.signals, recorder2.signals)

        # disconnect recorder 1
        recorder1.disconnect_from(operator)
        self.assertTrue(self._apply_topology_changes(), 1)
        self.circuit.emit_value(emitter, 1)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(recorder1.signals), 1)
        self.assertEqual(len(recorder2.signals), 2)
        self.assertEqual([value.as_number() for value in recorder2.get_values()], [0, 1])

        # disconnect recorder 2
        recorder2.disconnect_from(operator)
        self.assertTrue(self._apply_topology_changes(), 1)
        self.circuit.emit_value(emitter, 1)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(recorder1.signals), 1)
        self.assertEqual(len(recorder2.signals), 2)

        # remove the operator
        del operator
        self.circuit.cleanup()
        self.circuit.emit_value(emitter, 2)  # flush out expired downstream
        self.assertEqual(self._handle_events(), 1)
        self.assertFalse(emitter.has_downstream())

    def test_double_connection(self):
        """
        Create two connections between the same Emitter/Receiver pair.
        """
        # create an emitter without connecting any receivers
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        self.assertFalse(emitter.has_downstream())

        # connect a receiver
        recorder: Recorder = self.create_recorder(number_schema)
        self.assertFalse(recorder.has_upstream())
        recorder.connect_to(make_handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertTrue(emitter.has_downstream())
        self.assertTrue(recorder.has_upstream())
        self.assertEqual(len(emitter._downstream), 1)
        self.assertEqual(len(recorder._upstream), 1)

        # a second connection between the emitter and receiver is silently ignored
        recorder.connect_to(make_handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertEqual(len(emitter._downstream), 1)
        self.assertEqual(len(recorder._upstream), 1)

        # just to make sure, emit a single value and see how many end up at the receiver
        self.circuit.emit_value(emitter, 46)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual([value.as_number() for value in recorder.get_values()], [46])

    def test_connect_disconnect_in_same_event(self):
        """
        We have opted to keep all topology changes in the same queue to make sure that creation and removal of the same
        edge in one event is handled correctly.
        """
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        recorder: Recorder = self.create_recorder(number_schema)

        # connect - disconnect => disconnected
        recorder.connect_to(make_handle(emitter))
        recorder.disconnect_from(make_handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 2)
        self.circuit.emit_value(emitter, 79)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(recorder.signals), 0)

        # connect - disconnect - connect => connected
        recorder.connect_to(make_handle(emitter))
        recorder.disconnect_from(make_handle(emitter))
        recorder.connect_to(make_handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 3)
        self.circuit.emit_value(emitter, 83)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual([value.as_number() for value in recorder.get_values()], [83])

        # disconnect - connect - disconnect => disconnected
        recorder.disconnect_from(make_handle(emitter))
        recorder.connect_to(make_handle(emitter))
        recorder.disconnect_from(make_handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 3)
        self.circuit.emit_value(emitter, 49)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual([value.as_number() for value in recorder.get_values()], [83])  # unchanged

    def test_disconnect_from_unconnected(self):
        """
        Fail gracefully when attempting to disconnect from an Emitter handle that has expired.
        """
        emitter1: AbstractEmitter = self.create_emitter(number_schema)
        emitter2: AbstractEmitter = self.create_emitter(number_schema)
        handle1: AbstractEmitter.Handle = make_handle(emitter1)
        handle2: AbstractEmitter.Handle = make_handle(emitter2)

        receiver: AbstractReceiver = self.create_receiver(number_schema)
        receiver.connect_to(handle1)
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertTrue(receiver.has_upstream())

        # disconnecting from a not-connected emitter will be ignored
        receiver.disconnect_from(handle2)
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertTrue(receiver.has_upstream())

        # disconnecting from an expired handle won't even cause a topology change
        del emitter2
        receiver.disconnect_from(handle2)
        self.assertEqual(self._apply_topology_changes(), 0)

    def test_signal_source(self):
        """
        Checks that a Receiver is able to identify which upstream Emitter a Signal originated from.
        """
        emitter1: AbstractEmitter = self.create_emitter(number_schema)
        emitter2: AbstractEmitter = self.create_emitter(number_schema)
        recorder: Recorder = self.create_recorder(number_schema)

        # connect the recorder to the two emitters and check that nothing else has happened
        recorder.connect_to(make_handle(emitter1))
        recorder.connect_to(make_handle(emitter2))
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

        # callback function for the receiver to call when it receives a FailureSignal
        def throw_exception(_: ValueSignal):
            raise RuntimeError()

        # create a custom receiver
        receiver: AbstractReceiver = self.create_receiver(number_schema, on_value=throw_exception)

        # error handling callback for the circuit
        error_handler_called: bool = False

        def error_handling(_: Circuit.Error):
            nonlocal error_handler_called
            error_handler_called = True

        self.circuit.set_error_callback(error_handling)

        # create a custom emitter and connect the receiver to it
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        receiver.connect_to(make_handle(emitter))
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

        # callback function for the receiver to call when it receives a FailureSignal
        def throw_exception(signal: FailureSignal):
            raise signal.get_error()

        # create a custom receiver
        receiver: AbstractReceiver = create_receiver(self.circuit, number_schema, on_failure=throw_exception)

        # error handling callback for the circuit
        error_handler_called: bool = False

        def error_handling(_: Circuit.Error):
            nonlocal error_handler_called
            error_handler_called = True

        self.circuit.set_error_callback(error_handling)

        # create a custom emitter and connect the receiver to it
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        receiver.connect_to(make_handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 1)

        # let the emitter fail
        self.circuit.emit_failure(emitter, RuntimeError())
        self.assertEqual(self._handle_events(), 1)

        # ake sure that the error handler got called and everything is in order
        self.assertTrue(error_handler_called)

    def test_exception_on_complete(self):
        """
        Tests what happens if a Receiver throws an exception while processing a CompletionSignal.
        """

        # callback function for the receiver to call when it receives a FailureSignal
        def throw_exception(_: CompletionSignal):
            raise RuntimeError()

        # create a custom receiver
        receiver: AbstractReceiver = self.create_receiver(number_schema, on_completion=throw_exception)

        # error handling callback for the circuit
        error_handler_called: bool = False

        def error_handling(_: Circuit.Error):
            nonlocal error_handler_called
            error_handler_called = True

        self.circuit.set_error_callback(error_handling)

        # create a custom emitter and connect the receiver to it
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        receiver.connect_to(make_handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 1)

        # let the emitter fail
        self.circuit.emit_completion(emitter)
        self.assertEqual(self._handle_events(), 1)

        # ake sure that the error handler got called and everything is in order
        self.assertTrue(error_handler_called)

    def test_double_completion(self):
        """
        Make sure that an Emitter, once completed, can never emit any signal again.
        """
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        recorder: Recorder = Recorder.record(emitter)
        self.assertEqual(self._apply_topology_changes(), 1)

        self.circuit.emit_completion(emitter)
        self.circuit.emit_value(emitter, 1)
        self.circuit.emit_failure(emitter, ValueError())
        self.circuit.emit_completion(emitter)
        self.assertEqual(self._handle_events(), 4)

        self.assertEqual(len(recorder.signals), 1)
        self.assertIsInstance(recorder.signals[0], CompletionSignal)

    def test_simple_operator(self):
        """
        0, 1, 2, 3 -> multiply by two -> 0, 2, 4, 6
        """
        # set up the Circuit
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        operator: Operator = create_operator(self.circuit, number_schema,
                                             lambda value: value.modified().set(value.as_number() * 2)
                                             )
        operator.connect_to(make_handle(emitter))
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
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        operator: Operator = create_operator(self.circuit, number_schema,
                                             lambda value: Value(str(value.as_number())),
                                             output_schema=string_schema)
        operator.connect_to(make_handle(emitter))
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

        class GroupTwo(Operation):
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

            def __call__(self, value: Value) -> Optional[Value]:
                if self._last_value is None:
                    self._last_value = value.as_number()
                else:
                    result = self._output_prototype.modified().set({"x": self._last_value, "y": value.as_number()})
                    self._last_value = None
                    return result

        # set up the Circuit
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        operator: Operator = self.circuit.create_element(Operator, GroupTwo())
        operator.connect_to(make_handle(emitter))
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
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        operator: Operator = create_operator(self.circuit, number_schema, raise_on(4))
        operator.connect_to(make_handle(emitter))
        recorder: Recorder = Recorder.record(operator)
        self.assertEqual(self._apply_topology_changes(), 2)

        # try to emit all numbers [0, 10)
        for number in range(10):
            self.circuit.emit_value(emitter, number)
        self.assertEqual(self._handle_events(), 10)

        # even though we handled 10 events, the operator failed while handling input 4
        self.assertEqual([signal.get_source() for signal in recorder.get_failures()], [operator.get_id()])
        self.assertEqual([value.as_number() for value in recorder.get_values()], [0, 1, 2, 3])

    def test_create_connect_bad_operator(self):
        """
        Check that a Receiver of Schema X cannot create-connect an upstream Operator with Schema Y
        """
        receiver: AbstractReceiver = self.create_receiver(number_schema)

        self.assertEqual(len(self.errors), 0)
        result: Any = receiver.create_operator(create_operation(string_schema, lambda v: v))
        self.assertIsNone(result)
        self.assertEqual(len(self.errors), 1)
        self.assertEqual(self.errors[0].kind, Circuit.Error.Kind.WRONG_VALUE_SCHEMA)

    def test_operator_autocomplete_on_complete(self):
        """
        Operators automatically complete, once they have no upstream left.
        """
        emitter1: AbstractEmitter = self.create_emitter(number_schema)
        emitter2: AbstractEmitter = self.create_emitter(number_schema)
        recorder: Recorder = self.create_recorder(number_schema)
        operator_handle: Operator.CreatorHandle = recorder.create_operator(create_operation(number_schema, lambda v: v))
        operator_handle.connect_to(make_handle(emitter1))
        operator_handle.connect_to(make_handle(emitter2))
        operator: Operator = operator_handle._element()
        self.assertIsNotNone(operator)
        self.assertEqual(self._apply_topology_changes(), 3)

        self.assertFalse(operator.is_completed())
        self.assertEqual(len(operator._upstream), 2)
        self.circuit.emit_completion(emitter1)
        self.assertEqual(self._handle_events(), 1)

        self.assertFalse(operator.is_completed())
        self.assertEqual(len(operator._upstream), 1)
        self.circuit.emit_completion(emitter2)
        self.assertEqual(self._handle_events(), 1)

        self.assertTrue(operator.is_completed())
        self.assertEqual(len(operator._upstream), 0)

    def test_operator_autocomplete_on_failure(self):
        """
        Operators automatically complete, once they have no upstream left.
        """
        emitter1: AbstractEmitter = self.create_emitter(number_schema)
        emitter2: AbstractEmitter = self.create_emitter(number_schema)
        recorder: Recorder = self.create_recorder(number_schema)
        operator_handle: Operator.CreatorHandle = recorder.create_operator(create_operation(number_schema, lambda v: v))
        operator_handle.connect_to(make_handle(emitter1))
        operator_handle.connect_to(make_handle(emitter2))
        operator: Operator = operator_handle._element()
        self.assertIsNotNone(operator)
        self.assertEqual(self._apply_topology_changes(), 3)

        self.assertFalse(operator.is_completed())
        self.assertEqual(len(operator._upstream), 2)
        self.circuit.emit_failure(emitter1, ValueError())
        self.assertEqual(self._handle_events(), 1)

        self.assertFalse(operator.is_completed())
        self.assertEqual(len(operator._upstream), 1)
        self.circuit.emit_failure(emitter2, ValueError())
        self.assertEqual(self._handle_events(), 1)

        self.assertTrue(operator.is_completed())
        self.assertEqual(len(operator._upstream), 0)

    def test_bad_emitter(self):
        """
        Test for Emitters that try to emit Value types that they cannot.
        """
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        recorder: Recorder = Recorder.record(emitter)
        self.assertTrue(self._apply_topology_changes(), 1)

        # emit a value to make sure everything works
        self.circuit.emit_value(emitter, Value(98))
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(recorder.signals), 1)
        self.assertEqual(recorder.signals[0].get_value().as_number(), 98)

        # emitting anything that can be casted to the right value type works as well
        self.circuit.emit_value(emitter, Value(9436))
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(recorder.signals), 2)
        self.assertEqual(recorder.signals[1].get_value().as_number(), 9436)

        # emitting wrong value types will throw an exception right away
        with self.assertRaises(TypeError):
            self.circuit.emit_value(emitter, Value("nope"))
        self.assertEqual(len(recorder.signals), 2)

        # same goes for anything that cannot be cast to the right value type
        with self.assertRaises(TypeError):
            self.circuit.emit_value(emitter, None)
        with self.assertRaises(TypeError):
            self.circuit.emit_value(emitter, TypeError())
        with self.assertRaises(TypeError):
            self.circuit.emit_value(emitter, "string")
        with self.assertRaises(TypeError):
            self.circuit.emit_value(emitter, "0.1")
        self.assertEqual(len(recorder.signals), 2)

    def test_bad_operator(self):
        """
        Test for Operators that don't deliver the Value types that they promise.
        """
        # set up the circuit
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        operator: Operator = create_operator(self.circuit, number_schema, lambda value: value, string_schema)
        operator.connect_to(make_handle(emitter))
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
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        rec1 = Recorder.record(emitter)
        rec2 = Recorder.record(emitter)
        rec3 = Recorder.record(emitter)
        rec4 = Recorder.record(emitter)
        rec5 = Recorder.record(emitter)
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
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        recorder: Recorder = self.create_recorder(number_schema)
        recorder.connect_to(make_handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertEqual(len(emitter._downstream), 1)
        del recorder
        emitter._complete()
        self.assertEqual(len(emitter._downstream), 0)

    def test_expired_receiver_on_failure(self):
        """
        See test_expired_receiver_on_value
        """
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        recorder: Recorder = self.create_recorder(number_schema)
        recorder.connect_to(make_handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertEqual(len(emitter._downstream), 1)
        del recorder
        emitter._fail(ValueError())
        self.assertEqual(len(emitter._downstream), 0)

    def test_connect_to_expired_handle(self):
        """
        Fail gracefully when attempting to connect to an Emitter handle that has expired.
        """
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        handle: AbstractEmitter.Handle = make_handle(emitter)
        del emitter

        receiver: AbstractReceiver = self.create_receiver(number_schema)
        receiver.connect_to(handle)
        self.assertEqual(self._apply_topology_changes(), 0)
        self.assertFalse(receiver.has_upstream())

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
        # error handling callback for the circuit
        error_producer: Optional[int] = None

        def error_handling(error: Circuit.Error):
            nonlocal error_producer
            self.assertEqual(error.kind, Circuit.Error.Kind.NO_DAG)
            error_producer = error.element().get_id()

        self.circuit.set_error_callback(error_handling)

        # create the Circuit
        noop1: Operator = create_operator(self.circuit, number_schema, lambda value: value)
        recorder1: Recorder = Recorder.record(noop1)
        noop2: Operator = create_operator(self.circuit, number_schema, lambda value: value)
        recorder2: Recorder = Recorder.record(noop1)
        noop3: Operator = create_operator(self.circuit, number_schema, lambda value: value)
        recorder3: Recorder = Recorder.record(noop3)
        noop2.connect_to(make_handle(noop1))
        noop3.connect_to(make_handle(noop1))
        noop1.connect_to(make_handle(noop2))
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

    def test_fail_in_cycle(self):
        """
        Emitters check for cycles in their _emit function but *not* in their _complete and _fail functions because those
        complete the Emitter anyway. Still, This is definitely an edge case that requires special attention.

        +------------ +
        |    +--> B   |
        |    |        |
        +--> A--> C-->+
             |
             +--> D
        """

        def node_a_error_on_value(_: Value):
            raise RuntimeError()

        # create the Circuit
        a: Operator = create_operator(self.circuit, number_schema, node_a_error_on_value)
        b: Recorder = self.create_recorder(number_schema)
        c: Operator = create_operator(self.circuit, number_schema, lambda value: value)  # noop
        d: Recorder = self.create_recorder(number_schema)
        b.connect_to(make_handle(a))
        c.connect_to(make_handle(a))
        d.connect_to(make_handle(a))
        a.connect_to(make_handle(c))
        self.assertEqual(self._apply_topology_changes(), 4)

        # emit a single value from a
        self.circuit.emit_value(a, 75)
        self.assertEqual(self._handle_events(), 1)

        # what should have happened is:
        self.assertTrue(a.is_completed())  # A has completed through failure
        self.assertTrue(c.is_completed())  # C has completed because it has no more upstream emitters
        self.assertEqual(len(b.get_values()), 1)  # B has received the value before A failed
        self.assertEqual(len(d.get_values()), 0)  # D had not yet received the value before A failed
        self.assertFalse(a.has_downstream())  # everyone disconnected from A ...
        self.assertFalse(b.has_upstream())
        self.assertFalse(c.has_upstream())
        self.assertFalse(d.has_upstream())
        self.assertFalse(a.has_upstream())  # and A disconnected from C

    def test_sorter(self):
        """
        Proof-of-concept test case
        """

        class Sorter(AbstractReceiver, AbstractEmitter):
            """
            The sorter is a special kind of Circuit element, that sorts ValueSignals by their Emitter ID and
            emits them as a list when that Emitter has completed.
            """

            def __init__(self, circuit: 'Circuit', element_id: Element.ID, input_schema: Value.Schema):
                """
                Constructor.
                :param circuit: Circuit owning this Circuit element.
                :param input_schema: Schema of the input value to the sorter.
                """
                AbstractReceiver.__init__(self, input_schema)
                AbstractEmitter.__init__(self, input_schema.as_list())

                self._circuit: Circuit = circuit  # is constant
                self._element_id: Element.ID = element_id  # is constant
                self._storage: Dict[int, List[Value]] = {}

            def get_id(self) -> Element.ID:  # final, noexcept
                return self._element_id

            def get_circuit(self) -> 'Circuit':  # final, noexcept
                return self._circuit

            def _on_value(self, signal: ValueSignal):
                value: Value = signal.get_value()
                assert value.schema == self.get_input_schema()
                source: int = signal.get_source()
                if source in self._storage:
                    self._storage[source].append(value)
                else:
                    self._storage[source] = [value]

            def _on_completion(self, signal: CompletionSignal):
                source: int = signal.get_source()
                values: Optional[List[Value]] = self._storage.get(source, None)
                if values is not None:
                    self._emit(Value(values))
                    del self._storage[source]

            def _on_failure(self, signal: FailureSignal):
                self._on_completion(signal)

        # create the circuit
        a: AbstractEmitter = self.create_emitter(number_schema)
        b: AbstractEmitter = self.create_emitter(number_schema)
        c: AbstractEmitter = self.create_emitter(number_schema)
        sorter: Sorter = self.circuit.create_element(Sorter, number_schema)
        recorder: Recorder = self.create_recorder(Value([0]).schema)
        recorder.connect_to(make_handle(sorter))
        sorter.connect_to(make_handle(b))
        sorter.connect_to(make_handle(c))
        sorter.connect_to(make_handle(a))
        self.assertEqual(self._apply_topology_changes(), 4)

        # emit random values in random order
        a_values = [random_int(0, 100) for _ in range(random_int(1, 10))]
        b_values = [random_int(0, 100) for _ in range(random_int(1, 10))]
        c_values = [random_int(0, 100) for _ in range(random_int(1, 10))]
        a.to_emit = copy(a_values)
        b.to_emit = copy(b_values)
        c.to_emit = copy(c_values)
        for emitter in random_shuffle(*([a] * len(a.to_emit) + [b] * len(b.to_emit) + [c] * len(c.to_emit))):
            self.circuit.emit_value(emitter, emitter.to_emit.pop(0))
        self.assertEqual(len(a.to_emit), 0)
        self.assertEqual(len(b.to_emit), 0)
        self.assertEqual(len(c.to_emit), 0)
        self.assertEqual(self._handle_events(), len(a_values) + len(b_values) + len(c_values))
        self.assertEqual(len(recorder.signals), 0)

        # complete a and see what happens
        self.circuit.emit_completion(a)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(recorder.signals), 1)
        self.assertEqual(len(recorder.get_values()), 1)
        self.assertEqual(recorder.get_values()[0], Value(a_values))

        # b emits another value
        self.circuit.emit_value(b, 101)
        b_values.append(101)
        self.assertEqual(self._handle_events(), 1)

        # c completes without emitting again
        self.circuit.emit_completion(c)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(recorder.signals), 2)
        self.assertEqual(len(recorder.get_values()), 2)
        self.assertEqual(recorder.get_values()[1], Value(c_values))

        # b completes through failure
        self.circuit.emit_failure(b, ValueError())
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len(recorder.signals), 3)
        self.assertEqual(len(recorder.get_values()), 3)
        self.assertEqual(recorder.get_values()[2], Value(b_values))

    def test_emit_value_with_expired_emitter(self):
        """
        Makes sure that Events fail gracefully if they encounter an expired Handle.
        """
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        self.circuit.emit_value(emitter, 234)
        del emitter
        self.assertEqual(self._handle_events(), 1)

    def test_emit_failure_with_expired_emitter(self):
        """
        Makes sure that Events fail gracefully if they encounter an expired Handle.
        """
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        self.circuit.emit_failure(emitter, ValueError())
        del emitter
        self.assertEqual(self._handle_events(), 1)

    def test_emit_completion_with_expired_emitter(self):
        """
        Makes sure that Events fail gracefully if they encounter an expired Handle.
        """
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        self.circuit.emit_completion(emitter)
        del emitter
        self.assertEqual(self._handle_events(), 1)

    def test_connection_with_expired_emitter(self):
        """
        Makes sure that Events fail gracefully if they encounter an expired Handle.
        Note that this should be impossible for the user to achieve since you need to call the Circuit method directly,
        without going through the Receiver.
        """
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        receiver: AbstractReceiver = self.create_receiver(number_schema)
        self.circuit.create_connection(emitter, receiver)
        del emitter
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertFalse(receiver.has_upstream())

    def test_connection_with_expired_receiver(self):
        """
        Makes sure that Events fail gracefully if they encounter an expired Handle.
        Note that this should be impossible for the user to achieve since you need to call the Circuit method directly,
        without going through the Receiver.
        """
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        receiver: AbstractReceiver = self.create_receiver(number_schema)
        self.circuit.create_connection(emitter, receiver)
        del receiver
        self.assertEqual(self._apply_topology_changes(), 1)
        self.assertFalse(emitter.has_downstream())

    def test_already_completed_connection_with_expired_receiver(self):
        """
        Makes sure that Events fail gracefully if they encounter an expired Handle.
        Note that this should be impossible for the user to achieve since you need to call the Circuit method directly,
        without going through the Receiver.
        """
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        receiver: AbstractReceiver = self.create_receiver(number_schema)
        self.circuit.emit_completion(emitter)
        self.assertEqual(self._handle_events(), 1)
        self.circuit.create_connection(emitter, receiver)
        self.assertEqual(self._apply_topology_changes(), 1)
        del receiver
        self.assertEqual(self._handle_events(), 1)
        self.assertFalse(emitter.has_downstream())

    def test_creator_handle_with_expired_element(self):
        """
        Makes sure that Events fail gracefully if they encounter an expired Handle.
        """
        receiver: AbstractReceiver = self.create_receiver(number_schema)
        operation: Operation = create_operation(number_schema, lambda v: v)
        handle: Operator.CreatorHandle = receiver.create_operator(operation)
        self.assertEqual(self._apply_topology_changes(), 1)
        del receiver

        # connect_to
        emitter: AbstractEmitter = self.create_emitter(number_schema)
        handle.connect_to(make_handle(emitter))
        self.assertEqual(self._apply_topology_changes(), 0)
        self.assertFalse(emitter.has_downstream())

        # create_operator
        result: Any = handle.create_operator(operation)
        self.assertIsNone(result)

    def test_fake_signals(self):
        """
        Since the on_X methods are public, I guess you could try to call them with foreign Signals? Of course, in C++
        you couldn't create instances of Signals in the first place, that would be restricted to Emitters ... but for
        coverage let's pretend like you can create your own Signals and are desperate to try to mess with the Receiver.
        """
        receiver: AbstractReceiver = self.create_receiver(number_schema)
        receiver.create_operator(create_operation(number_schema, lambda v: v))

        with self.assertRaises(AssertionError):
            receiver.on_value(ValueSignal(123, Value(0)))
        with self.assertRaises(AssertionError):
            receiver.on_failure(FailureSignal(123, ValueError()))
        with self.assertRaises(AssertionError):
            receiver.on_completion(CompletionSignal(123))


########################################################################################################################
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
        self.emitter: AbstractEmitter = self.create_emitter(number_schema)
        self.recorder1: Recorder = Recorder.record(self.emitter)
        self.recorder2: Recorder = Recorder.record(self.emitter)
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
        self.recorder2.disconnect_from(make_handle(self.emitter))
        self.assertEqual(self._apply_topology_changes(), 1)
        self.circuit.emit_value(self.emitter, 1)
        self.assertEqual(self._handle_events(), 1)

        # make sure that only one receiver got the value
        self.assertEqual([value.as_number() for value in self.recorder1.get_values()], [0, 1])
        self.assertEqual([value.as_number() for value in self.recorder2.get_values()], [0])

        # disconnect the other receiver
        self.recorder1.disconnect_from(make_handle(self.emitter))
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
        self.emitter: AbstractEmitter = self.create_emitter(number_schema)

    def test_connection_mid_event(self):
        """
        Changes in Circuit topology are performed in the Event epilogue, which means that they might happen after user-
        defined code that was executed later.
        """
        operators: List[Operator] = []  # we need to keep the created receivers alive until the end of the test

        def create_another_operator(_: ValueSignal):
            operator: Operator = create_operator(self.circuit, number_schema, lambda value: value)
            operators.append(operator)
            emitter: Optional[AbstractEmitter.Handle] = operator._find_emitter(self.emitter.get_id())
            self.assertIsNotNone(emitter)
            operator.connect_to(emitter)

        # create a custom receiver and connect it to the circuit's emitter
        receiver: AbstractReceiver = create_receiver(self.circuit, number_schema, on_value=create_another_operator)
        receiver.connect_to(make_handle(self.emitter))
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

        class DisconnectOnValue(AbstractReceiver):
            def __init__(self, circuit: 'Circuit', element_id: Element.ID):
                AbstractReceiver.__init__(self, number_schema)

                self._circuit: Circuit = circuit  # is constant
                self._element_id: Element.ID = element_id  # is constant

            def get_id(self) -> Element.ID:  # final, noexcept
                return self._element_id

            def get_circuit(self) -> 'Circuit':  # final, noexcept
                return self._circuit

            def _on_value(self, signal: ValueSignal):  # virtual
                emitter_handle: Optional[AbstractEmitter.Handle] = self._find_emitter(signal.get_source())
                if emitter_handle is not None:
                    self.disconnect_from(emitter_handle)

        # we need to keep the created receivers alive until the end of the test
        receivers: List[AbstractReceiver] = []
        for index in range(3):
            # create receivers and monkey patch their on_value method
            receiver: AbstractReceiver = self.circuit.create_element(DisconnectOnValue)
            receivers.append(receiver)

            # connect each operator to the emitter
            emitter: Optional[AbstractEmitter.Handle] = receiver._find_emitter(self.emitter.get_id())
            self.assertIsNotNone(emitter)
            receiver.connect_to(emitter)

        # make sure the emitter sees all of the receivers
        self.assertEqual(self._apply_topology_changes(), len(receivers))
        self.assertEqual(len([rec for rec in self.emitter._downstream if rec() is not None]), len(receivers))

        # emit a value and all operators should disconnect
        self.circuit.emit_value(self.emitter, 0)
        self.assertEqual(self._handle_events(), 1)
        self.assertEqual(len([rec for rec in self.emitter._downstream if rec() is not None]), 0)


class OrderedEmissionTestCase(BaseTestCase):
    """
    Almost the same Circuit, but connected in a different order.
    """

    def setUp(self):
        BaseTestCase.setUp(self)

        class NodeDOp(Operation):
            def __init__(self):
                self.first: Optional[float] = None

            def get_input_schema(self) -> Value.Schema:
                return number_schema

            def get_output_schema(self) -> Value.Schema:
                return number_schema

            def __call__(self, value: Value) -> Optional[Value]:
                if self.first is None:
                    self.first = value.as_number()
                else:
                    result: float = self.first - value.as_number()
                    self.first = None
                    return Value(result)

        # create the Circuit
        self.a: AbstractEmitter = self.create_emitter(number_schema)
        self.b: Operator = create_operator(self.circuit, number_schema, lambda value: Value(value.as_number() + 2))
        self.c: Operator = create_operator(self.circuit, number_schema, lambda value: Value(value.as_number() + 3))
        self.d: Operator = self.circuit.create_element(Operator, NodeDOp())
        self.recorder: Recorder = self.create_recorder(number_schema)

    def test_b_then_c(self):
        """
            +--> B (adds 2) -->+
            |                  v
        A --+                  D (first - second) --> Recorder
            |                  ^
            +--> C (adds 3) -->+
        """
        self.b.connect_to(make_handle(self.a))
        self.c.connect_to(make_handle(self.a))
        for emitter in random_shuffle(self.b, self.c):  # the order of B-D and C-D connections does not matter
            self.d.connect_to(make_handle(emitter))
        self.recorder.connect_to(make_handle(self.d))
        self.assertEqual(self._apply_topology_changes(), 5)

        self.circuit.emit_value(self.a, 0)
        self.assertEqual(self._handle_events(), 1)

        recorded_values: List[Value] = self.recorder.get_values()
        self.assertEqual(len(recorded_values), 1)
        self.assertEqual(recorded_values[0].as_number(), -1)

    def test_c_then_b(self):
        """
            +--> C (adds 3) -->+
            |                  v
        A --+                  D (first - second) --> Recorder
            |                  ^
            +--> B (adds 2) -->+
        """
        self.c.connect_to(make_handle(self.a))
        self.b.connect_to(make_handle(self.a))
        for emitter in random_shuffle(self.b, self.c):  # the order of B-D and C-D connections does not matter
            self.d.connect_to(make_handle(emitter))
        self.recorder.connect_to(make_handle(self.d))
        self.assertEqual(self._apply_topology_changes(), 5)

        self.circuit.emit_value(self.a, 0)
        self.assertEqual(self._handle_events(), 1)

        recorded_values: List[Value] = self.recorder.get_values()
        self.assertEqual(len(recorded_values), 1)
        self.assertEqual(recorded_values[0].as_number(), 1)


class SignalStatusTestCase(BaseTestCase):
    """
    Almost the same Circuit, but connected in a different order.
    """

    def _create_ignorer(self) -> Recorder:
        """
        Records the value if it has not been accepted yet, but doesn't modify the Signal.
        """

        class Ignorer(Recorder):

            def __init__(self, circuit: Circuit, element_id: Element.ID):
                Recorder.__init__(self, circuit, element_id, number_schema)

            def _on_value(self, signal: ValueSignal):
                if not signal.is_accepted():
                    Recorder._on_value(self, signal)

        return self.circuit.create_element(Ignorer)

    def _create_accepter(self) -> Recorder:
        """
        Always records the value, and accepts the Signal.
        """

        class Accepter(Recorder):

            def __init__(self, circuit: Circuit, element_id: Element.ID):
                Recorder.__init__(self, circuit, element_id, number_schema)

            def _on_value(self, signal: ValueSignal):
                Recorder._on_value(self, signal)
                signal.accept()

        return self.circuit.create_element(Accepter)

    def _create_blocker(self) -> Recorder:
        """
        Always records the value, and blocks the Signal.
        """

        class Blocker(Recorder):

            def __init__(self, circuit: Circuit, element_id: Element.ID):
                Recorder.__init__(self, circuit, element_id, number_schema)

            def _on_value(self, signal: ValueSignal):
                Recorder._on_value(self, signal)
                signal.block()
                if signal.is_blockable():
                    signal.block()  # again ... for coverage

        return self.circuit.create_element(Blocker)

    def test_signal_status(self):
        """
        Checks that the Signal status mechanism.
        """
        distributor: AbstractEmitter = self.create_emitter(number_schema, is_blockable=True)
        ignore1: Recorder = self._create_ignorer()  # records all values
        accept1: Recorder = self._create_accepter()  # records and accepts all values
        ignore2: Recorder = self._create_ignorer()  # should not record any since all are accepted
        accept2: Recorder = self._create_accepter()  # should record the same as accept1
        block1: Recorder = self._create_blocker()  # should record the same as accept1
        block2: Recorder = self._create_blocker()  # should not record any values

        # order matters here as the receivers are called in the order they connected
        distributor_handle: AbstractEmitter.Handle = make_handle(distributor)
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

    def test_unblockable_signal(self):
        distributor: AbstractEmitter = self.create_emitter(number_schema)  # default, unblockable signal
        ignorer: Recorder = self._create_ignorer()  # records all values
        accepter: Recorder = self._create_accepter()  # records and accepts all values without effect
        blocker: Recorder = self._create_blocker()  # record and blocks all values without effect
        recorder: Recorder = self.create_recorder(number_schema)  # should record all values emitted from distributor

        # order matters here as the receivers are called in the order they connected
        distributor_handle: AbstractEmitter.Handle = make_handle(distributor)
        ignorer.connect_to(distributor_handle)
        accepter.connect_to(distributor_handle)
        blocker.connect_to(distributor_handle)
        recorder.connect_to(distributor_handle)
        self.assertEqual(self._apply_topology_changes(), 4)

        for x in range(1, 5):
            self.circuit.emit_value(distributor, x)
        self.assertEqual(self._handle_events(), 4)

        # everybody got all the values since unblockable means the signal cannot be stopped
        self.assertEqual([value.as_number() for value in ignorer.get_values()], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in accepter.get_values()], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in blocker.get_values()], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in recorder.get_values()], [1, 2, 3, 4])


if __name__ == '__main__':
    unittest.main()

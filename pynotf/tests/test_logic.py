import unittest
import logging
from enum import Enum
from typing import List
from weakref import ref as weak
from random import randint

from pynotf.logic import Switch, Receiver, Emitter
from pynotf.value import Value

from tests.utils import *

########################################################################################################################
# TEST CASE
########################################################################################################################

"""

Circuit
* create and remove x2 to ensure that no global state is destroyed at the end of the first
* create two circuits at the same time, make sure that they are separate things

Emitters and Receivers
* finish an emitter through complete or failure and check that receivers disconnect
* create a linear circuit
* create a branching circuit

Ownership
* create an emitter whose ownership is passed from one subscriber to another one and make sure it is deleted afterwards
* create an emitter with on outside reference who outlives multiple receiver popping in and out of existence
* create a receiver that creates multiple emitters in sequence for a single slot (thereby dropping the old one)

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
"""


class TestCase(unittest.TestCase):

    def setUp(self):
        logging.disable(logging.CRITICAL)

    def tearDown(self):
        logging.disable(logging.NOTSET)

    # EMITTER ##########################################################################################################

    def test_emitter_schema(self):
        """
        Tests whether the output Schema of an Emitter is set correctly
        """
        for schema in basic_schemas:
            emitter: Emitter = create_emitter(schema)
            self.assertEqual(emitter.get_output_schema(), schema)

    def test_emitting(self):
        """
        Test the emission process over a lifetime of an Emitter.
        Is run twice, once with the emitter completing, once with it failing.
        """
        for phase in ["complete", "error"]:
            with self.subTest(phase=phase):
                # create an emitter and a receiver
                emitter: Emitter = create_emitter(number_schema)
                recorder: Recorder = record(emitter)

                # emit a few number values and check that they are received
                numbers: List[int] = []
                for i in range(randint(3, 8)):
                    number: int = randint(0, 100)
                    numbers.append(number)
                    emitter.emit(number)
                self.assertEqual(len(recorder.values), len(numbers))
                self.assertEqual([recorder.values[i].as_number() for i in range(len(numbers))], numbers)

                # emit a number and have it converted to a value implicitly
                number: int = randint(0, 100)
                emitter.emit(number)
                self.assertEqual(len(recorder.values), len(numbers))
                self.assertEqual(recorder.values[-1].as_number(), number)

                # make sure that you cannot emit anything that is not a number
                with self.assertRaises(TypeError):
                    emitter.emit(None)  # wrong schema
                with self.assertRaises(TypeError):
                    emitter.emit(Value([]))  # not convertible to number
                with self.assertRaises(TypeError):
                    emitter.emit(Nope())  # not convertible to value

                # either complete or fail
                if phase == "complete":
                    emitter._complete()
                else:
                    emitter._error(RuntimeError())

                # either way, the emitter is now completed
                self.assertTrue(emitter.is_completed())

                # emitting any further values will result in an error
                with self.assertRaises(RuntimeError):
                    emitter.emit(randint(0, 100))

                # with completing, the receiver has now disconnected from the emitter
                self.assertEqual(len(recorder._upstream), 0)


    def test_connecting_with_wrong_schema(self):
        with self.assertRaises(TypeError):
            emitter = NumberEmitter()
            receiver = Recorder(Value("String").schema)
            receiver.connect_to(emitter)

    def test_connecting_to_completed_emitter(self):
        emitter = NumberEmitter()
        emitter._complete()

        receiver = Recorder(emitter.output_schema)
        self.assertEqual(len(receiver.completed), 0)
        receiver.connect_to(emitter)
        self.assertEqual(len(receiver.completed), 1)

    def test_exception_on_error(self):
        emt = NumberEmitter()
        rec = ExceptionOnCompleteReceiver()
        rec.connect_to(emt)
        emt._error(NameError())  # should be safely ignored

    def test_exception_on_complete(self):
        emt = NumberEmitter()
        rec = ExceptionOnCompleteReceiver()
        rec.connect_to(emt)
        emt._complete()  # should be safely ignored

    def test_double_connection(self):
        emitter: Emitter = NumberEmitter()

        receiver = Recorder(emitter.output_schema)
        self.assertEqual(len(emitter._receivers), 0)

        receiver.connect_to(emitter)
        self.assertEqual(len(emitter._receivers), 1)

        receiver.connect_to(emitter)  # is ignored
        self.assertEqual(len(emitter._receivers), 1)

        emitter.emit(46)
        self.assertEqual(len(receiver.values), 1)

    def test_disconnect(self):
        emitter: Emitter = NumberEmitter()
        receiver = record(emitter)

        emitter.emit(98)
        self.assertEqual(len(receiver.values), 1)
        self.assertEqual(receiver.values[0].as_number(), 98)

        receiver.disconnect_from(emitter)
        emitter.emit(367)
        self.assertEqual(len(receiver.values), 1)
        self.assertEqual(receiver.values[0].as_number(), 98)

        not_a_receiver = Recorder(emitter.output_schema)
        not_a_receiver.disconnect_from(emitter)

    def test_simple_switch(self):
        """
        0, 1, 2, 3 -> 7, 8, 9, 10 -> (7, 8), (9, 10) -> (7, 8), (9, 10)
        """
        emitter = NumberEmitter()
        switch = Switch(AddConstantOperation(7), GroupTwoOperation(),
                        Switch.NoOp(GroupTwoOperation().output_schema))
        switch.connect_to(emitter)
        recorder = record(switch)

        for x in range(4):
            emitter.emit(Value(x))

        expected = [(7, 8), (9, 10)]
        for index, value in enumerate(recorder.values):
            self.assertEqual(expected[index], (value["x"].as_number(), value["y"].as_number()))

    def test_switch_wrong_type(self):
        switch = Switch(Switch.NoOp(Value(0).schema))
        switch.on_next(Emitter.Signal(NumberEmitter()), Value("Not A Number"))

    def test_switch_error(self):
        emitter = NumberEmitter()
        switch = Switch(ErrorOperation(4))
        switch.connect_to(emitter)
        recorder = record(switch)

        with self.assertRaises(TypeError):
            emitter.emit("Not A Number")

        # it's not the ErrorOperation that fails, but trying to emit another value
        for x in range(10):
            emitter.emit(x)
        self.assertTrue(len(emitter.exceptions) > 0)
        self.assertTrue(switch.is_completed())

        expected = [0, 1, 2, 3]
        self.assertEqual(len(recorder.values), len(expected))
        for i in range(len(expected)):
            self.assertEqual(expected[i], recorder.values[i].as_number())

    def test_no_empty_switch(self):
        with self.assertRaises(ValueError):
            _ = Switch()

    def test_mismatched_operations(self):
        with self.assertRaises(TypeError):
            Switch(GroupTwoOperation(), AddConstantOperation(7))

    def test_receiver_lifetime(self):
        emitter: Emitter = NumberEmitter()
        rec1 = record(emitter)
        rec2 = record(emitter)
        rec3 = record(emitter)
        rec4 = record(emitter)
        rec5 = record(emitter)
        self.assertEqual(len(emitter._receivers), 5)

        emitter.emit(23)
        for rec in (rec1, rec2, rec3, rec4, rec5):
            self.assertEqual(len(rec.values), 1)
            self.assertEqual(rec.values[0].as_number(), 23)
        del rec

        del rec1  # delete first
        self.assertEqual(len(emitter._receivers), 5)  # rec1 has expired but hasn't been removed yet
        emitter.emit(54)  # remove rec1
        self.assertEqual(len(emitter._receivers), 4)

        del rec5  # delete last
        self.assertEqual(len(emitter._receivers), 4)  # rec5 has expired but hasn't been removed yet
        emitter.emit(25)  # remove rec5
        self.assertEqual(len(emitter._receivers), 3)

        del rec3  # delete center
        self.assertEqual(len(emitter._receivers), 3)  # rec3 has expired but hasn't been removed yet
        emitter.emit(-2)  # remove rec3
        self.assertEqual(len(emitter._receivers), 2)

        emitter._complete()
        self.assertEqual(len(emitter._receivers), 0)

    def test_expired_receiver_on_complete(self):
        emitter: Emitter = NumberEmitter()
        rec1 = record(emitter)
        self.assertEqual(len(emitter._receivers), 1)
        del rec1
        emitter._complete()
        self.assertEqual(len(emitter._receivers), 0)

    def test_expired_receiver_on_failure(self):
        emitter: Emitter = NumberEmitter()
        rec1 = record(emitter)
        self.assertEqual(len(emitter._receivers), 1)
        del rec1
        emitter._error(ValueError())
        self.assertEqual(len(emitter._receivers), 0)

    def test_add_receivers(self):
        emitter: Emitter = NumberEmitter()
        receiver: AddAnotherReceiverReceiver = AddAnotherReceiverReceiver(emitter, modulus=3)

        receiver.connect_to(emitter)
        self.assertEqual(count_live_receivers(emitter), 1)
        emitter.emit(1)
        self.assertEqual(count_live_receivers(emitter), 2)
        emitter.emit(2)
        self.assertEqual(count_live_receivers(emitter), 3)
        emitter.emit(3)  # activates modulus
        self.assertEqual(count_live_receivers(emitter), 1)

        emitter.emit(4)
        emitter._complete()  # this too tries to add a Receiver, but is rejected
        self.assertEqual(count_live_receivers(emitter), 0)

    def test_remove_receivers(self):
        emitter: Emitter = NumberEmitter()
        rec1: Receiver = DisconnectReceiver(emitter)

        self.assertEqual(count_live_receivers(emitter), 1)
        emitter.emit(0)
        self.assertEqual(count_live_receivers(emitter), 0)

        rec2: Receiver = DisconnectReceiver(emitter)
        rec3: Receiver = DisconnectReceiver(emitter)
        self.assertEqual(count_live_receivers(emitter), 2)
        emitter.emit(1)
        self.assertEqual(count_live_receivers(emitter), 0)

    def test_signal_source(self):
        emt1: Emitter = NumberEmitter()
        emt2: Emitter = NumberEmitter()
        rec = record(emt1)
        rec.connect_to(emt2)

        emt1.emit(1)
        emt1.emit(2)
        emt2.emit(1)
        emt1.emit(3)
        emt2.emit(2000)

        self.assertEqual([value.as_number() for value in rec.values], [1, 2, 1, 3, 2000])
        self.assertEqual([signal.source for signal in rec.signals], [id(emt1), id(emt1), id(emt2), id(emt1), id(emt2)])

    def test_signal_status(self):
        class Ignore(Recorder):
            """
            Records the value if it has not been accepted yet, but doesn't modify the Signal.
            """

            def __init__(self):
                Recorder.__init__(self, Value(0).schema)

            def on_next(self, signal: Emitter.Signal, value: Value):
                if signal.is_blockable() and not signal.is_accepted():
                    Recorder.on_next(self, signal, value)

        class Accept(Recorder):
            """
            Always records the value, and accepts the Signal.
            """

            def __init__(self):
                Recorder.__init__(self, Value(0).schema)

            def on_next(self, signal: Emitter.Signal, value: Value):
                Recorder.on_next(self, signal, value)
                if signal.is_blockable():
                    signal.accept()

        class Blocker(Recorder):
            """
            Always records the value, and blocks the Signal.
            """

            def __init__(self):
                Recorder.__init__(self, Value(0).schema)

            def on_next(self, signal: Emitter.Signal, value: Value):
                Recorder.on_next(self, signal, value)
                signal.block()
                signal.block()  # again ... for coverage

        class Distributor(NumberEmitter):
            def _emit(self, receivers: List['Receiver'], value: Value):
                signal = Emitter.Signal(self, is_blockable=True)
                for receiver in receivers:
                    receiver.on_next(signal, value)
                    if signal.is_blocked():
                        return

        distributor: Distributor = Distributor()
        ignore1: Ignore = Ignore()  # records all values
        accept1: Accept = Accept()  # records and accepts all values
        ignore2: Ignore = Ignore()  # should not record any since all are accepted
        accept2: Accept = Accept()  # should record the same as accept1
        block1: Blocker = Blocker()  # should record the same as accept1
        block2: Blocker = Blocker()  # should not record any values

        # order matters here, as the Receivers are called in the order they connected
        ignore1.connect_to(distributor)
        accept1.connect_to(distributor)
        ignore2.connect_to(distributor)
        accept2.connect_to(distributor)
        block1.connect_to(distributor)
        block2.connect_to(distributor)

        for x in range(1, 5):
            distributor.emit(x)

        self.assertEqual([value.as_number() for value in ignore1.values], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in accept1.values], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in ignore2.values], [])
        self.assertEqual([value.as_number() for value in accept2.values], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in block1.values], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in block2.values], [])

    def test_connection_during_emit(self):
        class ConnectOther(Recorder):
            """
            Connects another Recorder when receiving a value.
            """

            def __init__(self, emitter: Emitter, other: Recorder):
                Recorder.__init__(self, Value(0).schema)
                self._emitter = emitter
                self._other = other

            def on_next(self, signal: Emitter.Signal, value: Value):
                self._other.connect_to(self._emitter)
                Recorder.on_next(self, signal, value)

        emt: Emitter = NumberEmitter()
        rec2 = Recorder(Value(0).schema)
        rec1 = ConnectOther(emt, rec2)
        rec1.connect_to(emt)

        self.assertEqual(len(rec1.values), 0)
        self.assertEqual(len(rec2.values), 0)

        self.assertEqual(len(emt._receivers), 1)
        emt.emit(45)
        self.assertEqual(len(emt._receivers), 2)
        self.assertEqual(len(rec1.values), 1)
        self.assertEqual(rec1.values[0].as_number(), 45)
        self.assertEqual(len(rec2.values), 0)

        emt.emit(89)
        self.assertEqual(len(rec1.values), 2)
        self.assertEqual(rec1.values[1].as_number(), 89)
        self.assertEqual(len(rec2.values), 1)
        self.assertEqual(rec2.values[0].as_number(), 89)

    def test_disconnect_during_emit(self):
        class DisconnectOther(Recorder):
            """
            Disconnects another Recorder when receiving a value.
            """

            def __init__(self, emitter: Emitter, other: Recorder):
                Recorder.__init__(self, Value(0).schema)
                self._emitter = emitter
                self._other = other

            def on_next(self, signal: Emitter.Signal, value: Value):
                self._other.disconnect_from(self._emitter)
                Recorder.on_next(self, signal, value)

        emt: Emitter = NumberEmitter()
        rec2 = Recorder(Value(0).schema)
        rec1 = DisconnectOther(emt, rec2)
        rec1.connect_to(emt)
        rec2.connect_to(emt)

        self.assertEqual(len(rec1.values), 0)
        self.assertEqual(len(rec2.values), 0)

        self.assertEqual(len(emt._receivers), 2)
        emt.emit(45)
        self.assertEqual(len(emt._receivers), 1)
        self.assertEqual(len(rec1.values), 1)
        self.assertEqual(rec1.values[0].as_number(), 45)
        self.assertEqual(len(rec2.values), 1)
        self.assertEqual(rec2.values[0].as_number(), 45)

        emt.emit(89)
        self.assertEqual(len(rec1.values), 2)
        self.assertEqual(rec1.values[1].as_number(), 89)
        self.assertEqual(len(rec2.values), 1)

    def test_removal_during_emit(self):
        class RemoveOther(Recorder):
            """
            Removes another Recorder when receiving a value.
            """

            def __init__(self, emitter: Emitter):
                Recorder.__init__(self, Value(0).schema)
                self._emitter = emitter
                self._other = Recorder(Value(0).schema)
                self._other.connect_to(self._emitter)

            def on_next(self, signal: Emitter.Signal, value: Value):
                self._other = None
                Recorder.on_next(self, signal, value)

        emt: Emitter = NumberEmitter()
        rec = RemoveOther(emt)
        rec.connect_to(emt)

        self.assertEqual(len(emt._receivers), 2)
        emt.emit(30)
        self.assertEqual(len(emt._receivers), 2)  # although one of them should have expired by now

        emt.emit(93)
        self.assertEqual(len(emt._receivers), 1)

    def test_sorting(self):
        class NamedReceiver(Receiver):

            class Status(Enum):
                NOT_CALLED = 0
                FIRST = 1
                ACCEPTED = 2

            def __init__(self, name: str):
                Receiver.__init__(self, Value(0).schema)
                self.name: str = name
                self.status = self.Status.NOT_CALLED

            def on_next(self, signal: Emitter.Signal, value: Value):
                if not signal.is_accepted():
                    self.status = self.Status.FIRST
                    signal.accept()
                else:
                    self.status = self.Status.ACCEPTED
                    signal.block()

        class SortByName(NumberEmitter):

            def _emit(self, receivers: List['Receiver'], value: Value):
                # split the receivers into sortable and un-sortable
                named_recs = []
                other_recs = []
                for rec in receivers:
                    if isinstance(rec, NamedReceiver):
                        named_recs.append(rec)
                    else:
                        other_recs.append(rec)
                named_recs: List = sorted(named_recs, key=lambda x: x.name)

                # re-apply the order back to the Emitter
                self._sort_receivers([receivers.index(x) for x in (named_recs + other_recs)])

                # emit to the sorted Receivers first
                signal = Emitter.Signal(self, is_blockable=True)
                for named_rec in named_recs:
                    named_rec.on_next(signal, value)
                    if signal.is_blocked():
                        return

                # unsorted Receivers should not be able to block the emitting process
                signal = Emitter.Signal(self, is_blockable=False)
                for other_rec in other_recs:
                    other_rec.on_next(signal, value)

        a: NamedReceiver = NamedReceiver("a")
        b: NamedReceiver = NamedReceiver("b")
        c: NamedReceiver = NamedReceiver("c")
        d: NamedReceiver = NamedReceiver("d")
        e: Recorder = Recorder(Value(0).schema)

        # connect in random order
        emt: Emitter = SortByName()
        for receiver in (e, c, b, a, d):
            receiver.connect_to(emt)
        del receiver

        emt.emit(938)
        self.assertEqual(a.status, NamedReceiver.Status.FIRST)
        self.assertEqual(b.status, NamedReceiver.Status.ACCEPTED)
        self.assertEqual(c.status, NamedReceiver.Status.NOT_CALLED)
        self.assertEqual(d.status, NamedReceiver.Status.NOT_CALLED)
        self.assertEqual(len(e.values), 0)
        self.assertEqual(emt._receivers, [weak(x) for x in [a, b, c, d, e]])

        # delete all but b and reset
        del a
        b.status = NamedReceiver.Status.NOT_CALLED
        del c
        del d

        emt.emit(34)
        self.assertEqual(b.status, NamedReceiver.Status.FIRST)
        self.assertEqual(len(e.values), 1)

    def test_apply_invalid_order(self):
        emt = NumberEmitter()
        _ = record(emt)
        _ = record(emt)
        _ = record(emt)

        with self.assertRaises(RuntimeError):
            emt._sort_receivers([])  # empty
        with self.assertRaises(RuntimeError):
            emt._sort_receivers([0, 1])  # too few indices
        with self.assertRaises(RuntimeError):
            emt._sort_receivers([0, 1, 2, 3])  # too many indices
        with self.assertRaises(RuntimeError):
            emt._sort_receivers([0, 1, 1])  # duplicate indices
        with self.assertRaises(RuntimeError):
            emt._sort_receivers([1, 2, 3])  # wrong indices
        emt._sort_receivers([0, 1, 2])  # success


if __name__ == '__main__':
    unittest.main()

import unittest
import logging
from enum import Enum
from typing import List
from weakref import ref as weak

from pynotf.logic import Operator, Subscriber, Publisher
from pynotf.value import Value

from tests.utils import NumberPublisher, Recorder, record, ErrorOperation, AddConstantOperation, GroupTwoOperation, \
    ExceptionOnCompleteSubscriber, AddAnotherSubscriberSubscriber, count_live_subscribers, UnsubscribeSubscriber


########################################################################################################################
# TEST CASE
########################################################################################################################


class TestCase(unittest.TestCase):

    def setUp(self):
        logging.disable(logging.CRITICAL)

    def tearDown(self):
        logging.disable(logging.NOTSET)

    def test_publisher_schema(self):
        publisher = NumberPublisher()
        self.assertEqual(publisher.output_schema, Value(0).schema)

    def test_publishing(self):
        class Nope:
            pass

        publisher = NumberPublisher()
        recorder = record(publisher)

        publisher.publish(Value(85))
        self.assertEqual(len(recorder.values), 1)
        self.assertEqual(recorder.values[0].as_number(), 85)

        publisher.publish(Value(254))
        self.assertEqual(len(recorder.values), 2)
        self.assertEqual(recorder.values[1].as_number(), 254)

        with self.assertRaises(TypeError):
            publisher.publish(None)  # empty (wrong) schema
        with self.assertRaises(TypeError):
            publisher.publish(Value({"x": 0, "y": 0}))  # wrong schema
        with self.assertRaises(TypeError):
            publisher.publish(Nope())  # not convertible to Value

        publisher._complete()
        self.assertTrue(publisher.is_completed())
        with self.assertRaises(RuntimeError):
            publisher.publish(0)  # completed

    def test_subscribing_with_wrong_schema(self):
        with self.assertRaises(TypeError):
            publisher = NumberPublisher()
            subscriber = Recorder(Value("String").schema)
            subscriber.subscribe_to(publisher)

    def test_subscribing_to_completed_publisher(self):
        publisher = NumberPublisher()
        publisher._complete()

        subscriber = Recorder(publisher.output_schema)
        self.assertEqual(len(subscriber.completed), 0)
        subscriber.subscribe_to(publisher)
        self.assertEqual(len(subscriber.completed), 1)

    def test_exception_on_error(self):
        pub = NumberPublisher()
        sub = ExceptionOnCompleteSubscriber()
        sub.subscribe_to(pub)
        pub._error(NameError())  # should be safely ignored

    def test_exception_on_complete(self):
        pub = NumberPublisher()
        sub = ExceptionOnCompleteSubscriber()
        sub.subscribe_to(pub)
        pub._complete()  # should be safely ignored

    def test_double_subscription(self):
        publisher: Publisher = NumberPublisher()

        subscriber = Recorder(publisher.output_schema)
        self.assertEqual(len(publisher._subscribers), 0)

        subscriber.subscribe_to(publisher)
        self.assertEqual(len(publisher._subscribers), 1)

        subscriber.subscribe_to(publisher)  # is ignored
        self.assertEqual(len(publisher._subscribers), 1)

        publisher.publish(46)
        self.assertEqual(len(subscriber.values), 1)

    def test_unsubscribe(self):
        publisher: Publisher = NumberPublisher()
        subscriber = record(publisher)

        publisher.publish(98)
        self.assertEqual(len(subscriber.values), 1)
        self.assertEqual(subscriber.values[0].as_number(), 98)

        subscriber.unsubscribe_from(publisher)
        publisher.publish(367)
        self.assertEqual(len(subscriber.values), 1)
        self.assertEqual(subscriber.values[0].as_number(), 98)

        not_a_subscriber = Recorder(publisher.output_schema)
        not_a_subscriber.unsubscribe_from(publisher)

    def test_simple_operator(self):
        """
        0, 1, 2, 3 -> 7, 8, 9, 10 -> (7, 8), (9, 10) -> (7, 8), (9, 10)
        """
        publisher = NumberPublisher()
        operator = Operator(AddConstantOperation(7), GroupTwoOperation(),
                            Operator.NoOp(GroupTwoOperation().output_schema))
        operator.subscribe_to(publisher)
        recorder = record(operator)

        for x in range(4):
            publisher.publish(Value(x))

        expected = [(7, 8), (9, 10)]
        for index, value in enumerate(recorder.values):
            self.assertEqual(expected[index], (value["x"].as_number(), value["y"].as_number()))

    def test_operator_wrong_type(self):
        operator = Operator(Operator.NoOp(Value(0).schema))
        operator.on_next(Publisher.Signal(NumberPublisher()), Value("Not A Number"))

    def test_operator_error(self):
        publisher = NumberPublisher()
        operator = Operator(ErrorOperation(4))
        operator.subscribe_to(publisher)
        recorder = record(operator)

        with self.assertRaises(TypeError):
            publisher.publish("Not A Number")

        # it's not the ErrorOperation that fails, but trying to publish another value
        for x in range(10):
            publisher.publish(x)
        self.assertTrue(len(publisher.exceptions) > 0)
        self.assertTrue(operator.is_completed())

        expected = [0, 1, 2, 3]
        self.assertEqual(len(recorder.values), len(expected))
        for i in range(len(expected)):
            self.assertEqual(expected[i], recorder.values[i].as_number())

    def test_no_empty_operator(self):
        with self.assertRaises(ValueError):
            _ = Operator()

    def test_mismatched_operations(self):
        with self.assertRaises(TypeError):
            Operator(GroupTwoOperation(), AddConstantOperation(7))

    def test_subscriber_lifetime(self):
        publisher: Publisher = NumberPublisher()
        sub1 = record(publisher)
        sub2 = record(publisher)
        sub3 = record(publisher)
        sub4 = record(publisher)
        sub5 = record(publisher)
        self.assertEqual(len(publisher._subscribers), 5)

        publisher.publish(23)
        for sub in (sub1, sub2, sub3, sub4, sub5):
            self.assertEqual(len(sub.values), 1)
            self.assertEqual(sub.values[0].as_number(), 23)
        del sub

        del sub1  # delete first
        self.assertEqual(len(publisher._subscribers), 5)  # sub1 has expired but hasn't been removed yet
        publisher.publish(54)  # remove sub1
        self.assertEqual(len(publisher._subscribers), 4)

        del sub5  # delete last
        self.assertEqual(len(publisher._subscribers), 4)  # sub5 has expired but hasn't been removed yet
        publisher.publish(25)  # remove sub5
        self.assertEqual(len(publisher._subscribers), 3)

        del sub3  # delete center
        self.assertEqual(len(publisher._subscribers), 3)  # sub3 has expired but hasn't been removed yet
        publisher.publish(-2)  # remove sub3
        self.assertEqual(len(publisher._subscribers), 2)

        publisher._complete()
        self.assertEqual(len(publisher._subscribers), 0)

    def test_expired_subscriber_on_complete(self):
        publisher: Publisher = NumberPublisher()
        sub1 = record(publisher)
        self.assertEqual(len(publisher._subscribers), 1)
        del sub1
        publisher._complete()
        self.assertEqual(len(publisher._subscribers), 0)

    def test_expired_subscriber_on_failure(self):
        publisher: Publisher = NumberPublisher()
        sub1 = record(publisher)
        self.assertEqual(len(publisher._subscribers), 1)
        del sub1
        publisher._error(ValueError())
        self.assertEqual(len(publisher._subscribers), 0)

    def test_add_subscribers(self):
        publisher: Publisher = NumberPublisher()
        subscriber: AddAnotherSubscriberSubscriber = AddAnotherSubscriberSubscriber(publisher, modulus=3)

        subscriber.subscribe_to(publisher)
        self.assertEqual(count_live_subscribers(publisher), 1)
        publisher.publish(1)
        self.assertEqual(count_live_subscribers(publisher), 2)
        publisher.publish(2)
        self.assertEqual(count_live_subscribers(publisher), 3)
        publisher.publish(3)  # activates modulus
        self.assertEqual(count_live_subscribers(publisher), 1)

        publisher.publish(4)
        publisher._complete()  # this too tries to add a subscriber, but is rejected
        self.assertEqual(count_live_subscribers(publisher), 0)

    def test_remove_subscribers(self):
        publisher: Publisher = NumberPublisher()
        sub1: Subscriber = UnsubscribeSubscriber(publisher)

        self.assertEqual(count_live_subscribers(publisher), 1)
        publisher.publish(0)
        self.assertEqual(count_live_subscribers(publisher), 0)

        sub2: Subscriber = UnsubscribeSubscriber(publisher)
        sub3: Subscriber = UnsubscribeSubscriber(publisher)
        self.assertEqual(count_live_subscribers(publisher), 2)
        publisher.publish(1)
        self.assertEqual(count_live_subscribers(publisher), 0)

    def test_signal_source(self):
        pub1: Publisher = NumberPublisher()
        pub2: Publisher = NumberPublisher()
        sub = record(pub1)
        sub.subscribe_to(pub2)

        pub1.publish(1)
        pub1.publish(2)
        pub2.publish(1)
        pub1.publish(3)
        pub2.publish(2000)

        self.assertEqual([value.as_number() for value in sub.values], [1, 2, 1, 3, 2000])
        self.assertEqual([signal.source for signal in sub.signals], [id(pub1), id(pub1), id(pub2), id(pub1), id(pub2)])

    def test_signal_status(self):
        class Ignore(Recorder):
            """
            Records the value if it has not been accepted yet, but doesn't modify the Signal.
            """

            def __init__(self):
                Recorder.__init__(self, Value(0).schema)

            def on_next(self, signal: Publisher.Signal, value: Value):
                if signal.is_blockable() and not signal.is_accepted():
                    Recorder.on_next(self, signal, value)

        class Accept(Recorder):
            """
            Always records the value, and accepts the Signal.
            """

            def __init__(self):
                Recorder.__init__(self, Value(0).schema)

            def on_next(self, signal: Publisher.Signal, value: Value):
                Recorder.on_next(self, signal, value)
                if signal.is_blockable():
                    signal.accept()

        class Blocker(Recorder):
            """
            Always records the value, and blocks the Signal.
            """

            def __init__(self):
                Recorder.__init__(self, Value(0).schema)

            def on_next(self, signal: Publisher.Signal, value: Value):
                Recorder.on_next(self, signal, value)
                signal.block()
                signal.block()  # again ... for coverage

        class Distributor(NumberPublisher):
            def _publish(self, subscribers: List['Subscriber'], value: Value):
                signal = Publisher.Signal(self, is_blockable=True)
                for subscriber in subscribers:
                    subscriber.on_next(signal, value)
                    if signal.is_blocked():
                        return

        distributor: Distributor = Distributor()
        ignore1: Ignore = Ignore()  # records all values
        accept1: Accept = Accept()  # records and accepts all values
        ignore2: Ignore = Ignore()  # should not record any since all are accepted
        accept2: Accept = Accept()  # should record the same as accept1
        block1: Blocker = Blocker()  # should record the same as accept1
        block2: Blocker = Blocker()  # should not record any values

        # order matters here, as the subscribers are called in the order they subscribed
        ignore1.subscribe_to(distributor)
        accept1.subscribe_to(distributor)
        ignore2.subscribe_to(distributor)
        accept2.subscribe_to(distributor)
        block1.subscribe_to(distributor)
        block2.subscribe_to(distributor)

        for x in range(1, 5):
            distributor.publish(x)

        self.assertEqual([value.as_number() for value in ignore1.values], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in accept1.values], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in ignore2.values], [])
        self.assertEqual([value.as_number() for value in accept2.values], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in block1.values], [1, 2, 3, 4])
        self.assertEqual([value.as_number() for value in block2.values], [])

    def test_subscription_during_publish(self):
        class SubscribeOther(Recorder):
            """
            Subscribes another Recorder when receiving a value.
            """

            def __init__(self, publisher: Publisher, other: Recorder):
                Recorder.__init__(self, Value(0).schema)
                self._publisher = publisher
                self._other = other

            def on_next(self, signal: Publisher.Signal, value: Value):
                self._other.subscribe_to(self._publisher)
                Recorder.on_next(self, signal, value)

        pub: Publisher = NumberPublisher()
        sub2 = Recorder(Value(0).schema)
        sub1 = SubscribeOther(pub, sub2)
        sub1.subscribe_to(pub)

        self.assertEqual(len(sub1.values), 0)
        self.assertEqual(len(sub2.values), 0)

        self.assertEqual(len(pub._subscribers), 1)
        pub.publish(45)
        self.assertEqual(len(pub._subscribers), 2)
        self.assertEqual(len(sub1.values), 1)
        self.assertEqual(sub1.values[0].as_number(), 45)
        self.assertEqual(len(sub2.values), 0)

        pub.publish(89)
        self.assertEqual(len(sub1.values), 2)
        self.assertEqual(sub1.values[1].as_number(), 89)
        self.assertEqual(len(sub2.values), 1)
        self.assertEqual(sub2.values[0].as_number(), 89)

    def test_unsubscribe_during_publish(self):
        class UnsubscribeOther(Recorder):
            """
            Unsubscribes another Recorder when receiving a value.
            """

            def __init__(self, publisher: Publisher, other: Recorder):
                Recorder.__init__(self, Value(0).schema)
                self._publisher = publisher
                self._other = other

            def on_next(self, signal: Publisher.Signal, value: Value):
                self._other.unsubscribe_from(self._publisher)
                Recorder.on_next(self, signal, value)

        pub: Publisher = NumberPublisher()
        sub2 = Recorder(Value(0).schema)
        sub1 = UnsubscribeOther(pub, sub2)
        sub1.subscribe_to(pub)
        sub2.subscribe_to(pub)

        self.assertEqual(len(sub1.values), 0)
        self.assertEqual(len(sub2.values), 0)

        self.assertEqual(len(pub._subscribers), 2)
        pub.publish(45)
        self.assertEqual(len(pub._subscribers), 1)
        self.assertEqual(len(sub1.values), 1)
        self.assertEqual(sub1.values[0].as_number(), 45)
        self.assertEqual(len(sub2.values), 1)
        self.assertEqual(sub2.values[0].as_number(), 45)

        pub.publish(89)
        self.assertEqual(len(sub1.values), 2)
        self.assertEqual(sub1.values[1].as_number(), 89)
        self.assertEqual(len(sub2.values), 1)

    def test_removal_during_publish(self):
        class RemoveOther(Recorder):
            """
            Removes another Recorder when receiving a value.
            """

            def __init__(self, publisher: Publisher):
                Recorder.__init__(self, Value(0).schema)
                self._publisher = publisher
                self._other = Recorder(Value(0).schema)
                self._other.subscribe_to(self._publisher)

            def on_next(self, signal: Publisher.Signal, value: Value):
                self._other = None
                Recorder.on_next(self, signal, value)

        pub: Publisher = NumberPublisher()
        sub = RemoveOther(pub)
        sub.subscribe_to(pub)

        self.assertEqual(len(pub._subscribers), 2)
        pub.publish(30)
        self.assertEqual(len(pub._subscribers), 2)  # although one of them should have expired by now

        pub.publish(93)
        self.assertEqual(len(pub._subscribers), 1)

    def test_sorting(self):
        class NamedSubscriber(Subscriber):

            class Status(Enum):
                NOT_CALLED = 0
                FIRST = 1
                ACCEPTED = 2

            def __init__(self, name: str):
                Subscriber.__init__(self, Value(0).schema)
                self.name: str = name
                self.status = self.Status.NOT_CALLED

            def on_next(self, signal: Publisher.Signal, value: Value):
                if not signal.is_accepted():
                    self.status = self.Status.FIRST
                    signal.accept()
                else:
                    self.status = self.Status.ACCEPTED
                    signal.block()

        class SortByName(NumberPublisher):

            def _publish(self, subscribers: List['Subscriber'], value: Value):
                # split the subscribers into sortable and un-sortable
                named_subs = []
                other_subs = []
                for sub in subscribers:
                    if isinstance(sub, NamedSubscriber):
                        named_subs.append(sub)
                    else:
                        other_subs.append(sub)
                named_subs: List = sorted(named_subs, key=lambda x: x.name)

                # re-apply the order back to the Publisher
                self._sort_subscribers([subscribers.index(x) for x in (named_subs + other_subs)])

                # publish to the sorted subscribers first
                signal = Publisher.Signal(self, is_blockable=True)
                for named_sub in named_subs:
                    named_sub.on_next(signal, value)
                    if signal.is_blocked():
                        return

                # unsorted Subscribers should not be able to block the publishing process
                signal = Publisher.Signal(self, is_blockable=False)
                for other_sub in other_subs:
                    other_sub.on_next(signal, value)

        a: NamedSubscriber = NamedSubscriber("a")
        b: NamedSubscriber = NamedSubscriber("b")
        c: NamedSubscriber = NamedSubscriber("c")
        d: NamedSubscriber = NamedSubscriber("d")
        e: Recorder = Recorder(Value(0).schema)

        # subscribe in random order
        pub: Publisher = SortByName()
        for subscriber in (e, c, b, a, d):
            subscriber.subscribe_to(pub)
        del subscriber

        pub.publish(938)
        self.assertEqual(a.status, NamedSubscriber.Status.FIRST)
        self.assertEqual(b.status, NamedSubscriber.Status.ACCEPTED)
        self.assertEqual(c.status, NamedSubscriber.Status.NOT_CALLED)
        self.assertEqual(d.status, NamedSubscriber.Status.NOT_CALLED)
        self.assertEqual(len(e.values), 0)
        self.assertEqual(pub._subscribers, [weak(x) for x in [a, b, c, d, e]])

        # delete all but b and reset
        del a
        b.status = NamedSubscriber.Status.NOT_CALLED
        del c
        del d

        pub.publish(34)
        self.assertEqual(b.status, NamedSubscriber.Status.FIRST)
        self.assertEqual(len(e.values), 1)

    def test_apply_invalid_order(self):
        pub = NumberPublisher()
        _ = record(pub)
        _ = record(pub)
        _ = record(pub)

        with self.assertRaises(RuntimeError):
            pub._sort_subscribers([])  # empty
        with self.assertRaises(RuntimeError):
            pub._sort_subscribers([0, 1])  # too few indices
        with self.assertRaises(RuntimeError):
            pub._sort_subscribers([0, 1, 2, 3])  # too many indices
        with self.assertRaises(RuntimeError):
            pub._sort_subscribers([0, 1, 1])  # duplicate indices
        with self.assertRaises(RuntimeError):
            pub._sort_subscribers([1, 2, 3])  # wrong indices
        pub._sort_subscribers([0, 1, 2])  # success


if __name__ == '__main__':
    unittest.main()

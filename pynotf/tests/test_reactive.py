import unittest
import logging
from enum import Enum
from typing import ClassVar, Optional, List
from pynotf.reactive import Operator, Subscriber, Publisher
from pynotf.structured_value import StructuredValue


class NumberPublisher(Publisher):
    def __init__(self):
        Publisher.__init__(self, StructuredValue(0).schema)


class AddConstant(Operator.Operation):
    _schema: ClassVar[StructuredValue.Schema] = StructuredValue(0).schema

    def __init__(self, addition: float):
        self._constant = addition

    @property
    def input_schema(self) -> StructuredValue.Schema:
        return self._schema

    @property
    def output_schema(self) -> StructuredValue.Schema:
        return self._schema

    def __call__(self, value: StructuredValue) -> StructuredValue:
        return value.modified().set(value.as_number() + self._constant)


class GroupTwo(Operator.Operation):
    _input_schema: ClassVar[StructuredValue.Schema] = StructuredValue(0).schema
    _output_prototype: ClassVar[StructuredValue] = StructuredValue({"x": 0, "y": 0})

    def __init__(self):
        self._last_value: Optional[float] = None

    @property
    def input_schema(self) -> StructuredValue.Schema:
        return self._input_schema

    @property
    def output_schema(self) -> StructuredValue.Schema:
        return self._output_prototype.schema

    def __call__(self, value: StructuredValue) -> Optional[StructuredValue]:
        if self._last_value is None:
            self._last_value = value.as_number()
        else:
            result = self._output_prototype.modified().set({"x": self._last_value, "y": value.as_number()})
            self._last_value = None
            return result


class ErrorOperator(Operator.Operation):
    """
    An Operation that raises a ValueError if a certain number is passed.
    """
    _schema: ClassVar[StructuredValue.Schema] = StructuredValue(0).schema

    def __init__(self, err_on_number: float):
        self._err_on_number: float = err_on_number

    @property
    def input_schema(self) -> StructuredValue.Schema:
        return self._schema

    @property
    def output_schema(self) -> StructuredValue.Schema:
        return self._schema

    def __call__(self, value: StructuredValue) -> Optional[StructuredValue]:
        if value.as_number() == self._err_on_number:
            raise ValueError("The error condition has occurred")
        return value


class Recorder(Subscriber):
    def __init__(self, schema: StructuredValue.Schema):
        super().__init__(schema)
        self.values: List[StructuredValue] = []
        self.signals: List[Publisher.Signal] = []
        self.completed: List[int] = []

    def on_next(self, signal: Publisher.Signal, value: StructuredValue):
        self.values.append(value)
        self.signals.append(signal)

    def on_complete(self, signal: Publisher.Signal):
        Subscriber.on_complete(self, signal)  # just for coverage
        self.completed.append(signal.source)


def record(publisher: [Publisher, Operator]) -> Recorder:
    recorder = Recorder(publisher.output_schema)
    recorder.subscribe_to(publisher)
    return recorder


class Nope:
    pass


coord2d_element = {"x": 0, "y": 0}
Coord2D = StructuredValue(coord2d_element)


#######################################################################################################################


class TestCase(unittest.TestCase):

    def setUp(self):
        logging.disable(logging.CRITICAL)

    def tearDown(self):
        logging.disable(logging.NOTSET)

    def test_publisher_schema(self):
        publisher = NumberPublisher()
        self.assertEqual(publisher.output_schema, StructuredValue(0).schema)

    def test_publishing(self):
        publisher = NumberPublisher()
        recorder = record(publisher)

        publisher.publish(StructuredValue(85))
        self.assertEqual(len(recorder.values), 1)
        self.assertEqual(recorder.values[0].as_number(), 85)

        publisher.publish(StructuredValue(254))
        self.assertEqual(len(recorder.values), 2)
        self.assertEqual(recorder.values[1].as_number(), 254)

        with self.assertRaises(TypeError):
            publisher.publish(None)  # empty (wrong) schema
        with self.assertRaises(TypeError):
            publisher.publish(StructuredValue(coord2d_element))  # wrong schema
        with self.assertRaises(TypeError):
            publisher.publish(Nope())  # not convertible to structured value

        publisher.complete()
        with self.assertRaises(RuntimeError):
            publisher.publish(0)  # completed

    def test_subscribing_with_wrong_schema(self):
        with self.assertRaises(TypeError):
            publisher = NumberPublisher()
            subscriber = Recorder(StructuredValue("String").schema)
            subscriber.subscribe_to(publisher)

    def test_subscribing_to_completed_publisher(self):
        publisher = NumberPublisher()
        publisher.complete()

        subscriber = Recorder(publisher.output_schema)
        self.assertEqual(len(subscriber.completed), 0)
        subscriber.subscribe_to(publisher)
        self.assertEqual(len(subscriber.completed), 1)

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
        0, 1, 2, 3 -> 7, 8, 9, 10 -> (7, 8), (9, 10)
        """
        publisher = NumberPublisher()
        operator = Operator(AddConstant(7), GroupTwo())
        operator.subscribe_to(publisher)
        recorder = record(operator)

        for x in range(4):
            publisher.publish(StructuredValue(x))

        expected = [(7, 8), (9, 10)]
        for index, value in enumerate(recorder.values):
            self.assertEqual(expected[index], (value["x"].as_number(), value["y"].as_number()))

    def test_operator_error(self):
        publisher = NumberPublisher()
        operator = Operator(ErrorOperator(4))
        operator.subscribe_to(publisher)
        recorder = record(operator)

        # it's not the ErrorOperator that fails, but trying to publish another value
        with self.assertRaises(RuntimeError):
            for x in range(10):
                publisher.publish(x)
        self.assertTrue(operator.is_completed())

        expected = [0, 1, 2, 3]
        self.assertEqual(len(recorder.values), len(expected))
        for i in range(len(expected)):
            self.assertEqual(expected[i], recorder.values[i].as_number())

    def test_no_empty_operator(self):
        with self.assertRaises(ValueError):
            _ = Operator()

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

        publisher.complete()
        self.assertEqual(len(publisher._subscribers), 0)

    def test_expired_subscriber_on_complete(self):
        publisher: Publisher = NumberPublisher()
        sub1 = record(publisher)
        self.assertEqual(len(publisher._subscribers), 1)
        del sub1
        publisher.complete()
        self.assertEqual(len(publisher._subscribers), 0)

    def test_expired_subscriber_on_failure(self):
        publisher: Publisher = NumberPublisher()
        sub1 = record(publisher)
        self.assertEqual(len(publisher._subscribers), 1)
        del sub1
        publisher.error(ValueError())
        self.assertEqual(len(publisher._subscribers), 0)

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
                Recorder.__init__(self, StructuredValue(0).schema)

            def on_next(self, signal: Publisher.Signal, value: StructuredValue):
                if signal.is_blockable() and not signal.is_accepted():
                    Recorder.on_next(self, signal, value)

        class Accept(Recorder):
            """
            Always records the value, and accepts the Signal.
            """

            def __init__(self):
                Recorder.__init__(self, StructuredValue(0).schema)

            def on_next(self, signal: Publisher.Signal, value: StructuredValue):
                Recorder.on_next(self, signal, value)
                if signal.is_blockable():
                    signal.accept()

        class Blocker(Recorder):
            """
            Always records the value, and blocks the Signal.
            """

            def __init__(self):
                Recorder.__init__(self, StructuredValue(0).schema)

            def on_next(self, signal: Publisher.Signal, value: StructuredValue):
                Recorder.on_next(self, signal, value)
                signal.block()
                signal.block()  # again ... for coverage

        class Distributor(NumberPublisher):
            @staticmethod
            def _is_blockable() -> bool:
                return True

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
                Recorder.__init__(self, StructuredValue(0).schema)
                self._publisher = publisher
                self._other = other

            def on_next(self, signal: Publisher.Signal, value: StructuredValue):
                self._other.subscribe_to(self._publisher)
                Recorder.on_next(self, signal, value)

        pub: Publisher = NumberPublisher()
        sub2 = Recorder(StructuredValue(0).schema)
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
                Recorder.__init__(self, StructuredValue(0).schema)
                self._publisher = publisher
                self._other = other

            def on_next(self, signal: Publisher.Signal, value: StructuredValue):
                self._other.unsubscribe_from(self._publisher)
                Recorder.on_next(self, signal, value)

        pub: Publisher = NumberPublisher()
        sub2 = Recorder(StructuredValue(0).schema)
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
                Recorder.__init__(self, StructuredValue(0).schema)
                self._publisher = publisher
                self._other = Recorder(StructuredValue(0).schema)
                self._other.subscribe_to(self._publisher)

            def on_next(self, signal: Publisher.Signal, value: StructuredValue):
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
                Subscriber.__init__(self, StructuredValue(0).schema)
                self.name: str = name
                self.status = self.Status.NOT_CALLED

            def on_next(self, signal: Publisher.Signal, value: StructuredValue):
                if not signal.is_accepted():
                    self.status = self.Status.FIRST
                    signal.accept()
                else:
                    self.status = self.Status.ACCEPTED
                    signal.block()

        class SortByName(NumberPublisher):
            @staticmethod
            def _is_blockable() -> bool:
                return True

            @staticmethod
            def _sort_subscribers(subscribers: List['Subscriber']) -> bool:
                by_name = sorted(subscribers, key=lambda subscriber: subscriber.name)
                was_changed = (by_name == subscribers)
                subscribers.clear()
                for new in by_name:
                    subscribers.append(new)
                return was_changed

        a: NamedSubscriber = NamedSubscriber("a")
        b: NamedSubscriber = NamedSubscriber("b")
        c: NamedSubscriber = NamedSubscriber("c")
        d: NamedSubscriber = NamedSubscriber("d")
        subs = [c, b, a, d]

        # subscribe in random order
        pub: Publisher = SortByName()
        for x in subs:
            x.subscribe_to(pub)

        pub.publish(938)
        self.assertEqual(a.status, NamedSubscriber.Status.FIRST)
        self.assertEqual(b.status, NamedSubscriber.Status.ACCEPTED)
        self.assertEqual(c.status, NamedSubscriber.Status.NOT_CALLED)
        self.assertEqual(d.status, NamedSubscriber.Status.NOT_CALLED)

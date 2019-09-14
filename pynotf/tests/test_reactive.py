import unittest
import logging
from typing import ClassVar, Optional, List
from pynotf.reactive import Pipeline, Subscriber, Publisher
from pynotf.structured_value import StructuredValue


class NumberPublisher(Publisher):
    def __init__(self):
        Publisher.__init__(self, StructuredValue(0).schema)


class AddConstant(Pipeline.Operation):
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


class GroupTwo(Pipeline.Operation):
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


class ErrorOperator(Pipeline.Operation):
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
        self.completed: List[Publisher] = []

    def on_next(self, publisher: Publisher, value: Optional[StructuredValue] = None):
        self.values.append(value)

    def on_complete(self, publisher: Publisher):
        Subscriber.on_complete(self, publisher)  # just for coverage
        self.completed.append(publisher)


def record(publisher: [Publisher, Pipeline]) -> Recorder:
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

    def test_simple_pipeline(self):
        """
        0, 1, 2, 3 -> 7, 8, 9, 10 -> (7, 8), (9, 10)
        """
        publisher = NumberPublisher()
        pipeline = Pipeline(AddConstant(7), GroupTwo())
        pipeline.subscribe_to(publisher)
        recorder = record(pipeline)

        for x in range(4):
            publisher.publish(StructuredValue(x))

        expected = [(7, 8), (9, 10)]
        for index, value in enumerate(recorder.values):
            self.assertEqual(expected[index], (value["x"].as_number(), value["y"].as_number()))

    def test_pipeline_error(self):
        pipeline = Pipeline(ErrorOperator(4))
        recorder = record(pipeline)

        # it's not the ErrorOperator that fails, but trying to publish another value
        with self.assertRaises(RuntimeError):
            for x in range(10):
                pipeline.publish(x)
        self.assertTrue(pipeline.is_completed())

        expected = [0, 1, 2, 3]
        self.assertEqual(len(recorder.values), len(expected))
        for i in range(len(expected)):
            self.assertEqual(expected[i], recorder.values[i].as_number())

    def test_no_empty_pipeline(self):
        with self.assertRaises(ValueError):
            _ = Pipeline()

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

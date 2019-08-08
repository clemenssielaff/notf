import unittest
from typing import Tuple, Optional, List, Union
from pynotf.structured_buffer import Element, Schema, StructuredBuffer
from pynotf.reactive import Subscriber, Operator, Publisher, connect_each


class RecorderSubscriber(Subscriber):
    def __init__(self, schema: Optional[Schema] = None):
        super().__init__(schema)
        self.values: List[Tuple[Union[Publisher, Operator], StructuredBuffer]] = []

    def on_next(self, publisher: Publisher, value: Optional[StructuredBuffer] = None):
        self.values.append((publisher, value))


class AddOne(Operator):
    def __init__(self):
        super().__init__(Element(0).schema)

    def on_next(self, publisher: Publisher, value: Optional[StructuredBuffer] = None):
        number = value.read().as_number()
        value.write().set(number + 1)
        self.publish(value)


class PassThrough(Operator):
    def __init__(self):
        super().__init__(Element(0).schema)

    def on_next(self, publisher: Publisher, value: Optional[StructuredBuffer] = None):
        self.publish(value)


#######################################################################################################################


class TestCase(unittest.TestCase):
    def test_concept(self):
        pub = Publisher(Element("").schema)
        sub = RecorderSubscriber(Element("").schema)
        pipe = pub | sub

        pub.publish(StructuredBuffer.create("DERBNESS"))
        self.assertEqual(len(sub.values), 1)
        self.assertEqual(sub.values[0][1].read().as_string(), "DERBNESS")

        pub.publish(StructuredBuffer.create("indeed"))
        self.assertEqual(len(sub.values), 2)
        self.assertEqual(sub.values[1][1].read().as_string(), "indeed")

        pipe.is_enabled = False
        pub.publish(StructuredBuffer.create("...not"))
        self.assertEqual(len(sub.values), 2)

    def test_split_value(self):
        pub = Publisher(Element(0).schema)
        sub1 = AddOne()
        sub2 = PassThrough()
        recorder = RecorderSubscriber(Element(0).schema)
        pub | sub1 | recorder
        pub | sub2 | recorder

        pub.publish(StructuredBuffer.create(10))
        self.assertEqual(len(recorder.values), 2)
        self.assertEqual(recorder.values[0][1].read().as_number(), 11)
        self.assertEqual(recorder.values[1][1].read().as_number(), 10)

    def test_connect_each(self):
        pub1 = Publisher(Element(0).schema)
        pub2 = Publisher(Element(0).schema)
        pub3 = Publisher(Element(0).schema)
        recorder = RecorderSubscriber(Element(0).schema)

        connect_each(pub1, pub2, pub3) | recorder

        pub1.publish(StructuredBuffer.create(1.))
        pub2.publish(StructuredBuffer.create(2.))
        pub3.publish(StructuredBuffer.create(3.))

        self.assertEqual(len(recorder.values), 3)
        self.assertEqual(recorder.values[0][1].read().as_number(), 1.)
        self.assertEqual(recorder.values[1][1].read().as_number(), 2.)
        self.assertEqual(recorder.values[2][1].read().as_number(), 3.)

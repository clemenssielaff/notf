import unittest
from typing import Tuple, Optional, List, Union
from pynotf.structured_buffer import String, Number, Schema, StructuredBuffer
from pynotf.reactive import Subscriber, Operator, Publisher


class RecorderSubscriber(Subscriber):
    def __init__(self, schema: Optional[Schema] = None):
        super().__init__(schema)
        self.values: List[Tuple[Union[Publisher, Operator], StructuredBuffer]] = []

    def on_next(self, publisher: Publisher, value: Optional[StructuredBuffer] = None):
        self.values.append((publisher, value))


class AddOne(Operator):
    def __init__(self):
        super().__init__(Number().schema)

    def on_next(self, publisher: Publisher, value: Optional[StructuredBuffer] = None):
        number = value.read().as_number()
        value.write().set(number + 1)
        self.next(value)


class PassThrough(Operator):
    def __init__(self):
        super().__init__(Number().schema)

    def on_next(self, publisher: Publisher, value: Optional[StructuredBuffer] = None):
        self.next(value)


#######################################################################################################################


class TestCase(unittest.TestCase):
    def test_concept(self):
        pub = Publisher(String().schema)
        sub = RecorderSubscriber(String().schema)
        pipe = pub | sub

        pub.next(StructuredBuffer.create(String("DERBNESS")))
        self.assertEqual(len(sub.values), 1)
        self.assertEqual(sub.values[0][1].read().as_string(), "DERBNESS")

        pub.next(StructuredBuffer.create(String("indeed")))
        self.assertEqual(len(sub.values), 2)
        self.assertEqual(sub.values[1][1].read().as_string(), "indeed")

        pipe.is_enabled = False
        pub.next(StructuredBuffer.create(String("...not")))
        self.assertEqual(len(sub.values), 2)

    def test_split_value(self):
        pub = Publisher(Number().schema)
        sub1 = AddOne()
        sub2 = PassThrough()
        recorder = RecorderSubscriber(Number().schema)
        pub | sub1 | recorder
        pub | sub2 | recorder

        pub.next(StructuredBuffer.create(Number(10)))
        self.assertEqual(len(recorder.values), 2)
        self.assertEqual(recorder.values[0][1].read().as_number(), 11)
        self.assertEqual(recorder.values[1][1].read().as_number(), 10)

from typing import Tuple
from structured_buffer import String, Number
from reactive import *


class RecorderSubscriber(Subscriber):
    def __init__(self, schema: Optional[Schema] = None):
        super().__init__(schema)
        self.values: List[Tuple[Union[Publisher, Operator], StructuredBuffer]] = []

    def on_next(self, publisher: Publisher, value: Any):
        self.values.append((publisher, value))


class AddOne(Operator):
    def __init__(self):
        super().__init__(Number().schema)

    def on_next(self, publisher: Publisher, value: StructuredBuffer):
        number = value.read().as_number()
        value.write().set(number + 1)
        self.next(value)


class PassThrough(Operator):
    def __init__(self):
        super().__init__(Number().schema)

    def on_next(self, publisher: Publisher, value: StructuredBuffer):
        self.next(value)


def sanity_check():
    pub = Publisher(String().schema)
    sub = RecorderSubscriber(String().schema)
    pipe = pub | sub

    pub.next(StructuredBuffer.create(String("DERBNESS")))
    assert (len(sub.values) == 1)
    assert (sub.values[0][1].read().as_string() == "DERBNESS")

    pub.next(StructuredBuffer.create(String("indeed")))
    assert (len(sub.values) == 2)
    assert (sub.values[1][1].read().as_string() == "indeed")

    pipe.is_enabled = False
    pub.next(StructuredBuffer.create(String("...not")))
    assert (len(sub.values) == 2)


def split_value():
    pub = Publisher(Number().schema)
    sub1 = AddOne()
    sub2 = PassThrough()
    recorder = RecorderSubscriber(Number().schema)
    pipe1 = pub | sub1 | recorder
    pipe2 = pub | sub2 | recorder

    pub.next(StructuredBuffer.create(Number(10)))
    assert (len(recorder.values) == 2)
    assert (recorder.values[0][1].read().as_number() == 11)
    assert (recorder.values[1][1].read().as_number() == 10)


sanity_check()
split_value()

from typing import List
from pynotf.logic import Operator, Subscriber, Publisher
from pynotf.value import Value


class Recorder(Subscriber):
    def __init__(self, schema: Value.Schema):
        super().__init__(schema)
        self.values: List[Value] = []
        self.signals: List[Publisher.Signal] = []
        self.completed: List[int] = []
        self.errors: List[BaseException] = []

    def on_next(self, signal: Publisher.Signal, value: Value):
        self.values.append(value)
        self.signals.append(signal)

    def on_error(self, signal: Publisher.Signal, exception: BaseException):
        Subscriber.on_error(self, signal, exception)  # just for coverage
        self.errors.append(exception)

    def on_complete(self, signal: Publisher.Signal):
        Subscriber.on_complete(self, signal)  # just for coverage
        self.completed.append(signal.source)


def record(publisher: [Publisher, Operator]) -> Recorder:
    recorder = Recorder(publisher.output_schema)
    recorder.subscribe_to(publisher)
    return recorder

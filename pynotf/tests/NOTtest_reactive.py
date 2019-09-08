from typing import ClassVar, Optional, List
from pynotf.reactive import Pipeline, Subscriber, Publisher
from pynotf.structured_value import StructuredValue


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


class Recorder(Subscriber):
    def __init__(self, schema: StructuredValue.Schema):
        super().__init__(schema)
        self.values: List[StructuredValue] = []

    def on_next(self, subscriber: Publisher, value: Optional[StructuredValue] = None):
        self.values.append(value)

    def on_complete(self, subscriber: Publisher):
        print("Completed")


def main():
    pipe = Pipeline(AddConstant(5), GroupTwo())
    recorder = Recorder(StructuredValue({"x": 0, "y": 0}).schema)

    pipe.subscribe(recorder)

    for x in range(30):
        pipe.next(StructuredValue(x))
    pipe.complete()

    for index, value in enumerate(recorder.values):
        print("{}: (x: {}, y: {})".format(index, value["x"].as_number(), value["y"].as_number()))


if __name__ == '__main__':
    main()

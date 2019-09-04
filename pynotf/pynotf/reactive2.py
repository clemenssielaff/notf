from typing import Tuple, Callable, Any
from abc import ABCMeta, abstractmethod
from weakref import WeakSet


class Observable:
    def __init__(self, operations: Tuple[Callable]):
        self._operations: Tuple[Callable] = operations
        self._observers: 'WeakSet[Observer]' = WeakSet()

    def __call__(self, value: Any):
        try:
            for operation in self._operations:
                value = operation(value)
        except Exception as exception:
            self.error(exception)

        for observer in self._observers:
            observer.on_next(value)

    def error(self, exception: Exception):
        for observer in self._observers:
            observer.on_error(exception)

    def complete(self):
        for observer in self._observers:
            observer.on_complete()

    def is_observed(self) -> bool:
        return len(self._observers) != 0


class Observer(metaclass=ABCMeta):

    @abstractmethod
    def on_next(self, value: Any):
        raise NotImplementedError()

    def on_error(self, exception: Exception):
        pass

    def on_complete(self):
        pass

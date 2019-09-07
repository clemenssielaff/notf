from typing import List, Optional, Iterable, Tuple
from abc import ABCMeta, abstractmethod
from weakref import ref as weak
from .structured_buffer import StructuredBuffer


class Observable:
    """
    An Observable publishes a single value to all subscribed Observers.
    """

    def __init__(self, schema: StructuredBuffer.Schema):
        self._output_schema: StructuredBuffer.Schema = schema
        """
        Is constant. Can be the empty Schema.
        """

        self._observers: List[weak] = []
        """
        Observers are unique but still stored in a list so we can be certain of their call order, which proves
        convenient for testing.
        """

        self._is_completed: bool = False
        """
        Once an observable is completed, it will no longer publish any values.
        """

    @property
    def output_schema(self) -> StructuredBuffer.Schema:
        """
        Schema of the observable value. Can be the empty Schema if this Observable does not publish any values.
        """
        return self._output_schema

    def _iter_observers(self) -> Iterable['Observer']:
        """
        Generate all subscribed Observers while removing those that have expired (without changing the order).
        """
        if self._is_completed:
            return  # no observers for you
        for index, weak_observer in enumerate(self._observers):
            observer = weak_observer()
            while observer is None:
                del self._observers[index]
                if index < len(self._observers):
                    observer = self._observers[index]()
                else:
                    break
            else:
                yield observer

    def _complete(self):
        """
        Complete the Observable, either because it was explicitly completed or because an error has occurred.
        """
        self._observers.clear()
        self._is_completed = True

    def next(self, value: Optional[StructuredBuffer] = None):
        """
        Push the given value to all subscribed Observers.
        :param value: Value to publish.
        :raise TypeError: If the Value's Schema doesn't match.
        :raise RuntimeError: If the Observable has already completed (either normally or though an error).
        """
        if self._is_completed:
            raise RuntimeError("Cannot publish from a completed Observable")

        if value is None:
            if not self._output_schema.is_empty():
                raise TypeError("Observable does not publish a value")
        elif value.schema != self.output_schema:
            raise TypeError("Observable cannot publish a value with the wrong Schema")

        for observer in self._iter_observers():
            observer.on_next(self, value)

    def error(self, exception: Exception):
        """
        Failure method, completes the Observable.
        :param exception:   The exception that has occurred.
        """
        for observer in self._iter_observers():
            observer.on_error(self, exception)
        self._complete()

    def complete(self):
        """
        Completes the Observable successfully.
        """
        for observer in self._iter_observers():
            observer.on_complete(self)
        self._complete()

    def subscribe(self, observer: 'Observer'):
        """
        Called when an Observer wants to subscribe.
        :param observer: New Observer.
        :raise TypeError: If the Observers's Schema doesn't match.
        """
        if observer.input_schema != self.output_schema:
            raise TypeError("Cannot subscribe to an Observable with a different Schema")

        if self._is_completed:
            observer.on_complete(self)

        else:
            weak_observer = weak(observer)
            if weak_observer not in self._observers:
                self._observers.append(weak_observer)

    def unsubscribe(self, observer: 'Observer'):
        """
        Removes the given Observer from the list of subscribed Observers if it is present. If it is not subscribed,
        the call is ignored.
        :param observer: Observer to unsubscribe.
        """
        weak_observer = weak(observer)
        if weak_observer in self._observers:
            self._observers.remove(weak_observer)


class Observer(metaclass=ABCMeta):
    """
    Observers contain strong references to the Observables they subscribe to.
    An Observer can observe multiple Observables.
    """

    def __init__(self, schema: StructuredBuffer.Schema):
        """
        Constructor.
        :param schema:      Schema defining the StructuredBuffers expected by this Observable.
        """
        self._input_schema: StructuredBuffer.Schema = schema

    @property
    def input_schema(self):
        """
        The expected value type.
        """
        return self._input_schema

    @abstractmethod
    def on_next(self, observable: Observable, value: Optional[StructuredBuffer] = None):
        """
        Abstract method called by any upstream Observable.
        :param observable   The Observable publishing the value, for identification purposes only.
        :param value        Published value, can be None.
        """
        raise NotImplementedError("Observer subclass does not implement its `on_next` method")

    def on_error(self, observable: Observable, exception: BaseException):
        """
        Default implementation of the "error" method: throws the given exception.

        Generally, it is advisable to override this method and handle the error somehow, instead of just letting it
        propagate all the way through the call stack, as this could stop the connected Observable from delivering any
        more messages to other Observers.
        :param observable:  The failed Observable, for identification purposes only.
        :param exception:   The exception that has occurred.
        """
        raise exception

    def on_complete(self, observable: Observable):
        """
        Default implementation of the "complete" operation, does nothing.
        :param observable:  The completed Observable, for identification purposes only.
        """
        pass


class Pipeline(Observer, Observable):
    """
    A Pipeline is a sequence of Operations that are applied to an input value.
    Not every input is guaranteed to produce an output as Operations are free to store or ignore input values.
    If an Operation returns a value, it is passed on to the next Operation. The last Operation publishes the value to
    all Observers of the Pipeline. If an Operation does not return a value, the following Operations are not called and
    no new value is published.
    If an Operation throws an exception, the Pipeline will fail as a whole.
    Operations are not able to complete the Pipeline, other than through failure.
    """

    class Operation(metaclass=ABCMeta):
        """
        Operations are Functors that can be part of a Pipeline.
        """

        @property
        @abstractmethod
        def input_schema(self) -> StructuredBuffer.Schema:
            """
            The Schema of the input value of the Operation.
            """
            raise NotImplementedError("An Operation must provide an input Schema property")

        @property
        @abstractmethod
        def output_schema(self) -> StructuredBuffer.Schema:
            """
            The Schema of the output value of the Operation (if there is one).
            """
            raise NotImplementedError("An Operation must provide an output Schema property")

        @abstractmethod
        def __call__(self, value: StructuredBuffer) -> Optional[StructuredBuffer]:
            """
            Operation implementation.
            :param value: Input value, must conform to the Schema specified by the `input_schema` property.
            :return: Either a new output value (conforming to the Schema specified by the `output_schema` property)
                or None.
            """
            raise NotImplementedError("Operation subclass does not implement its `__call_` method")

    def __init__(self, *operations: Operation):
        if len(operations) == 0:
            raise ValueError("Cannot create a Pipeline without a single Operation")

        Observer.__init__(self, operations[0].input_schema)
        Observable.__init__(self, operations[-1].output_schema)

        self._operations: Tuple[Pipeline.Operation] = operations

    def on_next(self, observable: Observable, value: Optional[StructuredBuffer] = None):
        """
        Abstract method called by any upstream Observable.
        :param observable   The Observable publishing the value, for identification purposes only.
        :param value        Published value, can be None.
        """
        self.next(value)

    def next(self, value: Optional[StructuredBuffer] = None):
        try:
            for operation in self._operations:
                value = operation(value)
                if value is None:
                    return
        except Exception as exception:
            self.error(exception)

        Observable.next(self, value)

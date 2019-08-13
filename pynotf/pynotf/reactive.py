from typing import Union, List, Optional
from enum import Enum, unique, auto
from .structured_buffer import StructuredBuffer, Schema


class Publisher:
    """
    Pushes values downstream.
    """

    @unique
    class State(Enum):
        """
        State of the Publisher.
        The state transition diagram is pretty easy:

                            +-> FAILED
                          /
            -> RUNNING - + --> COMPLETED
        """
        RUNNING = auto()
        COMPLETED = auto()
        ERROR = auto()

    class Result:
        def __init__(self):
            self._is_completed: bool = False
            self._exception: Optional[BaseException] = None

        @property
        def is_completed(self) -> bool:
            return self._is_completed

        @property
        def exception(self) -> Optional[BaseException]:
            return self._exception

        def complete(self, exception: Optional[BaseException] = None):
            if self._is_completed:
                raise RuntimeError("Cannot complete a Publisher more than once")



    def __init__(self, schema: Optional[Schema] = None):
        """
        Constructor.
        :param schema:  Schema defining the StructuredBuffers published by this Publisher.
                        Can be empty if this Publisher only produces signals.
        """
        self.output_schema: Optional[Schema] = schema
        # is constant

        self.subscribers: List[Subscriber] = []
        # subscribers are unique but still stored in a list so we can be certain of their call order, which proved
        # convenient for testing.

        self.state: Publisher.State = Publisher.State.RUNNING
        self.exception: Optional[BaseException] = None
        # if self.exception is not None, the state must be ERROR

    def is_completed(self) -> bool:
        """
        Checks if this Publisher has already completed (either normally or though an error).
        """
        return self.state != Publisher.State.RUNNING

    def is_failed(self) -> bool:
        """
        Checks if this Publisher has completed with an error.
        """
        return self.state == Publisher.State.FAILED

    def subscribe(self, subscriber: 'Subscriber'):
        """
        Called when a Subscriber wants to subscribe to this Publisher.
        If the Subscriber's data kind does not match the Publisher's data kind, the subscription is rejected.

        :param subscriber: New Subscriber.
        :raise ValuerError: If the Subscriber's data schema doesn't match this Publisher's.
        """
        if subscriber.input_schema is not None and subscriber.input_schema != self.output_schema:
            raise ValueError("Cannot subscribe to a Publisher with a different data Schema")

        if self.is_completed():
            subscriber.on_complete(self)
            return

        if subscriber not in self.subscribers:
            self.subscribers.append(subscriber)

    def publish(self, value: StructuredBuffer = None, publisher: Optional['Publisher'] = None):
        """
        Default publishing implementation, forwards the value to all Subscribers.
        :param value:       Value to publish. If this is a Publisher without a Schema, the value will be ignored.
        :param publisher:   Publisher to identify as, defaults to `self`. Allows a Publisher to mimic another.
        """
        if self.output_schema is not None:
            if value is None:
                raise ValueError("Publisher requires a value to publish")
            if value._schema != self.output_schema:
                raise ValueError("Publisher cannot publish data with the given data Schema")

        if self.is_completed():
            raise RuntimeError("Cannot publish from a completed Publisher")

        if publisher is None:
            publisher = self

        for subscriber in self.subscribers:
            if self.output_schema is None:
                subscriber.on_next(publisher)
            else:
                # We need to create a new StructuredBuffer value for each subscriber. This way, every subscriber can
                # choose to write to its own value without affecting the others
                # Internally, they all reference the same buffer anyway and should be cheap to copy.
                new_value = StructuredBuffer(self.output_schema, value._buffer, subscriber.dictionary)
                subscriber.on_next(publisher, new_value)

    def error(self, exception: BaseException):
        """
        Fail method, completes the Publisher.
        :param exception:   The exception that has occurred.
        """
        if self.is_completed():
            return

        self.state = self.State.ERROR

        for subscriber in self.subscribers:
            subscriber.on_error(self, exception)
        self.subscribers.clear()

    def complete(self):
        """
        Completes the Publisher successfully.
        """
        if self.is_completed():
            return

        self.state = self.State.COMPLETED

        for subscriber in self.subscribers:
            subscriber.on_complete(self)
        self.subscribers.clear()

    def __or__(self, other: 'Subscriber') -> 'Pipeline':
        """
        Pipe operator.
        :param other: B in A | B
        """
        return Pipeline(self, other)


class Subscriber:

    def __init__(self, schema: Optional[Schema] = None, dictionary: Optional[StructuredBuffer.Dictionary] = None):
        """
        Constructor.
        :param schema:      Schema defining the StructuredBuffers published by this Publisher.
                            Can be None if this Subscriber only receives signals.
        :param dictionary   Dictionary used to access the given values.
        """
        self.input_schema: Optional[Schema] = schema
        self.dictionary: Optional[StructuredBuffer.Dictionary] = dictionary

    def on_next(self, publisher: Publisher, value: Optional[StructuredBuffer] = None):
        """
        Abstract method called by any upstream publisher.
        
        :param publisher    The Publisher publishing the value, for identification purposes only.
        :param value        Published value, can be None.
        """
        raise NotImplementedError("Subscriber subclass does not implement its `on_next` method")

    def on_error(self, publisher: Publisher, exception: BaseException):
        """
        Default implementation of the "error" method: throws the given exception.

        Generally, it is advisable to override this method and handle the error somehow, instead of just letting it
        propagate all the way through the call stack, as this could stop the connected Publisher from delivering any
        more messages to other Subscribers.

        :param publisher:   The Publisher publishing the value, for identification purposes only.
        :param exception:   The exception that has occurred.
        """
        raise exception

    def on_complete(self, publisher: Publisher):
        """
        Default implementation of the "complete" operation does nothing.

        :param publisher:   The Publisher publishing the value, for identification purposes only.
        """
        pass


class Operator(Subscriber, Publisher):
    def __init__(self, schema: Optional[Schema] = None):
        Subscriber.__init__(self, schema)
        Publisher.__init__(self, schema)

    def on_next(self, publisher: Publisher, value: Optional[StructuredBuffer] = None):
        raise NotImplementedError("Subscriber subclass does not implement its `on_next` method")


class Pipeline:
    class _ToggleOp(Operator):
        def __init__(self, schema: Optional[Schema]):
            super().__init__(schema)
            self.is_enabled = True

        def on_next(self, publisher: Publisher, value: Optional[StructuredBuffer] = None):
            assert (value._schema == self.input_schema)
            if not self.is_enabled:
                return
            self.publish(value, publisher)

    def __init__(self, source: Publisher, target: Union[Operator, Subscriber]):
        """
        Constructor.
        :param source: First Publisher in the Pipeline.
        :param target: First Subscriber in the Pipeline.
        """
        self.source = source
        self.toggle = self._ToggleOp(self.source.output_schema)
        self.operators: List[Union[Operator, Subscriber]] = [target]

        self.source.subscribe(self.toggle)
        self.toggle.subscribe(target)

    def __or__(self, other: 'Subscriber') -> 'Pipeline':
        """
        Pipe operator.
        :param other: B in A | B
        """
        self.operators[-1].subscribe(other)
        self.operators.append(other)
        return self

    @property
    def is_enabled(self) -> bool:
        return self.toggle.is_enabled

    @is_enabled.setter
    def is_enabled(self, value: bool):
        self.toggle.is_enabled = value


def connect_each(*publishers: [Publisher]):
    """
    Allows multiple publishers to subscribe the same subscriber downstream.
    Example:

        connect_each(pub1, pub2, pub3) | subscriber

    is the same as

        pub1.subscribe(subscriber)
        pub2.subscribe(subscriber)
        pub3.subscribe(subscriber)

    :param publishers: All publishers to subscribe.
    :return: Helper object that can be used on the left hand side of a pipe operation.
    """
    class LeftHandSide:
        def __or__(self, subscriber: Subscriber) -> None:
            """
            Pipe operator.
            """
            for publisher in publishers:
                publisher.subscribe(subscriber)
    return LeftHandSide()

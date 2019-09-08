from typing import List, Optional, Iterable, Any, Tuple
from abc import ABCMeta, abstractmethod
from weakref import ref as weak
from pynotf.structured_buffer import StructuredBuffer


class Publisher(metaclass=ABCMeta):
    """
    A Publisher keeps a list of (weak references to) Subscribers that are called when a new value is published or the
    Publisher completes, either through an error or successfully.
    This base class does not provide an implementation for `next` and does not check/restrict the type of values (if
    any) that are published.

    Note: This would usually be named an "Observable", while the Subscriber (below) would usually be called "Observer".
    See: http://reactivex.io/documentation/observable.html
    However, these names are crap. Here is a comprehensive list why:
        - An Observable actively pushes its value downstream even though the name implies that it is purely passive.
          The Observer, respectively, doesn't do much observing at all (no polling etc.) but is instead the passive one.
        - The words Observable and Observer look very similar, especially when you just glance at the words to navigate
          the code. Furthermore, the first six characters are the same, which means that there is no way to use code-
          completion without having to type more than half of the word.
        - When you add another qualifier (like "Value"), you end up with "ObservableValue" and "ValueObserver". Or
          "ValuedObservable" and "ValueObserver" if you want to keep the "Value" part up front. Both solutions are
          inferior to "ValuePublisher" and "ValueSubscriber", two words that look distinct from each other and do not
          need any grammatical artistry to make sense.
        - Lastly, with "Observable" and "Observer", there is only one verb you can use to describe what they are doing:
          "to observe" / "to being observed" (because what else can they do?). This leads to incomprehensible sentences
          like: "All Observers contain strong references to the Observables they observe, each Observer can observe
          multiple Observables while each Observable can be observed by multiple Observers".
          The same sentence with the notf naming of Publisher / Subscriber: "All Subscribers contain strong references
          to the Publishers they subscribe to, each Subscriber can subscribe to multiple Publishers while each Publisher
          can publish to multiple Subscribers". It's not great prose either way, but at least the second version doesn't
          make me want to change my profession.
    I am aware that the Publish-subscribe pattern (https://en.wikipedia.org/wiki/Publish%E2%80%93subscribe_pattern) is
    generally understood to mean something different. And while I can understand that all easy names are already used in
    the literature, I refuse to let that impact the quality of the code. Especially since the publisher-subscriber
    relationship is an apt analogy to what is really happening in code (more so than observable-observer).
    </rant>
    """

    def __init__(self):
        """
        Default constructor.
        """

        self._subscribers: List[weak] = []
        """
        Subscribers are unique but still stored in a list so we can be certain of their call order, which proves
        convenient for testing.
        """

        self._is_completed: bool = False
        """
        Once a Publisher is completed, it will no longer publish any values.
        """

    def _iter_subscribers(self) -> Iterable['Subscriber']:
        """
        Generate all active Subscribers, while removing those that have expired (without changing the order).
        """
        if self._is_completed:
            return  # no subscribers for you
        for index, weak_subscriber in enumerate(self._subscribers):
            subscriber = weak_subscriber()
            while subscriber is None:
                del self._subscribers[index]
                if index < len(self._subscribers):
                    subscriber = self._subscribers[index]()
                else:
                    break
            else:
                yield subscriber

    def _complete(self):
        """
        Complete the Publisher, either because it was explicitly completed or because an error has occurred.
        """
        self._subscribers.clear()
        self._is_completed = True

    @abstractmethod
    def next(self, value: Any):
        """
        Push the given value to all active Subscribers.
        :param value: Value to publish.
        :raise TypeError: If the value cannot be published by this Publisher.
        :raise RuntimeError: If the Publisher has already completed (either normally or though an error).
        """
        raise NotImplementedError("Subscriber must implement its `next` method")

    def error(self, exception: Exception):
        """
        Failure method, completes the Publisher.
        :param exception:   The exception that has occurred.
        """
        for subscriber in self._iter_subscribers():
            subscriber.on_error(self, exception)
        self._complete()

    def complete(self):
        """
        Completes the Publisher successfully.
        """
        for subscriber in self._iter_subscribers():
            subscriber.on_complete(self)
        self._complete()

    def subscribe(self, subscriber: 'Subscriber'):
        """
        Called when an Subscriber wants to subscribe.
        :param subscriber: New Subscriber.
        """
        if self._is_completed:
            subscriber.on_complete(self)

        else:
            weak_subscriber = weak(subscriber)
            if weak_subscriber not in self._subscribers:
                self._subscribers.append(weak_subscriber)

    def unsubscribe(self, subscriber: 'Subscriber'):
        """
        Removes the given Subscriber if it is subscribed. If not, the call is ignored.
        :param subscriber: Subscriber to unsubscribe.
        """
        weak_subscriber = weak(subscriber)
        if weak_subscriber in self._subscribers:
            self._subscribers.remove(weak_subscriber)


class Subscriber(metaclass=ABCMeta):
    """
    Subscribers contain strong references to the Publishers they subscribe to.
    An Subscriber can be subscribed to multiple Publishers.
    """

    @abstractmethod
    def on_next(self, subscriber: Publisher, value: Optional[Any]):
        """
        Abstract method called by any upstream Publisher.
        :param subscriber   The Publisher publishing the value, for identification purposes only.
        :param value        Published value, can be None.
        """
        raise NotImplementedError("Subscriber subclass does not implement its `on_next` method")

    def on_error(self, subscriber: Publisher, exception: BaseException):
        """
        Default implementation of the "error" method: throws the given exception.

        Generally, it is advisable to override this method and handle the error somehow, instead of just letting it
        propagate all the way through the call stack, as this could stop the connected Publisher from delivering any
        more messages to other Subscribers.
        :param subscriber:  The failed Publisher, for identification purposes only.
        :param exception:   The exception that has occurred.
        """
        raise exception

    def on_complete(self, subscriber: Publisher):
        """
        Default implementation of the "complete" operation, does nothing.
        :param subscriber:  The completed Publisher, for identification purposes only.
        """
        pass


class StructuredPublisher(Publisher):
    """
    An Publisher publishing a StructuredBuffer.
    Provides some additional checks
    """

    def __init__(self, schema: StructuredBuffer.Schema):
        """
        Constructor.
        :param schema: Schema of the Value published by this Publisher.
        """
        Publisher.__init__(self)

        self._output_schema: StructuredBuffer.Schema = schema
        """
        Is constant.
        """

    @property
    def output_schema(self) -> StructuredBuffer.Schema:
        """
        Schema of the published value. Can be the empty Schema if this Publisher does not publish any values.
        """
        return self._output_schema

    def next(self, value: StructuredBuffer):
        """
        Push the given value to all active Subscribers.
        :param value: Value to publish.
        :raise TypeError: If the Value's Schema doesn't match.
        :raise RuntimeError: If the Publisher has already completed (either normally or though an error).
        """
        if self._is_completed:
            raise RuntimeError("Cannot publish from a completed Publisher")

        if value.schema != self.output_schema:
            raise TypeError("Publisher cannot publish a value with the wrong Schema")

        for subscriber in self._iter_subscribers():
            subscriber.on_next(self, value)

    def subscribe(self, subscriber: 'StructuredSubscriber'):
        """
        Called when an Subscriber wants to subscribe.
        :param subscriber: New Subscriber.
        :raise TypeError: If the Subscriber's Schema doesn't match.
        """
        if subscriber.input_schema != self.output_schema:
            raise TypeError("Cannot subscribe to an Publisher with a different Schema")
        Publisher.subscribe(self, subscriber)


class StructuredSubscriber(Subscriber):

    def __init__(self, schema: StructuredBuffer.Schema):
        """
        Constructor.
        :param schema:      Schema defining the StructuredBuffers expected by this Publisher.
        """
        Subscriber.__init__(self)

        self._input_schema: StructuredBuffer.Schema = schema
        """
        Is constant.
        """

    @abstractmethod
    def on_next(self, subscriber: Publisher, value: StructuredBuffer):
        """
        Abstract method called by any upstream Publisher.
        :param subscriber   The Publisher publishing the value, for identification purposes only.
        :param value        Published value, can be None.
        """
        raise NotImplementedError("Subscriber subclass does not implement its `on_next` method")

    @property
    def input_schema(self):
        """
        The expected value type.
        """
        return self._input_schema


class Pipeline(StructuredSubscriber, StructuredPublisher):
    """
    A Pipeline is a sequence of Operations that are applied to an input value.
    Not every input is guaranteed to produce an output as Operations are free to store or ignore input values.
    If an Operation returns a value, it is passed on to the next Operation. The last Operation publishes the value to
    all Subscribers of the Pipeline. If an Operation does not return a value, the following Operations are not called
    and no new value is published.
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

        StructuredSubscriber.__init__(self, operations[0].input_schema)
        StructuredPublisher.__init__(self, operations[-1].output_schema)

        self._operations: Tuple[Pipeline.Operation] = operations

    def on_next(self, subscriber: Publisher, value: Optional[StructuredBuffer] = None):
        """
        Abstract method called by any upstream Publisher.
        :param subscriber   The Publisher publishing the value, for identification purposes only.
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

        StructuredPublisher.next(self, value)

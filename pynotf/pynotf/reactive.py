from typing import List, Optional, Iterable, Any, Tuple
from abc import ABCMeta, abstractmethod
import logging
from traceback import format_exc
from weakref import ref as weak
from pynotf.structured_value import StructuredValue


class Publisher:
    """
    A Publisher keeps a list of (weak references to) Subscribers that are called when a new value is published or the
    Publisher completes, either through an error or successfully.
    Publishers publish a single StructuredValue and contain additional checks to make sure their Subscribers are
    compatible.

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

    def __init__(self, schema: StructuredValue.Schema = StructuredValue.Schema()):
        """
        Constructor.
        :param schema: Schema of the Value published by this Publisher, defaults to the empty Schema.
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

        self._output_schema: StructuredValue.Schema = schema
        """
        Is constant.
        """

    @property
    def output_schema(self) -> StructuredValue.Schema:
        """
        Schema of the published value. Can be the empty Schema if this Publisher does not publish a meaningful value.
        """
        return self._output_schema

    def publish(self, value: Any = StructuredValue()):
        """
        Push the given value to all active Subscribers.
        :param value: Value to publish, can be empty if this Publisher does not publish a meaningful value.
        :raise TypeError: If the Value's Schema doesn't match.
        :raise RuntimeError: If the Publisher has already completed (either normally or though an error).
        """
        if self._is_completed:
            raise RuntimeError("Cannot publish from a completed Publisher")

        if not isinstance(value, StructuredValue):
            try:
                value = StructuredValue(value)
            except ValueError:
                raise TypeError("Publisher can only publish values that are implicitly convertible to StructuredValues")

        if value.schema != self.output_schema:
            raise TypeError("Publisher cannot publish a value with the wrong Schema")

        for subscriber in self._iter_subscribers():
            subscriber.on_next(self, value)

    def error(self, exception: Exception):
        """
        Failure method, completes the Publisher.
        :param exception:   The exception that has occurred.
        """
        logging.error(format_exc())

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

    def is_completed(self) -> bool:
        """
        Returns true iff the Publisher has been completed, either through an error or normally.
        """
        return self._is_completed

    def _subscribe(self, subscriber: 'Subscriber'):
        """
        Adds a new Subscriber to receive published values.
        :param subscriber: New Subscriber.
        :raise TypeError: If the Subscriber's input Schema does not match.
        """
        if subscriber.input_schema != self.output_schema:
            raise TypeError("Cannot subscribe to an Publisher with a different Schema")

        if self._is_completed:
            subscriber.on_complete(self)

        else:
            weak_subscriber = weak(subscriber)
            if weak_subscriber not in self._subscribers:
                self._subscribers.append(weak_subscriber)

    def _unsubscribe(self, subscriber: 'Subscriber'):
        """
        Removes the given Subscriber if it is subscribed. If not, the call is ignored.
        :param subscriber: Subscriber to unsubscribe.
        """
        weak_subscriber = weak(subscriber)
        if weak_subscriber in self._subscribers:
            self._subscribers.remove(weak_subscriber)

    def _iter_subscribers(self) -> Iterable['Subscriber']:
        """
        Generate all active Subscribers, while removing those that have expired (without changing the order).
        """
        assert (not self._is_completed)
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


class Subscriber(metaclass=ABCMeta):
    """
    Subscribers contain strong references to the Publishers they subscribe to.
    An Subscriber can be subscribed to multiple Publishers.
    """

    def __init__(self, schema: StructuredValue.Schema = StructuredValue.Schema()):
        """
        Constructor.
        :param schema:      Schema defining the StructuredBuffers expected by this Publisher.
                            Can be empty if this Subscriber does not expect a meaningful value.
        """
        self._input_schema: StructuredValue.Schema = schema
        """
        Is constant.
        """

    @property
    def input_schema(self):
        """
        The expected value type.
        """
        return self._input_schema

    @abstractmethod
    def on_next(self, publisher: Publisher, value: Optional[StructuredValue]):
        """
        Abstract method called by any upstream Publisher.
        :param publisher    The Publisher publishing the value, for identification purposes only.
        :param value        Published value, can be None.
        """
        pass

    def on_error(self, publisher: Publisher, exception: BaseException):
        """
        Default implementation of the "error" method: does nothing.
        :param publisher:   The failed Publisher, for identification purposes only.
        :param exception:   The exception that has occurred.
        """
        pass

    def on_complete(self, publisher: Publisher):
        """
        Default implementation of the "complete" operation, does nothing.
        :param publisher:   The completed Publisher, for identification purposes only.
        """
        pass

    def subscribe_to(self, publisher: Publisher):
        """
        Subscribe to the given Publisher.
        :param publisher:   Publisher to subscribe to. If this Subscriber is already subscribed, this does nothing.
        :raise TypeError:   If the Publishers's input Schema does not match.
        """
        publisher._subscribe(self)

    def unsubscribe_from(self, publisher: Publisher):
        """
        Unsubscribes from the given Publisher.
        :param publisher:   Publisher to unsubscribe from. If this Subscriber is not subscribed, this does nothing.
        """
        publisher._unsubscribe(self)


class Pipeline(Subscriber, Publisher):
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
        def input_schema(self) -> StructuredValue.Schema:
            """
            The Schema of the input value of the Operation.
            """
            pass

        @property
        @abstractmethod
        def output_schema(self) -> StructuredValue.Schema:
            """
            The Schema of the output value of the Operation (if there is one).
            """
            pass

        @abstractmethod
        def __call__(self, value: StructuredValue) -> Optional[StructuredValue]:
            """
            Operation implementation.
            :param value: Input value, must conform to the Schema specified by the `input_schema` property.
            :return: Either a new output value (conforming to the Schema specified by the `output_schema` property)
                or None.
            """
            pass

    def __init__(self, *operations: Operation):
        if len(operations) == 0:
            raise ValueError("Cannot create a Pipeline without a single Operation")

        self._operations: Tuple[Pipeline.Operation] = operations
        """
        Tuple of Operations in order of execution.
        """

        Subscriber.__init__(self, self.input_schema)
        Publisher.__init__(self, self.output_schema)

    @property
    def input_schema(self) -> StructuredValue.Schema:
        """
        The Schema of the input value of the Pipeline.
        """
        return self._operations[0].input_schema

    @property
    def output_schema(self) -> StructuredValue.Schema:
        """
        The Schema of the output value of the Pipeline.
        """
        return self._operations[-1].output_schema

    def on_next(self, publisher: Publisher, value: Optional[StructuredValue] = None):
        """
        Abstract method called by any upstream Publisher.
        :param publisher   The Publisher publishing the value, for identification purposes only.
        :param value        Published value, can be None.
        """
        self.publish(value)

    def publish(self, value: Optional[Any] = None):
        if not (value is None or isinstance(value, StructuredValue)):
            value = StructuredValue(value)
        try:
            for operation in self._operations:
                value = operation(value)
                if value is None:
                    return
        except Exception as exception:
            self.error(exception)

        Publisher.publish(self, value)


"""
Additional Thoughts
===================

Scheduling
----------
When the application executor schedules a new value to be published, it could determine the effect of the change by
traversing the dependency DAG prior to the actual change. Take the following DAG topology:

        +-> B ->+
        |       |  
    A --+       +-> D
        |       |
        +-> C ->+ 

Whenever A changes, it will update B, which in turn will update D. A will then continue to update C, which will in turn
update D a second time. This would have been avoided if we scheduled the downstream propagation and would have updated B
and C before D.
While this particular example could be fixed by simply doing a breadth-first traversal instead of a depth-first one, 
this solution does not work in the general case, as can be seen with the following example:

        +-> B --> E ->+
        |             |  
    A --+             +-> D
        |             |
        +-> C ------->+ 

In order to guarantee the minimum amount of work, we will need a scheduler.
However, this is an optimization only - even the worst possible execution order will still produce the correct result
eventually.
"""

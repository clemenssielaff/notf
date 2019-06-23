from typing import Any
from enum import Enum, unique, auto


class Publisher:

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

    def __init__(self):
        self.subscribers = set()
        self.status: 'State' = self.State.RUNNING

    def is_completed(self) -> bool:
        """
        Checks if this Publisher has already completed (either normally or though an error).
        """
        return self.status != self.State.RUNNING

    def is_failed(self) -> bool:
        """
        Checks if this Publisher has completed with an error.
        """
        return self.status == self.State.FAILED

    def subscribe(self, subscriber: 'Subscriber') -> bool:
        """
        Called when a Subscriber wants to subscribe to this Publisher.
        If the Subscriber's data type does not match the Publisher's data type, the subscription is rejected.

        :param subscriber: New Subscriber.
        :returns:          True iff the Subscriber was added, false if it was rejected.
        """
        raise NotImplementedError("Publisher subclass does not implement its `subscribe` method")

    def get_subscriber_count(self) -> int:
        """
        :return: Number of connected Subscribers.
        """
        return len(self.subscribers)


class Subscriber:

    def on_next(self, publisher: Publisher, value: Any = None):
        """
        Abstract method called by any upstream publisher.
        
        :param publisher    The Publisher publishing the value, for identification purposes only.
        :param value        Published value.
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


class Relay(Subscriber, Publisher):
    pass


class Pipeline:
    pass

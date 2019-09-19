from threading import Thread, current_thread, Lock
from typing import Any
from time import sleep
import asyncio
from functools import partial
from pynotf.reactive import Subscriber, Publisher
from pynotf.structured_value import StructuredValue


class Executor:
    def __init__(self):
        self._loop: asyncio.AbstractEventLoop = asyncio.new_event_loop()
        self._lock: Lock = Lock()
        self._thread: Thread = Thread(target=self._run)
        self._thread.start()

    def schedule(self, *args, **kwargs):
        assert current_thread() != self._thread, "Do not call Executor.schedule from the Executor's own thread"
        with self._lock:
            if self._loop.is_running():
                self._loop.call_soon_threadsafe(partial(*args, **kwargs))

    def stop(self):
        with self._lock:
            self._loop.call_soon_threadsafe(self._loop.stop)
            self._thread.join()

    def _run(self):
        asyncio.set_event_loop(self._loop)
        try:
            self._loop.run_forever()
        finally:
            self._loop.run_until_complete(self._loop.shutdown_asyncgens())
            self._loop.close()


class Fact(Publisher):
    """
    notf Facts represent changeable, external truths that the Executor logic can react to.
    To the Executor logic they appear as simple Publishers that are owned and managed by a single Service in a
    thread-safe fashion. The service will update the Fact as new information becomes available, completes the Fact's
    Publisher when the Service is stopped or the Fact has expired (for example, when a sensor is disconnected) and
    forwards appropriate exceptions to the Subscribers of the Fact should an error occur.
    Examples of Facts are:
        - the latest reading of a sensor
        - the user performed a mouse click (incl. position, button and modifiers)
        - the complete chat log
        - the expiration signal of a timer
    Facts usually consist of a Value, but can also exist without one (in which case the None Value is used).
    Empty Fact act as a signal informing the logic that an event has occurred but without additional information.
    """

    def __init__(self, executor: Executor, schema: StructuredValue.Schema = StructuredValue.Schema()):
        Publisher.__init__(self, schema)
        self._executor: Executor = executor

    def publish(self, value: Any = StructuredValue()):
        """
        Push the given value to all active Subscribers.
        :param value: Value to publish, can be empty if this Publisher does not publish a meaningful value.
        """
        self._executor.schedule(Publisher.publish, self, value)

    def error(self, exception: Exception):
        """
        Failure method, completes the Publisher.
        :param exception:   The exception that has occurred.
        """
        self._executor.schedule(Publisher.error, self)

    def complete(self):
        """
        Completes the Publisher successfully.
        """
        self._executor.schedule(Publisher.complete, self)


class StringFact(Fact):
    def __init__(self, executor: Executor):
        Fact.__init__(self, executor, StructuredValue("").schema)


class Printer(Subscriber):
    def __init__(self):
        super().__init__(StructuredValue("").schema)

    def on_next(self, publisher: Publisher, value: StructuredValue):
        print(value.as_string())


class Tester:
    def __init__(self, app: Executor):
        self._executor: Executor = app
        self._thread: Thread = Thread(target=self._run)
        self._thread.start()

    def _run(self):
        pub = StringFact(self._executor)
        sub = Printer()
        sub.subscribe_to(pub)

        for x in range(3):
            pub.publish("Hello number {}".format(x))
            sleep(1)

    def join(self):
        self._thread.join()


if __name__ == '__main__':
    def main():
        app = Executor()
        tester = Tester(app)
        tester.join()
        app.stop()
        print("Done")


    main()

"""
Additional Thoughts
===================



Widgets and Logic
-----------------
We have now established that Logic elements (Publisher and Subscribers) must be protected from removal while an update
is being handled. The update process is also protected from the unexpected addition of elements, but that does not 
matter for the following question, which is about the relationship between logic elements and Widgets.

First off, Widgets own their logic elements. This is elementary and one of the features of notf: a Widget is fully
defined by the combined state of its Properties, while Signals and Slots are tied to the type of Widget like methods are
to an C++ object. But do some logic elements also own their Widget? Properties should not, they are basically dumb 
values that exist in the context of a Widget but are fully unaware of it. Signals do not, they are outward facing only
and do not carry any additional state beyond that of what any Publisher has.
Slots however can (and often will) require a strong reference to the Widget that they live on. What now if we face
problem 1 from the chapter "Logic Modifications" and Widget B removes Widget C and vice-versa? 

It is clear that an immediate removal is out of the question (see above). We still need to remove them, but only when it
is absolutely safe to do so. And the only time during an update process when it is safe to do so is at the very end.
At the moment, the Widget is flagged for deletion, a strong reference to it is added to a set of "to delete" Widgets in
the Graph. Then, at the very end of the update, we discard every Widget that has an ancestor in the "to delete" set and
delete the rest. This way, we keep the re-layouting to a minimum. 

Note that it is not possible to check whether a Widget is going to be removed at the end of the update process or not,
even though such a method would be easy enough to implement. The problem is the same as with an immediate removal. 
Depending on the execution order, the flag of Widget A would either say "this Widget is going to be deleted" if B 
was updated first or "this Widget is not scheduled for removal" if A is updated first. The result is not wrong, but
essentially useless.
The same goes for anything else that might be used as "removal light", like removing the Widget as child of the parent
so it does not get found anymore when the hierarchy is traversed.

Properties
----------
Widget Properties are Operators. Instead of having a fixed min- or max or validation function, we can simply define them
as Operators with a list of Operations that are applied to each new input.
"""

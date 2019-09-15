from threading import Thread, current_thread, Lock
from typing import Any
from time import sleep
import asyncio
from functools import partial
from pynotf.reactive import Subscriber, Publisher
from pynotf.structured_value import StructuredValue as Value


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

    def __init__(self, executor: Executor, schema: Value.Schema = Value.Schema()):
        Publisher.__init__(self, schema)
        self._executor: Executor = executor

    def publish(self, value: Any = Value()):
        """
        Push the given value to all active Subscribers.
        :param value: Value to publish, can be empty if this Publisher does not publish a meaningful value.
        """
        self._executor.call(Publisher.publish, self, value)

    def error(self, exception: Exception):
        """
        Failure method, completes the Publisher.
        :param exception:   The exception that has occurred.
        """
        self._executor.call(Publisher.error, self)

    def complete(self):
        """
        Completes the Publisher successfully.
        """
        self._executor.call(Publisher.complete, self)


class StringFact(Fact):
    def __init__(self, executor: Executor):
        Fact.__init__(self, executor, Value("").schema)


class Printer(Subscriber):
    def __init__(self):
        super().__init__(Value("").schema)

    def on_next(self, publisher: Publisher, value: Value):
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


Logic Modifications
-------------------
Unlike static DAGs, we allow Nodes to modify the DAG "in flight". That is, while an event is processed. This can lead to
several problems.
Problem 1:
    
        +-> B 
        |
    A --+
        |
        +-> C
    
    Lets assume that B and C are Nodes that are subscribed to updates from A. B reacts by removing C and adding "B" to a
    log file, while C reacts by removing B and adding "C" to a log file. Depending on which Node receives the update
    first, the log file with either say "B" _or_ "C". Since Publishers do not adhere to any order in their Subscribers,
    the result of this setup is essentially random.

Problem 2:
    
    A +--> B
      | 
      +..> C

    B is a Node that represents a canvas-like Widget. Whenever the user clicks into the Widget, it will create a new 
    child Widget C, which an also be clicked on. Whenever the user clicks on C, it disappears.
    The problem arises when B receives an update from A, creates C and immediately subscribes C to A. Let's assume that
    this does not cause a problem with A's list of subscribers changing its size mid-iteration. It is fair to assume
    that C will be updated by A after B has been updated. Therefore, after B has been updated, the *same* click is 
    immediately forwarded to C, which causes it to disappear. In effect, C will never show up.

There are multiple ways these problems could be addressed.
Solution 1:
    Have a Scheduler determine the order of all updates beforehand. During that step, all expired Subscribers are 
    removed and a backup strong reference for all live Subscribers are stored in the Scheduler to ensure that they
    survive (which mitigates Problem 1). Additionally, when new Subscribers are added to any Publisher during the update
    process, they will not be part of the Scheduler and are simply not called.

Solution 2:
    A two-phase update. In phase 1, all Publishers weed out expired Subscribers and keep an internal copy of strong
    references to all live ones. In phase 2, this copy is then iterated and any changes to the original list of
    Subscribers do not affect the update process.

Solution 3:
    Do not allow the direct addition or removal ob subscriptions and instead record them in a buffer. This buffer is 
    then executed at the end of the update process, cleanly separating the old and the new DAG state.

As far as I can see right now, solution 1 appears to be the best. It has no downsides compared to the other ones and
centralizes the ordering of Subscribers in one place, which will also become very important when we start to think about
event-like handling of updates, where earlier Subscribers can basically decide whether they want to "accept" the event,
effectively stopping its further propagation, or if they want to pass it on to the next Subscriber.
Solution 2 is similar to the first one, but it does not allow for a centralized scheduling and instead distributes the
logic throughout all Subscribers. It also requires each Publisher to keep two lists of Subscribers around, which is
wasteful.
Solution 3 has the large disadvantage that objects are not ready to be interacted with at the point where they are
created, because they are not - only the request to create them some time later. It would be possible to add additional
methods to schedule a subscription to a yet-to-be created Publisher (for example) but that will get tedious fast and 
does nothing for general scheduling.


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
Widget Properties are Pipelines. Instead of having a fixed min- or max or validation function, we can simply define them
as Pipelines with a list of Operations that are applied to each new input.
"""
from threading import Thread
from typing import Any
from time import sleep
from inspect import iscoroutine
import asyncio
from functools import partial
from pynotf.reactive import Subscriber, Publisher
from pynotf.structured_value import StructuredValue


class LogicExecutor:

    def __init__(self):
        self._loop: asyncio.AbstractEventLoop = asyncio.new_event_loop()
        self._thread: Thread = Thread(target=self._run)
        self._cancel_all = False
        self._thread.start()

    def schedule(self, func):
        if self._loop.is_running():
            if iscoroutine(func):
                self._loop.call_soon_threadsafe(partial(self._loop.create_task, func))
            else:
                self._loop.call_soon_threadsafe(func)

    def finish(self):
        self.stop(force=False)

    def stop(self, force=True):

        async def _stop():
            self._cancel_all = force
            self._loop.stop()

        self.schedule(_stop())
        self._thread.join()

    def _run(self):
        asyncio.set_event_loop(self._loop)
        try:
            self._loop.run_forever()
        finally:
            pending_tasks = [task for task in asyncio.Task.all_tasks() if not task.done()]
            if self._cancel_all:
                for task in pending_tasks:
                    task.cancel()
            try:
                self._loop.run_until_complete(asyncio.gather(*pending_tasks))
            except asyncio.CancelledError:
                pass
            finally:
                self._loop.close()


class Fact(Publisher):
    """
    Facts represent changeable, external truths that the Application Logic can react to.
    To the Application Logic they appear as simple Publishers that are owned and managed by a single Service in a
    thread-safe fashion. The Service will update the Fact as new information becomes available, completes the Fact's
    Publisher when the Service is stopped or the Fact has expired (for example, when a sensor is disconnected) and
    forwards appropriate exceptions to the Subscribers of the Fact should an error occur.
    Examples of Facts are:
        - the latest reading of a sensor
        - the user performed a mouse click (incl. position, button and modifiers)
        - the complete chat log
        - the expiration signal of a timer
    Facts consist of a Value, possibly the empty None Value. Empty Facts simply informing the logic that an event has
    occurred without additional information.
    """

    def __init__(self, executor: LogicExecutor, schema: StructuredValue.Schema = StructuredValue.Schema()):
        Publisher.__init__(self, schema)
        self._executor: LogicExecutor = executor

    def publish(self, value: Any = StructuredValue()):
        """
        Push the given value to all active Subscribers.
        :param value: Value to publish, can be empty if this Publisher does not publish a meaningful value.
        """
        self._executor.schedule(partial(Publisher.publish, self, value))

    def error(self, exception: Exception):
        """
        Failure method, completes the Publisher.
        :param exception:   The exception that has occurred.
        """
        self._executor.schedule(partial(Publisher.error, self))

    def complete(self):
        """
        Completes the Publisher successfully.
        """
        self._executor.schedule(partial(Publisher.complete, self))


class StringFact(Fact):
    def __init__(self, executor: LogicExecutor):
        Fact.__init__(self, executor, StructuredValue("").schema)


class Printer(Subscriber):
    def __init__(self):
        super().__init__(StructuredValue("").schema)

    def on_next(self, publisher: Publisher, value: StructuredValue):
        print("Immediate message: {}".format(value.as_string()))
        asyncio.get_running_loop().create_task(print_delayed(value.as_string()))


async def print_delayed(string: str):
    await asyncio.sleep(0.5)
    print(f"Delayed message: {string}")


class Tester:
    def __init__(self, app: LogicExecutor):
        self._executor: LogicExecutor = app
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
        app = LogicExecutor()
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

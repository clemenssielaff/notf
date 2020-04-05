import asyncio
from enum import Enum, auto, unique
from inspect import iscoroutinefunction
from functools import partial
from traceback import format_exc
from logging import error as print_error
from threading import Thread, Lock


class Executor:
    """
    The Application logic runs on a separate thread that is further split up into multiple fibers. In Python, we use the
    asyncio module for this, in C++ this would be boost.fiber. Regardless, we need a mechanism for cooperative
    concurrency (not parallelism) that makes sure that only one path of execution is active at any time.
    It is the executor's job to provide this functionality.
    """

    @unique
    class _Status(Enum):
        """
        Status of the Executor with the following state transition diagram:
            --> Running --> Stopping --> Stopped
                  |                        ^
                  +------------------------+
        """
        RUNNING = 0
        STOPPING = auto()
        STOPPED = auto()

    def __init__(self):
        """
        Default constructor.
        """

        self._loop: asyncio.AbstractEventLoop = asyncio.new_event_loop()
        """
        Python asyncio event loop, used to schedule coroutines.
        """

        self._thread: Thread = Thread(target=self._run)
        """
        Logic thread, runs the Logic's event loop.
        """

        self._status = self._Status.RUNNING
        """
        Used for managing the Executor shutdown process. As soon as the `_stop` coroutine (in Executor.stop) is 
        scheduled, the status switches from RUNNING to either STOPPING or STOPPED. Only an Executor in the RUNNING
        state is able to schedule new coroutines.
        The difference between STOPPING and STOPPED is that a STOPPING Executor will wait until all of its scheduled
        coroutines have finished, whereas a STOPPED Executor will cancel everything that is still waiting and finish
        the Logic thread as soon as possible. 
        """

        self._mutex: Lock = Lock()
        """
        Mutex protecting the status, in C++ an atomic value would do.
        """

        self._thread.start()  # start the logic thread immediately

    def schedule(self, func, *args, **kwargs):
        """
        Schedule a new function or coroutine to run if the logic is running.

        :param func: Function to execute in the logic thread.
        :param args: Variadic arguments for the function.
        :param kwargs: Keyword arguments for the function.
        """
        with self._mutex:
            if self._status is not self._Status.RUNNING:
                return
        self._schedule(func, *args, **kwargs)

    def _schedule(self, func, *args, **kwargs):
        """
        Internal scheduling, is thread-safe.

        :param func: Function to execute in the logic thread.
        :param args: Variadic arguments for the function.
        :param kwargs: Keyword arguments for the function.
        """
        if iscoroutinefunction(func):
            self._loop.call_soon_threadsafe(partial(self._loop.create_task, func(*args, **kwargs)))
        else:
            self._loop.call_soon_threadsafe(partial(func, *args, **kwargs))

    def finish(self):
        """
        Finish all waiting coroutines and then stop the Executor.
        """
        self.stop(force=False)

    def stop(self, force=True):
        """
        Stop the Executor, optionally cancelling all waiting coroutines.
        :param force: If true (the default), all waiting coroutines will be cancelled.
        """
        with self._mutex:
            if self._status is not self._Status.RUNNING:
                return

            if force:
                self._status = self._Status.STOPPED
            else:
                self._status = self._Status.STOPPING

        async def _stop():
            self._loop.stop()

        self._schedule(_stop)

        self._thread.join()

    @staticmethod
    def exception_handler():
        """
        Exception handling for exceptions that escape the loop.
        Prints the exception stack trace.
        Is a public method so it can be monkey-patched at runtime (e.g. for testing).
        """
        print_error(
            "\n==================================\n"
            f"{format_exc()}"
            "==================================\n")

    def _run(self):
        """
        Logic thread code.
        """
        self._loop.set_exception_handler(lambda *args: self.exception_handler())
        asyncio.set_event_loop(self._loop)

        try:
            self._loop.run_forever()
        finally:
            pending_tasks = [task for task in asyncio.Task.all_tasks() if not task.done()]
            with self._mutex:
                cancel_all = self._status is self._Status.STOPPED
                self._status = self._Status.STOPPED
            if cancel_all:
                for task in pending_tasks:
                    task.cancel()

            # noinspection PyBroadException
            try:
                self._loop.run_until_complete(asyncio.gather(*pending_tasks))
            except asyncio.CancelledError:
                pass
            except Exception:
                self.exception_handler()
            finally:
                self._loop.close()

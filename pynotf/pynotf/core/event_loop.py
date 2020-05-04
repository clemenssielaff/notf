from __future__ import annotations

from typing import Any
from inspect import iscoroutinefunction

import curio


# EVENT LOOP ###########################################################################################################

class EventLoop:

    def __init__(self):
        self._events = curio.UniversalQueue()
        self._should_close: bool = False

    def schedule(self, event) -> None:
        self._events.put(event)

    def close(self):
        self._should_close = True
        self._events.put(None)

    def run(self):
        curio.run(self._loop)

    async def _loop(self):
        new_tasks = []
        active_tasks = set()
        finished_tasks = []

        async def finish_when_complete(coroutine, *args) -> Any:
            """
            Executes a coroutine, and automatically joins
            :param coroutine: Coroutine to execute.
            :param args: Arguments passed to the coroutine.
            :return: Returns the result of the given coroutine (retrievable on join).
            """
            try:
                task_result: Any = await coroutine(*args)
            except curio.errors.TaskCancelled:
                return None
            finished_tasks.append(await curio.current_task())
            return task_result

        while True:
            event = await self._events.get()

            # close the loop
            if self._should_close:
                for task in active_tasks:
                    await task.cancel()
                for task in active_tasks:
                    await task.join()
                return

            # execute a synchronous function first
            assert isinstance(event, tuple) and len(event) > 0
            if iscoroutinefunction(event[0]):
                new_tasks.append(event)
            else:
                event[0](*event[1:])

            # start all tasks that might have been kicked off by a (potential) synchronous function call above
            for task_args in new_tasks:
                active_tasks.add(await curio.spawn(finish_when_complete(*task_args)))
            new_tasks.clear()

            # finally join all finished task
            for task in finished_tasks:
                assert task in active_tasks
                await task.join()
                active_tasks.remove(task)
            finished_tasks.clear()

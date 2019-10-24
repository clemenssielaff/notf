import unittest
import asyncio
from typing import List
import logging
from time import sleep
from pynotf.value import Value
from pynotf.scene import Executor, Fact
from tests.utils import record


########################################################################################################################
# TEST HELPER
########################################################################################################################

class IntFact(Fact):
    def __init__(self, executor: Executor):
        Fact.__init__(self, executor, Value(0).schema)


async def add_delayed(state: List[int], number: int):
    await asyncio.sleep(0.01)
    state.append(number)


def add_immediate(state: List[int], number: int):
    state.append(number)


async def fail_delayed():
    await asyncio.sleep(0.01)
    raise ValueError("Delayed fail")


def fail_immediate():
    raise ValueError("Immediate fail")


########################################################################################################################
# TEST CASE
########################################################################################################################


class TestCase(unittest.TestCase):

    def setUp(self):
        logging.disable(logging.CRITICAL)

    def tearDown(self):
        logging.disable(logging.NOTSET)

    def test_executor_stop(self):
        state: List[int] = []

        executor = Executor()
        executor.schedule(add_delayed, state, 1)
        executor.schedule(add_delayed, state, 2)
        executor.schedule(add_immediate, state, 3)
        executor.schedule(add_delayed, state, 4)
        executor.schedule(add_immediate, state, 5)
        executor.stop()
        executor.schedule(add_delayed, state, 6)
        executor.schedule(add_immediate, state, 7)
        sleep(0.1)

        # We call stop before any of the delayed functions have time to add to the state.
        # Therefore only the immediate functions affect it, but only up to the point where we call `stop`
        self.assertEqual(state, [3, 5])

    def test_executor_finish(self):
        state: List[int] = []

        executor = Executor()
        executor.schedule(add_delayed, state, 1)
        executor.schedule(add_delayed, state, 2)
        executor.schedule(add_immediate, state, 3)
        executor.schedule(add_delayed, state, 4)
        executor.schedule(add_immediate, state, 5)
        executor.finish()
        executor.schedule(add_delayed, state, 6)
        executor.stop(force=False)
        executor.schedule(add_immediate, state, 7)
        sleep(0.1)

        # Almost same as the test case above, but instead of calling `stop`, we call `finish`, which gives all running
        # coroutines time to finish.
        self.assertEqual(state, [3, 5, 1, 2, 4])

    def test_executor_failure(self):
        state: List[int] = []

        executor = Executor()
        original_handler = executor.exception_handler
        executor.exception_handler = lambda: state.append(0 if original_handler() is None else 0)

        executor.schedule(add_delayed, state, 1)
        executor.schedule(add_immediate, state, 2)
        executor.schedule(add_delayed, state, 3)
        executor.schedule(fail_immediate)
        executor.schedule(add_immediate, state, 4)
        executor.schedule(add_delayed, state, 5)
        executor.schedule(fail_delayed)
        executor.finish()
        executor.schedule(add_delayed, state, 6)
        executor.schedule(fail_delayed)
        executor.schedule(add_immediate, state, 7)
        executor.schedule(fail_immediate)
        sleep(0.1)

        # Whenever an exception is encountered, we add a zero to the state but continue the execution.
        self.assertEqual(state, [2, 0, 4, 1, 3, 5, 0])

    def test_fact(self):
        executor = Executor()

        fact1 = IntFact(executor)
        recorder1 = record(fact1)

        fact2 = IntFact(executor)
        recorder2 = record(fact2)
        error = RuntimeError("Nope")

        fact1.publish(2)
        fact1.publish(7)
        fact1.publish(-23)
        fact1.complete()
        fact1.publish(456)

        fact2.publish(8234)
        fact2.error(error)
        fact2.publish(-6)

        executor.finish()

        values1 = [v.as_number() for v in recorder1.values]
        self.assertEqual(values1, [2, 7, -23])
        self.assertEqual(recorder1.completed, [id(fact1)])

        values2 = [v.as_number() for v in recorder2.values]
        self.assertEqual(values2, [8234])
        self.assertEqual(recorder2.errors, [error])


if __name__ == '__main__':
    unittest.main()

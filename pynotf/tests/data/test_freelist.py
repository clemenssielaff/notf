import unittest
import random
from typing import Set

from pynotf.data import FreeList
from pynotf.data.freelist import WORD_SIZE


########################################################################################################################
# TEST CASE
########################################################################################################################


class TestCase(unittest.TestCase):

    def test_random(self):
        """
        A short list of names I would expect to be stored in a Bonsai.
        """
        free_list: FreeList = FreeList()
        self.assertEqual(free_list.count_free(), len(free_list))

        # check representation
        self.assertEqual(repr(free_list), f'FreeList({"1" * WORD_SIZE})')

        # reserving too little does nothing
        free_list.reserve(2)
        self.assertEqual(free_list.count_free(), len(free_list))

        # acquire 200 times
        for index in range(200):
            actual: int = free_list.acquire()
            self.assertEqual(index, actual)
        self.assertEqual(free_list.count_free(200), 0)

        # release a random 100 elements
        free_elements: Set[int] = set()
        for index in random.sample(range(200), 100):
            free_list.release(index)
            self.assertTrue(index not in free_elements)
            free_elements.add(index)
        self.assertEqual(free_list.count_free(200), 100)

        # acquire 50 elements
        for _ in range(50):
            actual: int = free_list.acquire()
            self.assertTrue(actual in free_elements)
            free_elements.remove(actual)
        self.assertEqual(free_list.count_free(200), 50)

        # releasing an unacquired index does nothing
        free_list.release(855)
        self.assertEqual(free_list.count_free(200), 50)

    def test_from_string(self):
        self.assertEqual(repr(FreeList.create_from_string('1')),
                         f'FreeList(1{"0" * (WORD_SIZE - 1)})')
        self.assertEqual(repr(FreeList.create_from_string('0' * WORD_SIZE)),
                         f'FreeList({"0" * WORD_SIZE}, {"1" * WORD_SIZE})')


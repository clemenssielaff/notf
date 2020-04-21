"""
Codename: ZebraList
"""
from __future__ import annotations

from typing import List, Optional, Tuple
from math import log2

from pynotf.extra import chunks

########################################################################################################################

WORD_SIZE: int = 64


class FreeList:
    """
    1 -> Free
    0 -> Used
    """

    def __init__(self, initial_size: int = 1):
        self._words: List[int] = []
        self._first_free: int = 0
        self.reserve(initial_size)

    def __repr__(self) -> str:
        # noinspection PyStringFormat
        return 'FreeList({})'.format(', '.join(f'{{0:0{WORD_SIZE}b}}'.format(word)[::-1] for word in self._words))

    def __len__(self):
        return len(self._words) * WORD_SIZE

    def __contains__(self, index: int) -> bool:
        """
        Checks if a given index is free.
        Indices that are outside of the range of the free list are considered occupied.
        """
        word_index, bit_index = self._get_word_and_bit_index(index)
        if word_index is None:
            return False
        return (self._words[word_index] >> bit_index) & 1 == 1

    @staticmethod
    def from_string(values: str) -> FreeList:
        # extend the string with ones to align it with the word size
        values = f'{values}{"1" * (0 if len(values) % WORD_SIZE == 0 else 64 - len(values) % WORD_SIZE)}'

        free_list: FreeList = FreeList(0)

        # words
        word_index = 0
        words: List[int] = [int(chunk[::-1], 2) for chunk in chunks(values, WORD_SIZE)]
        for word in words:
            if word != 0:
                break
            word_index += 1
        if word_index == len(words):
            words.append((2 ** WORD_SIZE) - 1)
        free_list._words = words

        # first free
        assert word_index < len(words)
        word: int = words[word_index]
        bit_index: int = int(log2(word & -word))
        free_list._first_free = word_index * WORD_SIZE + bit_index

        return free_list

    def count_free(self, end_index: Optional[int] = None) -> int:
        """
        This can be a lot more performant in compiled code, see here:
            https://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetNaive
        :param end_index: One past the highest index to take into account.
        :return: Number of free bits before the end index.
        """
        if end_index is None:
            return sum(bin(word).count('1') for word in self._words)

        assert end_index >= 0
        last_word_index: int = min(end_index // WORD_SIZE, len(self._words) - 1)
        last_bit_index: int = min(WORD_SIZE, end_index - (last_word_index * WORD_SIZE))
        return (sum(bin(word).count('1') for word in self._words[:last_word_index]) +
                bin(self._words[last_word_index] & ((2 ** last_bit_index) - 1)).count('1'))

    def reserve(self, size: int) -> None:
        if size > len(self._words) * WORD_SIZE:
            self._words.extend(((2 ** WORD_SIZE) - 1) for _ in range((size // WORD_SIZE) + 1 - len(self._words)))

    def acquire(self) -> int:
        result: int = self._first_free

        # unset the corresponding bit
        word_index, bit_index = self._get_word_and_bit_index(self._first_free)
        assert word_index is not None
        self._words[word_index] &= ~(1 << bit_index)

        # advance the word index to the first word with a free bit
        word_count = len(self._words)
        while self._words[word_index] == 0:
            word_index += 1

            # if all remaining words are zero, we need to add a new word
            if word_index == word_count:
                self._words.append((2 ** WORD_SIZE) - 1)
                self._first_free = word_count * WORD_SIZE
                return result

        # find the free bit and mark it as the current "first free" one
        word: int = self._words[word_index]
        bit_index = int(log2(word & -word))
        assert 0 <= bit_index < WORD_SIZE
        self._first_free = (word_index * WORD_SIZE) + bit_index

        return result

    def release(self, index: int) -> None:
        word_index, bit_index = self._get_word_and_bit_index(index)
        if word_index is None:
            return  # we haven't even stored a word for that index yet, no need to unset anything

        # update the first free index
        if index < self._first_free:
            assert (self._words[word_index] >> bit_index) & 1 == 0  # ensure that the unset bit was actually set
            self._first_free = index

        # set the bit
        self._words[word_index] |= 1 << bit_index

    def _get_word_and_bit_index(self, index: int) -> Tuple[Optional[int], int]:
        assert index >= 0

        word_index: int = index // WORD_SIZE
        if word_index >= len(self._words):
            return None, 0  # the index is not part of the free list

        bit_index: int = index - (word_index * WORD_SIZE)
        assert 0 <= bit_index < WORD_SIZE

        return word_index, bit_index

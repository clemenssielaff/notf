import unittest
from typing import List
import logging

from pyrsistent import field

from pynotf.data import Storage, Table, RowHandle, HandleError, TableRow
from pynotf.data.table import mutate_data


########################################################################################################################
# UTILS
########################################################################################################################

class TestRow(TableRow):
    __table_index__: int = 0
    name = field(type=str, mandatory=True)

class TestRow2(TableRow):
    __table_index__: int = 1
    name = field(type=str, mandatory=True)

########################################################################################################################
# TEST CASE
########################################################################################################################

class TestCase(unittest.TestCase):

    def setUp(self) -> None:
        self._previous_log_level = logging.getLogger().getEffectiveLevel()
        logging.getLogger().setLevel(logging.ERROR)

    def tearDown(self) -> None:
        logging.getLogger().setLevel(self._previous_log_level)

    def test_simple(self):
        """
        A Storage with a single, simple Table with some basic operations on it.
        """
        storage: Storage = Storage(
            properties=TestRow
        )
        self.assertEqual(len(storage), 1)

        # add the first row
        table: Table = storage[0]
        self.assertEqual(len(table), 0)
        self.assertEqual(table.get_columns(), ['name'])
        row0: RowHandle = table.add_row(name="zero")
        self.assertEqual(len(table), 1)
        self.assertEqual(row0, table.get_handle(0))
        self.assertFalse(table.get_handle(1))

        # add the second row and store its handle
        self.assertFalse(table.is_handle_valid(RowHandle(index=1, generation=1)))
        row1: RowHandle = table.add_row(name="one")
        self.assertEqual(len(table), 2)
        self.assertTrue(row1)
        self.assertFalse(row1.is_null())
        self.assertTrue(table.is_handle_valid(row1))
        self.assertEqual(row1.table, 0)
        self.assertEqual(row1.index, 1)
        self.assertEqual(row1.generation, 1)

        # store the Storage data at this point
        storage_data = storage.get_data()

        # add a third row
        row2 = table.add_row(name="two")
        self.assertEqual(len(table), 3)

        # remove the middle row
        table.remove_row(row1)
        self.assertEqual(len(table), 3)  # the size of the table does not change
        self.assertFalse(table.is_handle_valid(row1))
        self.assertTrue(row1)
        self.assertFalse(row1.is_null())
        self.assertFalse(table.get_handle(1))

        # update a cell
        self.assertEqual(table[row2]['name'], 'two')
        table[row2]['name'] = 'the new two'
        self.assertEqual(table[row2]['name'], 'the new two')

        # add a new row into the "empty" row in the table
        row3: RowHandle = table.add_row(name="three")
        self.assertEqual(len(table), 3)  # the size of the table does not change
        self.assertEqual(row1.index, row3.index)
        self.assertEqual(table[row3]['name'], 'three')
        self.assertFalse(table.is_handle_valid(row1))

        # restore the Storage data from above
        storage.set_data(storage_data)
        self.assertEqual(len(table), 2)
        self.assertEqual(table[row1]['name'], 'one')
        self.assertTrue(table.is_handle_valid(row1))

    def test_storage_names(self):
        with self.assertRaises(NameError):
            Storage(
                Properties=TestRow,
                properties=TestRow2,
            )

    def test_row_handle_equality(self):
        self.assertEqual(RowHandle(0, 1, 2), RowHandle(0, 1, 2))
        self.assertNotEqual(RowHandle(0, 1, 2), RowHandle(1, 1, 2))
        self.assertNotEqual(RowHandle(0, 1, 2), RowHandle(0, 2, 2))
        self.assertNotEqual(RowHandle(0, 1, 2), RowHandle(0, 1, 3))
        self.assertNotEqual(RowHandle(0, 1, 2), RowHandle())
        self.assertFalse(RowHandle() == "your mother")

    def test_row_handle_validity(self):
        storage: Storage = Storage(
            properties=TestRow
        )
        table: Table = storage[0]
        table.add_row(name="name")

        self.assertTrue(table.is_handle_valid(RowHandle(0, 0, 1)))
        self.assertFalse(table.is_handle_valid(RowHandle(1, 0, 1)))
        self.assertFalse(table.is_handle_valid(RowHandle(0, 1, 1)))
        self.assertFalse(table.is_handle_valid(RowHandle(0, 0, 2)))
        self.assertFalse(table.is_handle_valid(RowHandle(0, 0, 0)))

        with self.assertRaises(HandleError):
            _ = table[RowHandle(1, 0, 1)]
        with self.assertRaises(HandleError):
            _ = table[RowHandle(0, 1, 1)]
        with self.assertRaises(HandleError):
            _ = table[RowHandle(0, 0, 2)]
        with self.assertRaises(HandleError):
            _ = table[RowHandle(0, 0, 0)]

    def test_null_row_handles(self):
        self.assertTrue(RowHandle(0, 0, 1))
        self.assertTrue(RowHandle().is_null())
        self.assertFalse(RowHandle())

    def test_generation_overflow(self):
        storage: Storage = Storage(
            properties=TestRow
        )
        table: Table = storage[0]

        # add a row
        handle_original: RowHandle = table.add_row(name="original")
        self.assertTrue(len(table), 1)
        self.assertEqual(handle_original.generation, 1)

        # let's pretend we are removing and creating the same row 2**31 times
        generation_bits = RowHandle._GENERATION_SIZE - 1
        storage.set_data(mutate_data(storage.get_data(), [0, 0], lambda row: row.set('_gen', 2 ** generation_bits)))
        handle_faked: RowHandle = RowHandle(0, 0, 2 ** generation_bits)

        # remove the row a final time
        table.remove_row(handle_faked)

        # and now, re-create it again
        handle_wrapped = table.add_row(name="wrapped")
        self.assertTrue(len(table), 1)
        self.assertEqual(handle_original, handle_wrapped)  # ta-da! Wrapped.

    def test_to_string(self):
        """
        Basic test of the string representation of a table (for coverage, mostly).
        """
        storage: Storage = Storage(
            properties=TestRow
        )

        table: Table = storage[0]
        row1: RowHandle = table.add_row(name="a")
        table.add_row(name="b")
        table.remove_row(row1)

        table_string: str = storage[0].to_string()
        lines: List[str] = table_string.split('\n')
        self.assertEqual(len(lines), 8)

        self.assertIsNotNone(str(table))  # ... for coverage

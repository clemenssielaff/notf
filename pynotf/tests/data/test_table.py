import unittest
from typing import List

from pynotf.data import Storage, Table, RowHandle, HandleError
from pynotf.data.table import mutate_data


########################################################################################################################
# TEST CASE
########################################################################################################################


class TestCase(unittest.TestCase):

    def test_simple(self):
        """
        A Storage with a single, simple Table with some basic operations on it.
        """
        storage: Storage = Storage(
            properties=dict(
                name=str,
            )
        )
        self.assertEqual(len(storage), 1)

        # add the first row
        table: Table = storage[0]
        self.assertEqual(len(table), 0)
        table.add_row(name="one")
        self.assertEqual(len(table), 1)

        # add the second row and store its handle
        self.assertFalse(table.is_handle_valid(RowHandle(index=1, generation=1)))
        row2: RowHandle = table.add_row(name="two")
        self.assertEqual(len(table), 2)
        self.assertTrue(row2)
        self.assertFalse(row2.is_null())
        self.assertTrue(table.is_handle_valid(row2))
        self.assertEqual(row2.table, 0)
        self.assertEqual(row2.index, 1)
        self.assertEqual(row2.generation, 1)

        # store the Storage data at this point
        storage_data = storage.get_data()

        # add a third row
        row3 = table.add_row(name="three")
        self.assertEqual(len(table), 3)

        # remove the middle row
        table.remove_row(row2)
        self.assertEqual(len(table), 3)  # the size of the table does not change
        self.assertFalse(table.is_handle_valid(row2))
        self.assertTrue(row2)
        self.assertFalse(row2.is_null())

        # update a cell
        self.assertEqual(table[row3]['name'], 'three')
        table[row3]['name'] = 'the new two'
        self.assertEqual(table[row3]['name'], 'the new two')

        # add a new row into the "empty" row in the table
        row4: RowHandle = table.add_row(name="four")
        self.assertEqual(len(table), 3)  # the size of the table does not change
        self.assertEqual(row2.index, row4.index)
        self.assertEqual(table[row4]['name'], 'four')
        self.assertFalse(table.is_handle_valid(row2))

        # restore the Storage data from above
        storage.set_data(storage_data)
        self.assertEqual(len(table), 2)
        self.assertEqual(table[row2]['name'], 'two')
        self.assertTrue(table.is_handle_valid(row2))

    def test_storage_names(self):
        with self.assertRaises(NameError):
            Storage(
                Properties=dict(name=str),
                properties=dict(value=int)
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
            properties=dict(
                name=str,
            )
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
            properties=dict(
                name=str,
            )
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
            properties=dict(
                name=str,
                value=int,
            )
        )

        table: Table = storage[0]
        row1: RowHandle = table.add_row(name="a", value=1)
        table.add_row(name="b", value=1)
        table.remove_row(row1)

        table_string: str = storage[0].to_string()
        lines: List[str] = table_string.split('\n')
        self.assertEqual(len(lines), 8)

        self.assertIsNotNone(str(table))  # ... for coverage

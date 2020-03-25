import unittest
from typing import List

from pynotf.data import Storage, Table, RowHandle


########################################################################################################################
# TEST CASE
########################################################################################################################


class TestCase(unittest.TestCase):

    def test_simple(self):
        """
        A Storage with a single, simple Table with some basic operations on it.
        """
        storage: Storage = Storage(
            dict(
                name=str,
            )
        )
        self.assertEqual(len(storage), 1)

        # add the first row
        table = storage[0]
        self.assertEqual(len(table), 0)
        table.add_row(name="one")
        self.assertEqual(len(table), 1)

        # add the second row and store its handle
        self.assertFalse(table.is_handle_valid(RowHandle(index=1, generation=1)))
        row2: RowHandle = table.add_row(name="two")
        self.assertEqual(len(table), 2)
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

    def test_to_string(self):
        """
        Basic test of the string representation of a table (for coverage, mostly).
        """
        storage: Storage = Storage(
            dict(
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

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
        storage: Storage = Storage(dict(
            properties=dict(
                name=str,
            )
        ))
        self.assertEqual(list(storage.keys()), ["properties"])

        # add the first row
        table = storage['properties']
        self.assertEqual(len(table), 0)
        table.add_row(name="one")
        self.assertEqual(len(table), 1)

        # add the second row and store its handle
        self.assertFalse(table.is_handle_valid(RowHandle(index=1, _gen=1)))
        row_handle: RowHandle = table.add_row(name="two")
        self.assertEqual(len(table), 2)
        self.assertTrue(table.is_handle_valid(row_handle))
        self.assertEqual(row_handle['index'], 1)
        self.assertEqual(row_handle['_gen'], 1)

        # store the Storage data at this point
        storage_data = storage.get_data()

        # add a third row
        table.add_row(name="three")
        self.assertEqual(len(table), 3)

        # remove the middle row
        table.remove_row(1)
        self.assertEqual(len(table), 3)  # the size of the table does not change
        self.assertFalse(table.is_handle_valid(row_handle))

        # update a cell
        self.assertEqual(table[2]['name'], 'three')
        table[2]['name'] = 'the new two'
        self.assertEqual(table[2]['name'], 'the new two')

        # add a new row into the "empty" row in the table
        table.add_row(name="four")
        self.assertEqual(len(table), 3)  # the size of the table does not change
        self.assertEqual(table[1]['name'], 'four')
        self.assertFalse(table.is_handle_valid(row_handle))

        # restore the Storage data from above
        storage.set_data(storage_data)
        self.assertEqual(len(table), 2)
        self.assertEqual(table[1]['name'], 'two')
        self.assertTrue(table.is_handle_valid(row_handle))

    def test_invalid_handle(self):
        """
        Tables in a Storage cannot have the same name (case insensitive)
        """
        with self.assertRaises(NameError):
            Storage({
                "table": dict(),
                "Table": dict(),
            })

    def test_to_string(self):
        """
        Basic test of the string representation of a table (for coverage, mostly).
        """
        storage: Storage = Storage(dict(
            properties=dict(
                name=str,
                value=int,
            )
        ))

        table: Table = storage["properties"]
        table.add_row(name="a", value=1)
        table.add_row(name="b", value=1)
        table.remove_row(0)

        table_string: str = storage["properties"].to_string()
        lines: List[str] = table_string.split('\n')
        self.assertEqual(len(lines), 8)

        self.assertIsNotNone(str(table))  # ... for coverage

import unittest
import sys
from pynotf.structured_buffer import StructuredBuffer, Kind

test_dictionary = {
        "coordinates": [
            {
                "x": 0,
                "someName": "SUCCESS"
            },
            {
                "x": 2,
                "someName": "Hello world"
            }
        ],
        "name": "Hello again",
        "other map": {
            "key": "string",
            "a number": 847
        }
    }

original = StructuredBuffer(test_dictionary)


#######################################################################################################################


class TestCase(unittest.TestCase):
    def test_check_kind(self):
        self.assertEqual(Kind.LIST, sys.maxsize - 3)
        self.assertEqual(Kind.MAP, sys.maxsize - 2)
        self.assertEqual(Kind.NUMBER, sys.maxsize - 1)
        self.assertEqual(Kind.STRING, sys.maxsize)

    def test_check_schema(self):
        self.assertEqual(original.schema, [9223372036854775805,
                                           3,
                                           3,
                                           9223372036854775807,
                                           6,
                                           9223372036854775804,
                                           9223372036854775805,
                                           2,
                                           9223372036854775806,
                                           9223372036854775807,
                                           9223372036854775805,
                                           2,
                                           9223372036854775807,
                                           9223372036854775806])

    def test_repr(self):
        expected = """  0: Map
  1:  ↳ Size: 3
  2: → 5
  3: String
  4: → 10
  5: List
  6: Map
  7:  ↳ Size: 2
  8: Number
  9: String
 10: Map
 11:  ↳ Size: 2
 12: String
 13: Number
"""
        self.assertEqual(str(original.schema), expected)

    def test_invalid_element(self):
        with self.assertRaises(ValueError):
            StructuredBuffer(None)

    def test_valid_lists(self):
        StructuredBuffer([False, 1, 2.])  # list of numbers
        StructuredBuffer(['a', "bee", r"""zebra"""])  # list of strings
        StructuredBuffer([[1, 2], [3, 4]])  # lists of lists
        StructuredBuffer([dict(a=1, b=2), dict(a=3, b=-4)])  # lists of dicts

    def test_invalid_lists(self):
        # list cannot be empty
        with self.assertRaises(ValueError):
            StructuredBuffer([])

        # list elements must have the same schema
        with self.assertRaises(ValueError):
            StructuredBuffer(['a', 2])
        with self.assertRaises(ValueError):
            StructuredBuffer([[1, 2], ["a", "b"]])
        with self.assertRaises(ValueError):
            StructuredBuffer([dict(a=1, b=2), dict(a="1", b=2)])

        # if the list contains maps, the maps must have the same keys
        with self.assertRaises(ValueError):
            StructuredBuffer([dict(a=1, b=2), dict(a=1, c=3)])
        with self.assertRaises(ValueError):
            StructuredBuffer([dict(a=1, b=2), dict(a=1)])

    def test_invalid_map(self):
        with self.assertRaises(ValueError):
            StructuredBuffer(dict())  # maps cannot be empty
        with self.assertRaises(ValueError):
            StructuredBuffer({1: "nope"})  # maps keys must be of kind string

    def test_create_buffer_from_schema(self):
        self.assertEqual(StructuredBuffer(original.schema).buffer, [[], '', '', 0])

    def test_create_buffer_from_element(self):
        buffer = StructuredBuffer(StructuredBuffer(test_dictionary))
        self.assertEqual(buffer.schema, original.schema)

    def test_basic_get_set(self):
        copy = StructuredBuffer(original)

        self.assertEqual(original.read()["coordinates"][1]["x"].as_number(), 2.0)
        self.assertEqual(copy.read()["coordinates"][1]["x"].as_number(), 2.0)

        copy.write()["coordinates"][1]["x"].set(-123)
        self.assertEqual(original.read()["coordinates"][1]["x"].as_number(), 2.0)
        self.assertEqual(copy.read()["coordinates"][1]["x"].as_number(), -123.0)

    def test_change_subtree(self):
        copy = StructuredBuffer(original)
        self.assertEqual(len(copy.read()["coordinates"]), 2)
        self.assertEqual(copy.read()["coordinates"][0]["someName"].as_string(), "SUCCESS")

        copy.write()["coordinates"].set([dict(x=42, someName="answer")])
        self.assertEqual(len(original.read()["coordinates"]), 2)
        self.assertEqual(original.read()["coordinates"][0]["someName"].as_string(), "SUCCESS")
        self.assertEqual(len(copy.read()["coordinates"]), 1)
        self.assertEqual(copy.read()["coordinates"][0]["someName"].as_string(), "answer")

        with self.assertRaises(ValueError):
            copy.write()["coordinates"].set([{"x": 7}])

        # TODO: test that assigning the same value (ground and non-ground) really does not change the buffer

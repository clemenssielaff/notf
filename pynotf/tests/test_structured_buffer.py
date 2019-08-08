import unittest
from pynotf.structured_buffer import StructuredBuffer, Element

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

original = StructuredBuffer.create(test_dictionary)


#######################################################################################################################


class TestCase(unittest.TestCase):
    def test_check_schema(self):
        self.assertEqual(original.schema, [9223372036854775807,
                                           3,
                                           3,
                                           9223372036854775805,
                                           6,
                                           9223372036854775806,
                                           9223372036854775807,
                                           2,
                                           9223372036854775804,
                                           9223372036854775805,
                                           9223372036854775807,
                                           2,
                                           9223372036854775805,
                                           9223372036854775804])

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
            Element(None)

    def test_valid_lists(self):
        Element([False, 1, 2.])  # list of numbers
        Element(['a', "bee", r"""zebra"""])  # list of strings
        Element([[1, 2], [3, 4]])  # lists of lists
        Element([dict(a=1, b=2), dict(a=3, b=-4)])  # lists of dicts

    def test_invalid_lists(self):
        # list cannot be empty
        with self.assertRaises(ValueError):
            Element([])

        # list elements must have the same schema
        with self.assertRaises(ValueError):
            Element(['a', 2])
        with self.assertRaises(ValueError):
            Element([[1, 2], ["a", "b"]])
        with self.assertRaises(ValueError):
            Element([dict(a=1, b=2), dict(a="1", b=2)])

        # if the list contains maps, the maps must have the same keys
        with self.assertRaises(ValueError):
            Element([dict(a=1, b=2), dict(a=1, c=3)])
        with self.assertRaises(ValueError):
            Element([dict(a=1, b=2), dict(a=1)])

    def test_invalid_map(self):
        with self.assertRaises(ValueError):
            Element(dict())  # maps cannot be empty
        with self.assertRaises(ValueError):
            Element({1: "nope"})  # maps keys must be of type string

    def test_create_buffer_from_schema(self):
        self.assertEqual(StructuredBuffer.create(original.schema).buffer, [[], '', '', 0])

    def test_create_buffer_from_element(self):
        buffer = StructuredBuffer.create(Element(test_dictionary))
        self.assertEqual(buffer.schema, original.schema)

    def test_basic_get_set(self):
        copy = StructuredBuffer.create(original)

        self.assertEqual(original.read()["coordinates"][1]["x"].as_number(), 2.0)
        self.assertEqual(copy.read()["coordinates"][1]["x"].as_number(), 2.0)

        copy.write()["coordinates"][1]["x"].set(-123)
        self.assertEqual(original.read()["coordinates"][1]["x"].as_number(), 2.0)
        self.assertEqual(copy.read()["coordinates"][1]["x"].as_number(), -123.0)

    def test_change_subtree(self):
        copy = StructuredBuffer.create(original)
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

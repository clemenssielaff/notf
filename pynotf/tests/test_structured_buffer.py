import unittest
import sys
from pynotf.structured_buffer import StructuredBuffer

test_value = {
    "coords": [
        {
            "x": 0,
            "someName": "SUCCESS",
        },
        {
            "x": 2,
            "someName": "Hello world",
        }
    ],
    "name": "Mr. Okay",
    "my_map": {
        "key": "string",
        "a number": 847,
    },
    "pos": 32.2,
    "number_list": [2, 23.1, -347],
}
test_buffer = StructuredBuffer(test_value)


#######################################################################################################################


class TestCase(unittest.TestCase):
    def test_check_kind(self):
        self.assertEqual(StructuredBuffer.Kind.LIST, sys.maxsize - 3)
        self.assertEqual(StructuredBuffer.Kind.MAP, sys.maxsize - 2)
        self.assertEqual(StructuredBuffer.Kind.NUMBER, sys.maxsize - 1)
        self.assertEqual(StructuredBuffer.Kind.STRING, sys.maxsize)

    def test_schema(self):
        self.assertEqual(len(test_buffer.schema), 18)
        self.assertEqual(str(test_buffer.schema), """  0: Map
  1:  ↳ Size: 5
  2: → 7
  3: String
  4: → 12
  5: Number
  6: → 16
  7: List
  8: Map
  9:  ↳ Size: 2
 10: Number
 11: String
 12: Map
 13:  ↳ Size: 2
 14: String
 15: Number
 16: List
 17: Number
""")

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
        self.assertEqual(StructuredBuffer(test_buffer.schema).buffer, [[], '', '', 0, 0, []])

    def test_accessor_len(self):
        accessor = test_buffer["coords"]
        self.assertEqual(len(accessor), 2)
        accessor = accessor[0]
        self.assertEqual(len(accessor), 2)
        accessor = accessor["x"]
        self.assertEqual(len(accessor), 0)

    def test_accessor(self):
        self.assertEqual(test_buffer["pos"].as_number(), 32.2)
        with self.assertRaises(ValueError):
            test_buffer["name"].as_number()  # read string as number
        with self.assertRaises(ValueError):
            test_buffer["coords"].as_number()  # read list as number
        with self.assertRaises(ValueError):
            test_buffer["my_map"].as_number()  # read map as number

        self.assertEqual(test_buffer["name"].as_string(), "Mr. Okay")
        with self.assertRaises(ValueError):
            test_buffer["pos"].as_string()  # read number as string
        with self.assertRaises(ValueError):
            test_buffer["coords"].as_string()  # read list as string
        with self.assertRaises(ValueError):
            test_buffer["my_map"].as_string()  # read map as string

        # access number in list
        self.assertEqual(test_buffer["number_list"][1].as_number(), 23.1)

        # access map using an invalid key
        with self.assertRaises(KeyError):
            _ = test_buffer["my_map"]["not a key"]

    def test_immutability(self):
        modified = test_buffer.modify()["coords"][0]["x"].set(1)
        self.assertEqual(modified["coords"][0]["x"].as_number(), 1)
        self.assertEqual(test_buffer["coords"][0]["x"].as_number(), 0)

        modified2 = modified.modify()["name"].set("Mr. VeryWell")
        self.assertEqual(modified2["name"].as_string(), "Mr. VeryWell")
        self.assertEqual(modified["name"].as_string(), "Mr. Okay")

    def test_change_subtree(self):
        # change a subtree part of the buffer tree
        modified = test_buffer.modify()["coords"].set([dict(x=42, someName="answer")])
        self.assertEqual(len(modified["coords"]), 1)
        self.assertEqual(modified["coords"][0]["someName"].as_string(), "answer")

        # fail to change the subtree to one with a different schema
        with self.assertRaises(ValueError):
            test_buffer.modify()["coords"].set([{"x": 7}])

        # fail to set a list to empty
        with self.assertRaises(ValueError):
            test_buffer.modify()["coords"].set([])

    def test_get_failure(self):
        with self.assertRaises(KeyError):  # access a list using a key
            _ = test_buffer["coords"]["not a map"]
        with self.assertRaises(KeyError):  # access a map using an index
            _ = test_buffer["my_map"][34]

        with self.assertRaises(KeyError):  # access a list using an invalid index
            _ = test_buffer["coords"][-132]
        with self.assertRaises(KeyError):
            _ = test_buffer["coords"][9845]

        with self.assertRaises(KeyError):  # access a number using an index
            _ = test_buffer["pos"][0]
        with self.assertRaises(KeyError):  # access a number using a key
            _ = test_buffer["pos"]["what"]

        with self.assertRaises(KeyError):  # access a string using an index
            _ = test_buffer["name"][0]
        with self.assertRaises(KeyError):  # access a string using a key
            _ = test_buffer["name"]["what"]

    def test_set_failures(self):
        with self.assertRaises(ValueError):  # set number to string
            test_buffer.modify()["pos"].set("0.23")
        with self.assertRaises(ValueError):  # set number to list
            test_buffer.modify()["pos"].set([])
        with self.assertRaises(ValueError):  # set number to map
            test_buffer.modify()["pos"].set({})

        with self.assertRaises(ValueError):  # set string to number
            test_buffer.modify()["name"].set(0.23)
        with self.assertRaises(ValueError):  # set string to list
            test_buffer.modify()["name"].set(["hello"])
        with self.assertRaises(ValueError):  # set string to map
            test_buffer.modify()["name"].set({"hello": "you"})

        with self.assertRaises(ValueError):  # set list to number
            test_buffer.modify()["coords"].set(0.23)
        with self.assertRaises(ValueError):  # set list to string
            test_buffer.modify()["coords"].set("nope")
        with self.assertRaises(ValueError):  # set list to map
            test_buffer.modify()["coords"].set({"hello": "you"})

        with self.assertRaises(ValueError):  # set map to number
            test_buffer.modify()["my_map"].set(0.23)
        with self.assertRaises(ValueError):  # set map to string
            test_buffer.modify()["my_map"].set("nope")
        with self.assertRaises(ValueError):  # set map to list
            test_buffer.modify()["my_map"].set([{"key": "b", "a number": 322}])

    def test_modify_with_equal_value(self):
        # update with the same number
        modified = test_buffer.modify()["pos"].set(32.2)
        self.assertEqual(id(test_buffer), id(modified))

        # update with the same string
        modified = test_buffer.modify()["name"].set("Mr. Okay")
        self.assertEqual(id(test_buffer), id(modified))

        # update with the same list
        modified = test_buffer.modify()["number_list"].set([2, 23.1, -347])
        self.assertEqual(id(test_buffer), id(modified))

        # update with the same map
        modified = test_buffer.modify()["my_map"].set({"key": "string", "a number": 847})
        self.assertEqual(id(test_buffer), id(modified))

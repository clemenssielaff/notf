import unittest
import sys
from pynotf.structured_buffer import StructuredBuffer

test_value = {
    "coords": [
        {
            "x": 0,
            "someName": "SUCCESS",
            "number_list": [1],
        },
        {
            "x": 2,
            "someName": "Hello world",
            "number_list": [2, 3],
        }
    ],
    "name": "Mr. Okay",
    "my_map": {
        "key": "string",
        "list_in_the_middle": ["I'm in the middle"],
        "a number": 847,
    },
    "pos": 32.2,
    "number_list": [2, 23.1, -347],
    "nested_list": [
        ["a", "b"],
        ["c", "d"],
    ],
}
test_buffer = StructuredBuffer(test_value)


#######################################################################################################################


class TestCase(unittest.TestCase):
    def test_check_kind(self):
        self.assertEqual(StructuredBuffer.Kind.LIST, sys.maxsize - 3)
        self.assertEqual(StructuredBuffer.Kind.MAP, sys.maxsize - 2)
        self.assertEqual(StructuredBuffer.Kind.NUMBER, sys.maxsize - 1)
        self.assertEqual(StructuredBuffer.Kind.STRING, sys.maxsize)

    def test_buffer(self):
        self.assertEqual(test_buffer._buffer,
                         [[2, [0.0, "SUCCESS", [1, 1]], [2.0, "Hello world", [2, 2, 3]]],
                          "Mr. Okay",
                          ["string", [1, "I'm in the middle"], 847.0],
                          32.2,
                          [3, 2.0, 23.1, -347.0],
                          [2, [2, "a", "b"], [2, "c", "d"]]])

    def test_schema(self):
        self.assertEqual(len(test_buffer._schema), 27)
        self.assertEqual(str(test_buffer._schema), """  0: Map
  1:  ↳ Size: 6
  2: → 8
  3: String
  4: → 15
  5: Number
  6: → 22
  7: → 24
  8: List
  9: Map
 10:  ↳ Size: 3
 11: Number
 12: String
 13: List
 14: Number
 15: Map
 16:  ↳ Size: 3
 17: String
 18: → 20
 19: Number
 20: List
 21: String
 22: List
 23: Number
 24: List
 25: List
 26: String
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

        # list elements must have the same output_schema
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
        self.assertEqual(StructuredBuffer(test_buffer._schema)._buffer, [[], '', '', [], 0, 0, [], []])

    def test_kind(self):
        self.assertEqual(test_buffer.kind, StructuredBuffer.Kind.MAP)

    def test_keys(self):
        self.assertEqual(test_buffer.keys(), [key for key in test_value.keys()])
        self.assertIsNone(StructuredBuffer(0).keys())

    def test_accessor_len(self):
        accessor = test_buffer["coords"]
        self.assertEqual(len(accessor), 2)
        accessor = accessor[0]
        self.assertEqual(len(accessor), 3)
        accessor = accessor["x"]
        self.assertEqual(len(accessor), 0)

    def test_accessor_keys(self):
        self.assertEqual(test_buffer["coords"][0].keys(), ["x", "someName", "number_list"])
        self.assertIsNone(test_buffer["name"].keys())

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
        modified = test_buffer.modified()["coords"][0]["x"].set(1)
        self.assertEqual(modified["coords"][0]["x"].as_number(), 1)
        self.assertEqual(test_buffer["coords"][0]["x"].as_number(), 0)

        modified2 = modified.modified()["name"].set("Mr. VeryWell")
        self.assertEqual(modified2["name"].as_string(), "Mr. VeryWell")
        self.assertEqual(modified["name"].as_string(), "Mr. Okay")

    def test_change_subtree(self):
        # change a subtree part of the buffer tree
        modified = test_buffer.modified()["coords"].set([dict(x=42, someName="answer", number_list=[-1, -2])])
        self.assertEqual(len(modified["coords"]), 1)
        self.assertEqual(modified["coords"][0]["someName"].as_string(), "answer")

        # fail to change the subtree to one with a different output_schema
        with self.assertRaises(ValueError):
            test_buffer.modified()["coords"].set([{"x": 7}])

        # fail to set a list to empty
        with self.assertRaises(ValueError):
            test_buffer.modified()["coords"].set([])

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
            test_buffer.modified()["pos"].set("0.23")
        with self.assertRaises(ValueError):  # set number to list
            test_buffer.modified()["pos"].set([])
        with self.assertRaises(ValueError):  # set number to map
            test_buffer.modified()["pos"].set({})

        with self.assertRaises(ValueError):  # set string to number
            test_buffer.modified()["name"].set(0.23)
        with self.assertRaises(ValueError):  # set string to list
            test_buffer.modified()["name"].set(["hello"])
        with self.assertRaises(ValueError):  # set string to map
            test_buffer.modified()["name"].set({"hello": "you"})

        with self.assertRaises(ValueError):  # set list to number
            test_buffer.modified()["coords"].set(0.23)
        with self.assertRaises(ValueError):  # set list to string
            test_buffer.modified()["coords"].set("nope")
        with self.assertRaises(ValueError):  # set list to map
            test_buffer.modified()["coords"].set({"hello": "you"})

        with self.assertRaises(ValueError):  # set map to number
            test_buffer.modified()["my_map"].set(0.23)
        with self.assertRaises(ValueError):  # set map to string
            test_buffer.modified()["my_map"].set("nope")
        with self.assertRaises(ValueError):  # set map to list
            test_buffer.modified()["my_map"].set([{"key": "b", "a number": 322}])

    def test_modify_with_equal_value(self):
        # update with the same number
        modified = test_buffer.modified()["pos"].set(32.2)
        self.assertEqual(id(test_buffer), id(modified))

        # update with the same string
        modified = test_buffer.modified()["name"].set("Mr. Okay")
        self.assertEqual(id(test_buffer), id(modified))

        # update with the same list
        modified = test_buffer.modified()["number_list"].set([2, 23.1, -347])
        self.assertEqual(id(test_buffer), id(modified))

        # update with the same map
        modified = test_buffer.modified()["my_map"].set({"key": "string",
                                                       "list_in_the_middle": ["I'm in the middle"],
                                                       "a number": 847})
        self.assertEqual(id(test_buffer), id(modified))

    def test_change_list_size(self):
        # update list in nested list
        self.assertEqual(len(test_buffer["nested_list"][0]), 2)
        modified = test_buffer.modified()["nested_list"][0].set(["x", "y", "z"])
        self.assertEqual(len(modified["nested_list"][0]), 3)

        # update list in dict
        self.assertEqual(len(test_buffer["coords"][1]["number_list"]), 2)
        modified = test_buffer.modified()["coords"][1]["number_list"].set([6, 7, 8, 9])
        self.assertEqual(len(modified["coords"][1]["number_list"]), 4)

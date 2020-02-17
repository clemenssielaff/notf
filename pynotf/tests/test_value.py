import unittest
import sys
from pynotf.data import Value

########################################################################################################################
# TEST HELPER
########################################################################################################################

test_element = {
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
test_value = Value(test_element)
float_value = Value(42.24)
string_value = Value("hello")
none_value = Value()


########################################################################################################################
# TEST CASE
########################################################################################################################


class TestCase(unittest.TestCase):
    def test_check_kind(self):
        self.assertEqual(Value.Kind.NONE, 0)
        self.assertEqual(Value.Kind.LIST, sys.maxsize - 3)
        self.assertEqual(Value.Kind.MAP, sys.maxsize - 2)
        self.assertEqual(Value.Kind.NUMBER, sys.maxsize - 1)
        self.assertEqual(Value.Kind.STRING, sys.maxsize - 0)

    def test_value(self):
        self.assertEqual(test_value._buffer,
                         [[2, [0.0, "SUCCESS", [1, 1]], [2.0, "Hello world", [2, 2, 3]]],
                          "Mr. Okay",
                          ["string", [1, "I'm in the middle"], 847.0],
                          32.2,
                          [3, 2.0, 23.1, -347.0],
                          [2, [2, "a", "b"], [2, "c", "d"]]])

    def test_schema(self):
        self.assertEqual(str(none_value.schema), "")
        self.assertEqual(len(test_value._schema), 27)
        self.assertEqual(str(test_value._schema), """  0: Map
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

    def test_equality(self):
        self.assertEqual(Value(Value.Schema()), none_value)
        self.assertEqual(Value(test_element), test_value)
        self.assertNotEqual(none_value, test_value)
        self.assertNotEqual(none_value, None)

    def test_invalid_element(self):
        class Nope:
            pass

        with self.assertRaises(ValueError):
            Value(Nope())

    def test_valid_lists(self):
        Value([False, 1, 2.])  # list of numbers
        Value(['a', "bee", r"""zebra"""])  # list of strings
        Value([[1, 2], [3, 4]])  # lists of lists
        Value([dict(a=1, b=2), dict(a=3, b=-4)])  # lists of dicts

    def test_invalid_lists(self):
        # list cannot be empty on creation
        with self.assertRaises(ValueError):
            Value([])

        # list elements must have the same output_schema
        with self.assertRaises(ValueError):
            Value(['a', 2])
        with self.assertRaises(ValueError):
            Value([[1, 2], ["a", "b"]])
        with self.assertRaises(ValueError):
            Value([dict(a=1, b=2), dict(a="1", b=2)])

        # if the list contains maps, the maps must have the same keys
        with self.assertRaises(ValueError):
            Value([dict(a=1, b=2), dict(a=1, c=3)])
        with self.assertRaises(ValueError):
            Value([dict(a=1, b=2), dict(a=1)])

    def test_invalid_map(self):
        with self.assertRaises(ValueError):
            Value(dict())  # maps cannot be empty
        with self.assertRaises(ValueError):
            Value({1: "nope"})  # maps keys must be of kind string

    def test_create_value_from_schema(self):
        self.assertEqual(Value(test_value._schema)._buffer, [[], '', '', [], 0, 0, [], []])

    def test_kind(self):
        self.assertEqual(none_value.kind, Value.Kind.NONE)
        self.assertEqual(float_value.kind, Value.Kind.NUMBER)
        self.assertEqual(string_value.kind, Value.Kind.STRING)
        self.assertEqual(test_value.kind, Value.Kind.MAP)

    def test_keys(self):
        self.assertEqual(test_value.keys(), [key for key in test_element.keys()])
        self.assertIsNone(Value(0).keys())

    def test_accessor_len(self):
        accessor = test_value["coords"]
        self.assertEqual(len(accessor), 2)
        accessor = accessor[0]
        self.assertEqual(len(accessor), 3)
        accessor = accessor["x"]
        self.assertEqual(len(accessor), 0)

    def test_accessor_keys(self):
        self.assertEqual(test_value["coords"][0].keys(), ["x", "someName", "number_list"])
        self.assertIsNone(test_value["name"].keys())

    def test_accessor(self):
        self.assertEqual(float_value.as_number(), 42.24)
        with self.assertRaises(ValueError):
            float_value.as_string()  # read number as string
        self.assertEqual(string_value.as_string(), "hello")
        with self.assertRaises(ValueError):
            string_value.as_number()  # read string as number

        self.assertEqual(test_value["pos"].as_number(), 32.2)
        with self.assertRaises(ValueError):
            test_value["name"].as_number()  # read string as number
        with self.assertRaises(ValueError):
            test_value["coords"].as_number()  # read list as number
        with self.assertRaises(ValueError):
            test_value["my_map"].as_number()  # read map as number

        self.assertEqual(test_value["name"].as_string(), "Mr. Okay")
        with self.assertRaises(ValueError):
            test_value["pos"].as_string()  # read number as string
        with self.assertRaises(ValueError):
            test_value["coords"].as_string()  # read list as string
        with self.assertRaises(ValueError):
            test_value["my_map"].as_string()  # read map as string

        # access number in list
        self.assertEqual(test_value["number_list"][1].as_number(), 23.1)

        # access map using an invalid key
        with self.assertRaises(KeyError):
            _ = test_value["my_map"]["not a key"]

        # check for None value
        self.assertTrue(none_value.is_none())
        self.assertFalse(test_value.is_none())

    def test_immutability(self):
        modified = test_value.modified()["coords"][0]["x"].set(1)
        self.assertEqual(modified["coords"][0]["x"].as_number(), 1)
        self.assertEqual(test_value["coords"][0]["x"].as_number(), 0)

        modified2 = modified.modified()["name"].set("Mr. VeryWell")
        self.assertEqual(modified2["name"].as_string(), "Mr. VeryWell")
        self.assertEqual(modified["name"].as_string(), "Mr. Okay")

    def test_change_subtree(self):
        # change a list in the value
        modified = test_value.modified()["coords"].set([dict(x=42, someName="answer", number_list=[-1, -2])])
        self.assertEqual(len(modified["coords"]), 1)
        self.assertEqual(modified["coords"][0]["someName"].as_string(), "answer")

        # change a map in the value
        modified = test_value.modified()["my_map"].set({"key": "changed",
                                                        "list_in_the_middle": ["I am a changed string"],
                                                        "a number": 124, })
        self.assertEqual(modified["my_map"]["key"].as_string(), "changed")

        # fail to change the subtree to one with different keys
        with self.assertRaises(ValueError):
            test_value.modified()["my_map"].set({"wrong key": "changed",
                                                 "wrong list": ["I am a changed string"],
                                                 "wrong number": 124, })
        with self.assertRaises(ValueError):
            test_value.modified()["my_map"].set({"list_in_the_middle": ["I am a changed string"],
                                                 "key": "changed",
                                                 "a number": 124, })

        # fail to change the subtree to one with a different output_schema
        with self.assertRaises(ValueError):
            test_value.modified()["coords"].set([{"x": 7}])

    def test_get_failure(self):
        with self.assertRaises(KeyError):  # access a list using a key
            _ = test_value["coords"]["not a map"]
        with self.assertRaises(KeyError):  # access a map using an index
            _ = test_value["my_map"][34]

        with self.assertRaises(KeyError):  # access a list using an invalid index
            _ = test_value["coords"][-132]
        with self.assertRaises(KeyError):
            _ = test_value["coords"][9845]

        with self.assertRaises(KeyError):  # access a number using an index
            _ = test_value["pos"][0]
        with self.assertRaises(KeyError):  # access a number using a key
            _ = test_value["pos"]["what"]

        with self.assertRaises(KeyError):  # access a string using an index
            _ = test_value["name"][0]
        with self.assertRaises(KeyError):  # access a string using a key
            _ = test_value["name"]["what"]

        # fail to get anything from a None value
        with self.assertRaises(ValueError):
            _ = none_value.as_number()
        with self.assertRaises(ValueError):
            _ = none_value.as_string()
        with self.assertRaises(KeyError):
            _ = none_value[0]

    def test_set_failures(self):
        with self.assertRaises(ValueError):  # set number to string
            test_value.modified()["pos"].set("0.23")
        with self.assertRaises(ValueError):  # set number to list
            test_value.modified()["pos"].set([])
        with self.assertRaises(ValueError):  # set number to map
            test_value.modified()["pos"].set({})

        with self.assertRaises(ValueError):  # set string to number
            test_value.modified()["name"].set(0.23)
        with self.assertRaises(ValueError):  # set string to list
            test_value.modified()["name"].set(["hello"])
        with self.assertRaises(ValueError):  # set string to map
            test_value.modified()["name"].set({"hello": "you"})

        with self.assertRaises(ValueError):  # set list to number
            test_value.modified()["coords"].set(0.23)
        with self.assertRaises(ValueError):  # set list to string
            test_value.modified()["coords"].set("nope")
        with self.assertRaises(ValueError):  # set list to map
            test_value.modified()["coords"].set({"hello": "you"})

        with self.assertRaises(ValueError):  # set map to number
            test_value.modified()["my_map"].set(0.23)
        with self.assertRaises(ValueError):  # set map to string
            test_value.modified()["my_map"].set("nope")
        with self.assertRaises(ValueError):  # set map to list
            test_value.modified()["my_map"].set([{"key": "b", "a number": 322}])

    def test_modify_with_equal_value(self):
        # update with the same number
        modified = test_value.modified()["pos"].set(32.2)
        self.assertEqual(id(test_value), id(modified))

        # update with the same string
        modified = test_value.modified()["name"].set("Mr. Okay")
        self.assertEqual(id(test_value), id(modified))

        # update with the same list
        modified = test_value.modified()["number_list"].set([2, 23.1, -347])
        self.assertEqual(id(test_value), id(modified))

        # update with the same map
        modified = test_value.modified()["my_map"].set({"key": "string",
                                                        "list_in_the_middle": ["I'm in the middle"],
                                                        "a number": 847})
        self.assertEqual(id(test_value), id(modified))

    def test_change_list_size(self):
        # update list in nested list
        self.assertEqual(len(test_value["nested_list"][0]), 2)
        modified = test_value.modified()["nested_list"][0].set(["x", "y", "z"])
        self.assertEqual(len(modified["nested_list"][0]), 3)

        # update list in dict
        self.assertEqual(len(test_value["coords"][1]["number_list"]), 2)
        modified = test_value.modified()["coords"][1]["number_list"].set([6, 7, 8, 9])
        self.assertEqual(len(modified["coords"][1]["number_list"]), 4)

        # set the list to empty
        modified = test_value.modified()["coords"].set([])
        self.assertEqual(len(modified["coords"]), 0)

    def test_schema_as_list(self):
        # number turns into a list of numbers
        self.assertEqual(Value.Schema([0]), Value.Schema(0).as_list())

        # string turns into a list of strings
        self.assertEqual(Value.Schema([""]), Value.Schema("").as_list())

        # list turns into a list of lists
        self.assertEqual(Value.Schema([[0]]), Value.Schema([0]).as_list())

        # map turns into a list of maps
        self.assertEqual(Value.Schema([{"x": 0}]), Value.Schema({"x": 0}).as_list())

    def test_list_initialize_numbers(self):
        zero: Value = Value(0)
        twenty: Value = Value(20)
        nineteen: Value = Value(19)
        list_initialized: Value = Value([zero, twenty, nineteen])
        element_initialized: Value = Value([0, 20, 19])
        self.assertEqual(list_initialized.schema, element_initialized.schema)
        self.assertEqual(list_initialized._buffer, element_initialized._buffer)
        self.assertEqual(list_initialized._dictionary, element_initialized._dictionary)

    def test_list_initialize_lists(self):
        one: Value = Value([1])
        five: Value = Value([1, 2, 3, 4, 5])
        three: Value = Value([1, 2, 3])
        list_initialized: Value = Value([one, five, three])
        element_initialized: Value = Value([[1], [1, 2, 3, 4, 5], [1, 2, 3]])
        self.assertEqual(list_initialized.schema, element_initialized.schema)
        self.assertEqual(list_initialized._buffer, element_initialized._buffer)
        self.assertEqual(list_initialized._dictionary, element_initialized._dictionary)

    def test_bad_list_initializer(self):
        with self.assertRaises(ValueError):
            Value([Value(0), Value("")])


if __name__ == '__main__':
    unittest.main()

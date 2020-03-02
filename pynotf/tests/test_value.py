import unittest
import sys
from math import pi, trunc, floor, ceil

from pynotf.data import Value, set_value

from pyrsistent import pvector as vec

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

empty_list = vec((0,))


########################################################################################################################
# TEST CASE
########################################################################################################################


class TestCase(unittest.TestCase):
    def test_check_kind(self):
        self.assertEqual(Value.Kind.LIST, sys.maxsize - 3)
        self.assertEqual(Value.Kind.RECORD, sys.maxsize - 2)
        self.assertEqual(Value.Kind.NUMBER, sys.maxsize - 1)
        self.assertEqual(Value.Kind.STRING, sys.maxsize - 0)

    def test_value(self):
        self.assertEqual(test_value._data,
                         vec([
                             vec([2, vec([0.0, "SUCCESS", vec([1, 1])]), vec([2.0, "Hello world", vec([2, 2, 3])])]),
                             "Mr. Okay",
                             vec(["string", vec([1, "I'm in the middle"]), 847.0]),
                             32.2,
                             vec([3, 2.0, 23.1, -347.0]),
                             vec([2, vec([2, "a", "b"]), vec([2, "c", "d"])])]))

    def test_copy_constructor(self):
        other_value: Value = Value(test_value)
        self.assertEqual(other_value.get_schema(), test_value.get_schema())
        self.assertEqual(id(other_value._data), id(test_value._data))

    def test_schema(self):
        self.assertEqual(str(none_value.get_schema()), "")
        self.assertEqual(len(test_value._schema), 27)
        self.assertEqual(str(test_value._schema), """  0: Record
  1:  ↳ Size: 6
  2: → 8
  3: String
  4: → 15
  5: Number
  6: → 22
  7: → 24
  8: List
  9: Record
 10:  ↳ Size: 3
 11: Number
 12: String
 13: List
 14: Number
 15: Record
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
        with self.assertRaises(ValueError):
            Value(type('Unsupported', (object,), dict())())

    def test_valid_lists(self):
        Value([False, 1, 2.])  # list of numbers
        Value(['a', "bee", r"""zebra"""])  # list of strings
        Value([[1, 2], [3, 4]])  # lists of lists
        Value([dict(a=1, b=2), dict(a=3, b=-4)])  # lists of named records
        Value([(1, 2), (3, -4)])  # lists of unnamed records
        Value([Value(dict(a=1, b=2)), Value(dict(a=3, b=-4))])  # lists of named record Values
        Value([Value((1, 2)), Value((3, -4))])  # lists of unnamed record Values

    def test_invalid_lists(self):
        with self.assertRaises(ValueError):
            Value([]) # list cannot be empty on creation

        # list elements must have the same output_schema
        with self.assertRaises(ValueError):
            Value(['a', 2])
        with self.assertRaises(ValueError):
            Value([[1, 2], ["a", "b"]])
        with self.assertRaises(ValueError):
            Value([dict(a=1, b=2), dict(a="1", b=2)])

        # if the list contains named records, the records must have the same keys
        with self.assertRaises(ValueError):
            Value([dict(a=1, b=2), dict(a=1, c=3)])
        with self.assertRaises(ValueError):
            Value([dict(a=1, b=2), dict(a=1)])
        with self.assertRaises(ValueError):
            Value([dict(a=1, b=2), dict(a=1)])
        with self.assertRaises(ValueError):
            Value([Value(dict(a=1, b=2)), Value(dict(a=3, c=-4))])

    def test_valid_record(self):
        Value((1, 2, 3, 4)) # build from tuple
        Value(dict(a=Value(1), b=Value(2))) # build from other values

    def test_invalid_record(self):
        # records cannot be empty
        with self.assertRaises(ValueError):
            Value(dict())
        with self.assertRaises(ValueError):
            Value(tuple())

        # record keys must be of kind string
        with self.assertRaises(ValueError):
            Value({1: "nope"})

    def test_create_value_from_schema(self):
        self.assertEqual(Value(test_value._schema)._data,
                         vec([empty_list, '', vec(['', empty_list, 0.0]), 0.0, empty_list, empty_list]))

    def test_kind(self):
        self.assertEqual(none_value.get_kind(), Value.Kind.NONE)
        self.assertEqual(float_value.get_kind(), Value.Kind.NUMBER)
        self.assertEqual(string_value.get_kind(), Value.Kind.STRING)
        self.assertEqual(test_value.get_kind(), Value.Kind.RECORD)

    def test_keys(self):
        self.assertEqual(set(test_value.get_keys()), set(test_element.keys()))
        # TODO: update this test once the dictionary stores keys in order
        self.assertIsNone(Value(0).get_keys())

    def test_none(self):
        none: Value = Value()
        self.assertEqual(none, Value(None))
        self.assertEqual(len(none), 0)
        set_value(none, None)  # noop but okay

    def test_accessor_len(self):
        accessor = test_value["coords"]
        self.assertEqual(len(accessor), 2)
        accessor = accessor[0]
        self.assertEqual(len(accessor), 3)
        accessor = accessor["x"]
        self.assertEqual(len(accessor), 0)

    def test_accessor(self):
        self.assertEqual(float(float_value), 42.24)
        with self.assertRaises(TypeError):
            str(float_value)  # read number as string
        self.assertEqual(str(string_value), "hello")
        with self.assertRaises(TypeError):
            float(string_value)  # read string as number

        self.assertEqual(float(test_value["pos"]), 32.2)
        with self.assertRaises(TypeError):
            float(test_value["name"])  # read string as number
        with self.assertRaises(TypeError):
            float(test_value["coords"])  # read list as number
        with self.assertRaises(TypeError):
            float(test_value["my_map"])  # read map as number

        self.assertEqual(str(test_value["name"]), "Mr. Okay")
        with self.assertRaises(TypeError):
            str(test_value["pos"])  # read number as string
        with self.assertRaises(TypeError):
            str(test_value["coords"])  # read list as string
        with self.assertRaises(TypeError):
            str(test_value["my_map"])  # read map as string

        # access number in list
        self.assertEqual(float(test_value["number_list"][1]), 23.1)

        # access map using an invalid key
        with self.assertRaises(KeyError):
            _ = test_value["my_map"]["not a key"]

        # check for None value
        self.assertTrue(none_value.is_none())
        self.assertFalse(test_value.is_none())

    def test_record_access(self):
        self.assertEqual(test_value["coords"][0]["someName"], test_value[0][0][1])

    def test_immutability(self):
        modified = set_value(test_value, 1, "coords", 0, "x")
        self.assertEqual(float(modified["coords"][0]["x"]), 1)
        self.assertEqual(float(test_value["coords"][0]["x"]), 0)

        modified2 = set_value(modified, "Mr. VeryWell", "name")
        self.assertEqual(str(modified2["name"]), "Mr. VeryWell")
        self.assertEqual(str(modified["name"]), "Mr. Okay")

    def test_change_subtree(self):
        # change a list in the value
        modified = set_value(test_value, [dict(x=42, someName="answer", number_list=[-1, -2])], "coords")
        self.assertEqual(len(modified["coords"]), 1)
        self.assertEqual(str(modified["coords"][0]["someName"]), "answer")

        # change a map in the value
        modified = set_value(test_value, {"key": "changed",
                                          "list_in_the_middle": ["I am a changed string"],
                                          "a number": 124, }, "my_map")
        self.assertEqual(str(modified["my_map"]["key"]), "changed")

        # fail to change the subtree to one with a different schema
        with self.assertRaises(TypeError):
            set_value(test_value, [{"x": 7}], "coords")
        with self.assertRaises(TypeError):  # shuffled
            set_value(test_value, {"list_in_the_middle": ["I am a changed string"],
                                   "key": "changed",
                                   "a number": 124, }, "my_map")

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
        with self.assertRaises(TypeError):
            _ = float(none_value)
        with self.assertRaises(TypeError):
            _ = str(none_value)
        with self.assertRaises(KeyError):
            _ = none_value[0]

        # fail to get a named child from anything but a named record
        with self.assertRaises(KeyError):
            _ = Value(1)["first"]
        with self.assertRaises(KeyError):
            _ = Value("first")["first"]
        with self.assertRaises(KeyError):
            _ = Value([1, 2, 3])["first"]
        with self.assertRaises(KeyError):
            _ = Value((1, 2, 3))["first"]

    def test_set_failures(self):
        with self.assertRaises(TypeError):  # set number to string
            set_value(test_value, "0.23", "pos")
        with self.assertRaises(TypeError):  # set number to list
            set_value(test_value, [], "pos")
        with self.assertRaises(TypeError):  # set number to map
            set_value(test_value, {'x': 3}, "pos")

        with self.assertRaises(TypeError):  # set string to number
            set_value(test_value, 0.23, "name")
        with self.assertRaises(TypeError):  # set string to list
            set_value(test_value, ["hello"], "name")
        with self.assertRaises(TypeError):  # set string to map
            set_value(test_value, {"hello": "you"}, "name")

        with self.assertRaises(TypeError):  # set list to number
            set_value(test_value, 0.23, "coords")
        with self.assertRaises(TypeError):  # set list to string
            set_value(test_value, "nope", "coords")
        with self.assertRaises(TypeError):  # set list to map
            set_value(test_value, {"hello": "you"}, "coords")

        with self.assertRaises(TypeError):  # set map to number
            set_value(test_value, 0.23, "my_map")
        with self.assertRaises(TypeError):  # set map to string
            set_value(test_value, "nope", "my_map")
        with self.assertRaises(TypeError):  # set map to list
            set_value(test_value, [{"key": "b", "a number": 322}], "my_map")

        # the none value cannot be changed and nothing can be set to none
        with self.assertRaises(ValueError):
            set_value(none_value, 1)
        with self.assertRaises(ValueError):
            set_value(none_value, "")
        with self.assertRaises(ValueError):
            set_value(none_value, [])
        with self.assertRaises(ValueError):
            set_value(Value(1), None)
        with self.assertRaises(ValueError):
            set_value(Value("string"), None)
        with self.assertRaises(ValueError):
            set_value(Value(['list', 'of', 'strings']), None)


    def test_modify_with_equal_value(self):
        # update with the same number
        modified = set_value(test_value, 32.2, "pos")
        self.assertEqual(id(test_value), id(modified))

        # update with the same string
        modified = set_value(test_value, "Mr. Okay", "name")
        self.assertEqual(id(test_value), id(modified))

        # update with the same list
        modified = set_value(test_value, [2, 23.1, -347], "number_list")
        self.assertEqual(id(test_value), id(modified))

        # update with the same map
        modified = set_value(test_value, {"key": "string",
                                          "list_in_the_middle": ["I'm in the middle"],
                                          "a number": 847}, "my_map")
        self.assertEqual(id(test_value), id(modified))

    def test_change_list_size(self):
        # update list in nested list
        self.assertEqual(len(test_value["nested_list"][0]), 2)
        modified = set_value(test_value, ["x", "y", "z"], "nested_list", 0)
        self.assertEqual(len(modified["nested_list"][0]), 3)

        # update list in dict
        self.assertEqual(len(test_value["coords"][1]["number_list"]), 2)
        modified = set_value(test_value, [6, 7, 8, 9], "coords", 1, "number_list")
        self.assertEqual(len(modified["coords"][1]["number_list"]), 4)

        # set the list to empty
        modified = set_value(test_value, [], "coords")
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
        self.assertEqual(list_initialized.get_schema(), element_initialized.get_schema())
        self.assertEqual(list_initialized._data, element_initialized._data)
        self.assertEqual(list_initialized._dictionary, element_initialized._dictionary)

    def test_list_initialize_lists(self):
        one: Value = Value([1])
        five: Value = Value([1, 2, 3, 4, 5])
        three: Value = Value([1, 2, 3])
        list_initialized: Value = Value([one, five, three])
        element_initialized: Value = Value([[1], [1, 2, 3, 4, 5], [1, 2, 3]])
        self.assertEqual(list_initialized.get_schema(), element_initialized.get_schema())
        self.assertEqual(list_initialized._data, element_initialized._data)
        self.assertEqual(list_initialized._dictionary, element_initialized._dictionary)

    def test_create_empty_list(self):
        value: Value = Value([1])
        empty: Value = set_value(value, [])
        self.assertEqual(len(empty), 0)

    def test_bad_list_initializer(self):
        with self.assertRaises(ValueError):
            Value([Value(0), Value("")])

    # noinspection PyTypeChecker
    def test_implicit_numerics(self):
        one: Value = Value(1)
        two: Value = Value(2)
        three: Value = Value(3)
        four: Value = Value(4)
        pie: Value = Value(pi)

        # comparison
        self.assertTrue(one == one)
        self.assertFalse(one == three)
        self.assertTrue(one != three)
        self.assertFalse(one != one)
        self.assertTrue(one < three)
        self.assertFalse(three < one)
        self.assertTrue(one <= three)
        self.assertFalse(three <= one)
        self.assertTrue(three <= three)
        self.assertTrue(three <= three)

        # unary
        self.assertEqual(+one, Value(1))
        self.assertEqual(-one, Value(-1))
        self.assertEqual(abs(-three), three)
        self.assertEqual(trunc(pie), three)
        self.assertEqual(floor(pie), three)
        self.assertEqual(ceil(pie), four)
        self.assertEqual(round(pie, 2), Value(3.14))

        # binary
        self.assertEqual(three / 2, Value(1.5))
        self.assertEqual(four / 2, two)
        self.assertEqual(three // 2, one)
        self.assertEqual(four // 2, two)
        self.assertEqual(4. / two, 2.)
        self.assertEqual(4. // two, 2)
        self.assertEqual(two + one, three)
        self.assertEqual(two + 1, three)
        self.assertEqual(2 + one, 3.)
        self.assertEqual(two * two, four)
        self.assertEqual(two * 2, four)
        self.assertEqual(2 * two, 4.)
        self.assertEqual(three % two, one)
        self.assertEqual(three % 2, one)
        self.assertEqual(3 % two, 1.)
        self.assertEqual(two ** two, four)
        self.assertEqual(two ** 2, four)
        self.assertEqual(2 ** two, 4.)

        # unsupported
        unsupported = type('Unsupported', (object,), dict())()
        with self.assertRaises(TypeError):
            _ = one < unsupported
        with self.assertRaises(TypeError):
            _ = one <= unsupported
        with self.assertRaises(TypeError):
            _ = one / unsupported
        with self.assertRaises(TypeError):
            _ = one // unsupported
        with self.assertRaises(TypeError):
            _ = one + unsupported
        with self.assertRaises(TypeError):
            _ = unsupported + one
        with self.assertRaises(TypeError):
            _ = one * unsupported
        with self.assertRaises(TypeError):
            _ = unsupported * one
        with self.assertRaises(TypeError):
            _ = one % unsupported
        with self.assertRaises(TypeError):
            _ = one ** unsupported


if __name__ == '__main__':
    unittest.main()

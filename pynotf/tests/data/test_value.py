import unittest
import logging
from math import pi, trunc, floor, ceil

from pynotf.data import Value, mutate_value, multimutate_value

from pyrsistent import pvector as vec

########################################################################################################################
# TEST HELPER
########################################################################################################################

nested_test_element = {
    "a nested number": 48,
    "a nested record": {
        "does this work": "jup"
    },
    "a doubly nested value": Value(789),
    "a nested None": Value(),
}

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
        [],
    ],
    "unnamed record": (2, "hello", dict(x=3)),
    "a nested value": Value(nested_test_element),
}
test_value = Value(test_element)
float_value = Value(42.24)
string_value = Value("hello")
value_value = Value(Value(1))
none_value = Value()

empty_list = vec((0,))


########################################################################################################################
# TEST CASE
########################################################################################################################


class TestCase(unittest.TestCase):

    def setUp(self) -> None:
        self._previous_log_level = logging.getLogger().getEffectiveLevel()
        logging.getLogger().setLevel(logging.ERROR)

    def tearDown(self) -> None:
        logging.getLogger().setLevel(self._previous_log_level)

    def test_value(self):
        self.assertEqual(test_value._data,
                         vec([
                             vec([2, vec([0.0, "SUCCESS", vec([1, 1])]), vec([2.0, "Hello world", vec([2, 2, 3])])]),
                             "Mr. Okay",
                             vec(["string", vec([1, "I'm in the middle"]), 847.0]),
                             32.2,
                             vec([3, 2.0, 23.1, -347.0]),
                             vec([3, vec([2, "a", "b"]), vec([2, "c", "d"]), vec([0])]),
                             vec([2, "hello", vec([3])]),
                             Value(nested_test_element),
                         ]))

    def test_repr(self):
        self.assertEqual(repr(Value()), 'Value(None)')
        self.assertEqual(repr(Value(89)), 'Value(89)')
        self.assertEqual(repr(Value(1.23)), 'Value(1.23)')
        self.assertEqual(repr(Value("limbo man")), 'Value("limbo man")')
        self.assertEqual(repr(Value(Value(852))), 'Value(Value(852))')
        self.assertEqual(repr(Value([1, 2.34, 3])), 'Value([1, 2.34, 3])')
        self.assertEqual(repr(Value([dict(x=3), dict(x=89.12), dict(x=80)])), 'Value([{x: 3}, {x: 89.12}, {x: 80}])')
        self.assertEqual(repr(Value((1, ['foo', 'bla'], dict(what='the funk')))),
                         'Value({1, ["foo", "bla"], {what: "the funk"}})')

    def test_schema(self):
        self.assertEqual(len(none_value.get_schema()), 0)
        self.assertEqual(len(test_value._schema), 36)
        self.assertEqual(str(test_value._schema), """  0: Record
  1:  ↳ Size: 8
  2: → 10
  3: String
  4: → 17
  5: Number
  6: → 24
  7: → 26
  8: → 29
  9: Value
 10: List
 11: Record
 12:  ↳ Size: 3
 13: Number
 14: String
 15: List
 16: Number
 17: Record
 18:  ↳ Size: 3
 19: String
 20: → 22
 21: Number
 22: List
 23: String
 24: List
 25: Number
 26: List
 27: List
 28: String
 29: Record
 30:  ↳ Size: 3
 31: Number
 32: String
 33: Record
 34:  ↳ Size: 1
 35: Number
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
            Value([])  # list cannot be empty on creation

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

    def test_valid_record(self):
        Value((1, 2, 3, 4))  # build from tuple
        Value(dict(a=Value(1), b=Value(2)))  # build from other values

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
        self.assertEqual(
            Value(test_value._schema)._data,
            vec([empty_list, '', vec(['', empty_list, 0.0]), 0.0, empty_list,
                 empty_list, vec([0.0, '', vec([0.0])]), Value()]))

    def test_kind(self):
        self.assertEqual(none_value.get_kind(), Value.Kind.NONE)
        self.assertEqual(value_value.get_kind(), Value.Kind.VALUE)
        self.assertEqual(float_value.get_kind(), Value.Kind.NUMBER)
        self.assertEqual(string_value.get_kind(), Value.Kind.STRING)
        self.assertEqual(test_value.get_kind(), Value.Kind.RECORD)

    def test_keys(self):
        self.assertEqual(test_value.get_keys(), list(test_element.keys()))
        self.assertTrue(len(Value(0).get_keys()) == 0)

    def test_schema_value(self):
        """
        You can store and restore Value Schemas in a Value.
        """
        # store in and restore a Schema from a Value
        schema_value: Value = Value(schema=test_value.get_schema())
        self.assertEqual(Value.Schema.from_value(schema_value['schema']), test_value.get_schema())

        # explicit None schema
        none_schema: Value.Schema = Value.Schema.create(None)
        self.assertEqual(len(none_schema), 0)

        # none Schema through an empty list
        int_list_value: Value = Value([0])
        empty_list_value: Value = mutate_value(int_list_value, [])
        self.assertEqual(Value.Schema.from_value(empty_list_value), none_schema)

    def test_none(self):
        none: Value = Value()
        self.assertEqual(none, Value(None))
        self.assertEqual(len(none), 0)
        mutate_value(none, None)  # noop but okay

        # None cannot be nested
        with self.assertRaises(ValueError):
            _ = Value([None])
        with self.assertRaises(ValueError):
            _ = Value(dict(a=None))

    def test_any(self):
        """
        Values can contain other Values recursively.
        This allows them to change what type of data they contain at runtime.
        """
        v: Value = Value(x=Value(1))
        self.assertEqual(float(v['x']), 1)  # here is is a number
        v = mutate_value(v, 'x', Value("derbness"))
        self.assertEqual(str(v['x']), 'derbness')  # here is a string

        any_value: Value = Value(Value())
        self.assertTrue(any_value.get_schema().is_any())

    def test_len(self):
        accessor = test_value["coords"]
        self.assertEqual(len(accessor), 2)
        accessor = accessor[0]
        self.assertEqual(len(accessor), 3)
        accessor = accessor["x"]
        self.assertEqual(len(accessor), 0)

    def test_get_value(self):
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

        # check for None value
        self.assertTrue(none_value.is_none())
        self.assertFalse(test_value.is_none())

    def test_get_value_without_creating_a_new_one(self):
        """
        If a Value stores another Value, it can be returned without having to construct a new Value around it.
        """
        # get value from list without copy
        value_in_list: Value = Value([Value(1)])
        self.assertEqual(id(value_in_list._data[1]), id(value_in_list[0]))

        # get value from an unnamed record without copy
        value_in_unnamed_record = Value((Value(1),))
        self.assertEqual(id(value_in_unnamed_record._data[0]), id(value_in_unnamed_record[0]))

        # get value from an named record without copy
        value_in_named_record = Value(dict(x=Value(1)))
        self.assertEqual(id(value_in_named_record._data[0]), id(value_in_named_record[0]))

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

        # access map using an invalid key
        with self.assertRaises(KeyError):
            _ = test_value["my_map"]["not a key"]

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

        # cannot traverse a value with anything but integers and strings
        with self.assertRaises(KeyError):
            _ = test_value[type('Unsupported', (object,), dict())()]
        with self.assertRaises(KeyError):
            # noinspection PyTypeChecker
            _ = test_value[1.0]

    def test_record_access(self):
        self.assertEqual(test_value["coords"][0]["someName"], test_value[0][0][1])

    def test_immutability(self):
        modified = mutate_value(test_value, ("coords", 0, "x"), 1)
        self.assertEqual(float(modified["coords"][0]["x"]), 1)
        self.assertEqual(float(test_value["coords"][0]["x"]), 0)

        modified2 = mutate_value(modified, "name", "Mr. VeryWell")
        self.assertEqual(str(modified2["name"]), "Mr. VeryWell")
        self.assertEqual(str(modified["name"]), "Mr. Okay")

    def test_mutation(self):
        # change a list in the value
        modified = mutate_value(test_value, "coords", [dict(x=42, someName="answer", number_list=[-1, -2])])
        self.assertEqual(len(modified["coords"]), 1)
        self.assertEqual(str(modified["coords"][0]["someName"]), "answer")

        # change a map in the value
        modified = mutate_value(test_value, "my_map", {"key": "changed",
                                                       "list_in_the_middle": ["I am a changed string"],
                                                       "a number": 124, })
        self.assertEqual(str(modified["my_map"]["key"]), "changed")

        # change an item in a list by negative index
        modified = mutate_value(test_value, ("number_list", -2), 123)
        self.assertEqual(float(modified["number_list"][1]), 123)
        self.assertEqual(float(modified[4][1]), 123)
        self.assertEqual(float(modified[4][-2]), 123)

        # change an item in a record by negative index
        modified = mutate_value(test_value, ("coords", -1, -2), "derb")
        self.assertEqual(str(modified["coords"][1]["someName"]), "derb")
        self.assertEqual(str(modified[0][-1][-2]), "derb")

    def test_mutate_with_value(self):
        """
        Allow mutating values with other values of the same Schema.
        """
        int_value: Value = Value(-1)
        self.assertEqual(int(int_value), -1)
        int_value2: Value = mutate_value(int_value, Value(42))
        self.assertEqual(int(int_value2), 42)

    def test_mutation_failures(self):
        with self.assertRaises(TypeError):  # set number to string
            mutate_value(test_value, "pos", "0.23")
        with self.assertRaises(TypeError):  # set number to list
            mutate_value(test_value, "pos", [])
        with self.assertRaises(TypeError):  # set number to map
            mutate_value(test_value, "pos", {'x': 3})

        with self.assertRaises(TypeError):  # set string to number
            mutate_value(test_value, "name", 0.23)
        with self.assertRaises(TypeError):  # set string to list
            mutate_value(test_value, "name", ["hello"])
        with self.assertRaises(TypeError):  # set string to map
            mutate_value(test_value, "name", {"hello": "you"})

        with self.assertRaises(TypeError):  # set list to number
            mutate_value(test_value, "coords", 0.23)
        with self.assertRaises(TypeError):  # set list to string
            mutate_value(test_value, "coords", "nope")
        with self.assertRaises(TypeError):  # set list to map
            mutate_value(test_value, "coords", {"hello": "you"})

        with self.assertRaises(TypeError):  # set map to number
            mutate_value(test_value, "my_map", 0.23)
        with self.assertRaises(TypeError):  # set map to string
            mutate_value(test_value, "my_map", "nope")
        with self.assertRaises(TypeError):  # set map to list
            mutate_value(test_value, "my_map", [{"key": "b", "a number": 322}])

        # the none value cannot be changed and nothing can be set to none
        with self.assertRaises(ValueError):
            mutate_value(none_value, 1)
        with self.assertRaises(ValueError):
            mutate_value(none_value, "")
        with self.assertRaises(ValueError):
            mutate_value(none_value, [])
        with self.assertRaises(ValueError):
            mutate_value(Value(1), None)
        with self.assertRaises(ValueError):
            mutate_value(Value("string"), None)
        with self.assertRaises(ValueError):
            mutate_value(Value(['list', 'of', 'strings']), None)

        # path traversal errors
        with self.assertRaises(IndexError):
            mutate_value(test_value, ("pos", 0), 0)  # cannot move past a ground value
        with self.assertRaises(IndexError):
            mutate_value(test_value, ("name", 1), "")
        with self.assertRaises(IndexError):
            mutate_value(test_value, ("pos", "foo"), 0)
        with self.assertRaises(IndexError):
            mutate_value(test_value, ("name", "bar"), "")

        with self.assertRaises(KeyError):
            mutate_value(test_value, "not a key", 0)
        with self.assertRaises(KeyError):
            mutate_value(test_value, ("unnamed record", "has no names"), 0)
        with self.assertRaises(KeyError):
            mutate_value(test_value, ("number_list", "string as list index"), 0)

        # fail to change the subtree to one with a different schema
        with self.assertRaises(TypeError):
            mutate_value(test_value, "coords", [{"x": 7}])
        with self.assertRaises(TypeError):  # shuffled
            mutate_value(test_value, "my_map", {"list_in_the_middle": ["I am a changed string"],
                                                "key": "changed",
                                                "a number": 124, })

        # mutation of none value
        with self.assertRaises(ValueError):
            mutate_value(none_value, 1, 2)

    def test_mutate_with_equal_value(self):
        # update with the same number
        modified = mutate_value(test_value, "pos", 32.2)
        self.assertEqual(id(test_value), id(modified))

        # update with the same string
        modified = mutate_value(test_value, "name", "Mr. Okay")
        self.assertEqual(id(test_value), id(modified))

        # update with the same list
        modified = mutate_value(test_value, "number_list", [2, 23.1, -347])
        self.assertEqual(id(test_value), id(modified))

        # update with the same map
        modified = mutate_value(test_value, "my_map", {"key": "string",
                                                       "list_in_the_middle": ["I'm in the middle"],
                                                       "a number": 847})
        self.assertEqual(id(test_value), id(modified))

        # update with the same empty list
        modified = mutate_value(test_value, ("nested_list", 2), [])
        self.assertEqual(id(test_value), id(modified))

        # pathless update with same ground
        modified = mutate_value(float_value, 42.24)
        self.assertEqual(id(float_value), id(modified))

    def test_change_list_size(self):
        # update list in nested list
        self.assertEqual(len(test_value["nested_list"][0]), 2)
        modified = mutate_value(test_value, ("nested_list", 0), ["x", "y", "z"])
        self.assertEqual(len(modified["nested_list"][0]), 3)

        # update list in dict
        self.assertEqual(len(test_value["coords"][1]["number_list"]), 2)
        modified = mutate_value(test_value, ("coords", 1, "number_list"), [6, 7, 8, 9])
        self.assertEqual(len(modified["coords"][1]["number_list"]), 4)

        # set the list to empty
        modified = mutate_value(test_value, "coords", [])
        self.assertEqual(len(modified["coords"]), 0)

    def test_multimutate(self):
        """
        Multi-mutate allows multiple mutations in one operation.
        """
        new_value: Value = multimutate_value(
            test_value,
            (1, "Mrs. Also Okay"),
            ("pos", 0.01),
            (("my_map", "key"), "hole"),
            ((5, 1), ["what"]),
        )
        self.assertEqual(str(new_value[1]), "Mrs. Also Okay")
        self.assertEqual(float(new_value["pos"]), 0.01)
        self.assertEqual(str(new_value["my_map"]["key"]), "hole")
        self.assertEqual(new_value[5][1], Value(["what"]))

        # You cannot mutate None
        with self.assertRaises(ValueError):
            multimutate_value(none_value, (0, 3))

    def test_multimutate_with_same_result(self):
        """
        If the result of a multi-mutation is the same as the original Value, that one is returned.
        """
        result: Value = multimutate_value(
            test_value,
            (1, "Mr. Okay"),
            ("pos", 32.2),
            (("my_map", "key"), "string"),
            ((5, 1), ["c", "d"]),
        )
        self.assertEqual(id(result), id(test_value))

    def test_schema_from_value(self):
        """
        Retrieve Schemas from a Value (list of numbers).
        """
        schema: Value.Schema = Value.Schema.from_value(Value([
            0, 8, 8, 254, 13, 253, 18, 19, 21, 255, 1, 0, 3, 253, 254, 1, 253, 0,
            3, 254, 2, 253, 1, 254, 1, 253, 1, 1, 254, 0, 3, 253, 254, 0, 1, 253]))
        self.assertEqual(schema, test_value.get_schema())

        self.assertIsNone(Value.Schema.from_value(Value()))
        self.assertIsNone(Value.Schema.from_value(Value(1)))
        self.assertIsNone(Value.Schema.from_value(Value("")))
        self.assertIsNone(Value.Schema.from_value(Value(dict(a=1))))

    def test_schema_as_list(self):
        # number turns into a list of numbers
        self.assertEqual(Value.Schema.create([0]), Value.Schema.create(0).as_list())

        # string turns into a list of strings
        self.assertEqual(Value.Schema.create([""]), Value.Schema.create("").as_list())

        # list turns into a list of lists
        self.assertEqual(Value.Schema.create([[0]]), Value.Schema.create([0]).as_list())

        # map turns into a list of maps
        self.assertEqual(Value.Schema.create([{"x": 0}]), Value.Schema.create({"x": 0}).as_list())

        # value turns into a list of values
        self.assertEqual(Value.Schema.create([Value(0)]), Value.Schema.create(Value(0)).as_list())

        # none turns into None
        self.assertEqual(Value.Schema(), Value.Schema().as_list())

    def test_create_empty_list(self):
        value: Value = Value([1])
        empty: Value = mutate_value(value, [])
        self.assertEqual(len(empty), 0)

    def test_bad_list_initializer(self):
        with self.assertRaises(ValueError):
            Value([0, ""])

    # noinspection PyTypeChecker
    def test_implicit_numeric(self):
        one: Value = Value(1)
        two: Value = Value(2)
        three: Value = Value(3)
        four: Value = Value(4)
        pie: Value = Value(pi)

        # int
        self.assertEqual(int(four), 4)
        self.assertEqual(int(pie), 3)
        with self.assertRaises(TypeError):
            int(Value("string"))

        # comparison
        self.assertTrue(one == one)
        self.assertTrue(one == 1)
        self.assertTrue(one == 1.)
        self.assertFalse(one == three)
        self.assertFalse(one == 3)
        self.assertFalse(one == 3.)
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
        self.assertEqual(two - one, one)
        self.assertEqual(two - 1, one)
        self.assertEqual(2 - one, 1.)
        self.assertEqual(two * two, four)
        self.assertEqual(two * 2, four)
        self.assertEqual(2 * two, 4.)
        self.assertEqual(three % two, one)
        self.assertEqual(three % 2, one)
        self.assertEqual(3 % two, 1.)
        self.assertEqual(two ** two, four)
        self.assertEqual(two ** 2, four)
        self.assertEqual(2 ** two, 4.)

        # rich comparison
        self.assertTrue(two >= two)
        self.assertTrue(two >= one)
        self.assertFalse(two >= three)
        self.assertTrue(two >= 1)
        self.assertTrue(two >= 2.)
        self.assertFalse(two >= 3)
        self.assertTrue(two > one)
        self.assertFalse(two > two)
        self.assertTrue(two > 1.)
        self.assertFalse(two > 2)

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
            _ = one - unsupported
        with self.assertRaises(TypeError):
            _ = unsupported - one
        with self.assertRaises(TypeError):
            _ = one * unsupported
        with self.assertRaises(TypeError):
            _ = unsupported * one
        with self.assertRaises(TypeError):
            _ = one % unsupported
        with self.assertRaises(TypeError):
            _ = one ** unsupported
        with self.assertRaises(TypeError):
            _ = two >= None
        with self.assertRaises(TypeError):
            _ = two > None

    def test_implicit_string(self):
        self.assertEqual(test_value["my_map"]['key'], "string")
        self.assertNotEqual(test_value["name"], "string")

    def test_get_subschema_edge_case(self):
        value: Value = Value(dict(
            first=dict(
                all=1,
                children=2,
                are=3,
                ground=4
            ),
            second=dict(
                not_empty=1
            )
        ))
        self.assertEqual(value["second"]["not_empty"], 1)

    def test_construct_with_empty_list(self):
        """
        We allow nested lists to be empty as long as there is at least one not-empty one for reference
        """
        value1: Value = Value([[1, 2, 3], []])
        value2: Value = Value([[], [1, 2, 3]])
        value3: Value = Value([[], [1, 2, 3], []])
        value4: Value = Value([[], [], [1, 2, 3]])
        self.assertEqual(value1.get_schema(), Value([[1]]).get_schema())
        self.assertEqual(value1.get_schema(), value2.get_schema())
        self.assertEqual(value1.get_schema(), value3.get_schema())
        self.assertEqual(value1.get_schema(), value4.get_schema())

        # do not allow lists of empty lists
        with self.assertRaises(ValueError):
            Value([[], [], []])

    def test_as_and_from_json(self):
        """
        Simple conversion of the test Value to and from JSON.
        """
        json_string: str = test_value.as_json()
        restored_value: Value = Value.from_json(json_string)
        self.assertEqual(test_value, restored_value)

    def test_from_json_with_empty_list(self):
        """
        Load a Value from JSON that contains an empty list.
        """
        json_string: str = """{
            "head": {
                "type": "notf-value",
                "version": 1,
                "binary": "z85<"
            },
            "body": {
                "schema": "7Je%0$&(q@",
                "data": {
                    "a": [1, 2, 3],
                    "b": []
                }
            }
        }"""
        value: Value = Value.from_json(json_string)
        self.assertEqual(float(value["a"][0]), 1)
        self.assertEqual(float(value["a"][1]), 2)
        self.assertEqual(float(value["a"][2]), 3)
        self.assertEqual(len(value["a"]), 3)
        self.assertEqual(len(value["b"]), 0)
        self.assertEqual(len(value), 2)

    def test_json_deserialization_failures(self):
        """
        Fail gracefully when encountering malformed JSON strings.
        """
        # head
        with self.assertRaises(ValueError):
            Value.from_json("")
        with self.assertRaises(ValueError):
            Value.from_json("{}")

        # type
        with self.assertRaises(ValueError):
            Value.from_json('{"head": {}}')
        with self.assertRaises(ValueError):
            Value.from_json('{"head": {"type": ""}}')
        with self.assertRaises(ValueError):
            Value.from_json('{"head": {"type": 1}}')
        with self.assertRaises(ValueError):
            Value.from_json('{"head": {"type": "wrong"}}')

        # binary
        with self.assertRaises(ValueError):
            Value.from_json('{"head": {"type": "notf-value"}}')
        with self.assertRaises(ValueError):
            Value.from_json('{"head": {"type": "notf-value", "binary": ""}}')
        with self.assertRaises(ValueError):
            Value.from_json('{"head": {"type": "notf-value", "binary": 0}}')
        with self.assertRaises(ValueError):
            Value.from_json('{"head": {"type": "notf-value", "binary": "unsupported"}}')

        # body
        with self.assertRaises(ValueError):
            Value.from_json('{"head": {"type": "notf-value", "binary": "z85<"}}')
        with self.assertRaises(ValueError):
            Value.from_json('{"head": {"type": "notf-value", "binary": "z85<"}, "body"}')
        with self.assertRaises(ValueError):
            Value.from_json('{"head": {"type": "notf-value", "binary": "z85<"}, "body": {}}')

        # schema
        with self.assertRaises(ValueError):
            Value.from_json('{"head": {"type": "notf-value", "binary": "z85<"}, "body": { "schema": 0}}')
        with self.assertRaises(ValueError):
            Value.from_json('{"head": {"type": "notf-value", "binary": "z85<"}, "body": { "schema": ""}}')

    def test_none_with_json(self):
        """
        Tests that even a None Value can be serialized properly.
        """
        json_string: str = Value().as_json()
        value: Value = Value.from_json(json_string)
        self.assertTrue(value.is_none())


if __name__ == '__main__':
    unittest.main()

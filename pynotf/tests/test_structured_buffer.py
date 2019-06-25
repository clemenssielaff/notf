import unittest
from pynotf.structured_buffer import *

original = StructuredBuffer.create(
    Map(
        coordinates=List(
            Map(
                x=Number(0),
                someName=String("SUCCESS")
            ),
            Map(
                x=Number(2),
                someName=String("Hello world")
            )
        ),
        name=String("Hello again"),
        othermap=Map(
            key=String("string"),
            anumber=Number(847)
        )
    )
)


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

    def test_create_empty(self):
        empty_buffer = StructuredBuffer.produce_buffer(original.schema)
        self.assertEqual(empty_buffer, [[], '', '', 0])

    def test_basic_get_set(self):
        copy = StructuredBuffer.create(original)

        self.assertEqual(original.read()["coordinates"][1]["x"].as_number(), 2.0)
        self.assertEqual(copy.read()["coordinates"][1]["x"].as_number(), 2.0)

        copy.write()["coordinates"][1]["x"].set(-123)
        self.assertEqual(original.read()["coordinates"][1]["x"].as_number(), 2.0)
        self.assertEqual(copy.read()["coordinates"][1]["x"].as_number(), -123.0)

    def test_change_subtree(self):
        copy = StructuredBuffer.create(original)
        self.assertEqual(copy.read()["coordinates"].get_list_size(), 2)
        self.assertEqual(copy.read()["coordinates"][0]["someName"].as_string(), "SUCCESS")

        copy.write()["coordinates"].set(List(Map(x=Number(42), someName=String("answer"))))
        self.assertEqual(original.read()["coordinates"].get_list_size(), 2)
        self.assertEqual(original.read()["coordinates"][0]["someName"].as_string(), "SUCCESS")
        self.assertEqual(copy.read()["coordinates"].get_list_size(), 1)
        self.assertEqual(copy.read()["coordinates"][0]["someName"].as_string(), "answer")

        with self.assertRaises(ValueError):
            copy.write()["coordinates"].set(List(Map(x=Number(7))))

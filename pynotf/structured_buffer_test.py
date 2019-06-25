from structured_buffer import *

original = StructuredBuffer.create(
    Map(
        coords=List(
            Map(
                x=Number(0),
                somname=String("SUCCESS")
            ),
            Map(
                x=Number(2),
                somname=String("Hello world")
            )
        ),
        name=String("Hello again"),
        othermap=Map(
            key=String("string"),
            anumber=Number(847)
        )
    )
)


def check_schema():
    assert (original.schema == [9223372036854775807,
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


def create_empty():
    empty_buffer = StructuredBuffer.produce_buffer(original.schema)
    assert (empty_buffer == [[], '', '', 0])


def basic_get_set():
    copy = StructuredBuffer.create(original)

    assert (original.read()["coords"][1]["x"].as_number() == 2.0)
    assert (copy.read()["coords"][1]["x"].as_number() == 2.0)

    copy.write()["coords"][1]["x"].set(-123)
    assert (original.read()["coords"][1]["x"].as_number() == 2.0)
    assert (copy.read()["coords"][1]["x"].as_number() == -123.0)


def change_subtree():
    copy = StructuredBuffer.create(original)
    assert (copy.read()["coords"].get_list_size() == 2)
    assert (copy.read()["coords"][0]["somname"].as_string() == "SUCCESS")

    copy.write()["coords"].set(List(Map(x=Number(42), somname=String("answer"))))
    assert (original.read()["coords"].get_list_size() == 2)
    assert (original.read()["coords"][0]["somname"].as_string() == "SUCCESS")
    assert (copy.read()["coords"].get_list_size() == 1)
    assert (copy.read()["coords"][0]["somname"].as_string() == "answer")

    failed_correctly = False
    try:
        copy.write()["coords"].set(List(Map(x=Number(7))))
    except ValueError:
        failed_correctly = True
    assert failed_correctly


check_schema()
create_empty()
basic_get_set()
change_subtree()

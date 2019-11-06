import unittest
import asyncio
from typing import List
from enum import Enum, auto
import logging
from time import sleep
from itertools import product

from pynotf.value import Value
from pynotf.logic import Emitter
from pynotf.scene import Executor, Fact, Scene, RootNode, Node, Property

from tests.utils import record, ErrorOperation, ClampOperation, StringifyOperation, NumberEmitter


########################################################################################################################
# TEST HELPER
########################################################################################################################


class TestNode(Node):
    def _produce_definition(self) -> 'Node.Definition':
        definition: Node.Definition = Node.Definition()
        definition.add_property("prop_number", Value(0), ClampOperation(0., 10.), ErrorOperation(4))
        definition.add_property("prop_string", Value(""))
        definition.add_signal("signal_number", Value(0).schema)
        definition.add_slot("slot_number", Value(0).schema)
        return definition


class IntFact(Fact):
    def __init__(self, executor: Executor):
        Fact.__init__(self, executor, Value(0).schema)


async def add_delayed(state: List[int], number: int):
    await asyncio.sleep(0.01)
    state.append(number)


def add_immediate(state: List[int], number: int):
    state.append(number)


async def fail_delayed():
    await asyncio.sleep(0.01)
    raise ValueError("Delayed fail")


def fail_immediate():
    raise ValueError("Immediate fail")


########################################################################################################################
# TEST CASE
########################################################################################################################


class TestCase(unittest.TestCase):

    def setUp(self):
        logging.disable(logging.CRITICAL)

    def tearDown(self):
        logging.disable(logging.NOTSET)

    def test_scene_setup(self):
        scene = Scene()

        node_a = scene.root.create_child(TestNode)
        node_b = scene.root.create_child(TestNode)
        self.assertNotEqual(node_a, node_b)
        self.assertNotEqual(node_a.uuid, node_b.uuid)

    def test_node(self):
        scene = Scene()
        node = scene.root.create_child(TestNode, "Herbert")

        self.assertEqual(node.name, "Herbert")
        self.assertEqual(node.get_property("prop_number").value.as_number(), 0)
        node.get_signal("signal_number")
        node.get_slot("slot_number")

        with self.assertRaises(KeyError):
            node.get_property("Not a Property")
        with self.assertRaises(KeyError):
            node.get_signal("Not a Signal")
        with self.assertRaises(KeyError):
            node.get_slot("Not a SLot")

    def test_property(self):
        scene = Scene()
        node = scene.root.create_child(TestNode)

        number_prop = node.get_property("prop_number")
        self.assertEqual(number_prop.node, node)
        self.assertEqual(number_prop.value.as_number(), 0)
        number_prop.value = 3.87
        self.assertEqual(number_prop.value.as_number(), 3.87)
        number_prop.value = 11
        self.assertEqual(number_prop.value.as_number(), 10)  # 10 is max
        number_prop.value = -7
        self.assertEqual(number_prop.value.as_number(), 0)  # 0 is min

        string_prop = node.get_property("prop_string")
        self.assertEqual(string_prop.value.as_string(), "")
        string_prop.value = Value("test string")
        self.assertEqual(string_prop.value.as_string(), "test string")
        string_prop.value = "test string"
        self.assertEqual(string_prop.value.as_string(), "test string")

        with self.assertRaises(TypeError):
            number_prop.value = None

        with self.assertRaises(TypeError):
            number_prop.value = Scene()

        with self.assertRaises(TypeError):
            number_prop.value = "test_string"

        number_prop.value = 4  # error condition

    def test_reactive_property(self):
        scene = Scene()
        node = scene.root.create_child(TestNode)
        prop: Property = node.get_property("prop_number")
        emitter1: Emitter = NumberEmitter()
        emitter2: Emitter = NumberEmitter()

        prop.connect_to(emitter1)
        prop.connect_to(emitter2)
        self.assertEqual(prop.value.as_number(), 0)

        emitter1.emit(2)
        self.assertEqual(prop.value.as_number(), 2)
        emitter2.emit(5)
        self.assertEqual(prop.value.as_number(), 5)
        emitter1.emit(3)
        emitter2.emit(9)
        emitter1.emit(8)
        self.assertEqual(prop.value.as_number(), 8)

        self.assertFalse(prop.is_completed())
        emitter1._complete()
        self.assertFalse(prop.is_completed())
        prop.complete()
        self.assertFalse(prop.is_completed())
        emitter2._error(RuntimeError())
        self.assertFalse(prop.is_completed())

    def test_create_property_error(self):
        class PropertyOperationsMustMatchValue(Node):
            def _produce_definition(self) -> 'Node.Definition':
                definition: Node.Definition = Node.Definition()
                definition.add_property("mismatch", Value(""), ClampOperation(0, 10))
                return definition

        class PropertiesCannotConvert(Node):
            def _produce_definition(self) -> 'Node.Definition':
                definition: Node.Definition = Node.Definition()
                definition.add_property("stringify", Value(0), StringifyOperation())
                return definition

        scene = Scene()
        with self.assertRaises(TypeError):
            scene.root.create_child(PropertyOperationsMustMatchValue)
        with self.assertRaises(TypeError):
            scene.root.create_child(PropertiesCannotConvert)

    def test_definition_name_error(self):
        class Kind(Enum):
            PROP = auto()
            SLOT = auto()
            SIGN = auto()

            @staticmethod
            def to_func(kind):
                if kind is Kind.PROP:
                    return Node.Definition.add_property
                elif kind is Kind.SLOT:
                    return Node.Definition.add_slot
                elif kind is Kind.SIGN:
                    return Node.Definition.add_signal
                assert False

        def make_error_class(first_kind: Kind, second_kind: Kind):
            class ErrorClass(Node):
                def _produce_definition(self) -> 'Node.Definition':
                    definition: Node.Definition = Node.Definition()
                    Kind.to_func(first_kind)(definition, "duplicate name", Value(0))
                    Kind.to_func(second_kind)(definition, "duplicate name", Value(0))
                    return definition

            return ErrorClass

        scene = Scene()
        for first, second in product((Kind.PROP, Kind.SLOT, Kind.SIGN), repeat=2):
            with self.assertRaises(NameError):
                scene.root.create_child(make_error_class(first, second))

    def test_create_child_error(self):
        scene = Scene()
        node_a = scene.root.create_child(TestNode)
        with self.assertRaises(TypeError):
            # noinspection PyTypeChecker
            node_a.create_child(int)

    def test_root_node(self):
        scene = Scene()
        root: RootNode = scene.root
        self.assertIsInstance(root, RootNode)

        with self.assertRaises(RuntimeError):
            root._produce_definition()

        self.assertEqual(scene.get_node(root.name), root)
        self.assertEqual('/', root.name)

    def test_node_names(self):
        scene = Scene()

        node_a = scene.root.create_child(TestNode, "a")
        self.assertEqual(scene.get_node("a"), node_a)
        scene.register_name(node_a)  # does nothing

        self.assertIsNone(scene.get_node("b"))
        node_b = scene.root.create_child(TestNode, "b")
        self.assertEqual(scene.get_node("b"), node_b)

        with self.assertRaises(NameError):
            scene.root.create_child(TestNode, "a")

        with self.assertRaises(NameError):
            scene.root.create_child(TestNode, scene.root.name)

        node_unnamed = scene.root.create_child(TestNode)
        scene.register_name(node_unnamed)  # does nothing

    def test_executor_stop(self):
        state: List[int] = []

        executor = Executor()
        executor.schedule(add_delayed, state, 1)
        executor.schedule(add_delayed, state, 2)
        executor.schedule(add_immediate, state, 3)
        executor.schedule(add_delayed, state, 4)
        executor.schedule(add_immediate, state, 5)
        executor.stop()
        executor.schedule(add_delayed, state, 6)
        executor.schedule(add_immediate, state, 7)
        sleep(0.1)

        # We call stop before any of the delayed functions have time to add to the state.
        # Therefore only the immediate functions affect it, but only up to the point where we call `stop`
        self.assertEqual(state, [3, 5])

    def test_executor_finish(self):
        state: List[int] = []

        executor = Executor()
        executor.schedule(add_delayed, state, 1)
        executor.schedule(add_delayed, state, 2)
        executor.schedule(add_immediate, state, 3)
        executor.schedule(add_delayed, state, 4)
        executor.schedule(add_immediate, state, 5)
        executor.finish()
        executor.schedule(add_delayed, state, 6)
        executor.stop(force=False)
        executor.schedule(add_immediate, state, 7)
        sleep(0.1)

        # Almost same as the test case above, but instead of calling `stop`, we call `finish`, which gives all running
        # coroutines time to finish.
        self.assertEqual(state, [3, 5, 1, 2, 4])

    def test_executor_failure(self):
        state: List[int] = []

        executor = Executor()
        original_handler = executor.exception_handler
        executor.exception_handler = lambda: state.append(0 if original_handler() is None else 0)

        executor.schedule(add_delayed, state, 1)
        executor.schedule(add_immediate, state, 2)
        executor.schedule(add_delayed, state, 3)
        executor.schedule(fail_immediate)
        executor.schedule(add_immediate, state, 4)
        executor.schedule(add_delayed, state, 5)
        executor.schedule(fail_delayed)
        executor.finish()
        executor.schedule(add_delayed, state, 6)
        executor.schedule(fail_delayed)
        executor.schedule(add_immediate, state, 7)
        executor.schedule(fail_immediate)
        sleep(0.1)

        # Whenever an exception is encountered, we add a zero to the state but continue the execution.
        self.assertEqual(state, [2, 0, 4, 1, 3, 5, 0])

    def test_fact(self):
        executor = Executor()

        fact1 = IntFact(executor)
        recorder1 = record(fact1)

        fact2 = IntFact(executor)
        recorder2 = record(fact2)
        error = RuntimeError("Nope")

        fact1.emit(2)
        fact1.emit(7)
        fact1.emit(-23)
        fact1._complete()
        fact1.emit(456)

        fact2.emit(8234)
        fact2._error(error)
        fact2.emit(-6)

        executor.finish()

        values1 = [v.as_number() for v in recorder1.values]
        self.assertEqual(values1, [2, 7, -23])
        self.assertEqual(recorder1.completed, [id(fact1)])

        values2 = [v.as_number() for v in recorder2.values]
        self.assertEqual(values2, [8234])
        self.assertEqual(recorder2.errors, [error])


if __name__ == '__main__':
    unittest.main()

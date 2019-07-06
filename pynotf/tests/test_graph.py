import unittest
from pynotf import Graph
from tests.test_classes import EmptyNode, AdderNode, StructuredBuffer, Number


class TestNodeRegistry(unittest.TestCase):
    def setUp(self) -> None:
        self.graph = Graph()

    def test_concept_proof(self):
        self.assertEqual(len(self.graph.registry), 0)
        node1 = self.graph.root.create_child(EmptyNode)
        self.assertEqual(len(self.graph.registry), 0)
        self.assertIsNotNone(node1.name)
        self.assertEqual(len(self.graph.registry), 1)

    def test_node_properties(self):
        node1 = self.graph.root.create_child(AdderNode)
        node2 = self.graph.root.create_child(AdderNode)
        node3 = self.graph.root.create_child(AdderNode)

        node1.property("out").subscribe(node3.property("in1"))
        node2.property("out").subscribe(node3.property("in2"))

        output = node3.property("out")
        self.assertEqual(output.value.read().as_number(), 0.0)

        node1.property("out").value = StructuredBuffer.create(Number(2.0))
        self.assertEqual(output.value.read().as_number(), 2.0)

        node2.property("out").value = StructuredBuffer.create(Number(6.0))
        self.assertEqual(output.value.read().as_number(), 8.0)

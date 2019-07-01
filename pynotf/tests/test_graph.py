import unittest
from pynotf.graph import NodeRegistry, Node, RootNode, Graph

#######################################################################################################################


class TestNodeRegistry(unittest.TestCase):
    def setUp(self) -> None:
        self.graph = Graph()

    def test_concept_proof(self):
        self.assertEqual(len(self.graph.registry), 0)
        node1 = self.graph.root.create_child(Node)
        self.assertEqual(len(self.graph.registry), 0)
        self.assertIsNotNone(node1.name)
        self.assertEqual(len(self.graph.registry), 1)

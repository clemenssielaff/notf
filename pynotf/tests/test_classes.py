from logging import warning
from typing import Optional, Dict
from pynotf import Operator, Publisher, StructuredBuffer, Node, Graph, connect_each, Element


# REACTIVE #############################################################################################################

class SumOperator(Operator):
    """
    Takes number values from an arbitrary number of sources and outputs their sum.
    """
    def __init__(self):
        super().__init__(Element(0).schema)
        self._values: Dict[Publisher, float] = {}

    def on_next(self, publisher: Publisher, value: Optional[StructuredBuffer] = None):
        assert value is not None
        self._values[publisher] = value.read().as_number()
        new_sum = 0.
        for value in self._values.values():
            new_sum += value
        self.publish(StructuredBuffer.create(new_sum))

    def on_complete(self, publisher: Publisher):
        if publisher in self._values:
            del self._values[publisher]

    def on_error(self, publisher: Publisher, exception: BaseException):
        warning("Caught exception in AddOperator from Publisher {}: {}".format(publisher, str(exception)))
        self.on_complete(publisher)


# GRAPH ################################################################################################################

class EmptyNode(Node):
    def _define_node(self, definition: Node.Definition):
        pass


class AdderNode(Node):
    """
    out = in1 + in2
    """

    def __init__(self, graph: Graph, parent: Node):
        super().__init__(graph, parent)

    def _define_node(self, definition: Node.Definition):
        # define properties
        a = definition.add_property("in1", 0, is_visible=True)
        b = definition.add_property("in2", 0, is_visible=True)
        c = definition.add_property("out", 0, is_visible=True)

        # set up the property operation
        adder = SumOperator()
        connect_each(a, b) | adder
        adder | c

from __future__ import annotations

from typing import List, Tuple, Set
from threading import Thread
from time import sleep

from pynotf.data import Value, Shape, get_mutated_value
from pynotf.core import OperatorIndex, LayoutIndex, get_app, Node, Service, Fact, Operator
from pynotf.extra.svg import parse_svg


class NumberProducer(Service):
    """
    Produces a new number every second
    """

    def __init__(self):
        self._thread: Thread = Thread(target=lambda: self._worker())
        self._continue: bool = True
        self._facts: Set[Fact] = set()
        self._number: int = 0

    def _worker(self):
        while self._continue:
            sleep(0.5)
            number: int = self._number
            self._number += 1
            live_facts: Set[Fact] = set()
            for fact in self._facts:
                if not fact.is_valid():
                    continue
                fact.update(Value(number))
                live_facts.add(fact)
            self._facts = live_facts

    def initialize(self) -> None:
        self._thread.start()

    def get_fact(self, query: str) -> Operator:
        """
        Either returns a valid Fact Operator or raises an exception.
        """
        new_fact: Fact = Fact(Value(0))
        self._facts.add(new_fact)
        return new_fact.get_operator()

    def shutdown(self) -> None:
        self._continue = False
        self._thread.join()
        self._facts.clear()


notf_shapes: List[Shape] = [
    shape for description in parse_svg('/home/clemens/code/notf/dev/img/notf_inline_white.svg')
    for shape in description.shapes]

pulsating_round_rect: Tuple[str, Value] = ('rounded_rect', Value(
    x=("constant", Value(0)),
    y=("constant", Value(0)),
    width=("expression", Value(
        source="max(0, node.grant.width)",
        kwargs=Value(),
    )),
    height=("expression", Value(
        source="max(0, node.grant.height)",
        kwargs=Value(),
    )),
    radius=("expression", Value(
        source="max(0, node.roundness)",
        kwargs=Value(),
    )),
))

count_presses_node: Value = get_mutated_value(Node.VALUE, dict(
    interops=[
        ('key_down', Value(), 0),
    ],
    states=[
        ("default", get_mutated_value(Node.STATE, dict(
            operators=[
                ('buffer', OperatorIndex.BUFFER, Value(schema=Value.Schema.create(Value()), time_span=1)),
                ('printer', OperatorIndex.PRINTER, Value(Value())),
                ('number_service_printer', OperatorIndex.PRINTER, Value(0)),
            ],
            connections=[
                ('/|mouse_click_fact', 'buffer'),
                ('buffer', 'printer'),
                # ('numbers:whatever', 'number_service_printer'),
            ],
            design=Value([
                ('fill', Value(
                    shape=('constant', Value([shape.as_value() for shape in notf_shapes])),
                    paint=('constant', Value(r=0.9, g=0.9, b=0.9, a=1)),
                    opacity=('constant', Value(1)),
                ))
            ]),
            children=[],
            layout=(LayoutIndex.OVERLAY, Value()),
            claim=dict(
                horizontal=dict(preferred=100, min=10, max=300, scale_factor=1, priority=0),
                vertical=dict(preferred=100, min=10, max=300, scale_factor=1, priority=0),
            )
        ))),
    ],
    transitions=[],
    initial="default",
))

countdown_node: Value = get_mutated_value(Node.VALUE, dict(
    interops=[
        ('key_down', Value(), 0),
        ('roundness', Value(10), 1),
        ('on_mouse_click', Value(x=0, y=0), 0),
    ],
    states=[
        ("default", get_mutated_value(Node.STATE, dict(
            operators=[
                ('factory', OperatorIndex.FACTORY, Value(id=OperatorIndex.COUNTDOWN, args=Value(start=5))),
                ('printer', OperatorIndex.PRINTER, Value(Value(0))),
                ('mouse_click_printer', OperatorIndex.PRINTER, Value(x=0, y=0)),
                ('mouse_pos_printer', OperatorIndex.PRINTER, Value(0, 0)),
                ('sine', OperatorIndex.SINE, Value()),
                # TODO: it is weird to have name identifiers for the Design, but INDEX for Layout and Operators
            ],
            connections=[
                ('/|key_fact', 'factory'),
                ('factory', 'printer'),
                ('/|mouse_click_fact', 'mouse_pos_printer'),
                ('sine', '|roundness'),
                ('|on_mouse_click', 'mouse_click_printer'),
            ],
            design=Value([
                ('fill', Value(
                    shape=pulsating_round_rect,
                    paint=('constant', Value(r=.5, g=.5, b=.5, a=1)),
                    opacity=('constant', Value(1)),  # TODO: remove default opacity when we have Value subsets
                )),
                ('mark', Value(
                    shape=pulsating_round_rect,
                    interop="on_mouse_click",
                )),
            ]),
            children=[],
            layout=(LayoutIndex.OVERLAY, Value()),
            claim=dict(
                horizontal=dict(preferred=100, min=200, max=300, scale_factor=1, priority=0),
                vertical=dict(preferred=100, min=200, max=300, scale_factor=1, priority=0),
            )
        ))),
    ],
    transitions=[],
    initial="default",
))

root_node: Value = get_mutated_value(Node.VALUE, dict(
    interops=[
        ('key_fact', Value(), 0),
        ('mouse_click_fact', Value(0, 0), 0),
        ('hitbox_fact', Value(0, 0), 0),
    ],
    states=[
        ("default", get_mutated_value(Node.STATE, dict(
            operators=[],
            connections=[],
            design=Value(),  # TODO: allow setting the background color in the root node?
            children=[
                ("herbert", countdown_node),
                ("zanzibar", count_presses_node),
            ],
            layout=(LayoutIndex.FLEXBOX, Value(
                margin=10,
            )),
            claim=dict(
                horizontal=dict(preferred=0, min=0, max=0, scale_factor=1, priority=0),
                vertical=dict(preferred=0, min=0, max=0, scale_factor=1, priority=0),
            )
        ))),
    ],
    transitions=[],
    initial="default",
))


def first_test() -> int:
    get_app().register_service('numbers', NumberProducer)
    return get_app().run(root_node)

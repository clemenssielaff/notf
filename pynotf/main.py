from __future__ import annotations

import sys
from typing import List, Tuple, Set
from threading import Thread
from time import sleep

from pynotf.data import Value, Shape, get_mutated_value
from pynotf.core import OperatorIndex, LayoutIndex, get_app, Node, Service, Fact, Operator
from pynotf.extra.svg import parse_svg


########################################################################################################################

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


########################################################################################################################

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
                ('/|mouse_fact', 'buffer'),
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
                ('/|mouse_fact', 'mouse_pos_printer'),
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

window: Value = get_mutated_value(Node.VALUE, dict(
    interops=[
    ],
    states=[
        ("default", get_mutated_value(Node.STATE, dict(
            operators=[
                ('sine', OperatorIndex.SINE, Value()),
                ('translate_expression', OperatorIndex.NODE_EXPRESSION, Value(
                    source="""self.get_interop('widget.xform').update(1, 0, 0, 1, float(arg), 0)"""
                )),
            ],
            connections=[
                ('sine', 'translate_expression')
            ],
            design=Value([
                # shadow
                ('fill', Value(
                    shape=('rounded_rect', Value(
                        x=("constant", Value(-10)),
                        y=("constant", Value(-10)),
                        width=("expression", Value(
                            source="max(0, node.grant.width+20)",
                            kwargs=Value(),
                        )),
                        height=("expression", Value(
                            source="max(0, node.grant.height+20)",
                            kwargs=Value(),
                        )),
                        radius=("constant", Value(0)),
                    )),
                    paint=('box_gradient', Value(
                        box=("expression", Value(
                            source="dict(x=0, y=-5, w=max(0, node.grant.width), h=max(0, node.grant.height))",
                            kwargs=Value(),
                        )),
                        radius=("constant", Value(6)),
                        feather=("constant", Value(10)),
                        inner_color=("constant", Value(r=0, g=0, b=0, a=.5)),
                        outer_color=("constant", Value(r=0, g=0, b=0, a=0)),
                    )),
                    opacity=('constant', Value(1)),
                )),

                # window
                ('fill', Value(
                    shape=('rounded_rect', Value(
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
                        radius=("constant", Value(3)),
                    )),
                    paint=('constant', Value(r=.157, g=.157, b=.18, a=1)),
                    opacity=('constant', Value(1)),  # TODO: remove default opacity when we have Value subsets
                )),

                # header background
                ('fill', Value(
                    shape=('rounded_rect', Value(
                        x=("constant", Value(1)),
                        y=("constant", Value(1)),
                        width=("expression", Value(
                            source="max(0, node.grant.width - 2)",
                            kwargs=Value(),
                        )),
                        height=("constant", Value(30)),
                        radius=("constant", Value(2)),
                    )),
                    paint=('linear_gradient', Value(
                        start_point=("constant", Value(x=0, y=0)),
                        end_point=("constant", Value(x=0, y=15)),
                        start_color=("constant", Value(r=1, g=1, b=1, a=0.03125)),
                        end_color=("constant", Value(r=0, g=0, b=0, a=0.0625)),
                    )),
                    opacity=('constant', Value(1)),
                )),

                # header separator
                ('stroke', Value(
                    shape=('lines', Value([
                        ('constant', Value(x=0.5, y=30.5)),
                        ("expression", Value(
                            source="dict(x=max(0.5, node.grant.width - 0.5), y=30.5)",
                            kwargs=Value(),
                        )),
                    ])),
                    paint=('constant', Value(r=0, g=0, b=0, a=0.125)),
                    opacity=('constant', Value(1)),
                    line_width=('constant', Value(1)),
                    cap=('constant', Value(0)),
                    join=('constant', Value(0)),
                )),

                # header text background
                ('write', Value(
                    pos=("expression", Value(
                        source="dict(x=(node.grant.width/2.), y=17.)",
                        kwargs=Value(),
                    )),
                    text=('constant', Value("Widgets 'n Stuff")),
                    font=('constant', Value("Roboto")),
                    size=('constant', Value(15)),
                    color=('constant', Value(r=0, g=0, b=0, a=0.5)),
                    blur=('constant', Value(2)),
                    letter_spacing=('constant', Value(0)),
                    line_height=('constant', Value(1)),
                    text_align=('constant', Value(18)),  # Align.CENTER | Align.MIDDLE
                    width=('constant', Value()),  # None
                )),

                # header text
                ('write', Value(
                    pos=("expression", Value(
                        source="dict(x=(node.grant.width/2.), y=17.)",
                        kwargs=Value(),
                    )),
                    text=('constant', Value("Widgets 'n Stuff")),
                    font=('constant', Value("Roboto")),
                    size=('constant', Value(15)),
                    color=('constant', Value(r=0.86, g=0.86, b=0.86, a=.8)),
                    blur=('constant', Value(0)),
                    letter_spacing=('constant', Value(0)),
                    line_height=('constant', Value(1)),
                    text_align=('constant', Value(18)),  # Align.CENTER | Align.MIDDLE
                    width=('constant', Value()),  # None
                )),
            ]),
            children=[],
            layout=(LayoutIndex.OVERLAY, Value()),
            claim=dict(
                horizontal=dict(preferred=300, min=300, max=600, scale_factor=1, priority=0),
                vertical=dict(preferred=400, min=400, max=400, scale_factor=1, priority=0),
            )
        ))),
    ],
    transitions=[],
    initial="default",
))

root_node: Value = get_mutated_value(Node.VALUE, dict(
    interops=[
        ('key_fact', Value(), 0),
        ('mouse_fact', Value(0, 0), 0),
        ('hitbox_fact', Value(0, 0), 0),
    ],
    states=[
        ("default", get_mutated_value(Node.STATE, dict(
            operators=[],
            connections=[],
            design=Value(),  # TODO: allow setting the background color in the root node?
            children=[
                # ("herbert", countdown_node),
                # ("zanzibar", count_presses_node),
                ("window", window),
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

# MAIN #################################################################################################################

if __name__ == "__main__":
    get_app().register_service('numbers', NumberProducer)
    sys.exit(get_app().run(root_node))

# TODO: CONTINUE HERE
#   I have made a lot of progress to define the whole application as pure data. Most of the application-specific code is
#   defined here, and almost of all it is actually data.
#   There are a few avenues that we need to explore from here:
#   * I want to be able to define the WHOLE Application using one or multiple JSON files with serialized Values.
#       * Then, I want the Application to update in real-time whenever I update one of the relevant files.
#   * We also need better Operators, the ones we have right now are overly constrained. Instead, we need more
#       expressions, and arguments that control a relative small number of build-in Operators.
#   * Maybe get rid of the Operator Index / Layout Index stuff and replace those with names?
#   * Now that NodeDescriptions can be expressed as Values, I want to be able to create new child nodes at runtime and
#       have them placed in the Layout of the parent
#   We also need a lot more improvements on various fronts:
#   * Text drawing
#   * Read Paints (or at least colors) from SVG
#   * Better & More Layouts
#   The biggest unknown as of now is how this would integrate with Services to ensure that facts about the world are
#   updated reliably and in time, but I am fairly certain that this is either a domain-specific problem or already
#   solved and largely independent of the res of the code.
#   Finally, the goal of this whole exercise should be an editor that would allow me to bootstrap the notf design studio
#   from scratch. At first, I can probably start with a text editor and a live-updating application. Then, I would want
#   to use the application to visualize and eventually modify its own logic, so I don't have to use the text editor any
#   more. I suppose, I will have to use inkscape for quite a while for the design, but maybe not. Having the Designs
#   visualized in editor should not be too hard... And once I have that, I hope for a Singularity like event, where the
#   development speed of notf increases due to increased functionality of notf.
#   To the moon, my friends.

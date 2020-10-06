from __future__ import annotations

from random import randint
from typing import List, Tuple

from pynotf.data import Value, get_mutated_value
from pynotf.core import OperatorIndex, LayoutIndex, Node, get_app

# TODO: it is clear now that Node States can (and do) share most of their Design and Operators. But the transition still
#   clears and re-creates everything. This is not only inefficient, it also forces me to literally copy large parts of
#   the state when creating a new one.
window_design: Value = Value([
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
])


def produce_rect(index: int, count: int) -> Value:
    color: float = 0.4 + 0.5 * (index / count)
    # size_min: float = float(randint(0, 50))
    # size_preferred: float = size_min + float(randint(0, 50))
    # size_max: float = size_preferred + float(randint(0, 50))
    size_min: float = 20.
    size_preferred: float = 40.
    size_max: float = 60.
    return get_mutated_value(Node.VALUE, dict(
        interops=[],
        states=[
            ("default", get_mutated_value(Node.STATE, dict(
                operators=[],
                connections=[],
                design=Value([
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
                            radius=("constant", Value(0)),
                        )),
                        paint=('constant', Value(r=color, g=color, b=color, a=1)),
                        opacity=('constant', Value(1)),
                    ))
                ]),
                children=[],
                layout=(LayoutIndex.OVERLAY, Value()),
                claim=dict(
                    horizontal=(size_preferred, size_min,  size_max, 1, 0),
                    vertical=(size_preferred, size_min, size_max, 1, 0),
                )
            ))),
        ],
        transitions=[
            ('default', 'clicked'),
        ],
        initial="default",
    ))


def produce_rects(count: int) -> List[Tuple[str, Value]]:
    return [(f'rect{i}', produce_rect(i, count)) for i in range(count)]


window: Value = get_mutated_value(Node.VALUE, dict(
    interops=[
        # ('on_mouse_click', Value(x=0, y=0), 0),
    ],
    states=[
        ("default", get_mutated_value(Node.STATE, dict(
            operators=[
                # operator pair to move the window slowly from left to right and back (for testing)
                ('sine', OperatorIndex.SINE, Value()),
                ('translate_expression', OperatorIndex.NODE_EXPRESSION, Value(
                    schema=[253],  # single number
                    source="""node.get_interop('widget.xform').update(1, 0, 0, 1, float(arg), 0)""",
                )),
                ('filter_header_clicks', OperatorIndex.FILTER_EXPRESSION, Value(
                    schema=list(Value(x=0, y=0).get_schema()),  # mouse position
                    source="""
Aabrf(0, 0, node.grant.width, 30).contains(
    (node.window_xform.get_inverse() * M3f(e=float(arg['x']), f=float(arg['y']))).get_translation())
""",
                )),
                ('transition_to_clicked', OperatorIndex.NODE_TRANSITION, Value(target="clicked")),
            ],
            connections=[
                ('sine', 'translate_expression'),
                ('/|mouse_click_fact', 'filter_header_clicks'),
                ('filter_header_clicks', 'transition_to_clicked'),
            ],
            design=window_design,
            children=produce_rects(30),
            layout=(LayoutIndex.FLEXBOX, Value(
                spacing=10,
                cross_spacing=10,
                direction=1,
                alignment=1,
                padding=(0, 30, 0, 0),
                wrap=2,
            )),
            claim=dict(
                horizontal=(300, 300, 600, 1, 0),
                vertical=(400, 400, 400, 1, 0),
            )
        ))),
        ("clicked", get_mutated_value(Node.STATE, dict(
            operators=[
                ('filter_header_clicks', OperatorIndex.FILTER_EXPRESSION, Value(
                    schema=list(Value(x=0, y=0).get_schema()),  # mouse position
                    source="""
Aabrf(0, 0, node.grant.width, 30).contains(
    (node.window_xform.get_inverse() * M3f(e=float(arg['x']), f=float(arg['y']))).get_translation())
""",
                )),
                ('printer', OperatorIndex.PRINTER, Value(x=0, y=0)),
            ],
            connections=[
                ('/|mouse_click_fact', 'filter_header_clicks'),
                ('filter_header_clicks', 'printer'),
            ],
            design=window_design,
            children=produce_rects(30),
            layout=(LayoutIndex.FLEXBOX, Value(
                spacing=10,
                cross_spacing=10,
                direction=1,
                alignment=1,
                padding=(0, 30, 0, 0),
                wrap=2,
            )),
            claim=dict(
                horizontal=(300, 300, 600, 1, 0),
                vertical=(400, 400, 400, 1, 0),
            )
        ))),
    ],
    transitions=[
        ('default', 'clicked'),
    ],
    initial="default",
))

root_node: Value = get_mutated_value(Node.VALUE, dict(
    interops=[
        ('mouse_click_fact', Value(0, 0), 0),
    ],
    states=[
        ("default", get_mutated_value(Node.STATE, dict(
            operators=[],
            connections=[],
            design=Value(),  # TODO: allow setting the background color in the root node?
            children=[
                ("window", window),
            ],
            layout=(LayoutIndex.OVERLAY, Value()),
            claim=dict(
                horizontal=(0, 0, 0, 1, 0),
                vertical=(0, 0, 0, 1, 0),
            )
        ))),
    ],
    transitions=[],
    initial="default",
))


def pane() -> int:
    return get_app().run(root_node)

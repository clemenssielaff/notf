from __future__ import annotations

from math import inf

from pynotf.data import Value, get_mutated_value
from pynotf.core import LayoutIndex, Node, get_app


def get_box(index: int, count: int) -> Value:
    color: float = 0.4 + 0.5 * (index / count)
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
                    horizontal=dict(preferred=40, min=20, max=60, scale_factor=1, priority=0),
                    vertical=dict(preferred=40, min=20, max=60, scale_factor=1, priority=0),
                )
            ))),
        ],
        transitions=[
            ('default', 'clicked'),
        ],
        initial="default",
    ))


container: Value = get_mutated_value(Node.VALUE, dict(
        interops=[],
        states=[
            ("default", get_mutated_value(Node.STATE, dict(
                operators=[],
                connections=[],
                design=Value(),
                children=[(f'window{i}', get_box(i, 10)) for i in range(30)],
                layout=(LayoutIndex.FLEXBOX, Value(
                    spacing=10,
                    cross_spacing=5,
                    direction=1,
                    alignment=1,
                    padding=(0, 0, 0, 0),
                    wrap=2,
                )),
                claim=dict(
                    horizontal=dict(preferred=0, min=0, max=inf, scale_factor=1, priority=0),
                    vertical=dict(preferred=0, min=0, max=inf, scale_factor=1, priority=0),
                )
            ))),
        ],
        transitions=[],
        initial="default",
    ))

root_node: Value = get_mutated_value(Node.VALUE, dict(
    interops=[
        ('mouse_buttons', Value(pos=dict(x=0, y=0), action=0, button=0, buttons=0, modifiers=0), 0),
        ('mouse_position', Value(pos=dict(x=0, y=0), delta=dict(x=0, y=0), buttons=0, modifiers=0), 0),
    ],
    states=[
        ("default", get_mutated_value(Node.STATE, dict(
            operators=[],
            connections=[],
            design=Value(),
            children=[('container', container)],
            layout=(LayoutIndex.OVERLAY, Value()),
            # children=[(f'window{i}', get_box(i, 10)) for i in range(30)],
            # layout=(LayoutIndex.FLEXBOX, Value(
            #     spacing=10,
            #     cross_spacing=5,
            #     direction=1,
            #     alignment=1,
            #     padding=(0, 0, 0, 0),
            #     wrap=2,
            # )),
            claim=dict(
                horizontal=dict(preferred=0, min=0, max=inf, scale_factor=1, priority=0),
                vertical=dict(preferred=0, min=0, max=inf, scale_factor=1, priority=0),
            )
        ))),
    ],
    transitions=[],
    initial="default",
))


def flexbox() -> int:
    return get_app().run(root_node)

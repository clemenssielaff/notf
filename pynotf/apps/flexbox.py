from __future__ import annotations

from pynotf.data import Value, get_mutated_value
from pynotf.core import LayoutIndex, Node, get_app

window: Value = get_mutated_value(Node.VALUE, dict(
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
                    paint=('constant', Value(r=0.9, g=0.9, b=0.9, a=1)),
                    opacity=('constant', Value(1)),
                ))
            ]),
            children=[],
            layout=(LayoutIndex.OVERLAY, Value()),
            claim=dict(
                horizontal=dict(preferred=100, min=50, max=200, scale_factor=1, priority=0),
                vertical=dict(preferred=100, min=50, max=200, scale_factor=1, priority=0),
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
        ('mouse_fact', Value(0, 0), 0),
    ],
    states=[
        ("default", get_mutated_value(Node.STATE, dict(
            operators=[],
            connections=[],
            design=Value(),
            children=[(f'window{i}', window) for i in range(4)],
            layout=(LayoutIndex.FLEXBOX, Value(
                margin=10,
                spacing=2,
                direction=4,
                alignment=3,
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


def flexbox() -> int:
    return get_app().run(root_node)

from __future__ import annotations

import sys
from typing import List

from pynotf.data import Value, Path, Claim, Shape
from pynotf.core import NodeDescription, OperatorIndex, Design, LayoutDescription, LayoutIndex, NodeStateDescription, \
    get_app
from pynotf.extra.svg import parse_svg

NOTF_SHAPES: List[Shape] = []
for path_description in parse_svg('/home/clemens/code/notf/dev/img/notf_inline_white.svg'):
    NOTF_SHAPES.extend(path_description.shapes)

count_presses_node: NodeDescription = NodeDescription(
    interface=dict(
        key_down=Value(),
        bring_front=Value(),
    ),
    states=dict(
        default=NodeStateDescription(
            operators=dict(
                buffer=(OperatorIndex.BUFFER, Value(schema=Value.Schema(), time_span=1)),
                printer=(OperatorIndex.PRINTER, Value()),
            ),
            connections=[
                (Path('/:mouse_fact'), Path('buffer')),
                (Path('buffer'), Path('printer')),
            ],
            design=Design(
                Design.FillCall(
                    shape=Design.ConstantShapes(NOTF_SHAPES),
                    paint=Design.SolidColor(Design.Constant(Value(r=1, g=1, b=1, a=1))),
                )),
            children=dict(),
            layout=LayoutDescription(
                LayoutIndex.OVERLAY,
                Value(),
            ),
            claim=Claim(Claim.Stretch(10, 200, 200), Claim.Stretch(10, 200, 200)),
        ),
    ),
    transitions=[],
    initial_state='default',
)

pulsating_round_rect: Design.RoundedRect = Design.RoundedRect(
    x=Design.Constant(Value(0)), y=Design.Constant(Value(0)),
    width=Design.Expression(Value(0).get_schema(), "max(0, node.grant.width)"),
    height=Design.Expression(Value(0).get_schema(), "max(0, node.grant.height)"),
    radius=Design.Expression(Value(0).get_schema(), "max(0, node.roundness)"))

countdown_node: NodeDescription = NodeDescription(
    interface=dict(
        key_down=Value(),
        roundness=Value(10),
        bring_front=Value(),
        on_mouse_click=Value(x=0, y=0),
    ),
    states=dict(
        default=NodeStateDescription(
            operators=dict(
                factory=(OperatorIndex.FACTORY, Value(id=OperatorIndex.COUNTDOWN, args=Value(start=5))),
                printer=(OperatorIndex.PRINTER, Value(0)),
                mouse_click_printer=(OperatorIndex.PRINTER, Value(x=0, y=0)),
                mouse_pos_printer=(OperatorIndex.PRINTER, Value(0, 0)),
                sine=(OperatorIndex.SINE, Value()),
            ),
            connections=[
                (Path('/:key_fact'), Path('factory')),
                (Path('factory'), Path('printer')),
                (Path('/:mouse_fact'), Path('mouse_pos_printer')),
                (Path('sine'), Path(':roundness')),
                (Path(':on_mouse_click'), Path('mouse_click_printer')),
            ],
            design=Design(
                Design.FillCall(shape=pulsating_round_rect,
                                paint=Design.SolidColor(Design.Constant(Value(r=.5, g=.5, b=.5, a=1)))),
                Design.HitboxCall(shape=pulsating_round_rect, interface="on_mouse_click"),
            ),
            children=dict(),
            layout=LayoutDescription(
                LayoutIndex.OVERLAY,
                Value(),
            ),
            claim=Claim(Claim.Stretch(100, 200, 300), Claim.Stretch(100, 200, 300)),
        ),
    ),
    transitions=[],
    initial_state='default',
)

root_node: NodeDescription = NodeDescription(
    interface=dict(
        key_fact=Value(),
        mouse_fact=Value(0, 0),
        hitbox_fact=Value(0, 0),
    ),
    states=dict(
        default=NodeStateDescription(
            operators={},
            connections=[],
            design=Design(),
            children=dict(
                herbert=countdown_node,
                zanzibar=count_presses_node,
            ),
            layout=LayoutDescription(
                LayoutIndex.FLEXBOX,
                Value(margin=-100),
            ),
            claim=Claim(),
        )
    ),
    transitions=[],
    initial_state='default',
)

# MAIN #################################################################################################################

if __name__ == "__main__":
    sys.exit(get_app().run(root_node))

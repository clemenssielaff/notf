from __future__ import annotations

import sys
from typing import List, Tuple

from pynotf.data import Value, Shape, mutate_value
from pynotf.core import OperatorIndex, LayoutIndex, get_app, Node
from pynotf.extra.svg import parse_svg

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

count_presses_node: Value = mutate_value(Node.VALUE, dict(
    interface=[
        ('key_down', Value()),
        ('bring_front', Value()),
    ],
    states=[
        ("default", mutate_value(Node.STATE, dict(
            operators=[
                ('buffer', OperatorIndex.BUFFER, Value(schema=Value.Schema.create(Value()), time_span=1)),
                ('printer', OperatorIndex.PRINTER, Value(Value())),
            ],
            connections=[
                ('/:mouse_fact', 'buffer'),
                ('buffer', 'printer'),
            ],
            design=Value([
                ('fill', Value(
                    shape=('constant', Value([shape.as_value() for shape in notf_shapes])),
                    paint=('constant', Value(r=1, g=1, b=1, a=1)),
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

countdown_node: Value = mutate_value(Node.VALUE, dict(
    interface=[
        ('key_down', Value()),
        ('roundness', Value(10)),
        ('bring_front', Value()),
        ('on_mouse_click', Value(x=0, y=0)),
    ],
    states=[
        ("default", mutate_value(Node.STATE, dict(
            operators=[
                ('factory', OperatorIndex.FACTORY, Value(id=OperatorIndex.COUNTDOWN, args=Value(start=5))),
                ('printer', OperatorIndex.PRINTER, Value(Value(0))),
                ('mouse_click_printer', OperatorIndex.PRINTER, Value(x=0, y=0)),
                ('mouse_pos_printer', OperatorIndex.PRINTER, Value(0, 0)),
                ('sine', OperatorIndex.SINE, Value()),
                # TODO: it is weird to have name identifiers for the Design, but INDEX for Layout and Operators
            ],
            connections=[
                ('/:key_fact', 'factory'),
                ('factory', 'printer'),
                ('/:mouse_fact', 'mouse_pos_printer'),
                ('sine', ':roundness'),
                (':on_mouse_click', 'mouse_click_printer'),
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

root_node: Value = mutate_value(Node.VALUE, dict(
    interface=[
        ('key_fact', Value()),
        ('mouse_fact', Value(0, 0)),
        ('hitbox_fact', Value(0, 0)),
    ],
    states=[
        ("default", mutate_value(Node.STATE, dict(
            operators=[],
            connections=[],
            design=Value(),
            children=[
                ("herbert", countdown_node),
                ("zanzibar", count_presses_node),
            ],
            layout=(LayoutIndex.FLEXBOX, Value(
                margin=-100,
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
#   solved and largely independent of the rest of the code.
#   Finally, the goal of this whole exercise should be an editor that would allow me to bootstrap the notf design studio
#   from scratch. At first, I can probably start with a text editor and a live-updating application. Then, I would want
#   to use the application to visualize and eventually modify its own logic, so I don't have to use the text editor any
#   more. I suppose, I will have to use inkscape for quite a while for the design, but maybe not. Having the Designs
#   visualized in editor should not be too hard... And once I have that, I hope for a Singularity like event, where the
#   development speed of notf increases due to increased functionality of notf.
#   To the moon, my friends.

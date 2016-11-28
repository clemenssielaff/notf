from random import randint
from notf import *

def fill_color(painter, color):
    painter.begin()
    widget_size = painter.get_widget_size()
    # widget_size.width -= 50
    rect = Aabr(0, 0, widget_size.width, widget_size.height)
    painter.rect(rect)
    painter.set_fill(color)
    painter.fill()

def fill_orange(painter):
    fill_color(painter, Color("#c34200"))


def main():
    solid_orange = CanvasComponent()
    solid_orange.set_paint_function(fill_orange)

    factory = StateMachineFactory()
    default_state = factory.add_state("default")
    default_state.attach_component(solid_orange)
    state_machine = factory.produce(default_state)

    stack_layout = StackLayout(Layout.Direction.LEFT_TO_RIGHT)
    stack_layout.set_wrap(Layout.Wrap.WRAP)
    stack_layout.set_spacing(5)
    stack_layout.set_cross_spacing(10)
    stack_layout.set_padding(Padding.all(10))
    stack_layout.set_alignment(Layout.Alignment.START)
    stack_layout.set_cross_alignment(Layout.Alignment.START)
    stack_layout.set_content_alignment(Layout.Alignment.START)

    MAX_MIN = 100
    MAX_MAX = 200
    for i in range(50):
        widget = Widget()
        widget.set_state_machine(state_machine)
        stack_layout.add_item(widget)

        h_stretch = ClaimStretch()
        h_stretch.set_min(randint(0,MAX_MIN))
        h_stretch.set_max(randint(h_stretch.get_min(), MAX_MAX))
        h_stretch.set_preferred(randint(h_stretch.get_min(), h_stretch.get_max()))

        v_stretch = ClaimStretch()
        v_stretch.set_min(randint(0,MAX_MIN))
        v_stretch.set_max(randint(v_stretch.get_min(), MAX_MAX))
        v_stretch.set_preferred(randint(v_stretch.get_min(), v_stretch.get_max()))

        claim = Claim()
        claim.set_horizontal(h_stretch)
        claim.set_vertical(v_stretch)
        widget.set_claim(claim)

    window = Window()
    window.get_layout_root().set_item(stack_layout)

if __name__ == "__main__":
    main()

from importlib import reload
from random import randint
from notf import *


def fill_color(painter, color):
    painter.begin()
    widget_size = painter.get_widget_size()
    rect = Aabr(0, 0, widget_size.width, widget_size.height)
    painter.rect(rect)
    painter.set_fill(color)
    painter.fill()


def fill_orange(painter):
    fill_color(painter, Color("#c34200"))


def fill_blue(painter):
    fill_color(painter, Color("#2b60b8"))


def main():
    # produce the types that we are going to use in our glorious UI
    solid_orange = CanvasComponent()
    solid_orange.set_paint_function(fill_orange)
    orange_factory = StateMachineFactory()
    orange_state = orange_factory.add_state("orange")
    orange_state.attach_component(solid_orange)
    orange_machine = orange_factory.produce(orange_state)

    solid_blue = CanvasComponent()
    solid_blue.set_paint_function(fill_blue)
    blue_factory = StateMachineFactory()
    blue_state = blue_factory.add_state("blue")
    blue_state.attach_component(solid_blue)
    blue_machine = blue_factory.produce(blue_state)

    # produce the flex layout in the background
    stack_layout = StackLayout(Layout.Direction.LEFT_TO_RIGHT)
    stack_layout.set_wrap(Layout.Wrap.WRAP)
    stack_layout.set_spacing(5)
    stack_layout.set_cross_spacing(10)
    stack_layout.set_padding(Padding.all(10))
    stack_layout.set_alignment(Layout.Alignment.START)
    stack_layout.set_cross_alignment(Layout.Alignment.START)
    stack_layout.set_content_alignment(Layout.Alignment.START)
    max_min = 100
    max_max = 200
    for i in range(50):
        widget = Widget()
        widget.set_state_machine(orange_machine)
        stack_layout.add_item(widget)

        h_stretch = ClaimStretch()
        h_stretch.set_min(randint(0, max_min))
        h_stretch.set_max(randint(h_stretch.get_min(), max_max))
        h_stretch.set_preferred(randint(h_stretch.get_min(), h_stretch.get_max()))

        v_stretch = ClaimStretch()
        v_stretch.set_min(randint(0, max_min))
        v_stretch.set_max(randint(v_stretch.get_min(), max_max))
        v_stretch.set_preferred(randint(v_stretch.get_min(), v_stretch.get_max()))

        claim = Claim()
        claim.set_horizontal(h_stretch)
        claim.set_vertical(v_stretch)
        widget.set_claim(claim)

    # and the fill layout in the foreground
    widget = Widget()
    widget.set_state_machine(blue_machine)
    fill_layout = Overlayout()
    fill_layout.set_padding(Padding.all(50))
    fill_layout.add_item(widget)

    window_layout = Overlayout()
    window_layout.add_item(stack_layout)
    window_layout.add_item(fill_layout)
    window = Window()
    window.get_layout_root().set_item(window_layout)


def produce_notf():
    import build_notf
    reload(build_notf)
    build_notf.produce()


if __name__ == "__main__":
    main()

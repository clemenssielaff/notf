from random import randint
from notf import *

def fill_color(painter, color):
    painter.begin()
    widget_size = painter.get_widget_size()
    rect = Aabr(0, 0, widget_size.width, widget_size.height)
    painter.rect(rect)
    painter.set_fill(color)
    painter.fill()

class OrangeWidget(Widget):
    def paint(self, painter):
        fill_color(painter, Color("#c34200"))

class BlueWidget(Widget):
    def paint(self, painter):
        fill_color(painter, Color("#2b60b8"))

def main():
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

        orange_box = OrangeWidget()
        orange_box.set_claim(claim)
        stack_layout.add_item(orange_box)

    # and the fill layout in the foreground
    blue_box = BlueWidget()
    fill_layout = Overlayout()
    fill_layout.set_padding(Padding.all(50))
    fill_layout.add_item(blue_box)

    window_layout = Overlayout()
    window_layout.add_item(stack_layout)
    window_layout.add_item(fill_layout)
    window = Window()
    window.get_layout_root().set_item(window_layout)


def produce_notf():
    import build_notf
    build_notf.produce()


if __name__ == "__main__":
    main()
    pass

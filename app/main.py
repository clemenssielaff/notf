from random import randint
from notf import *

class ButtonWidget(Widget):
    def __init__(self):
        super().__init__()

        self.color = Color("#c34200")

        max_min = 100
        max_max = 200

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
        self.set_claim(claim)

    def paint(self, painter):
        painter.begin()
        widget_size = painter.get_widget_size()
        rect = Aabr(0, 0, widget_size.width, widget_size.height)
        painter.rect(rect)
        painter.set_fill(self.color)
        painter.fill()


class ButtonController(Controller):
    def __init__(self):

        self._widget = ButtonWidget()
        super().__init__(self._widget)

        #self.add_state("orange", self._turn_orange, self._nothin)
        self.add_state("blue", self._turn_blue, self._nothin)

        #self.transition_to("orange")

    def _nothin(self):
        pass

    def _turn_orange(self):
        pass
        #self._widget._color = Color("#c34200")

    def _turn_blue(self):
        pass
        #self._widget._color = Color("#2b60b8")

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

    for i in range(0):
        stack_layout.add_item(ButtonController())

    # and the fill layout in the foreground
    background_rect = ButtonWidget()
    fill_layout = Overlayout()
    fill_layout.set_padding(Padding.all(50))
    fill_layout.add_item(background_rect)

    window_layout = Overlayout()
    window_layout.add_item(fill_layout)
    window_layout.add_item(stack_layout)
    window = Window()
    window.get_layout_root().set_item(window_layout)


def produce_notf():
    import build_notf
    build_notf.produce()


if __name__ == "__main__":
    main()
    pass

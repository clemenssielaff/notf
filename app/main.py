from random import randint
from notf import *
from notf import _Font
import gc


class ButtonWidget(Widget):
    def __init__(self, number):
        super().__init__()

        self.color = Color("#000000")
        self.number = number
        self._font = _Font.fetch("Roboto-Regular")

        min_min = 20
        max_min = 100
        max_max = 200

        h_stretch = ClaimStretch()
        h_stretch.set_min(randint(min_min, max_min))
        h_stretch.set_max(randint(h_stretch.get_min(), max_max))
        h_stretch.set_preferred(randint(h_stretch.get_min(), h_stretch.get_max()))

        v_stretch = ClaimStretch()
        v_stretch.set_min(randint(min_min, max_min))
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
        painter.rounded_rect(rect, 12.)
        painter.set_fill(self.color)
        painter.fill()

        painter.set_text_align(Painter.Align(int(Painter.Align.CENTER) | int(Painter.Align.MIDDLE)))
        painter.set_fill(Color("#ffffff"))
        painter.text(widget_size.width / 2, widget_size.height/2, str(self.number))


class ButtonController(Controller):
    def __init__(self, number):
        super().__init__()

        self._widget = ButtonWidget(number)
        self.set_root_item(self._widget)

        self.add_state("orange", self._turn_orange, self._disable_all)
        self.add_state("blue", self._turn_blue, self._disable_all)

        self._orange_connection = self.on_mouse_button.connect(
            lambda event: self.transition_to("orange"),
            lambda event : event.action == notf.MouseAction.PRESS)

        self._blue_connection = self.on_mouse_button.connect(
            lambda event: self.transition_to("blue"),
            lambda event : event.action == notf.MouseAction.PRESS)

        self._disable_all()
        self.transition_to("orange")

    def _disable_all(self):
        self.on_mouse_move.disable()
        self.on_mouse_button.disable()

    def _turn_orange(self):
        self.on_mouse_button.enable(self._blue_connection)
        self._widget.color = Color("#c34200")
        self._widget.redraw()

    def _turn_blue(self, _ = None):
        self.on_mouse_button.enable(self._orange_connection)
        self._widget.color = Color("2b60b8")
        self._widget.redraw()


class WindowController(Controller):
    def __init__(self):
        super().__init__()

        # produce the flex layout in the background
        stack_layout = StackLayout(Layout.Direction.LEFT_TO_RIGHT)
        stack_layout.set_wrap(Layout.Wrap.WRAP)
        stack_layout.set_spacing(5)
        stack_layout.set_cross_spacing(10)
        stack_layout.set_padding(Padding.all(10))
        stack_layout.set_alignment(Layout.Alignment.START)
        stack_layout.set_cross_alignment(Layout.Alignment.START)
        stack_layout.set_content_alignment(Layout.Alignment.START)

        for i in range(64):
            stack_layout.add_item(ButtonController(i))

        # and the fill layout in the foreground
        background_rect = ButtonWidget("")
        fill_layout = Overlayout()
        fill_layout.set_padding(Padding.all(50))
        fill_layout.add_item(background_rect)

        window_layout = Overlayout()
        window_layout.add_item(fill_layout)
        window_layout.add_item(stack_layout)

        self.set_root_item(window_layout)

def main():
    window = Window()
    window_controller = WindowController()
    window.get_layout_root().set_item(window_controller)


def produce_notf():
    import build_notf
    build_notf.produce()


if __name__ == "__main__":
    main()

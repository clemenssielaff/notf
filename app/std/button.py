from notf import *
from notf import _Font as Font


class _ButtonWidget(Widget):
    def __init__(self, label, color):
        super().__init__()
        self.label = label
        self.is_pressed = False
        self.color = color
        self._font = Font.fetch("Roboto-Bold")

        h_stretch = ClaimStretch()
        h_stretch.set_min(80)
        h_stretch.set_max(200)
        h_stretch.set_preferred(200)

        v_stretch = ClaimStretch()
        v_stretch.set_fixed(30)

        claim = Claim()
        claim.set_horizontal(h_stretch)
        claim.set_vertical(v_stretch)
        self.set_claim(claim)

    def paint(self, painter):

        painter.set_font_size(18.)
        painter.set_font(self._font)
        text_width = painter.text_bounds(0, 0, self.label).width
        horizontal_claim = self.get_claim().get_horizontal()
        horizontal_claim.set_min(text_width)
        horizontal_claim.set_preferred(text_width + 60)
        horizontal_claim.set_preferred(text_width + 120)
        claim = Claim()
        claim.set_horizontal(horizontal_claim)
        claim.set_vertical(self.get_claim().get_vertical())
        self.set_claim(claim)

        corner_radius = 4.
        widget_size = painter.get_widget_size()
        if self.is_pressed:
            gradient = painter.LinearGradient(0, 0, 0, widget_size.height,
                                              Color(0, 0, 0, 32), Color(255, 255, 255, 32))
        else:
            gradient = painter.LinearGradient(0, 0, 0, widget_size.height,
                                              Color(255, 255, 255, 32), Color(0, 0, 0, 32))

        # draw the base
        painter.begin()
        base = Aabr(1, 1, widget_size.width - 2, widget_size.height - 2)
        painter.rounded_rect(base, corner_radius - 1)
        painter.set_fill(self.color)  # color
        painter.fill()
        painter.set_fill(gradient)  # add gradient
        painter.fill()

        # draw the frame
        painter.begin()
        frame = Aabr(0.5, 0.5, widget_size.width - 1, widget_size.height - 1)
        painter.rounded_rect(frame, corner_radius - 0.5)
        painter.set_stroke(Color(0, 0, 0, 48))
        painter.stroke()

        # draw label
        painter.set_font_size(18.)
        painter.set_font(self._font)
        painter.set_text_align(painter.Align(int(painter.Align.LEFT) | int(painter.Align.MIDDLE)))
        painter.set_fill(Color(0, 0, 0, 160))  # dark inlet of the text
        painter.text((widget_size.width * 0.5) - (text_width * 0.5),
                     widget_size.height * 0.5 - 1,
                     self.label)
        painter.set_fill(Color(255, 255, 255, 160))  # actual text
        painter.text(widget_size.width * 0.5 - text_width * 0.5,
                     widget_size.height * 0.5,
                     self.label)


class Button(Controller):
    def __init__(self, label=""):
        super().__init__()

        # define properties
        self.label = Property(label, "label")
        self._original_label = label
        self.is_pressed = Property(False, "isPressed")
        self.color = Property(Color(0, 96, 128), "color")

        # define widgets
        self._widget = _ButtonWidget(self.label, self.color)
        self.set_root_item(self._widget)

        # define property callbacks
        def update_widget():
            self._widget.label = self.label.get_value()
            self._widget.color = self.color.get_value()
            self._widget.is_pressed = self.is_pressed.get_value()
            self._widget.redraw()

        self.label.on_value_change.connect(update_widget, lambda: True)
        self.is_pressed.on_value_change.connect(update_widget, lambda: True)
        self.color.on_value_change.connect(update_widget, lambda: True)

        # define transitions
        def turn_red_up():
            self.on_mouse_button.enable(self.can_press_red)
            self.label.set_value("Derbness achieved!")
            self.color.set_value(Color(128, 16, 8))
            self.is_pressed.set_value(False)

        def turn_red_down():
            self.on_mouse_button.enable(self.can_turn_blue)
            self.is_pressed.set_value(True)

        def turn_blue_up():
            self.on_mouse_button.enable(self.can_press_blue)
            self.label.set_value(self._original_label)
            self.color.set_value(Color(0, 96, 128))
            self.is_pressed.set_value(False)

        def turn_blue_down():
            self.on_mouse_button.enable(self.can_turn_red)
            self.is_pressed.set_value(True)

        def disable_all_actions():
            self.on_mouse_button.disable()

        # define states
        self.add_state("red_up", turn_red_up, disable_all_actions)
        self.add_state("red_down", turn_red_down, disable_all_actions)
        self.add_state("blue_up", turn_blue_up, disable_all_actions)
        self.add_state("blue_down", turn_blue_down, disable_all_actions)

        # define user interactions
        self.can_turn_red = self.on_mouse_button.connect(
            lambda event: self.transition_to("red_up"),
            lambda event: event.action == notf.MouseAction.RELEASE)

        self.can_press_red = self.on_mouse_button.connect(
            lambda event: self.transition_to("red_down"),
            lambda event: event.action == notf.MouseAction.PRESS)

        self.can_turn_blue = self.on_mouse_button.connect(
            lambda event: self.transition_to("blue_up"),
            lambda event: event.action == notf.MouseAction.RELEASE)

        self.can_press_blue = self.on_mouse_button.connect(
            lambda event: self.transition_to("blue_down"),
            lambda event: event.action == notf.MouseAction.PRESS)

        # enter start state
        disable_all_actions()
        self.transition_to("blue_up")

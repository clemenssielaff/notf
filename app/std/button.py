from notf import *


class _ButtonWidget(Widget):
    def __init__(self, label, color):
        super().__init__()
        self.label = label
        self.is_pressed = False
        self.color = color

        h_stretch = ClaimStretch()
        h_stretch.set_min(80)
        h_stretch.set_max(300)
        h_stretch.set_preferred(200)

        v_stretch = ClaimStretch()
        v_stretch.set_fixed(30)

        claim = Claim()
        claim.set_horizontal(h_stretch)
        claim.set_vertical(v_stretch)
        self.set_claim(claim)

    def paint(self, painter):
        widget_size = painter.get_widget_size()
        if self.is_pressed:
            gradient = painter.LinearGradient(0, 0, 0, widget_size.height,
                                              Color(0, 0, 0, 32), Color(255, 255, 255, 32))
        else:
            gradient = painter.LinearGradient(0, 0, 0, widget_size.height,
                                              Color(255, 255, 255, 32), Color(0, 0, 0, 32))
        rect = Aabr(1, 1, widget_size.width - 2, widget_size.height - 2)

        painter.begin()
        painter.rounded_rect(rect, 8)

        # draw flat button
        painter.set_fill(self.color)
        painter.fill()

        # add gradient
        painter.set_fill(gradient)
        painter.fill()


class Button(Controller):
    def __init__(self, label=""):
        super().__init__()

        # define properties
        self.label = Property(label, "label")
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
            self.color.set_value(Color(128, 0, 0))
            self.is_pressed.set_value(False)

        def turn_red_down():
            self.on_mouse_button.enable(self.can_turn_blue)
            self.is_pressed.set_value(True)

        def turn_blue_up():
            self.on_mouse_button.enable(self.can_press_blue)
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

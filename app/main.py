from notf import *

def fill_color(painter, color):
    painter.begin()
    widget_size = painter.get_widget_size()
    # widget_size.width -= 50
    rect = Aabr(0, 0, widget_size.width, widget_size.height)
    painter.rect(rect)
    painter.set_fill(color)
    painter.fill()

def fill_red(painter):
    fill_color(painter, Color(255, 0, 0))

def fill_green(painter):
    fill_color(painter, Color(0, 255, 0))

def fill_blue(painter):
    fill_color(painter, Color(0, 0, 255))


def main():
    solid_red = CanvasComponent()
    solid_red.set_paint_function(fill_red)

    solid_green = CanvasComponent()
    solid_green.set_paint_function(fill_green)

    solid_blue = CanvasComponent()
    solid_blue.set_paint_function(fill_blue)

    stack_layout = StackLayout(StackDirection.RIGHT_TO_LEFT)
    # stack_layout.set_spacing(50)
    # stack_layout.set_padding(Padding(25, 200, 25, 200))

    # for i in range(1):
    red_rect = Widget()
    red_rect.add_component(solid_red)

    green_rect = Widget()
    green_rect.add_component(solid_green)

    blue_rect = Widget()
    blue_rect.add_component(solid_blue)

    stack_layout.add_item(green_rect)
    stack_layout.add_item(red_rect)
    stack_layout.add_item(blue_rect)

    window = Window()
    window.get_layout_root().set_item(stack_layout)

    claim = Claim()
    direction = ClaimDirection()
    direction.set_max(100)
    print(direction)
    claim.set_horizontal(direction)
    red_rect.set_claim(claim)


if __name__ == "__main__":
    main()

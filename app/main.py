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
    solid_blue = CanvasComponent()
    solid_blue.set_paint_function(fill_blue)

    stack_layout = StackLayout(Layout.Direction.LEFT_TO_RIGHT)
    stack_layout.set_wrapping(True)
    stack_layout.set_spacing(5)
    stack_layout.set_padding(Padding.all(10))
    stack_layout.set_alignment(Layout.Alignment.SPACE_AROUND)
    #stack_layout.set_cross_alignment(Layout.Alignment.CENTER)

    for i in range(50):
        widget = Widget()
        widget.add_component(solid_blue)
        stack_layout.add_item(widget)

        stretch = ClaimStretch()
        stretch.set_fixed(20)
        claim = Claim()
        claim.set_horizontal(stretch)
        claim.set_vertical(stretch)
        widget.set_claim(claim)

    window = Window()
    window.get_layout_root().set_item(stack_layout)

if __name__ == "__main__":
    main()

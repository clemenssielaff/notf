from notf import *

def paint(painter):
    painter.start_path();
    painter.circle(50, 50, 50);
    painter.circle(100, 100, 20);
    painter.fill();

def main():
    canvas = CanvasComponent()
    canvas.set_paint_function(paint)

    circle = Widget()
    circle.add_component(canvas)

    window = Window()
    window.get_layout_root().set_item(circle)

if __name__ == "__main__":
    main()

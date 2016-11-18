from notf import *

def paint(painter):
    painter.begin()
    painter.circle(50, 50, 50);
    painter.circle(100, 100, 20);
    paint = painter.LinearGradient(Vector2(0, 0), Vector2(100, 0), Color(128, 128, 255), Color(255,128,128))
    painter.set_fill(paint)
    painter.fill()

    painter.begin()
    tex = Texture2("face.png", int(TextureFlags.GENERATE_MIPMAPS))
    img_rect = Aabr(128, 128)
    img_rect.center = Vector2(500, 400)
    img = painter.ImagePattern(tex, img_rect)
    painter.rect(img_rect)
    painter.set_fill(img)
    painter.fill()

    font = Font("Roboto-Bold")
    painter.set_font(font)
    painter.text(200, 200, "This is very derbe")

def main():
    canvas = CanvasComponent()
    canvas.set_paint_function(paint)

    circle = Widget()
    circle.add_component(canvas)

    window = Window()
    window.get_layout_root().set_item(circle)

    resource_manager = ResourceManager()
    resource_manager.cleanup()

if __name__ == "__main__":
    main()

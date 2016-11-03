from notf import *


def main():
    shader = Shader("sprite", "sprite.vert", "sprite.frag")
    sprite_renderer = SpriteRenderer(shader)
    blue_texture = TextureComponent(Texture2("blue.png"))
    red_texture = TextureComponent(Texture2("red.png"))
    green_texture = TextureComponent(Texture2("green.png"))

    background = Widget()
    background.add_component(sprite_renderer)
    background.add_component(blue_texture)

    left = Widget()
    left.add_component(sprite_renderer)
    left.add_component(green_texture)

    right = Widget()
    right.add_component(sprite_renderer)
    right.add_component(red_texture)

    horizontal_layout = StackLayout(STACK_DIRECTION.LEFT_TO_RIGHT)
    horizontal_layout.add_item(left)
    horizontal_layout.add_item(right)

    vertical_layout = StackLayout(STACK_DIRECTION.TOP_TO_BOTTOM)
    vertical_layout.add_item(horizontal_layout)
    vertical_layout.add_item(background)

    window = Window()
    window.get_layout_root().set_item(vertical_layout)


if __name__ == "__main__":
    main()

#include "breakout/gameobject.hpp"

#include "graphics/texture2.hpp"
#include "breakout/spriterenderer.hpp"

void GameObject::draw(SpriteRenderer& renderer)
{
    renderer.drawSprite(sprite.get(), position, size, rotation, color);
}

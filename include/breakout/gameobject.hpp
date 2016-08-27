#pragma once

#include <memory>

#include <glm/glm.hpp>

namespace signal {
class Texture2;
}
using namespace signal;

class SpriteRenderer;

class GameObject {

public: // methods
    explicit GameObject()
        : position(0, 0)
        , size(1, 1)
        , color(1)
        , velocity(0)
        , rotation(0)
        , is_solid(false)
        , is_destroyed(false)
        , sprite()
    {
    }

    GameObject(glm::vec2 pos, glm::vec2 size, std::shared_ptr<Texture2> sprite,
               glm::vec3 color = glm::vec3(1.0f), glm::vec2 velocity = glm::vec2(0.0f, 0.0f))
        : position(pos)
        , size(size)
        , color(color)
        , velocity(velocity)
        , rotation(0)
        , is_solid(false)
        , is_destroyed(false)
        , sprite(sprite)
    {
    }

    virtual ~GameObject() = default;

    virtual void draw(SpriteRenderer& renderer);

public: // fields
    glm::vec2 position;
    glm::vec2 size;
    glm::vec3 color;
    glm::vec2 velocity;
    float rotation;
    bool is_solid;
    bool is_destroyed;
    std::shared_ptr<Texture2> sprite;
};

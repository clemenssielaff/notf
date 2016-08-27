#pragma once

#include <glad/glad.h>

#include "breakout/gameobject.hpp"

namespace signal {
class Texture2;
}
using namespace signal;

class BallObject : public GameObject {

public: // methods
    explicit BallObject()
        : GameObject()
        , radius(12.5)
        , is_stuck(true)
    {
    }

    BallObject(glm::vec2 pos, float radius, glm::vec2 velocity, std::shared_ptr<Texture2> sprite)
        : GameObject(pos, glm::vec2(radius * 2, radius * 2), sprite, glm::vec3(1.0f), velocity)
        , radius(radius)
        , is_stuck(true)
    {
    }

    // Moves the ball, keeping it constrained within the window bounds (except bottom edge); returns new position
    glm::vec2 move(GLfloat dt, GLuint window_width);

    // Resets the ball to original state with given position and velocity
    void reset(glm::vec2 position, glm::vec2 velocity);

public: // fields
    float radius;
    bool is_stuck;
};

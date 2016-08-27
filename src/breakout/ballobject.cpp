#include "breakout/ballobject.hpp"

#include <algorithm>

glm::vec2 BallObject::move(GLfloat dt, GLuint window_width)
{
    // If not stuck to player board
    if (!is_stuck) {
        // Move the ball
        position += velocity * dt;

        // Then check if outside window bounds and if so, reverse velocity and restore at correct position
        if (position.x <= 0.0f) {
            velocity.x = -velocity.x;
            position.x = 0.0f;
        }
        else if (position.x + size.x >= window_width) {
            velocity.x = -velocity.x;
            position.x = window_width - size.x;
        }
        if (position.y <= 0.0f) {
            velocity.y = -velocity.y;
            position.y = 0.0f;
        }
    }
    return position;
}

// Resets the ball to initial Stuck Position (if ball is outside window bounds)
void BallObject::reset(glm::vec2 position, glm::vec2 velocity)
{
    position = position;
    velocity = velocity;
    is_stuck = true;
}

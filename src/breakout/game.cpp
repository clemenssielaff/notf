#include "breakout/game.hpp"

#include <algorithm>

#include <glm/gtc/matrix_transform.hpp>

#include "breakout/spriterenderer.hpp"
#include "core/glfw_wrapper.hpp"
#include "core/resource_manager.hpp"

namespace {
const glm::vec2 PLAYER_SIZE(100, 20);
const GLfloat PLAYER_VELOCITY(500.0f);
const glm::vec2 INITIAL_BALL_VELOCITY(100.0f, -350.0f);
const GLfloat BALL_RADIUS = 12.5f;

Direction VectorDirection(glm::vec2 target)
{
    glm::vec2 compass[] = {
        glm::vec2(0.0f, 1.0f), // up
        glm::vec2(1.0f, 0.0f), // right
        glm::vec2(0.0f, -1.0f), // down
        glm::vec2(-1.0f, 0.0f) // left
    };
    GLfloat max = 0.0f;
    GLint best_match = -1;
    for (GLint i = 0; i < 4; i++) {
        GLfloat dot_product = glm::dot(glm::normalize(target), compass[i]);
        if (dot_product > max) {
            max = dot_product;
            best_match = i;
        }
    }
    return static_cast<Direction>(best_match);
}

GLboolean check_collision(const GameObject& one, const GameObject& two) // AABB - AABB collision
{
    // Collision x-axis?
    bool collisionX = one.position.x + one.size.x >= two.position.x && two.position.x + two.size.x >= one.position.x;
    // Collision y-axis?
    bool collisionY = one.position.y + one.size.y >= two.position.y && two.position.y + two.size.y >= one.position.y;
    // Collision only if on both axes
    return collisionX && collisionY;
}

Collision check_collision(const BallObject& one, const GameObject& two) // AABB - Circle collision
{
    // Get center point circle first
    glm::vec2 center(one.position + one.radius);
    // Calculate AABB info (center, half-extents)
    glm::vec2 aabb_half_extents(two.size.x / 2, two.size.y / 2);
    glm::vec2 aabb_center(
        two.position.x + aabb_half_extents.x,
        two.position.y + aabb_half_extents.y);
    // Get difference vector between both centers
    glm::vec2 difference = center - aabb_center;
    glm::vec2 clamped = glm::clamp(difference, -aabb_half_extents, aabb_half_extents);
    // Add clamped value to AABB_center and we get the value of box closest to circle
    glm::vec2 closest = aabb_center + clamped;
    // Retrieve vector between center circle and closest point AABB and check if length <= radius
    difference = closest - center;

    if (glm::length(difference) < one.radius) // not <= since in that case a collision also occurs when object one exactly touches object two, which they are at the end of each collision resolution stage.
        return std::make_tuple(GL_TRUE, VectorDirection(difference), difference);
    else
        return std::make_tuple(GL_FALSE, UP, glm::vec2(0, 0));
}
}

Game::Game(GLuint width, GLuint height)
    : m_state(STATE::ACTIVE)
    , m_keys()
    , m_width(width)
    , m_height(height)
    , m_renderer()
    , m_resource_manager()
    , m_log_handler{128, 200} // initial size of the log buffers
    , m_levels()
    , m_current_level(2)
    , m_paddle()
    , m_ball()
{
    m_resource_manager.set_texture_directory("/home/clemens/code/signal-ui/res/textures");
    m_resource_manager.set_shader_directory("/home/clemens/code/signal-ui/res/shaders");

    // install the log handler first, to catch errors right away
    install_log_message_handler(std::bind(&LogHandler::push_log, &m_log_handler, std::placeholders::_1));
    m_log_handler.start();
}

Game::~Game()
{
    m_levels.clear();
    m_resource_manager.clear();
    m_log_handler.stop();
    m_log_handler.join();
}

void Game::init()
{
    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(m_width), static_cast<GLfloat>(m_height), 0.0f, -1.0f, 1.0f);

    m_resource_manager.get_texture("face.png");

    auto shader = m_resource_manager.build_shader("sprite", "sprite.vert", "sprite.frag");
    shader->use();
    shader->set_uniform("image", 0);
    shader->set_uniform("projection", projection);

    m_renderer = std::make_unique<SpriteRenderer>(std::move(shader));

    GLuint levelHeight = static_cast<GLuint>(m_height * 0.52);

    GameLevel one(&m_resource_manager);
    GameLevel two(&m_resource_manager);
    GameLevel three(&m_resource_manager);
    GameLevel four(&m_resource_manager);

    one.load("/home/clemens/code/signal-ui/res/levels/standard.lvl", m_width, levelHeight);
    two.load("/home/clemens/code/signal-ui/res/levels/small_gaps.lvl", m_width, levelHeight);
    three.load("/home/clemens/code/signal-ui/res/levels/space_invader.lvl", m_width, levelHeight);
    four.load("/home/clemens/code/signal-ui/res/levels/bounce_galore.lvl", m_width, levelHeight);
    m_levels.emplace_back(std::move(one));
    m_levels.emplace_back(std::move(two));
    m_levels.emplace_back(std::move(three));
    m_levels.emplace_back(std::move(four));

    glm::vec2 playerPos = glm::vec2(m_width / 2 - PLAYER_SIZE.x / 2, m_height - PLAYER_SIZE.y);
    m_paddle = GameObject(playerPos, PLAYER_SIZE, m_resource_manager.get_texture("paddle.png"));

    glm::vec2 ballPos = playerPos + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -BALL_RADIUS * 2);
    m_ball = BallObject(ballPos, BALL_RADIUS, INITIAL_BALL_VELOCITY, m_resource_manager.get_texture("face.png"));
}

void Game::update(GLfloat dt)
{
    m_ball.move(dt, m_width);
    do_collisions();

    if (m_ball.position.y >= m_height) {
        reset_level();
        reset_player();
    }
}

void Game::process_input(GLfloat dt)
{
    if (m_state == STATE::ACTIVE) {
        GLfloat velocity = PLAYER_VELOCITY * dt;
        // Move playerboard
        if (m_keys[GLFW_KEY_A]) {
            GLfloat ball_offset = m_ball.position.x - m_paddle.position.x;
            m_paddle.position.x = max(0.0f, m_paddle.position.x - velocity);
            if (m_ball.is_stuck) {
                m_ball.position.x = m_paddle.position.x + ball_offset;
            }
        }
        if (m_keys[GLFW_KEY_D]) {
            GLfloat ball_offset = m_ball.position.x - m_paddle.position.x;
            m_paddle.position.x = min(m_width - m_paddle.size.x, m_paddle.position.x + velocity);
            if (m_ball.is_stuck) {
                m_ball.position.x = m_paddle.position.x + ball_offset;
            }
        }
        if (m_keys[GLFW_KEY_SPACE]) {
            m_ball.is_stuck = false;
        }
    }
}

void Game::render()
{
    auto background = m_resource_manager.get_texture("background.jpg");

    if (m_state == STATE::ACTIVE) {
        m_renderer->drawSprite(background.get(), glm::vec2(0, 0), glm::vec2(m_width, m_height), 0.0f);
        m_levels[m_current_level].draw(*m_renderer);
        m_paddle.draw(*m_renderer);
        m_ball.draw(*m_renderer);
    }
}

void Game::do_collisions()
{
    for (GameObject& brick : m_levels[m_current_level].get_bricks()) {
        if (brick.is_destroyed) {
            continue;
        }
        Collision collision = check_collision(m_ball, brick);
        if (std::get<0>(collision)) // If collision is true
        {
            // Destroy block if not solid
            if (!brick.is_solid) {
                brick.is_destroyed = GL_TRUE;
            }
            // Collision resolution
            Direction dir = std::get<1>(collision);
            glm::vec2 diff_vector = std::get<2>(collision);
            if (dir == LEFT || dir == RIGHT) // Horizontal collision
            {
                m_ball.velocity.x = -m_ball.velocity.x; // Reverse horizontal velocity
                // Relocate
                GLfloat penetration = m_ball.radius - std::abs(diff_vector.x);
                if (dir == LEFT)
                    m_ball.position.x += penetration; // Move ball to right
                else
                    m_ball.position.x -= penetration; // Move ball to left;
            }
            else // Vertical collision
            {
                m_ball.velocity.y = -m_ball.velocity.y; // Reverse vertical velocity
                // Relocate
                GLfloat penetration = m_ball.radius - std::abs(diff_vector.y);
                if (dir == UP)
                    m_ball.position.y -= penetration; // Move ball bback up
                else
                    m_ball.position.y += penetration; // Move ball back down
            }
        }
    }

    // Also check collisions for player pad (unless stuck)
    Collision result = check_collision(m_ball, m_paddle);
    if (!m_ball.is_stuck && std::get<0>(result)) {
        // Check where it hit the board, and change velocity based on where it hit the board
        GLfloat centerBoard = m_paddle.position.x + m_paddle.size.x / 2;
        GLfloat distance = (m_ball.position.x + m_ball.radius) - centerBoard;
        GLfloat percentage = distance / (m_paddle.size.x / 2);
        // Then move accordingly
        GLfloat strength = 2.0f;
        glm::vec2 oldVelocity = m_ball.velocity;
        m_ball.velocity.x = INITIAL_BALL_VELOCITY.x * percentage * strength;
        //m_ball.velocity.y = -m_ball.velocity.y;
        m_ball.velocity = glm::normalize(m_ball.velocity) * glm::length(oldVelocity); // Keep speed consistent over both axes (multiply by length of old velocity, so total strength is not changed)
        // Fix sticky paddle
        m_ball.velocity.y = -1 * abs(m_ball.velocity.y);
    }
}

void Game::reset_level()
{
    GLuint levelHeight = static_cast<GLuint>(m_height * 0.52);
    if (m_current_level == 0) {
        m_levels[0].load("/home/clemens/code/signal-ui/res/levels/standard.lvl", m_width, levelHeight);
    }
    else if (m_current_level == 1) {
        m_levels[1].load("/home/clemens/code/signal-ui/res/levels/small_gaps.lvl", m_width, levelHeight);
    }
    else if (m_current_level == 2) {
        m_levels[2].load("/home/clemens/code/signal-ui/res/levels/space_invader.lvl", m_width, levelHeight);
    }
    else if (m_current_level == 3) {
        m_levels[3].load("/home/clemens/code/signal-ui/res/levels/bounce_galore.lvl", m_width, levelHeight);
    }
}

void Game::reset_player()
{
    // Reset player/ball stats
    m_paddle.size = PLAYER_SIZE;
    m_paddle.position = glm::vec2(m_width / 2 - PLAYER_SIZE.x / 2, m_height - PLAYER_SIZE.y);
    m_ball.position = m_paddle.position + glm::vec2(PLAYER_SIZE.x / 2 - BALL_RADIUS, -(BALL_RADIUS * 2));
    m_ball.velocity = INITIAL_BALL_VELOCITY;
    m_ball.is_stuck = true;
}

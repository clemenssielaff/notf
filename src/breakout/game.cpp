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
}

Game::Game(GLuint width, GLuint height)
    : m_state(STATE::ACTIVE)
    , m_keys()
    , m_width(width)
    , m_height(height)
    , m_renderer()
    , m_resource_manager("/home/clemens/code/signal-ui/res/textures",
                         "/home/clemens/code/signal-ui/res/shaders")
    , m_log_handler{128, 200} // initial size of the log buffers
    , m_levels()
    , m_current_level(0)
    , m_paddle()
    , m_ball()
{
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

void Game::update(double dt)
{
    m_ball.move(dt, m_width);
}

void Game::processInput(double dt)
{
    if (m_state == STATE::ACTIVE) {
        GLfloat velocity = PLAYER_VELOCITY * dt;
        // Move playerboard
        if (m_keys[GLFW_KEY_A]) {
            m_paddle.position.x = max(0.0f, m_paddle.position.x - velocity);
            if (m_ball.is_stuck){
                m_ball.position.x -= velocity;
            }
        }
        if (m_keys[GLFW_KEY_D]) {
            m_paddle.position.x = min(m_width - m_paddle.size.x, m_paddle.position.x + velocity);
            if (m_ball.is_stuck){
                m_ball.position.x += velocity;
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

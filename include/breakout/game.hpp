#pragma once

#include <memory>

#include "common/log.hpp"
#include "core/resource_manager.hpp"
#include "graphics/gl_forwards.hpp"
using namespace notf;

#include "breakout/ballobject.hpp"
#include "breakout/gamelevel.hpp"

class SpriteRenderer;

// Represents the four possible (collision) directions
enum Direction {
    UP,
    RIGHT,
    DOWN,
    LEFT
};

// Defines a Collision typedef that represents collision data
typedef std::tuple<GLboolean, Direction, glm::vec2> Collision;

class Game {

public: // enum
    /// @brief The current state of the game.
    enum class STATE {
        ACTIVE,
        MENU,
        WIN
    };

public: // methods
    /// @brief Constructor
    ///
    /// @param width    Width of the Window in pixels.
    /// @param height   Height of the Window in pixels.
    explicit Game(GLuint width, GLuint height);

    ~Game();

    // Initialize game state (load all shaders/textures/levels)
    void init();

    // GameLoop
    void process_input(GLfloat dt);
    void update(GLfloat dt);
    void render();
    void do_collisions();
    void reset_level();
    void reset_player();

    void set_state(STATE state) { m_state = state; }

    GLboolean get_key(int key) { return m_keys[key]; }
    void set_key(int key, GLboolean value) { m_keys[key] = value; }

private: // fields
    /// @brief Game state
    STATE m_state;

    /// @brief Keyboard state
    GLboolean m_keys[1024];

    /// @brief Width of the Window in pixels.
    GLuint m_width;

    /// @brief Height of the Window in pixels.
    GLuint m_height;

    std::unique_ptr<SpriteRenderer> m_renderer;

    ResourceManager m_resource_manager;

    /// @brief The log handler thread used to format and print out log messages in a thread-safe manner.
    LogHandler m_log_handler;

    std::vector<GameLevel> m_levels;
    size_t m_current_level;

    GameObject m_paddle;

    BallObject m_ball;
};

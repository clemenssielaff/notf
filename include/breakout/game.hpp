#pragma once

#include "graphics/gl_forwards.hpp"

namespace signal {

class Game {

public: // enum
    /// \brief The current state of the game.
    enum class STATE {
        ACTIVE,
        MENU,
        WIN
    };

public: // methods
    /// \brief Constructor
    ///
    /// \param width    Width of the Window in pixels.
    /// \param height   Height of the Window in pixels.
    explicit Game(GLuint width, GLuint height);

    ~Game();

    // Initialize game state (load all shaders/textures/levels)
    void Init();

    // GameLoop
    void ProcessInput(GLfloat dt);
    void Update(GLfloat dt);
    void Render();

private: // fields
    /// \brief Game state
//    GameState m_state;

    /// \brief Keyboard state
    GLboolean m_keys[1024];

    /// \brief Width of the Window in pixels.
    GLuint m_width;

    /// \brief Height of the Window in pixels.
    GLuint m_height;
};

} // namespace signal

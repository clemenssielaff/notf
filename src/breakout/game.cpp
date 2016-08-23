#include "breakout/game.hpp"

#include "breakout/resource_manager.hpp"

namespace signal {

Game::Game(GLuint width, GLuint height)
//    : m_state(STATE::ACTIVE)
    : m_keys()
    , m_width(width)
    , m_height(height)
{
}

Game::~Game()
{
}

void Game::Init()
{
}

void Game::Update(GLfloat dt)
{
}

void Game::ProcessInput(GLfloat dt)
{
}

void Game::Render()
{
}

} // namespace signal

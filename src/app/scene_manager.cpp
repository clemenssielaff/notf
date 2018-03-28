#include "app/scene_manager.hpp"

#include "app/window.hpp"

NOTF_OPEN_NAMESPACE

SceneManager::SceneManager(Window& window) : m_window(window), m_current_state(create_state()) {}

SceneManager::~SceneManager() = default;

void SceneManager::enter_state(StatePtr state)
{
    m_current_state = std::move(state);
    m_window.request_redraw();
}

NOTF_CLOSE_NAMESPACE

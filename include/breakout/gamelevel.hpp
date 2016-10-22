#pragma once

#include <vector>

#include <glad/glad.h>

#include "breakout/gameobject.hpp"
#include "breakout/spriterenderer.hpp"

#include "core/resource_manager.hpp"
using namespace notf;

class GameLevel {
public:
    explicit GameLevel(ResourceManager* resource_manager)
        : m_resource_manager(resource_manager)
    {
    }

    void load(const GLchar* file, GLuint levelWidth, GLuint levelHeight);

    void draw(SpriteRenderer& renderer);

    GLboolean isCompleted();

    std::vector<GameObject>& get_bricks() { return m_bricks; }

private:
    // Initialize level from tile data
    void init(std::vector<std::vector<GLuint>> tileData, GLuint screenWidth, GLuint screenHeight);

private: // fields
    std::vector<GameObject> m_bricks;

    ResourceManager* m_resource_manager;
};

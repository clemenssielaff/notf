#include "breakout/gamelevel.hpp"

#include <fstream>
#include <sstream>

void GameLevel::load(const GLchar* file, GLuint levelWidth, GLuint levelHeight)
{
    // Clear old data
    m_bricks.clear();
    // Load from file
    GLuint tileCode;
    std::string line;
    std::ifstream fstream(file);
    std::vector<std::vector<GLuint>> tileData;
    if (fstream) {
        while (std::getline(fstream, line)) // Read each line from level file
        {
            std::istringstream sstream(line);
            std::vector<GLuint> row;
            while (sstream >> tileCode) // Read each word seperated by spaces
                row.push_back(tileCode);
            tileData.push_back(row);
        }
        if (tileData.size() > 0) {
            this->init(tileData, levelWidth, levelHeight);
        }
    }
}

void GameLevel::draw(SpriteRenderer& renderer)
{
    for (GameObject& tile : m_bricks) {
        if (!tile.is_destroyed) {
            tile.draw(renderer);
        }
    }
}

GLboolean GameLevel::isCompleted()
{
    for (GameObject& tile : m_bricks) {
        if (!tile.is_solid && !tile.is_destroyed) {
            return GL_FALSE;
        }
    }
    return GL_TRUE;
}

void GameLevel::init(std::vector<std::vector<GLuint>> tileData, GLuint screenWidth, GLuint screenHeight)
{
    // Calculate dimensions
    GLuint height = static_cast<GLuint>(tileData.size());
    GLuint width = static_cast<GLuint>(tileData[0].size()); // Note we can index vector at [0] since this function is only called if height > 0
    GLfloat unit_width = screenWidth / static_cast<GLfloat>(width);
    GLfloat unit_height = screenHeight /static_cast<GLfloat>(height);
    // Initialize level tiles based on tileData
    for (GLuint y = 0; y < height; ++y) {
        for (GLuint x = 0; x < width; ++x) {
            // Check block type from level data (2D level array)
            if (tileData[y][x] == 1) // Solid
            {
                glm::vec2 pos(unit_width * x, unit_height * y);
                glm::vec2 size(unit_width, unit_height);
                GameObject obj(pos, size, m_resource_manager->get_texture("block_solid.png"), glm::vec3(0.8f, 0.8f, 0.7f));
                obj.is_solid = true;
                m_bricks.push_back(obj);
            }
            else if (tileData[y][x] > 1) // Non-solid; now determine its color based on level data
            {
                glm::vec3 color = glm::vec3(1.0f); // original: white
                if (tileData[y][x] == 2)
                    color = glm::vec3(0.2f, 0.6f, 1.0f);
                else if (tileData[y][x] == 3)
                    color = glm::vec3(0.0f, 0.7f, 0.0f);
                else if (tileData[y][x] == 4)
                    color = glm::vec3(0.8f, 0.8f, 0.4f);
                else if (tileData[y][x] == 5)
                    color = glm::vec3(1.0f, 0.5f, 0.0f);

                glm::vec2 pos(unit_width * x, unit_height * y);
                glm::vec2 size(unit_width, unit_height);
                m_bricks.push_back(GameObject(pos, size, m_resource_manager->get_texture("block.png"), color));
            }
        }
    }
}

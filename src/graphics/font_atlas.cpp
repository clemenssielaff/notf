#include "graphics/font_atlas.hpp"

#include <assert.h>
#include <limits>
#include <vector>

#include "common/float_utils.hpp"
#include "common/log.hpp"
#include "common/vector_utils.hpp"
#include "core/glfw_wrapper.hpp"

namespace anonymous {

/** Checks if a SkylineNode index is valid. */
bool is_valid_index(const size_t index) { return index != std::numeric_limits<size_t>::max(); }

} // namespace anonymous

namespace notf {

void FontAtlas::WasteMap::initialize(const coord_t width, const coord_t height)
{
    // start with a single big free rectangle, that spans the whole bin
    m_free_rects.clear();
    m_free_rects.emplace_back(0, 0, width, height);
}

FontAtlas::FontAtlas(const coord_t width, const coord_t height)
    : m_texture_id(0)
    , m_width(width)
    , m_height(height)
    , m_used_area(0)
    , m_nodes()
    , m_waste()
{
    // create the atlas texture
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &m_texture_id);

    // initialize
    reset();

    log_trace << "Created font atlas of size " << m_width << "x" << m_height << " with ID: " << m_texture_id;
}

FontAtlas::~FontAtlas()
{
    glDeleteTextures(1, &m_texture_id);
    log_trace << "Deleted font atlas with ID: " << m_texture_id;
}

void FontAtlas::reset()
{
    // create a flat skyline of zero height
    m_nodes.clear();
    m_nodes.emplace_back(0, 0, m_width);
    m_waste.initialize(m_width, m_height);
    m_used_area = 0;

    // fill the atlas with zeros
    const std::vector<uchar> zeros(static_cast<size_t>(m_width * m_height), 0);
    glBindTexture(GL_TEXTURE_2D, m_texture_id);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, m_width, m_height, 0, GL_RED, GL_UNSIGNED_BYTE, &zeros[0]);
}

FontAtlas::ScoredRect FontAtlas::_get_rect(const coord_t width, const coord_t height) const
{
    ScoredRect result;
    result.rect        = {0, 0, width, height};
    result.best_index  = std::numeric_limits<size_t>::max();
    result.best_width  = std::numeric_limits<coord_t>::max();
    result.best_height = std::numeric_limits<coord_t>::max();

    for (size_t node_index = 0; node_index < m_nodes.size(); ++node_index) {
        const SkylineNode& currrent_node = m_nodes[node_index];

        // if the rect is too wide, there's nothing more we can do
        if (currrent_node.x + width > m_width) {
            break;
        }

        // find the y-coordinate at which the rectangle could be placed above the current node
        coord_t y = 0;
        {
            int remaining_width = static_cast<int>(width);
            for (size_t spanned_index = node_index; remaining_width > 0; ++spanned_index) {
                assert(spanned_index < m_nodes.size());
                y = max(y, m_nodes[spanned_index].y);
                if (y + height > m_height) {
                    y = std::numeric_limits<coord_t>::max();
                    break;
                }
                remaining_width -= m_nodes[spanned_index].width;
            }
        }
        // treat the topmost y value as invalid,
        // because even if that was the correct answer, you would not be able to fit a rectangle there anyway
        if (y == std::numeric_limits<coord_t>::max()) {
            continue;
        }

        // if the rectangle fits, check if it is a better fit than the last one and update the result if it is
        if (y + height < result.best_height
            || (y + height == result.best_height && currrent_node.width < result.best_width)) {
            result.rect.x      = currrent_node.x;
            result.rect.y      = y;
            result.best_index  = node_index;
            result.best_width  = currrent_node.width;
            result.best_height = y + height;
        }
    }

    return result;
}

void FontAtlas::_add_node(const size_t node_index, const Rect& rect)
{
    // First track all wasted areas and mark them into the waste map if we're using one.
    //	AddWasteMapArea(skylineNodeIndex, rect.width, rect.height, rect.y);

    { // create and insert the new node
        SkylineNode new_node;
        new_node.x     = rect.x;
        new_node.y     = rect.y + rect.height;
        new_node.width = rect.width;

        assert(new_node.x + new_node.width <= m_width);
        assert(new_node.y <= m_height);

        m_nodes.emplace(iterator_at(m_nodes, node_index), std::move(new_node));
    }

    // adjust all affected nodes to the right
    const SkylineNode& new_node = m_nodes[node_index];
    const coord_t frontier      = new_node.x + new_node.width;
    for (size_t i = node_index + 1; i < m_nodes.size(); ++i) {
        SkylineNode& current_node = m_nodes[i];
        assert(new_node.x <= current_node.x);
        if (current_node.x < frontier) {
            // delete the current node if it was subsumed by the new one, and continue to the next node on the right
            if (current_node.x + current_node.width <= frontier) {
                m_nodes.erase(iterator_at(m_nodes, i));
                --i;
            }
            // ... or shrink the current node if a part of it overlaps the new node ...
            else {
                current_node.width -= frontier - current_node.x;
                current_node.x = frontier;
                break;
            }
        }

        // ... or do nothing if the current node is not affected
        else {
            break;
        }
    }

    // merge adjacent skyline nodes of the same y-coordinate
    for (size_t i = 0; i < m_nodes.size() - 1; ++i) {
        if (m_nodes[i].y == m_nodes[i + 1].y) {
            m_nodes[i].width += m_nodes[i + 1].width;
            m_nodes.erase(iterator_at(m_nodes, i + 1));
            --i;
        }
    }
}

} // namespace notf

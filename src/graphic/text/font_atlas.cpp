#include "notf/graphic/text/font_atlas.hpp"

#include <limits>

#include "notf/meta/assert.hpp"
#include "notf/meta/log.hpp"
#include "notf/meta/numeric.hpp"
#include "notf/meta/real.hpp"

#include "notf/common/color.hpp"
#include "notf/common/size2.hpp"
#include "notf/common/vector.hpp"

#include "notf/graphic/graphics_system.hpp"
#include "notf/graphic/opengl.hpp"
#include "notf/graphic/texture.hpp"

namespace { // anonymous

const size_t INVALID_SIZE_T = notf::max_value<size_t>();

} // namespace

NOTF_OPEN_NAMESPACE

void FontAtlas::WasteMap::initialize(const coord_t width, const coord_t height) {
    // start with a single big free rectangle, that spans the whole bin
    m_free_rects.clear();
    m_free_rects.emplace_back(0, 0, width, height);
}

void FontAtlas::WasteMap::add_waste(Glyph::Rect rect) { m_free_rects.emplace_back(std::move(rect)); }

Glyph::Rect FontAtlas::WasteMap::reclaim_rect(const coord_t width, const coord_t height) {
    Glyph::Rect result(0, 0, 0, 0);
    size_t best_node_index = INVALID_SIZE_T;

    { // try all free rects to find the best one for placement
        area_t least_waste = max_value<area_t>();
        for (size_t node_index = 0; node_index < m_free_rects.size(); ++node_index) {
            const Glyph::Rect& rect = m_free_rects[node_index];
            if (width == rect.width && height == rect.height) {
                // return early on a perfect fit
                result.x = rect.x;
                result.y = rect.y;
                result.width = width;
                result.height = height;
                goto return_success;
            }
            if (width < rect.width && height < rect.height) {
                // possible fit
                const area_t waste
                    = min(static_cast<area_t>(rect.width - width), static_cast<area_t>(rect.height - height));
                if (waste < least_waste) {
                    least_waste = waste;
                    result.x = rect.x;
                    result.y = rect.y;
                    best_node_index = node_index;
                }
            }
        }

        // return the empty rect, if you cannot fit the requested size
        if (best_node_index == INVALID_SIZE_T) { return result; }

        result.width = width;
        result.height = height;
    }

    { // keep track of the one or two generated smaller rects of free space
        const Glyph::Rect& free_rect = m_free_rects[best_node_index];

        Glyph::Rect vertical;
        vertical.x = free_rect.x + width;
        vertical.y = free_rect.y;
        vertical.width = free_rect.width - width;

        Glyph::Rect horizontal;
        horizontal.x = free_rect.x;
        horizontal.y = free_rect.y + height;
        horizontal.height = free_rect.height - height;

        if (width * horizontal.height <= vertical.width * height) {
            // split horizontally
            horizontal.width = free_rect.width;
            vertical.height = height;
        } else {
            // split vertically
            horizontal.width = width;
            vertical.height = free_rect.height;
        }

        // add the new rectangles to the pool, but only if they are not degenerate
        if (horizontal.width > 0 && horizontal.height > 0) { m_free_rects.emplace_back(std::move(horizontal)); }
        if (vertical.width > 0 && vertical.height > 0) { m_free_rects.emplace_back(std::move(vertical)); }
    }

return_success:
    m_free_rects.erase(iterator_at(m_free_rects, best_node_index));
    return result;
}

FontAtlas::FontAtlas() : m_texture(nullptr), m_width(512), m_height(512), m_used_area(0), m_nodes(), m_waste() {
    // create the atlas texture
    Texture::Args tex_args;
    tex_args.format = Texture::Format::GRAYSCALE;

    const auto current_guard = TheGraphicsSystem()->make_current();

    m_texture = Texture::create_empty("__notf_font_atlas", Size2i(m_width, m_height), tex_args);
    m_texture->set_wrap_x(Texture::Wrap::CLAMP_TO_EDGE);
    m_texture->set_wrap_y(Texture::Wrap::CLAMP_TO_EDGE);
    m_texture->set_min_filter(Texture::MinFilter::LINEAR);
    m_texture->set_mag_filter(Texture::MagFilter::LINEAR);

    // permanently bind the atlas texture to its slot (it is reserved and won't be rebound)
    const GLenum texture_slot = TheGraphicsSystem::get_environment().font_atlas_texture_slot;
    NOTF_CHECK_GL(glActiveTexture(GL_TEXTURE0 + texture_slot));
    NOTF_CHECK_GL(glBindTexture(GL_TEXTURE_2D, m_texture->get_id().get_value()));

    // initialize
    reset();

    NOTF_LOG_TRACE("Created font atlas of size {}x{} with TextureID {} bound on slot {}", m_width, m_height,
                   m_texture->get_id(), TheGraphicsSystem::get_environment().font_atlas_texture_slot);

    static_assert(std::is_pod<FitRequest>::value, "This compiler does not recognize notf::FontAtlas::FitRequest as a "
                                                  "POD.");
    static_assert(std::is_pod<SkylineNode>::value, "This compiler does not recognize notf::FontAtlas::SkylineNode as a "
                                                   "POD.");
    static_assert(std::is_pod<ScoredRect>::value, "This compiler does not recognize notf::FontAtlas::ScoredRect as a "
                                                  "POD.");
}

FontAtlas::~FontAtlas() { NOTF_LOG_TRACE("Deleted font atlas with texture ID: {}", m_texture->get_id()); }

void FontAtlas::reset() {
    // create a flat skyline of zero height
    m_nodes.clear();
    m_nodes.emplace_back(SkylineNode{0, 0, m_width});
    m_waste.initialize(m_width, m_height);
    m_used_area = 0;

    // fill the atlas with transparency
    m_texture->flood(Color::transparent());
}

Glyph::Rect FontAtlas::insert_rect(const coord_t width, const coord_t height) {
    { // try to fit the rectangle into the waste areas first...
        const Glyph::Rect rect = m_waste.reclaim_rect(width, height);
        if (rect.height != 0) {
            NOTF_ASSERT(rect.width != 0);
            m_used_area += rect.width * rect.height;
            return rect;
        }
    }

    // ... otherwise add it to the skyline
    ScoredRect result = _get_rect(width, height);
    if (result.rect.width == 0) {
        NOTF_ASSERT(result.rect.height == 0);
        NOTF_LOG_WARN("Failed to fit new rectangle of size {}x{} into FontAtlas", width, height);
    } else {
        NOTF_ASSERT(result.rect.height != 0);
        _add_node(result.node_index, result.rect);
        m_used_area += result.rect.width * result.rect.height;
    }

    return result.rect;
}

std::vector<FontAtlas::ProtoGlyph> FontAtlas::insert_rects(std::vector<FitRequest> named_extends) {
    std::vector<ProtoGlyph> result;

    // repeatedly go through all named extends, find the best one to insert and remove it
    while (!named_extends.empty()) {
        Glyph::Rect best_rect = {0, 0, 0, 0};
        size_t best_node_index = INVALID_SIZE_T;
        size_t best_extend_index = INVALID_SIZE_T;
        coord_t best_node_width = max_value<coord_t>();
        coord_t best_new_height = max_value<coord_t>();
        codepoint_t best_code_point = 0;

        // find the best fit
        for (size_t named_size_index = 0; named_size_index < named_extends.size(); ++named_size_index) {
            const FitRequest& extend = named_extends[named_size_index];
            const ScoredRect scored = _get_rect(extend.width, extend.height);
            if (scored.new_height < best_new_height
                || (scored.new_height == best_new_height && scored.node_width < best_node_width)) {
                best_rect = scored.rect;
                best_node_index = scored.node_index;
                best_extend_index = named_size_index;
                best_node_width = scored.node_width;
                best_new_height = scored.new_height;
                best_code_point = extend.code_point;
            }
        }

        // return what you got if you cannot fit anymore
        if (best_node_index == INVALID_SIZE_T) {
            NOTF_LOG_CRIT("Could not fit all requested rects into the font atlas");
            break;
        }
        NOTF_ASSERT(best_extend_index != INVALID_SIZE_T);

        // insert the new node into the atlas and add the resulting Rect to the results
        _add_node(best_node_index, best_rect);
        m_used_area += best_rect.width * best_rect.height;
        named_extends.erase(iterator_at(named_extends, best_extend_index));
        result.emplace_back(best_code_point, std::move(best_rect));
    }

    return result;
}

void FontAtlas::fill_rect(const Glyph::Rect& rect, const uchar* data) {
    if (rect.height == 0 || rect.width == 0 || !data) { return; }

    const auto current_guard = GraphicsContext::get().make_current();
    NOTF_CHECK_GL(glActiveTexture(GL_TEXTURE0 + TheGraphicsSystem::get_environment().font_atlas_texture_slot));
    NOTF_CHECK_GL(glPixelStorei(GL_UNPACK_ROW_LENGTH, rect.width));
    NOTF_CHECK_GL(glTexSubImage2D(GL_TEXTURE_2D, /* level = */ 0, rect.x, rect.y, rect.width, rect.height, GL_RED,
                                  GL_UNSIGNED_BYTE, data));
    NOTF_CHECK_GL(glPixelStorei(GL_UNPACK_ROW_LENGTH, m_texture->get_size().width()));
}

FontAtlas::ScoredRect FontAtlas::_get_rect(const coord_t width, const coord_t height) const {
    ScoredRect result;
    result.rect = {0, 0, 0, 0};
    result.node_index = INVALID_SIZE_T;
    result.node_width = max_value<coord_t>();
    result.new_height = max_value<coord_t>();

    for (size_t node_index = 0; node_index < m_nodes.size(); ++node_index) {
        const SkylineNode& current_node = m_nodes[node_index];

        // if the rect is too wide, there's nothing more we can do
        if (current_node.x + width >= m_width) { break; }

        // find the y-coordinate at which the rectangle could be placed above the current node
        coord_t y = 0;
        {
            int remaining_width = static_cast<int>(width);
            for (size_t spanned_index = node_index; remaining_width > 0; ++spanned_index) {
                NOTF_ASSERT(spanned_index < m_nodes.size());
                y = max(y, m_nodes[spanned_index].y);
                if (y + height >= m_height) {
                    y = max_value<coord_t>();
                    break;
                }
                remaining_width -= m_nodes[spanned_index].width;
            }
        }
        // treat the topmost y value as invalid,
        // because even if that was the correct answer, you would not be able to fit a rectangle there anyway
        if (y == max_value<coord_t>()) { continue; }

        // if the rectangle fits, check if it is a better fit than the last one and update the result if it is
        const coord_t new_height = y + height;
        if (new_height < result.new_height
            || (new_height == result.new_height && current_node.width < result.node_width)) {
            result.rect.x = current_node.x;
            result.rect.y = y;
            result.node_index = node_index;
            result.node_width = current_node.width;
            result.new_height = new_height;
        }
    }

    // if we were able to fit the rect, make the return value valid
    if (result.node_index != INVALID_SIZE_T) {
        result.rect.width = width;
        result.rect.height = height;
    }

    return result;
}

void FontAtlas::_add_node(const size_t node_index, const Glyph::Rect& rect) {
    const coord_t rect_right = rect.x + rect.width;
    NOTF_ASSERT(rect_right < m_width);
    NOTF_ASSERT(rect.y + rect.height < m_height);

    // identify and store generated waste
    for (size_t i = node_index; i < m_nodes.size(); ++i) {
        const SkylineNode& current_node = m_nodes[i];
        // unused area underneath the new node is waste
        if (current_node.x < rect_right) {
            NOTF_ASSERT(rect.y >= current_node.y);
            const coord_t current_right = min(static_cast<coord_t>(current_node.x + current_node.width), rect_right);
            m_waste.add_waste(
                Glyph::Rect(current_node.x, current_node.y, current_right - current_node.x, rect.y - current_node.y));
        } else {
            break;
        }
    }

    // create the new node
    NOTF_ASSERT(rect.y + rect.height <= max_value<coord_t>());
    const SkylineNode& new_node = *(m_nodes.emplace(
        iterator_at(m_nodes, node_index), SkylineNode{rect.x, static_cast<coord_t>(rect.y + rect.height), rect.width}));

    // adjust all affected nodes to the right
    for (size_t i = node_index + 1; i < m_nodes.size(); ++i) {
        SkylineNode& current_node = m_nodes[i];
        NOTF_ASSERT(new_node.x <= current_node.x);
        if (current_node.x < rect_right) {
            // delete the current node if it was subsumed by the new one, and continue to the next node on the right
            if (current_node.x + current_node.width <= rect_right) {
                m_nodes.erase(iterator_at(m_nodes, i));
                --i;
            }
            // ... or shrink the current node and stop if a part of it overlaps the new node ...
            else {
                current_node.width -= rect_right - current_node.x;
                current_node.x = rect_right;
                break;
            }
        }
        // ... or stop adjusting right away if the current node is not affected
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

NOTF_CLOSE_NAMESPACE

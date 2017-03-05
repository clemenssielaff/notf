#pragma once

#include <stddef.h>
#include <vector>

#include "common/aabr.hpp"
#include "graphics/cell.hpp"

namespace notf {

class RenderContext;

class RenderStats {

public: // methods
    /** Default Constructor. */
    RenderStats(const size_t buffer_size);

    /** Adds a new frame time to the buffer. */
    void update(const float frame_time);

    /** Renders the Stats to a RenderContext. */
    void render_stats(RenderContext& context);

private: // methods
    /** Calculates the average frame time of all frames in the buffer. */
    float get_average() const;

private: // fields
    /** Circular buffer to store the last BUFFER_SIZE render times. */
    std::vector<float> m_buffer;

    /** The `head` is the location of the latest value in the buffer. */
    size_t m_head;

    /** Frame time until the value is cut off at the top of the Stats. */
    float m_max_frame_time;

    /** Rectangle into which the stats are drawn. */
    Aabrf m_aabr;

    /** Cell to draw the Stats into. */
    Cell m_cell;
};

} // namespace notf

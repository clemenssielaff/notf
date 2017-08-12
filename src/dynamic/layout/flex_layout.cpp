#include "dynamic/layout/flex_layout.hpp"

#include <map>
#include <set>

#include "common/log.hpp"
#include "common/vector.hpp"
#include "core/item_container.hpp"

namespace { // anonymous
using namespace notf;

/** Helper struct used to abstract away which Claim-direction and stack-direction is used for layouting. */
struct ItemAdapter {
    float upper_bound;
    float preferred;
    float scale_factor;
    float result;
};

float distribute_surplus(float surplus, std::map<int, std::set<ItemAdapter*>>& batches)
{
    for (auto batch_it = batches.rbegin(); batch_it != batches.rend(); ++batch_it) {
        std::set<ItemAdapter*>& batch = batch_it->second;
        bool on_first_phase           = true;
        while (!batch.empty()) {

            // do nothing, if the surplus has been depleted
            if (abs(surplus) <= precision_high<float>()) {
                batch.clear();
                continue;
            }

            // first, supply space to all items that are smaller than their preferred size
            if (on_first_phase) {
                on_first_phase           = false;
                float total_deficit      = 0.f;
                float total_scale_factor = 0.f;
                for (auto it = batch.begin(); it != batch.end();) {
                    ItemAdapter* item  = *it;
                    float item_deficit = item->preferred - item->result;
                    assert(item_deficit >= 0);
                    if (item_deficit > 0) {
                        total_deficit += item_deficit;
                        total_scale_factor += item->scale_factor;
                        assert(item->scale_factor > 0);
                    }
                    else if (item->upper_bound - item->result <= precision_high<float>()) {
                        it = batch.erase(it);
                        continue;
                    }
                    ++it;
                }
                if (total_deficit > 0.f) {
                    total_deficit = min(total_deficit, surplus);
                    assert(total_scale_factor > 0);
                    const float deficit_per_scale_factor = total_deficit / total_scale_factor;
                    for (ItemAdapter* item : batch) {
                        const float item_deficit = item->preferred - item->result;
                        if (item_deficit > 0.f) {
                            if (item_deficit < deficit_per_scale_factor * item->scale_factor) {
                                surplus -= item->preferred - item->result;
                                item->result   = item->preferred;
                                on_first_phase = true;
                            }
                        }
                    }
                    if (!on_first_phase) {
                        for (auto it = batch.begin(); it != batch.end();) {
                            ItemAdapter* item = *it;
                            if (item->preferred - item->result > 0.f) {
                                item->result += deficit_per_scale_factor * item->scale_factor;
                            }
                            if (std::abs(item->result - item->upper_bound) <= precision_high<float>()) {
                                it = batch.erase(it);
                            }
                            else {
                                ++it;
                            }
                        }
                        surplus -= total_deficit;
                    }
                }
            }

            // in the second phase, distribute the remaining space
            else {
                float total_scale_factor = 0.f;
                for (ItemAdapter* item : batch) {
                    assert(item->upper_bound - item->result > 0);
                    assert(item->scale_factor > 0);
                    total_scale_factor += item->scale_factor;
                }
                float surplus_per_scale_factor = surplus / total_scale_factor;
                bool surplus_fits_in_batch     = true;
                for (auto it = batch.begin(); it != batch.end();) {
                    ItemAdapter* item = *it;
                    if (item->result + (item->scale_factor * surplus_per_scale_factor) > item->upper_bound) {
                        surplus_fits_in_batch = false;
                        surplus -= item->upper_bound - item->result;
                        item->result = item->upper_bound;
                        it           = batch.erase(it);
                    }
                    else {
                        ++it;
                    }
                }
                if (surplus_fits_in_batch) {
                    for (ItemAdapter* item : batch) {
                        item->result += item->scale_factor * surplus_per_scale_factor;
                    }
                    batch.clear();
                    surplus = 0.f;
                }
            }
        }
    }
    return surplus;
}

/** Calculates the alignment variables for the main axis. */
std::tuple<float, float> calculate_alignment(const FlexLayout::Alignment alignment, const size_t item_count, const float surplus)
{
    float alignment_start   = 0.f;
    float alignment_spacing = 0.f;
    switch (alignment) {
    case FlexLayout::Alignment::START:
        break;
    case FlexLayout::Alignment::END:
        alignment_start = surplus;
        break;
    case FlexLayout::Alignment::CENTER:
        alignment_start = surplus * 0.5f;
        break;
    case FlexLayout::Alignment::SPACE_BETWEEN:
        alignment_spacing = item_count > 1 ? surplus / static_cast<float>(item_count - 1) : 0.f;
        break;
    case FlexLayout::Alignment::SPACE_AROUND:
        alignment_start   = surplus / static_cast<float>(item_count * 2);
        alignment_spacing = alignment_start * 2.f;
        break;
    case FlexLayout::Alignment::SPACE_EQUAL:
        alignment_start   = surplus / static_cast<float>(item_count + 1);
        alignment_spacing = alignment_start;
        break;
    }
    return {alignment_start, alignment_spacing};
}

/** Calculates the cross offset to accomodate the cross alignment constraint. */
float cross_align_offset(const FlexLayout::Alignment alignment, const float item_size, const float available_size,
                         const bool cross_positive)
{
    if (available_size > item_size) {
        switch (alignment) {
        case FlexLayout::Alignment::START:
            return cross_positive ? 0 : available_size - item_size;
        case FlexLayout::Alignment::END:
            return cross_positive ? available_size - item_size : 0;
        case FlexLayout::Alignment::CENTER:
        case FlexLayout::Alignment::SPACE_BETWEEN:
        case FlexLayout::Alignment::SPACE_AROUND:
        case FlexLayout::Alignment::SPACE_EQUAL:
            return (available_size - item_size) * 0.5f;
        }
    }
    return 0.f;
}

detail::FlexSize getStartOffsets(const FlexLayout& flex_layout)
{
    detail::FlexSize offset;
    const Padding& padding      = flex_layout.get_padding();
    const FlexLayout::Wrap wrap = flex_layout.get_wrap();

    switch (flex_layout.get_direction()) {
    case FlexLayout::Direction::LEFT_TO_RIGHT:
        offset.main  = padding.left;
        offset.cross = (wrap == FlexLayout::Wrap::REVERSE ? padding.bottom : padding.top);
        break;
    case FlexLayout::Direction::RIGHT_TO_LEFT:
        offset.main  = padding.right;
        offset.cross = (wrap == FlexLayout::Wrap::REVERSE ? padding.bottom : padding.top);
        break;
    case FlexLayout::Direction::TOP_TO_BOTTOM:
        offset.main  = padding.top;
        offset.cross = (wrap == FlexLayout::Wrap::REVERSE ? padding.right : padding.left);
        break;
    case FlexLayout::Direction::BOTTOM_TO_TOP:
        offset.main  = padding.bottom;
        offset.cross = (wrap == FlexLayout::Wrap::REVERSE ? padding.right : padding.left);
        break;
    }

    return offset;
}

} // namespace anonymous

/**********************************************************************************************************************/

namespace notf {

FlexLayout::FlexLayout(const Direction direction)
    : Layout(std::make_unique<detail::ItemList>())
    , m_direction(direction)
    , m_main_alignment(Alignment::START)
    , m_cross_alignment(Alignment::START)
    , m_stack_alignment(Alignment::START)
    , m_wrap(Wrap::NO_WRAP)
    , m_padding(Padding::none())
    , m_spacing(0.f)
    , m_cross_spacing(0.f)
{
}

std::shared_ptr<FlexLayout> FlexLayout::create(const Direction direction)
{
    struct make_shared_enabler : public FlexLayout {
        make_shared_enabler(const Direction dir)
            : FlexLayout(dir) {}
    };
    return std::make_shared<make_shared_enabler>(direction);
}

void FlexLayout::set_direction(const Direction direction)
{
    if (m_direction == direction) {
        return;
    }
    m_direction = direction;
    _relayout();
}

void FlexLayout::set_alignment(const Alignment alignment)
{
    if (alignment == m_main_alignment) {
        return;
    }
    m_main_alignment = alignment;
    _relayout();
}

void FlexLayout::set_cross_alignment(Alignment alignment)
{
    if (alignment == Alignment::SPACE_BETWEEN
        || alignment == Alignment::SPACE_AROUND
        || alignment == Alignment::SPACE_EQUAL) {
        alignment = Alignment::CENTER;
    }
    if (alignment == m_cross_alignment) {
        return;
    }
    m_cross_alignment = alignment;
    _relayout();
}

void FlexLayout::set_stack_alignment(const Alignment alignment)
{
    if (alignment == m_stack_alignment) {
        return;
    }
    m_stack_alignment = alignment;
    _relayout();
}

void FlexLayout::set_wrap(const Wrap wrap)
{
    if (wrap == m_wrap) {
        return;
    }
    m_wrap = wrap;
    _relayout();
}

void FlexLayout::set_padding(const Padding& padding)
{
    if (!padding.is_valid()) {
        log_warning << "Ignoring invalid padding: " << padding;
        return;
    }
    if (padding == m_padding) {
        return;
    }
    m_padding = padding;
    _relayout();
}

void FlexLayout::set_spacing(float spacing)
{
    if (spacing < 0.f) {
        log_warning << "Cannot set spacing to less than zero, using zero instead.";
        spacing = 0.f;
    }
    if (std::abs(spacing - m_spacing) <= precision_high<float>()) {
        return;
    }
    m_spacing = spacing;
    _relayout();
}

void FlexLayout::set_cross_spacing(float spacing)
{
    if (spacing < 0.f) {
        log_warning << "Cannot set cross spacing to less than zero, using zero instead.";
        spacing = 0.f;
    }
    if (std::abs(spacing - m_cross_spacing) <= precision_high<float>()) {
        return;
    }
    m_cross_spacing = spacing;
    _relayout();
}

void FlexLayout::add_item(ItemPtr item)
{
    if (!item) {
        log_warning << "Cannot add an empty Item pointer to a Layout";
        return;
    }

    // if the item is already child of this Layout, place it at the end
    std::vector<ItemPtr>& items = static_cast<detail::ItemList*>(m_children.get())->items;
    if (has_child(item.get())) {
        remove_one_unordered(items, item);
    }

    // take ownership of the Item
    items.emplace_back(item);
    Item::_set_parent(item.get(), this);
    on_child_added(item.get());

    _relayout();
}

void FlexLayout::_remove_child(const Item* child_item)
{
    if (!child_item) {
        return;
    }

    auto& items = static_cast<detail::ItemList*>(m_children.get())->items;
    auto it     = std::find_if(std::begin(items), std::end(items),
                           [child_item](const ItemPtr& item) -> bool {
                               return item.get() == child_item;
                           });
    if (it == std::end(items)) {
        log_critical << "Cannot remove unknown child Item " << child_item->get_id()
                     << " from FlexLayout " << get_id();
        return;
    }

    log_trace << "Removing child Item " << child_item->get_id() << " from FlexLayout " << get_id();
    items.erase(it);
    on_child_removed(child_item);

    _relayout();
}

void FlexLayout::_get_widgets_at(const Vector2f& local_pos, std::vector<Widget*>& result) const
{
    // TODO: FlexLayout::get_widget_at is brute-force
    for (const ItemPtr& item : static_cast<detail::ItemList*>(m_children.get())->items) {
        const ScreenItem* screen_item = item->get_screen_item();
        if (screen_item && screen_item->get_aabr<Space::PARENT>().contains(local_pos)) {
            Vector2f item_pos = local_pos;
            screen_item->get_xform<Space::PARENT>().invert().transform(item_pos);
            ScreenItem::_get_widgets_at(screen_item, item_pos, result);
        }
    }
}

Claim FlexLayout::_consolidate_claim()
{
    /* if this layout wraps, it also cannot have a Claim, because the Claim would influence its size, would influence
     * its Claim etc.
     */
    if (is_wrapping()) {
        return Claim();
    }

    Claim claim = Claim::zero();
    auto& items = static_cast<detail::ItemList*>(m_children.get())->items;
    if (is_horizontal()) {
        for (const ItemPtr& item : items) {
            if (const ScreenItem* screen_item = item->get_screen_item()) {
                claim.add_horizontal(screen_item->get_claim());
            }
        }
        if (!items.empty()) {
            claim.get_horizontal().grow_by((items.size() - 1) * m_spacing);
        }
    }
    else {
        assert(is_vertical());
        for (const ItemPtr& item : items) {
            if (const ScreenItem* screen_item = item->get_screen_item()) {
                claim.add_vertical(screen_item->get_claim());
            }
        }
        if (!items.empty()) {
            claim.get_vertical().grow_by((items.size() - 1) * m_spacing);
        }
    }
    return claim;
}

void FlexLayout::_relayout()
{
    if (is_wrapping()) {
        _layout_wrapping();
    }
    else {
        _layout_not_wrapping();
    }
}

void FlexLayout::_layout_wrapping()
{
    const Size2f total_size = get_claim().apply(get_grant());

    detail::FlexSize available;
    {
        Size2f available_size = total_size;
        available_size.width -= m_padding.width();
        available_size.height -= m_padding.height();
        available.main  = is_horizontal() ? available_size.width : available_size.height;
        available.cross = is_horizontal() ? available_size.height : available_size.width;
    }

    // fill the items into stacks
    std::vector<std::vector<ScreenItem*>> stacks;
    std::vector<Claim::Stretch> cross_stretches;
    float used_cross_space = 0.f;
    {
        float current_size = 0;
        std::vector<ScreenItem*> current_stack;
        Claim::Stretch current_cross_stretch = Claim::Stretch(0, 0, 0);
        for (ItemPtr& item : static_cast<detail::ItemList*>(m_children.get())->items) {
            if (ScreenItem* screen_item = item->get_screen_item()) {
                const Claim& claim    = screen_item->get_claim();
                const float preferred = (is_horizontal() ? claim.get_horizontal() : claim.get_vertical()).get_preferred();
                const float addition  = preferred + m_spacing;

                // if the item doesn't fit on the current stack anymore, open a new one
                if (current_size + addition > available.main && !current_stack.empty()) {
                    stacks.emplace_back(std::move(current_stack));
                    cross_stretches.push_back(current_cross_stretch);
                    used_cross_space += current_cross_stretch.get_min();
                    current_stack.clear(); // re-using moved container, see http://stackoverflow.com/a/9168917/3444217
                    current_cross_stretch = Claim::Stretch(0, 0, 0);
                    current_size          = 0.f;
                }

                // add the item to the current stack
                current_size += addition;
                current_stack.push_back(screen_item);
                current_cross_stretch.maxed(is_horizontal() ? claim.get_vertical() : claim.get_horizontal());
            }
        }

        // add the last, partially filled stack to the layout
        stacks.emplace_back(std::move(current_stack));
        cross_stretches.push_back(current_cross_stretch);
        used_cross_space += current_cross_stretch.get_min();
    }
    const size_t stack_count = stacks.size();
    assert(stack_count == cross_stretches.size());
    used_cross_space += (stack_count - 1) * m_cross_spacing;

    // the cross layout of stacks is a regular stack layout in of itself
    std::vector<ItemAdapter> adapters;
    float cross_surplus = max(0, available.cross - used_cross_space);
    if (cross_surplus > 0) {
        adapters = std::vector<ItemAdapter>(stack_count);
        std::map<int, std::set<ItemAdapter*>> batches;
        for (size_t i = 0; i < stack_count; ++i) {
            adapters[i].upper_bound  = cross_stretches[i].get_max();
            adapters[i].preferred    = cross_stretches[i].get_preferred();
            adapters[i].scale_factor = cross_stretches[i].get_scale_factor();
            adapters[i].result       = cross_stretches[i].get_min();
            batches[cross_stretches[i].get_priority()].insert(&adapters[i]);
        }
        cross_surplus = distribute_surplus(cross_surplus, batches);
        assert(stack_count == adapters.size());
    }

    // determine values for alignment along the cross axis
    detail::FlexSize offset = getStartOffsets(*this);
    float cross_spacing;
    {
        float alignment_start;
        std::tie(alignment_start, cross_spacing) = calculate_alignment(m_stack_alignment, stack_count, cross_surplus);
        cross_spacing += m_cross_spacing;
        offset.cross += alignment_start;
    }

    // layout all stacks in order
    Aabrf new_aabr          = Aabrf::wrongest();
    size_t stack_index      = (m_wrap == Wrap::REVERSE ? stack_count - 1 : 0);
    const size_t last_index = (m_wrap == Wrap::REVERSE ? 0 : stack_count - 1);
    while (1) {
        const float stack_width = adapters.empty() ? cross_stretches[stack_index].get_min() : adapters[stack_index].result;
        const Size2f stack_size = is_horizontal() ? Size2f{available.main, stack_width} : Size2f{stack_width, available.main};
        Aabrf stack_aabr        = _layout_stack(stacks[stack_index], stack_size, offset);
        if (new_aabr.is_zero()) {
            new_aabr = stack_aabr;
        }
        else {
            new_aabr.unite(stack_aabr);
        }

        offset.cross += cross_spacing + stack_width;
        if (stack_index == last_index) {
            break;
        }
        if (m_wrap == Wrap::REVERSE) {
            --stack_index;
        }
        else {
            ++stack_index;
        }
    }

    // update the layout's Aabr
    Aabrf layout_aabr(total_size);

    if (new_aabr.is_valid()) {
        _set_aabr(std::move(new_aabr));
    }
    else {
        _set_aabr(Aabrf::zero());
    }
}

void FlexLayout::_layout_not_wrapping()
{
    std::vector<ItemPtr>& items = static_cast<detail::ItemList*>(m_children.get())->items;
    std::vector<ScreenItem*> screen_items;
    screen_items.reserve(items.size());
    for (ItemPtr& item : items) {
        if (ScreenItem* screen_item = item->get_screen_item()) {
            screen_items.emplace_back(screen_item);
        }
    }

    const Size2f total_size = get_claim().apply(get_grant());
    m_child_aabr            = _layout_stack(screen_items, total_size, getStartOffsets(*this));

    Aabrf layout_aabr = Aabrf::zero();
    if (is_horizontal()) {
        layout_aabr.set_left(min(0, m_child_aabr.left() - m_padding.left));
        layout_aabr.set_right(max(total_size.width, m_child_aabr.right() + m_padding.right));
        layout_aabr.set_bottom(m_child_aabr.bottom() - m_padding.bottom);
        layout_aabr.set_top(m_child_aabr.top() + m_padding.top);
    }
    else { // vertical
        layout_aabr.set_left(m_child_aabr.left() - m_padding.left);
        layout_aabr.set_right(m_child_aabr.right() + m_padding.right);
        layout_aabr.set_bottom(min(0, m_child_aabr.bottom() - m_padding.bottom));
        layout_aabr.set_top(max(total_size.height, m_child_aabr.top() + m_padding.top));
    }
    _set_aabr(std::move(layout_aabr));
}

Aabrf FlexLayout::_layout_stack(const std::vector<ScreenItem*>& stack, const Size2f total_size, detail::FlexSize offset)
{
    const size_t item_count = stack.size();
    if (item_count == 0) {
        return Aabrf::zero();
    }

    const bool is_positive = (m_direction == Direction::LEFT_TO_RIGHT) || (m_direction == Direction::BOTTOM_TO_TOP);

    // calculate the actual available size
    const detail::FlexSize available{
        max(0, total_size.width - m_padding.width() - (is_horizontal() ? m_spacing * (item_count - 1) : 0.f)),
        max(0, total_size.height - m_padding.height() - (is_horizontal() ? 0.f : m_spacing * (item_count - 1)))};

    // determine the size for each item
    std::vector<ItemAdapter> adapters(stack.size());
    float surplus;
    {
        // all elements get at least their minimum size
        float min_used = 0.f;
        std::map<int, std::set<ItemAdapter*>> batches;
        for (size_t i = 0; i < stack.size(); ++i) {
            const Claim& claim            = stack[i]->get_claim();
            const Claim::Stretch& stretch = is_horizontal() ? claim.get_horizontal() : claim.get_vertical();
            const std::pair<float, float> width_to_height = claim.get_width_to_height();
            if (width_to_height.first > 0.f) {
                if (is_horizontal()) {
                    adapters[i].upper_bound = min(stretch.get_max(), available.cross * width_to_height.second);
                }
                else {
                    adapters[i].upper_bound = min(stretch.get_max(), available.main / width_to_height.first);
                }
            }
            else {
                adapters[i].upper_bound = stretch.get_max();
            }
            adapters[i].preferred    = min(adapters[i].upper_bound, stretch.get_preferred());
            adapters[i].scale_factor = stretch.get_scale_factor();
            adapters[i].result       = min(adapters[i].upper_bound, stretch.get_min());
            min_used += adapters[i].result;
            batches[stretch.get_priority()].insert(&adapters[i]);
        }

        // layouting is the process by which the surplus space in the layout is distributed
        surplus = is_horizontal() ? available.main - min_used : available.cross - min_used;
        if (surplus > 0) {
            surplus = distribute_surplus(surplus, batches);
        }
    }

    // determine values for alignment along the primary axis
    float spacing = m_spacing;
    {
        float alignment_offset = 0.f;
        switch (m_main_alignment) {
        case FlexLayout::Alignment::START:
            break;
        case FlexLayout::Alignment::END:
            alignment_offset = surplus;
            break;
        case FlexLayout::Alignment::CENTER:
            alignment_offset = surplus * 0.5f;
            break;
        case FlexLayout::Alignment::SPACE_BETWEEN:
            spacing += max(0, item_count > 1 ? surplus / static_cast<float>(item_count - 1) : 0.f);
            break;
        case FlexLayout::Alignment::SPACE_AROUND:
            alignment_offset = surplus / static_cast<float>(item_count * 2);
            spacing += max(0, alignment_offset * 2.f);
            break;
        case FlexLayout::Alignment::SPACE_EQUAL:
            alignment_offset = surplus / static_cast<float>(item_count + 1);
            spacing += max(0, alignment_offset);
            break;
        }

        if (is_positive) {
            offset.main += alignment_offset;
        }
        else {
            offset.main = (is_horizontal() ? total_size.width : total_size.height) - (alignment_offset + offset.main);
        }
    }

    // apply the layout
    Aabrf stack_aabr = Aabrf::wrongest();
    for (size_t index = 0; index < stack.size(); ++index) {
        ScreenItem* child          = stack.at(index);
        const ItemAdapter& adapter = adapters[index];
        const std::pair<float, float> width_to_height = child->get_claim().get_width_to_height();
        assert(adapter.result >= 0.f);
        assert(adapter.result <= adapter.upper_bound);

        Size2f item_size;
        Vector2f item_pos;
        if (is_horizontal()) {
            const Claim::Stretch& vertical = child->get_claim().get_vertical();
            item_size                      = Size2f(adapter.result, min(vertical.get_max(), available.cross));
            if (width_to_height.first > 0.f) {
                item_size.height = min(item_size.height, adapter.result / width_to_height.first);
            }
            item_size.height = max(item_size.height, vertical.get_min());

            item_pos.x() = is_positive ? offset.main : offset.main - item_size.width;
            item_pos.y() = offset.cross
                + cross_align_offset(m_cross_alignment, item_size.height, available.cross, /*cross_positive*/ false);
        }
        else { // vertical
            const Claim::Stretch& horizontal = child->get_claim().get_horizontal();
            item_size                        = Size2f(min(horizontal.get_max(), available.main), adapter.result);
            if (width_to_height.first > 0.f) {
                item_size.width = min(item_size.width, adapter.result * width_to_height.second);
            }
            item_size.width = max(item_size.width, horizontal.get_min());

            item_pos.y() = is_positive ? offset.main : offset.main - item_size.height;
            item_pos.x() = offset.cross
                + cross_align_offset(m_cross_alignment, item_size.width, available.main, /*cross_positive*/ true);
        }
        _set_layout_xform(child, Xform2f::translation(item_pos));
        ScreenItem::_set_grant(child, item_size);

        stack_aabr._min.min(item_pos);
        stack_aabr._max.max(item_pos + Vector2f(item_size.width, item_size.height));

        offset.main += (adapter.result + spacing) * (is_positive ? 1 : -1);
    }
    return stack_aabr;
}

} // namespace notf

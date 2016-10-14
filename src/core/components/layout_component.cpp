//#include "core/components/layout_component.hpp"

//#include "common/log.hpp"
//#include "common/size_range.hpp"
//#include "core/widget.hpp"

//namespace { // anonymous

//// null value to reference
//const signal::SizeRange g_null_size = {};

//} // namespace anonymous

//namespace signal {

//LayoutComponent::LayoutComponent()
//    : m_layouts()
//    , m_internal_layout(std::make_unique<MakeSmartEnabler<InternalLayout>>(this))
//{
//}

//void LayoutComponent::set_internal_layout(std::shared_ptr<Layout> layout)
//{
//    if(m_layouts.find(layout) == m_layouts.end()){
//        log_critical << "Did not replace the internal Layout of LayoutComponent"
//                     << " with a LayoutItem that wasn't created by this Component";
//        return;
//    }
//    m_internal_layout->set_layout(std::move(layout));
//}

//LayoutItem::~LayoutItem()
//{
//}

//Layout::~Layout()
//{
//}

//const SizeRange& InternalLayout::get_horizontal_size() const
//{
//    if (m_layout) {
//        return m_layout->get_horizontal_size();
//    }
//    return g_null_size;
//}

//const SizeRange& InternalLayout::get_vertical_size() const
//{
//    if (m_layout) {
//        return m_layout->get_vertical_size();
//    }
//    return g_null_size;
//}

//} // namespace signal

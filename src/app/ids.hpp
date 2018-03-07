#pragma once

#include "app/forwards.hpp"
#include "common/id.hpp"

NOTF_OPEN_NAMESPACE

using ItemID = IdType<Item, size_t>;

using PropertyId = IdType<detail::PropertyBase, size_t>;

template<typename value_t>
using TypedPropertyId = IdType<PropertyId::type_t, PropertyId::underlying_t, value_t>;

using GraphicsProducerId = IdType<GraphicsProducer, size_t>;

using RenderTargetId = IdType<RenderTarget, size_t>;

NOTF_CLOSE_NAMESPACE

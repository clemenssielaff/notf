#pragma once

#include "app/forwards.hpp"
#include "common/id.hpp"

NOTF_OPEN_NAMESPACE

using ItemId = IdType<Item, uint>;

using PropertyId = IdType<detail::PropertyBase, uint>;

template<typename value_t>
using TypedPropertyId = IdType<PropertyId::type_t, PropertyId::underlying_t, value_t>;

using GraphicsProducerId = IdType<GraphicsProducer, uint>;

using RenderTargetId = IdType<RenderTarget, uint>;

NOTF_CLOSE_NAMESPACE

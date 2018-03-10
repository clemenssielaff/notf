#pragma once

#include "app/forwards.hpp"
#include "common/id.hpp"

NOTF_OPEN_NAMESPACE

using ItemId = IdType<Item, uint>;

using PropertyId = IdType<detail::PropertyBase, uint>;

using GraphicsProducerId = IdType<GraphicsProducer, uint>;

using RenderTargetId = IdType<RenderTarget, uint>;

NOTF_CLOSE_NAMESPACE

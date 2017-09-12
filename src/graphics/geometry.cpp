#include "graphics/geometry.hpp"

namespace notf {

namespace detail {

std::vector<GeometryFactoryImpl::Study> GeometryFactoryImpl::_produce(const SphereDefinition& /*definition*/)
{
    std::vector<Study> studies;
    studies.reserve(3);
    studies.emplace_back(Study{Vector4f(-1, 0), Vector4f(-1, 0, 0), Vector2f(0, 0)});
    studies.emplace_back(Study{Vector4f(0, 1), Vector4f(0, 1, 0), Vector2f(0.5, 1)});
    studies.emplace_back(Study{Vector4f(1, 0), Vector4f(1, 0, 0), Vector2f(1, 0)});
    return studies;
}

} // namespace detail

} // namespace notf

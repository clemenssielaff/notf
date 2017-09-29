#include "graphics/prefab_factory.hpp"

#include "utils/make_const.hpp"

namespace notf {

namespace detail {

PrefabFactoryImpl::Definition::~Definition() {}
PrefabFactoryImpl::Box::~Box() {}

PrefabFactoryImpl::Product PrefabFactoryImpl::_produce(const Box& def)
{
    Vector3d orient_axis = def.orient_axis;
    orient_axis.normalize();

    Vector3d up_axis = def.up_axis;
    up_axis.normalize();

    Vector3d depth_axis = make_const(orient_axis).cross(up_axis);
    depth_axis.normalize();

    std::vector<Study> studies(6 * 4);

    // vertex positions
    Vector3d v0 = def.center - (orient_axis * def.width) - (depth_axis * def.depth) - (up_axis * def.height);
    Vector3d v1 = def.center + (orient_axis * def.width) - (depth_axis * def.depth) - (up_axis * def.height);
    Vector3d v2 = def.center + (orient_axis * def.width) + (depth_axis * def.depth) - (up_axis * def.height);
    Vector3d v3 = def.center - (orient_axis * def.width) + (depth_axis * def.depth) - (up_axis * def.height);
    Vector3d v4 = def.center - (orient_axis * def.width) - (depth_axis * def.depth) + (up_axis * def.height);
    Vector3d v5 = def.center + (orient_axis * def.width) - (depth_axis * def.depth) + (up_axis * def.height);
    Vector3d v6 = def.center + (orient_axis * def.width) + (depth_axis * def.depth) + (up_axis * def.height);
    Vector3d v7 = def.center - (orient_axis * def.width) + (depth_axis * def.depth) + (up_axis * def.height);

    studies[0].position = v1; // right
    studies[1].position = v2;
    studies[2].position = v6;
    studies[3].position = v5;

    studies[4].position = v2; // back
    studies[5].position = v3;
    studies[6].position = v7;
    studies[7].position = v6;

    studies[8].position  = v3; // left
    studies[9].position  = v0;
    studies[10].position = v4;
    studies[11].position = v7;

    studies[12].position = v0; // front
    studies[13].position = v1;
    studies[14].position = v5;
    studies[15].position = v4;

    studies[16].position = v4; // top
    studies[17].position = v5;
    studies[18].position = v6;
    studies[19].position = v7;

    studies[20].position = v3; // bottom
    studies[21].position = v2;
    studies[22].position = v1;
    studies[23].position = v0;

    // vertex normals
    studies[0].normal = orient_axis; // right
    studies[1].normal = orient_axis;
    studies[2].normal = orient_axis;
    studies[3].normal = orient_axis;

    studies[4].normal = depth_axis; // back
    studies[5].normal = depth_axis;
    studies[6].normal = depth_axis;
    studies[7].normal = depth_axis;

    studies[8].normal  = -orient_axis; // left
    studies[9].normal  = -orient_axis;
    studies[10].normal = -orient_axis;
    studies[11].normal = -orient_axis;

    studies[12].normal = -depth_axis; // front
    studies[13].normal = -depth_axis;
    studies[14].normal = -depth_axis;
    studies[15].normal = -depth_axis;

    studies[16].normal = up_axis; // top
    studies[17].normal = up_axis;
    studies[18].normal = up_axis;
    studies[19].normal = up_axis;

    studies[20].normal = -up_axis; // bottom
    studies[21].normal = -up_axis;
    studies[22].normal = -up_axis;
    studies[23].normal = -up_axis;

    // texture coordinates
    studies[0].tex_coord = Vector2d(0, 0); // right
    studies[1].tex_coord = Vector2d(1, 0);
    studies[2].tex_coord = Vector2d(1, 1);
    studies[3].tex_coord = Vector2d(0, 1);

    studies[4].tex_coord = Vector2d(0, 0); // back
    studies[5].tex_coord = Vector2d(1, 0);
    studies[6].tex_coord = Vector2d(1, 1);
    studies[7].tex_coord = Vector2d(0, 1);

    studies[8].tex_coord  = Vector2d(0, 0); // left
    studies[9].tex_coord  = Vector2d(1, 0);
    studies[10].tex_coord = Vector2d(1, 1);
    studies[11].tex_coord = Vector2d(0, 1);

    studies[12].tex_coord = Vector2d(0, 0); // front
    studies[13].tex_coord = Vector2d(1, 0);
    studies[14].tex_coord = Vector2d(1, 1);
    studies[15].tex_coord = Vector2d(0, 1);

    studies[16].tex_coord = Vector2d(0, 0); // top
    studies[17].tex_coord = Vector2d(1, 0);
    studies[18].tex_coord = Vector2d(1, 1);
    studies[19].tex_coord = Vector2d(0, 1);

    studies[20].tex_coord = Vector2d(0, 0); // bottom
    studies[21].tex_coord = Vector2d(1, 0);
    studies[22].tex_coord = Vector2d(1, 1);
    studies[23].tex_coord = Vector2d(0, 1);

    // indices
    std::vector<GLuint> indices = {
        0, 2, 1,
        0, 3, 2,
        4, 6, 5,
        4, 7, 6,
        8, 10, 11,
        8, 9, 10,
        12, 14, 15,
        12, 13, 14,
        16, 18, 19,
        16, 17, 18,
        23, 21, 20,
        23, 22, 21};

    Product result;
    result.studies = std::move(studies);
    result.indices = std::move(indices);
    return result;
}

void PrefabFactoryImpl::_convert(const Vector3d& in, std::array<float, 4>& out)
{
    out[0] = static_cast<float>(in[0]);
    out[1] = static_cast<float>(in[1]);
    out[2] = static_cast<float>(in[2]);
    out[3] = 1;
}

void PrefabFactoryImpl::_convert(const Vector3d& in, std::array<half, 4>& out)
{
    out[0] = half(static_cast<float>(in[0]));
    out[1] = half(static_cast<float>(in[1]));
    out[2] = half(static_cast<float>(in[2]));
    out[3] = half(1);
}

void PrefabFactoryImpl::_convert(const Vector3d& in, std::array<float, 3>& out)
{
    out[0] = static_cast<float>(in[0]);
    out[1] = static_cast<float>(in[1]);
    out[2] = static_cast<float>(in[2]);
}
void PrefabFactoryImpl::_convert(const Vector3d& in, std::array<half, 3>& out)
{
    out[0] = half(static_cast<float>(in[0]));
    out[1] = half(static_cast<float>(in[1]));
    out[2] = half(static_cast<float>(in[2]));
}

void PrefabFactoryImpl::_convert(const Vector2d& in, std::array<float, 2>& out)
{
    out[0] = static_cast<float>(in[0]);
    out[1] = static_cast<float>(in[1]);
}
void PrefabFactoryImpl::_convert(const Vector2d& in, std::array<half, 2>& out)
{
    out[0] = half(static_cast<float>(in[0]));
    out[1] = half(static_cast<float>(in[1]));
}

} // namespace detail

} // namespace notf

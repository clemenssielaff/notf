#include "notf/graphic/prefab_factory.hpp"

/*
 * Face:
 *
 *  2 ----- 1
 *  |       |
 *  |       |
 *  |       |
 *  0 ----- 3
 *
 * Indices (for triangles):
 *
 *  0 1 2
 *  0 3 1
 */

NOTF_OPEN_NAMESPACE

// ================================================================================================================== //

namespace detail {

PrefabFactoryImpl::Definition::~Definition() {}
PrefabFactoryImpl::Box::~Box() {}
PrefabFactoryImpl::Sphere::~Sphere() {}

PrefabFactoryImpl::Product PrefabFactoryImpl::_produce(const Box& def)
{
    V3d orient_axis = def.orient_axis;
    orient_axis.normalize();

    V3d up_axis = def.up_axis;
    up_axis.normalize();

    V3d depth_axis = orient_axis.cross(up_axis);
    depth_axis.normalize();

    std::vector<Study> studies(6 * 4);

    // vertex positions
    V3d v0 = def.center - (orient_axis * def.width) - (depth_axis * def.depth) - (up_axis * def.height);
    V3d v1 = def.center + (orient_axis * def.width) - (depth_axis * def.depth) - (up_axis * def.height);
    V3d v2 = def.center + (orient_axis * def.width) + (depth_axis * def.depth) - (up_axis * def.height);
    V3d v3 = def.center - (orient_axis * def.width) + (depth_axis * def.depth) - (up_axis * def.height);
    V3d v4 = def.center - (orient_axis * def.width) - (depth_axis * def.depth) + (up_axis * def.height);
    V3d v5 = def.center + (orient_axis * def.width) - (depth_axis * def.depth) + (up_axis * def.height);
    V3d v6 = def.center + (orient_axis * def.width) + (depth_axis * def.depth) + (up_axis * def.height);
    V3d v7 = def.center - (orient_axis * def.width) + (depth_axis * def.depth) + (up_axis * def.height);

    studies[0].position = v2; // right
    studies[1].position = v5;
    studies[2].position = v6;
    studies[3].position = v1;

    studies[4].position = v3; // front
    studies[5].position = v6;
    studies[6].position = v7;
    studies[7].position = v2;

    studies[8].position = v0; // left
    studies[9].position = v7;
    studies[10].position = v4;
    studies[11].position = v3;

    studies[12].position = v1; // back
    studies[13].position = v4;
    studies[14].position = v5;
    studies[15].position = v0;

    studies[16].position = v0; // bottom
    studies[17].position = v2;
    studies[18].position = v3;
    studies[19].position = v1;

    studies[20].position = v7; // top
    studies[21].position = v5;
    studies[22].position = v4;
    studies[23].position = v6;

    // vertex normals
    studies[0].normal = orient_axis; // right
    studies[1].normal = orient_axis;
    studies[2].normal = orient_axis;
    studies[3].normal = orient_axis;

    studies[4].normal = depth_axis; // front
    studies[5].normal = depth_axis;
    studies[6].normal = depth_axis;
    studies[7].normal = depth_axis;

    studies[8].normal = -orient_axis; // left
    studies[9].normal = -orient_axis;
    studies[10].normal = -orient_axis;
    studies[11].normal = -orient_axis;

    studies[12].normal = -depth_axis; // back
    studies[13].normal = -depth_axis;
    studies[14].normal = -depth_axis;
    studies[15].normal = -depth_axis;

    studies[16].normal = -up_axis; // bottom
    studies[17].normal = -up_axis;
    studies[18].normal = -up_axis;
    studies[19].normal = -up_axis;

    studies[20].normal = up_axis; // top
    studies[21].normal = up_axis;
    studies[22].normal = up_axis;
    studies[23].normal = up_axis;

    // texture coordinates
    studies[0].tex_coord = V2d(0, 0); // right
    studies[1].tex_coord = V2d(1, 1);
    studies[2].tex_coord = V2d(0, 1);
    studies[3].tex_coord = V2d(1, 0);

    studies[4].tex_coord = V2d(0, 0); // front
    studies[5].tex_coord = V2d(1, 1);
    studies[6].tex_coord = V2d(0, 1);
    studies[7].tex_coord = V2d(1, 0);

    studies[8].tex_coord = V2d(0, 0); // left
    studies[9].tex_coord = V2d(1, 1);
    studies[10].tex_coord = V2d(0, 1);
    studies[11].tex_coord = V2d(1, 0);

    studies[12].tex_coord = V2d(0, 0); // back
    studies[13].tex_coord = V2d(1, 1);
    studies[14].tex_coord = V2d(0, 1);
    studies[15].tex_coord = V2d(1, 0);

    studies[16].tex_coord = V2d(0, 0); // bottom
    studies[17].tex_coord = V2d(1, 1);
    studies[18].tex_coord = V2d(0, 1);
    studies[19].tex_coord = V2d(1, 0);

    studies[20].tex_coord = V2d(0, 0); // top
    studies[21].tex_coord = V2d(1, 1);
    studies[22].tex_coord = V2d(0, 1);
    studies[23].tex_coord = V2d(1, 0);

    // indices
    std::vector<GLuint> indices = {0,  1,  2, // right
                                   0,  3,  1,

                                   4,  5,  6, // front
                                   4,  7,  5,

                                   8,  9,  10, // left
                                   8,  11, 9,

                                   12, 13, 14, // back
                                   12, 15, 13,

                                   16, 17, 18, // bottom
                                   16, 19, 17,

                                   20, 21, 22, // top
                                   20, 23, 21};

    Product result;
    result.studies = std::move(studies);
    result.indices = std::move(indices);
    return result;
}

PrefabFactoryImpl::Product PrefabFactoryImpl::_produce(const Sphere& def)
{
    const uint segment_count = max(3, def.segments);
    const uint ring_count = max(1, def.rings);

    const double ring_angle = 1. / static_cast<double>(ring_count + 1);
    const double segment_angle = 1. / static_cast<double>(segment_count);

    std::vector<Study> studies((segment_count * ring_count) + 2); // +2 pole vertices
    {                                                             // positions, normals and texture coordinates
        Study& south_pole = studies.front();
        south_pole.position = def.center + V3d(0, -def.radius, 0);
        south_pole.normal = V3d(0, -1, 0);
        south_pole.tex_coord = V2d(0, 0);

        for (uint r = 1; r <= ring_count; ++r) {
            for (uint s = 0; s < segment_count; ++s) {
                const double x = cos(2 * pi<double>() * s * segment_angle) * sin(pi<double>() * r * ring_angle);
                const double y = sin(pi<double>() * -0.5 + pi<double>() * r * ring_angle);
                const double z = sin(2 * pi<double>() * s * segment_angle) * sin(pi<double>() * r * ring_angle);

                Study& study = studies[((r - 1) * s) + 1];
                study.position = def.center + V3d(x * def.radius, y * def.radius, z * def.radius);
                study.normal = V3d(x, y, z);
                study.tex_coord = V2d(s * segment_angle * 2. * def.tileU, r * ring_angle * def.tileV);
            }
        }

        Study& north_pole = studies.back();
        north_pole.position = def.center + V3d(0, def.radius, 0);
        north_pole.normal = V3d(0, 1, 0);
        north_pole.tex_coord = V2d(0, 1);
    }

    //    std::vector<GLuint> indices((6 * segment_count) + (segment_count * (ring_count + 1)));
    std::vector<GLuint> indices(3 * segment_count);
    { // indices
        size_t i = 0;
        for (uint s = 2; s < segment_count; ++s) {
            indices.at(i++) = 0;
            indices.at(i++) = s;
            indices.at(i++) = s - 1;
        }

        //        for (uint r = 1; r <= ring_count; ++r) {
        //            for (uint s = 0; s < segment_count; ++s) {

        //                face->push_back((r + 0) * segment_count + (s + 0));
        //                face->push_back((r + 0) * segment_count + (s + 1));
        //                face->push_back((r + 1) * segment_count + (s + 1));
        //                face->push_back((r + 1) * segment_count + (s + 0));
        //            }
        //        }
    }

    Product result;
    result.studies = std::move(studies);
    result.indices = std::move(indices);
    return result;
}

void PrefabFactoryImpl::_convert(const V2d& in, V2f& out)
{
    out[0] = static_cast<float>(in[0]);
    out[1] = static_cast<float>(in[1]);
}

void PrefabFactoryImpl::_convert(const V2d& in, V2h& out)
{
    out[0] = half(static_cast<float>(in[0]));
    out[1] = half(static_cast<float>(in[1]));
}

void PrefabFactoryImpl::_convert(const V3d& in, V3f& out)
{
    out[0] = static_cast<float>(in[0]);
    out[1] = static_cast<float>(in[1]);
    out[2] = static_cast<float>(in[2]);
}

void PrefabFactoryImpl::_convert(const V3d& in, V3h& out)
{
    out[0] = half(static_cast<float>(in[0]));
    out[1] = half(static_cast<float>(in[1]));
    out[2] = half(static_cast<float>(in[2]));
}

void PrefabFactoryImpl::_convert(const V3d& in, V4f& out)
{
    out[0] = static_cast<float>(in[0]);
    out[1] = static_cast<float>(in[1]);
    out[2] = static_cast<float>(in[2]);
    out[3] = 1;
}

void PrefabFactoryImpl::_convert(const V3d& in, V4h& out)
{
    out[0] = half(static_cast<float>(in[0]));
    out[1] = half(static_cast<float>(in[1]));
    out[2] = half(static_cast<float>(in[2]));
    out[3] = half(1);
}

} // namespace detail

NOTF_CLOSE_NAMESPACE

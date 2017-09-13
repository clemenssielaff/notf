#include "graphics/geometry.hpp"

namespace notf {

namespace detail {

std::vector<GeometryFactoryImpl::Study> GeometryFactoryImpl::_produce(const BoxDefinition& def)
{
    Vector4f orient_axis = def.orient_axis;
    orient_axis.w()      = 0; // TODO: normalization with Vector4 must not use normalize with W
    orient_axis.normalize();
    orient_axis.w() = 1;

    Vector4f up_axis = def.up_axis;
    up_axis.w()      = 0;
    up_axis.normalize();
    up_axis.w() = 1;

    Vector4f depth_axis = orient_axis.cross(up_axis);
    depth_axis.w()      = 0;
    depth_axis.normalize();
    depth_axis.w() = 1;

    std::vector<Study> studies(6 * 4);

    // vertex positions
    const Vector4f v0 = def.center - (orient_axis * def.width) - (depth_axis * def.depth) - (up_axis * def.height);
    const Vector4f v1 = def.center + (orient_axis * def.width) - (depth_axis * def.depth) - (up_axis * def.height);
    const Vector4f v2 = def.center + (orient_axis * def.width) + (depth_axis * def.depth) - (up_axis * def.height);
    const Vector4f v3 = def.center - (orient_axis * def.width) + (depth_axis * def.depth) - (up_axis * def.height);
    const Vector4f v4 = def.center - (orient_axis * def.width) - (depth_axis * def.depth) + (up_axis * def.height);
    const Vector4f v5 = def.center + (orient_axis * def.width) - (depth_axis * def.depth) + (up_axis * def.height);
    const Vector4f v6 = def.center + (orient_axis * def.width) + (depth_axis * def.depth) + (up_axis * def.height);
    const Vector4f v7 = def.center - (orient_axis * def.width) + (depth_axis * def.depth) + (up_axis * def.height);

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
    studies[0].tex_coord = Vector2f(0, 0); // right
    studies[1].tex_coord = Vector2f(1, 0);
    studies[2].tex_coord = Vector2f(1, 1);
    studies[3].tex_coord = Vector2f(0, 1);

    studies[4].tex_coord = Vector2f(0, 0); // back
    studies[5].tex_coord = Vector2f(1, 0);
    studies[6].tex_coord = Vector2f(1, 1);
    studies[7].tex_coord = Vector2f(0, 1);

    studies[8].tex_coord  = Vector2f(0, 0); // left
    studies[9].tex_coord  = Vector2f(1, 0);
    studies[10].tex_coord = Vector2f(1, 1);
    studies[11].tex_coord = Vector2f(0, 1);

    studies[12].tex_coord = Vector2f(0, 0); // front
    studies[13].tex_coord = Vector2f(1, 0);
    studies[14].tex_coord = Vector2f(1, 1);
    studies[15].tex_coord = Vector2f(0, 1);

    studies[16].tex_coord = Vector2f(0, 0); // top
    studies[17].tex_coord = Vector2f(1, 0);
    studies[18].tex_coord = Vector2f(1, 1);
    studies[19].tex_coord = Vector2f(0, 1);

    studies[20].tex_coord = Vector2f(0, 0); // bottom
    studies[21].tex_coord = Vector2f(1, 0);
    studies[22].tex_coord = Vector2f(1, 1);
    studies[23].tex_coord = Vector2f(0, 1);

    // faces
    //    boxGeo->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
    //    boxGeo->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 4, 4));
    //    boxGeo->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 8, 4));
    //    boxGeo->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 12, 4));
    //    boxGeo->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 16, 4));
    //    boxGeo->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 20, 4));

    return studies;
}

} // namespace detail

} // namespace notf

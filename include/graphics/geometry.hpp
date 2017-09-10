#pragma once

#include "common/vector4.hpp"

namespace notf {

/** Factory class for gradually building up and producing Geometry. */
class GeometryFactory {

public: // types ******************************************************************************************************/
    /** Definition for a rectangular plane.
     * Default is a plane facing towards positive z (up).
     */
    struct RectangularPlaneDefinition {
        Vector4f center = Vector4f();
        Vector4f axisX  = Vector4f(1, 0, 0);
        Vector4f axisY  = Vector4f(0, 1, 0);
        float sizeX     = 1;
        float sizeY     = 1;
        float tileU     = 1;
        float tileV     = 1;
    };

    /** Definition for a circular plane.
     * Default is a plane facing towards positive z.
     */
    struct CircularPlaneDefinition {
        Vector4f center       = Vector4f();
        Vector4f upAxis       = Vector4f(0, 0, 1);
        Vector4f orientAxis   = Vector4f(1, 0, 0);
        float radius          = 1;
        unsigned int segments = 12;
        float tileU           = 1;
        float tileV           = 1;
    };

    /** Definition for a box. */
    struct BoxDefinition {
        Vector4f center     = Vector4f();
        Vector4f upAxis     = Vector4f(0, 0, 1);
        Vector4f orientAxis = Vector4f(1, 0, 0);
        float height        = 1;
        float width         = 1;
        float depth         = 1;
        float tileU         = 1;
        float tileV         = 1;
    };

    /** Definition for a sphere.
     * Implemented from:
     *     http://stackoverflow.com/a/26710491/3444217
     * Spheres are always created with poles in the vertical axis.
     */
    struct SphereDefinition {
        Vector4f center       = Vector4f();
        float radius          = 1;
        unsigned int rings    = 12; // latitude
        unsigned int segments = 24; // longitude
        float tileU           = 1;
        float tileV           = 1;
    };

    /** Definition for a cylinder.
     * Default is a cylinder along positive z.
     */
    struct CylinderDefinition {
        Vector4f center       = Vector4f();
        Vector4f upAxis       = Vector4f(0, 0, 1);
        Vector4f orientAxis   = Vector4f();
        float radius          = .5;
        float height          = 1;
        unsigned int segments = 12;
        float tileU           = 1;
        float tileV           = 1;
    };

    /** Definition for a cone.
     * Default is a cone along positive y.
     */
    struct ConeDefinition {
        Vector4f center       = Vector4f();
        Vector4f upAxis       = Vector4f(0, 0, 1);
        Vector4f orientAxis   = Vector4f();
        float radius          = .5;
        float height          = 1;
        unsigned int segments = 12;
        float tileU           = 1;
        float tileV           = 1;
    };

    /** Definition for a bone. */
    struct BoneDefinition {
        Vector4f center       = Vector4f();
        Vector4f upAxis       = Vector4f(0, 0, 1);
        Vector4f orientAxis   = Vector4f();
        float radius          = .5;
        float height          = 2;
        float midHeight       = .5;
        unsigned int segments = 4;
        float tileU           = 1;
        float tileV           = 1;
    };

    /** Definition for a curved strip.
     * Default "rainbow" surface facing positive z.
     */
    struct CurvedStripDefinition {
        Vector4f start        = Vector4f(10, 0, 0);
        Vector4f axisX        = Vector4f(1, 0, 0);
        Vector4f rotPivot     = Vector4f();
        Vector4f rotAxis      = Vector4f(0, 0, 1);
        float width           = 1;
        float angle           = 3.14159265359f;
        unsigned int segments = 32;
        float tileU           = 1;
        float tileV           = 10;
    };

public: // methods
    //    explicit GeometryFactory();

    //    ///
    //    /// \brief Produces the Geode that was successively built up until now.
    //    ///
    //    /// Clears the state of the factory.
    //    ///
    //    /// \return The newly created Geode.
    //    ///
    //    osg::ref_ptr<osg::Geode> produce();

    //    ///
    //    /// \brief Constructs a Rectangular Plane.
    //    ///
    //    /// \param def      Definition of object to add to the produced geometry.
    //    ///
    //    void add(const RectangularPlaneDefinition& def);

    //    ///
    //    /// \brief Constructs a Circular Plane.
    //    ///
    //    /// \param def      Definition of object to add to the produced geometry.
    //    ///
    //    void add(const CircularPlaneDefinition& def);

    //    ///
    //    /// \brief Constructs a Box.
    //    ///
    //    /// \param def      Definition of object to add to the produced geometry.
    //    ///
    //    void add(const BoxDefinition& def);

    //    ///
    //    /// \brief Constructs a Sphere.
    //    ///
    //    /// (adapted from http://www.ogre3d.org/tikiwiki/tiki-index.php?page=manualspheremeshes)
    //    ///
    //    /// \param def      Definition of object to add to the produced geometry.
    //    ///
    //    void add(const SphereDefinition& def);
};

} // namespace notf

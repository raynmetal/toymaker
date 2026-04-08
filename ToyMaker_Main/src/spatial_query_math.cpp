#include <cmath>
#include "toymaker/engine/spatial_query_math.hpp"

using namespace ToyMaker;

inline float squareDistance(const glm::vec3& vector) { return glm::dot(vector, vector); }

bool ToyMaker::ObjectBounds::isPositiveStrict() const {
    bool isBoundsNonZero;
    switch(mType) {
        case ToyMaker::ObjectBounds::TrueVolumeType::BOX:
            isBoundsNonZero = mTrueVolume.mBox.isPositiveStrict();
            break;
        case ToyMaker::ObjectBounds::TrueVolumeType::CAPSULE:
            isBoundsNonZero = mTrueVolume.mCapsule.isPositiveStrict();
            break;
        case ToyMaker::ObjectBounds::TrueVolumeType::SPHERE:
            isBoundsNonZero = mTrueVolume.mSphere.isPositiveStrict();
            break;
        default:
            assert(false && "Unrecognized shape passed");
    }
    return isBoundsNonZero;
}

glm::vec3 getSupportBox(const glm::vec3& axis, const AxisAlignedBounds& box);
glm::vec3 getSupportBox(const glm::vec3& axis, const ObjectBounds& box);
glm::vec3 getSupportSphere(const glm::vec3& axis, const ObjectBounds& sphere);
glm::vec3 getSupportCapsule(const glm::vec3& axis, const ObjectBounds& capsule);

std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> getBoxIntersections(const Ray& ray, const std::array<AreaTriangle, 12>& boxTriangles);
std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> getCapsuleIntersections(const Ray& ray, const ObjectBounds& capsule);
std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> getSphereIntersections(const Ray& ray, const ObjectBounds& sphere);
std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> getCylinderIntersections(
    const Ray& ray,
    const glm::vec3& cylinderOrigins,
    const glm::vec3& cylinderAxis,
    float cylinderHeight,
    float cylinderRadius
);
std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> getBoxIntersections(const Ray& ray, const ObjectBounds& bounds) {
    return getBoxIntersections(ray, bounds.getWorldOrientedBoxFaceTriangles());
}
std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> getBoxIntersections(const Ray& ray, const AxisAlignedBounds& bounds) {
    return getBoxIntersections(ray, bounds.getAxisAlignedBoxFaceTriangles());
}

// Gets the minimum and maximum points of a 3D box projected on an axis
std::pair<float, float> axisProjectBox(const glm::vec3& axis, const ObjectBounds& box);
// Gets the minimum and maximum points of a sphere projected on an axis
std::pair<float, float> axisProjectSphere(const glm::vec3& axis, const ObjectBounds& sphere);
// Gets the minimum and maximum points of a capsule projected on an axis
std::pair<float, float> axisProjectCapsule(const glm::vec3& axis, const ObjectBounds& capsule);

template<typename T, typename U>
inline glm::vec3 minkowskiDifference(const T& one, const U& two, const glm::vec3& along) {
    return one.getSupportAlong(along) - two.getSupportAlong(-along);
}

// Finds the next search direction for a simplex, mutating the simplex as it does so
glm::vec3  doSimplex(std::array<glm::vec3, 4>& simplex, uint8_t& nSimplexPoints, bool& found);
// Finds the next search direction for a simplex comprised of 2 points, mutating the simplex as it does so
glm::vec3 doSimplex2(std::array<glm::vec3, 4>& simplex, uint8_t& nSimplexPoints);
// Finds the next search direction for a simplex comprised of 3 points, mutating the simplex as it does so
glm::vec3 doSimplex3(std::array<glm::vec3, 4>& simplex, uint8_t& nSimplexPoints);
// Finds the next search direction for a simplex comprised of 4 points, mutating the simplex as it does so
glm::vec3 doSimplex4(std::array<glm::vec3, 4>& simplex, uint8_t& nSimplexPoints, bool& found);

// Gets the global orientation vectors of a box
std::array<glm::vec3, 3> getBoxEdgeAxes(const ObjectBounds& box);

/** 
 * Gets the point closest to some arbitrary point inside a box
 */
glm::vec3 getClosestPointOnBox(const glm::vec3& point, const ObjectBounds& box);

bool checkContainsPointBox(const glm::vec3& point, const ObjectBounds& box);
bool checkContainsPointCapsule(const glm::vec3& point, const ObjectBounds& capsule);
bool checkContainsPointSphere(const glm::vec3& point, const ObjectBounds& sphere);

/** 
 * @brief GJK implementation based on Casey Muratori's YouTube video on the same, which
 * is available [here](https://www.youtube.com/watch?v=Qupqu1xe7Io)
 */
template <typename T, typename U>
bool gjkOverlaps(const T& one, const U& two);
bool checkOverlaps1D(float shape1Min, float shape1Max, float shape2Min, float shape2Max);

/** 
 * Check whether 1d shape 1 is contained by 1d shape 2
 */
bool checkContains1D(float shape1Min, float shape1Max, float shape2Min, float shape2Max);

std::pair<bool, glm::vec3> ToyMaker::computeIntersection(const Ray& ray, const Plane& plane) {
    assert(ray.isSensible() && "Invalid ray provided");
    assert(plane.isSensible() && "Invalid plane provided");

    // ray is parallel to the plane (i.e., perpendicular to the plane's normal)
    if(glm::dot(plane.mNormal, ray.mDirection) == 0.f)  {
        // if start point does not lie on the plane, there is no point of intersection
        if(glm::dot(plane.mPointOnPlane - ray.mStart, plane.mNormal)) {
            return {false, glm::vec3{std::numeric_limits<float>::infinity()}};
        }

        // start point does lie on the plane, and therefore is the (first) point of intersection
        return {true, ray.mStart};
    }

    // work out point of intersection parametrically
    const glm::vec3 rayDirection { glm::normalize(ray.mDirection) };
    const glm::vec3 planeNormal { glm::normalize(glm::dot(plane.mNormal, rayDirection)<0.f? -plane.mNormal: plane.mNormal) };
    const float rayIntersectionDistance {
        glm::dot(planeNormal,  (plane.mPointOnPlane - ray.mStart))
        / glm::dot(planeNormal, rayDirection)
    };

    // If rayIntersectionDistance does not fall within the length specified for the ray, there is
    // no point of intersection
    if(rayIntersectionDistance < 0 || rayIntersectionDistance > ray.mLength) {
        return { false, glm::vec3(std::numeric_limits<float>::infinity()) };
    }

    return { true, ray.mStart + rayIntersectionDistance * rayDirection };
}

std::pair<bool, glm::vec3> ToyMaker::computeIntersection(const Ray& ray, const AreaTriangle& triangle) {
    assert(ray.isSensible() && "Invalid ray provided");
    assert(triangle.isSensible() && "Invalid triangle provided");

    const glm::vec3 triangleNormal { glm::cross(
        triangle.mPoints[2] - triangle.mPoints[0],
        triangle.mPoints[1] - triangle.mPoints[0]
    ) };
    // Find point of intersection with triangle's plane
    std::pair<bool, glm::vec3> planeIntersection { 
        computeIntersection(ray, Plane{ .mPointOnPlane{ triangle.mPoints[0] }, .mNormal{ triangleNormal } })
    };

    // no intersection with plane found, return false (whatever the plane intersection
    // computation returned)
    if(!planeIntersection.first) return planeIntersection;

    // See: https://math.stackexchange.com/questions/4322/check-whether-a-point-is-within-a-3d-triangle
    // Plane intersection found, see if intersection point lies within triangle.
    // Sum of areas of triangles formed between each pair of triangle points and the point of intersection
    // will be the same as the area of the triangle iff the point lies within the triangle
    const float triangleArea {
        glm::length(glm::cross(triangle.mPoints[1] - triangle.mPoints[0], triangle.mPoints[2] - triangle.mPoints[0]))
        / 2.f
    };
    const float alpha {
        glm::length(glm::cross(triangle.mPoints[0] - planeIntersection.second, triangle.mPoints[1] - planeIntersection.second))
        / (2.f * triangleArea)
    };
    const float beta {
        glm::length(glm::cross(triangle.mPoints[0] - planeIntersection.second, triangle.mPoints[2] - planeIntersection.second))
        / (2.f * triangleArea)
    };
    const float gamma {
        glm::length(glm::cross(triangle.mPoints[1] - planeIntersection.second, triangle.mPoints[2] - planeIntersection.second))
        / (2.f * triangleArea)
    };

    if(
        (alpha >= 0.f && alpha <= 1.f)
        && (beta >= 0.f && beta <= 1.f) 
        && (gamma >= 0.f && gamma <= 1.f)
        && std::fabs(alpha + beta + gamma - 1.f) <= std::numeric_limits<float>::epsilon()
    ) {
        return planeIntersection;
    }

    // No point of intersection found
    return { false, glm::vec3{std::numeric_limits<float>::infinity()} };
}

std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> ToyMaker::computeIntersections(const Ray& ray, const AxisAlignedBounds& bounds) {
    assert(ray.isSensible() && "Invalid ray provided");
    assert(
        bounds.isSensible() && "Invalid axis-aligned box provided"
    );

    // box with no volume provided
    if(!bounds.isPositiveStrict()) {
        return {false, {glm::vec3{std::numeric_limits<float>::infinity()}, glm::vec3{std::numeric_limits<float>::infinity()}}};
    }

    return getBoxIntersections(ray, bounds);
}

std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> ToyMaker::computeIntersections(const Ray& ray, const ObjectBounds& bounds) {
    assert(ray.isSensible() && "Invalid ray provided");
    assert(bounds.isSensible() && "Invalid bounds provided");

    // Bounds should be greater than zero, representing a real volume in the 
    // scene
    if(!bounds.isPositiveStrict()) {
        return {false, {glm::vec3{std::numeric_limits<float>::infinity()}, glm::vec3{std::numeric_limits<float>::infinity()}}};
    }

    // Perform intersection logic for each type of supported volume
    switch(bounds.mType) {
        case ToyMaker::ObjectBounds::TrueVolumeType::BOX:
            return getBoxIntersections(ray, bounds);
        case ToyMaker::ObjectBounds::TrueVolumeType::CAPSULE:
            return getCapsuleIntersections(ray, bounds);
        case ToyMaker::ObjectBounds::TrueVolumeType::SPHERE:
            return getSphereIntersections(ray, bounds);
        default:
            assert(false && "Unrecognized shape passed");
    }
}

bool ToyMaker::overlaps(const glm::vec3& point, const AxisAlignedBounds& bounds) {
    return contains(point, bounds);
}

bool ToyMaker::overlaps(const glm::vec3& point, const ObjectBounds& bounds) {
    return contains(point, bounds);
}

bool ToyMaker::overlaps(const Ray& ray, const AxisAlignedBounds& bounds) {
    assert(ray.isSensible() && "Invalid ray provided");
    assert(bounds.isSensible() && "Invalid axis aligned box provided");

    // box with no volume provided
    if(!bounds.isPositiveStrict()) {
        return false;
    }

    return (
        // either ray begins within the box, or...
        (
            contains(ray.mStart, bounds)
        // the ray intersects with the box
        ) || (
            static_cast<bool>(computeIntersections(ray, bounds).first)
        )
    );
}

bool ToyMaker::overlaps(const Ray& ray, const ObjectBounds& bounds) {
    assert(ray.isSensible() && "Invalid ray provided");
    assert(bounds.isSensible() && "Invalid object bounds provided");

    // exit early if bounds with no volume provided
    if(!bounds.isPositiveStrict()) {
        return false;
    }

    return (
        // either the ray begins within the bounds ...
        (
            contains(ray.mStart, bounds)
        // ... or it intersects the bounds.
        ) || (
            static_cast<bool>(computeIntersections(ray, bounds).first)
        )
    );
}

bool ToyMaker::overlaps(const AxisAlignedBounds& one, const AxisAlignedBounds& two) {
    assert (
        one.isSensible() && two.isSensible() && "Invalid axis aligned box provided"
    );

    const AxisAlignedBounds::Extents ourExtents { two.getAxisAlignedBoxExtents() };
    const glm::vec3& bottomCorner { ourExtents.second };
    const glm::vec3& topCorner { ourExtents.first };

    const AxisAlignedBounds::Extents otherExtents { one.getAxisAlignedBoxExtents() };
    const glm::vec3& otherBottomCorner { otherExtents.second };
    const glm::vec3& otherTopCorner { otherExtents.first };

    return (
        // overlap in x
        checkOverlaps1D(otherBottomCorner.x, otherTopCorner.x, bottomCorner.x, topCorner.x)
        // overlap in y
        && checkOverlaps1D(otherBottomCorner.y, otherTopCorner.y, bottomCorner.y, topCorner.y)
        // overlap in z
        && checkOverlaps1D(otherBottomCorner.z, otherTopCorner.z, bottomCorner.z, topCorner.z)
    );
}

bool ToyMaker::overlaps(const ObjectBounds& one, const ObjectBounds& two) {
    assert(one.isSensible() && two.isSensible() && "Invalid object bounds provided");

    // Both need to be non-degenerate bounds to actually overlap
    if(!(one.isPositiveStrict() && two.isPositiveStrict())) {
        return false;
    }

    return gjkOverlaps(one, two);
}

bool ToyMaker::overlaps(const AxisAlignedBounds& one, const ObjectBounds& two) {
    assert(one.isSensible() && two.isSensible() && "Invalid object bounds provided");

    // Both need to be non-degenerate bounds to actually overlap
    if(!(one.isPositiveStrict() && two.isPositiveStrict())) {
        return false;
    }

    return gjkOverlaps(one, two);
}

bool ToyMaker::contains(const glm::vec3& point, const AxisAlignedBounds& bounds) {
    assert(
        bounds.isSensible() && "Invalid axis aligned box provided"
    );
    assert(isFinite(point) && "Invalid point provided");

    const AxisAlignedBounds::Extents boxExtents { bounds.getAxisAlignedBoxExtents() };

    return point.x <= boxExtents.first.x
        && point.y <= boxExtents.first.y
        && point.z <= boxExtents.first.z

        && point.x >= boxExtents.second.x
        && point.y >= boxExtents.second.y
        && point.z >= boxExtents.second.z;
}

bool ToyMaker::contains(const Ray& ray, const AxisAlignedBounds& bounds) {
    assert(ray.isSensible() && "Invalid ray provided");
    assert(bounds.isSensible() && "Invalid axis-aligned box provided");
    if(!isFinite(ray.mLength)) return false;
    const glm::vec3 rayEnd { ray.mStart + glm::normalize(ray.mDirection) * ray.mLength };
    return contains(ray.mStart, bounds) && contains(rayEnd, bounds);
}

bool ToyMaker::contains(const AxisAlignedBounds& one, const AxisAlignedBounds& two) {
    assert(one.isSensible() && two.isSensible() && "Invalid axis-aligned box provided");

    const AxisAlignedBounds::Extents ourExtents { two.getAxisAlignedBoxExtents() };
    const glm::vec3& bottomCorner { ourExtents.second };
    const glm::vec3& topCorner { ourExtents.first };

    const AxisAlignedBounds::Extents otherExtents { one.getAxisAlignedBoxExtents() };
    const glm::vec3& otherBottomCorner { otherExtents.second };
    const glm::vec3& otherTopCorner { otherExtents.first };

    return (
        // contained in x
        checkContains1D(otherBottomCorner.x, otherTopCorner.x, bottomCorner.x, topCorner.x)
        // contained in y
        && checkContains1D(otherBottomCorner.y, otherTopCorner.y, bottomCorner.y, topCorner.y)
        // contained in z
        && checkContains1D(otherBottomCorner.z, otherTopCorner.z, bottomCorner.z, topCorner.z)
    );
}

bool ToyMaker::contains(const glm::vec3& point, const ObjectBounds& bounds) {
    assert(isFinite(point) && "Invalid point provided");
    assert(bounds.isSensible() && "Invalid bounds provided");

    // Bounds should be greater than zero, representing a real volume in the 
    // scene
    if(!bounds.isPositiveStrict()) {
        return false;
    }

    switch(bounds.mType) {
        case ToyMaker::ObjectBounds::TrueVolumeType::BOX:
            return checkContainsPointBox(point, bounds);
        case ToyMaker::ObjectBounds::TrueVolumeType::CAPSULE:
            return checkContainsPointCapsule(point, bounds);
        case ToyMaker::ObjectBounds::TrueVolumeType::SPHERE:
            return checkContainsPointSphere(point, bounds);
        default:
            assert(false && "Unrecognized shape passed");
    }
}

bool ToyMaker::contains(const Ray& ray, const ObjectBounds& bounds) {
    assert(ray.isSensible() && "Invalid ray provided");
    assert(bounds.isSensible() && "Invalid bounds provided");

    // Bounds should be greater than zero, representing a real volume in the 
    // scene
    if(!bounds.isPositiveStrict()) {
        return false;
    }

    // Infinite rays immediately fail containment in finite bounds
    if(!isFinite(ray.mLength)) return false;

    // Bounds contain a ray if it contains both the ray's start and end points
    const glm::vec3 rayStart { ray.mStart };
    const glm::vec3 rayEnd { rayStart + glm::normalize(ray.mDirection) * ray.mLength };
    return contains(ray.mStart, bounds) && contains(rayEnd, bounds);
}

bool ToyMaker::contains(const ObjectBounds& one, const AxisAlignedBounds& two) {
    assert(one.isSensible() && "Invalid object bounds provided");
    assert(two.isSensible() && "Invalid axis aligned bounds provided");

    // Always return false if either volume is degenerate
    if(!two.isPositiveStrict() || !one.isPositiveStrict()) {
        return false;
    }

    std::pair<float, float> oneProjectedX { one.getProjectionAlong(glm::vec3(1.f, 0.f, 0.f)) };
    std::pair<float, float> oneProjectedY { one.getProjectionAlong(glm::vec3(0.f, 1.f, 0.f)) };
    std::pair<float, float> oneProjectedZ { one.getProjectionAlong(glm::vec3(0.f, 0.f, 1.f)) };

    const AxisAlignedBounds::Extents ourExtents { two.getAxisAlignedBoxExtents() };
    const glm::vec3& bottomCorner { ourExtents.second };
    const glm::vec3& topCorner { ourExtents.first };

    return (
        // contained in x
        checkContains1D(oneProjectedX.first, oneProjectedX.second, bottomCorner.x, topCorner.x)
        // contained in y
        && checkContains1D(oneProjectedY.first, oneProjectedY.second, bottomCorner.y, topCorner.y)
        // contained in z
        && checkContains1D(oneProjectedZ.first, oneProjectedZ.second, bottomCorner.z, topCorner.z)
    );
}

ObjectBounds ObjectBounds::create(const VolumeBox& box, const glm::vec3& positionOffset, const glm::quat& orientationOffset) {
    return ObjectBounds {
        .mType { TrueVolumeType::BOX },
        .mTrueVolume { .mBox { box } },
        .mPositionOffset { positionOffset },
        .mOrientationOffset{ orientationOffset },
    };
}

ObjectBounds ObjectBounds::create(const VolumeSphere& sphere, const glm::vec3& positionOffset, const glm::quat& orientationOffset) {
    return ObjectBounds {
        .mType { TrueVolumeType::SPHERE },
        .mTrueVolume { .mSphere { sphere } },
        .mPositionOffset { positionOffset },
        .mOrientationOffset { orientationOffset },
    };
}

ObjectBounds ObjectBounds::create(const VolumeCapsule& capsule, const glm::vec3& positionOffset, const glm::quat& orientationOffset) {
    return ObjectBounds {
        .mType { TrueVolumeType::CAPSULE },
        .mTrueVolume { .mCapsule { capsule } },
        .mPositionOffset { positionOffset },
        .mOrientationOffset { orientationOffset },
    };
}

void ObjectBounds::applyModelMatrix(const glm::mat4& modelMatrix) {
    mPosition = static_cast<glm::vec3>(modelMatrix * glm::vec4{0.f, 0.f, 0.f, 1.f});
    mOrientation = glm::normalize(glm::quat_cast(glm::transpose(glm::inverse(modelMatrix))));
}

std::array<AreaTriangle, 12> ToyMaker::computeBoxFaceTriangles(const std::array<glm::vec3, 8>& boxCorners) {
    return std::array<AreaTriangle, 12> {{
        // left faces
        AreaTriangle{.mPoints{
            boxCorners[0],
            boxCorners[BoxCornerSpecifier::FRONT],
            boxCorners[BoxCornerSpecifier::FRONT|BoxCornerSpecifier::TOP]}},
        AreaTriangle{.mPoints{
            boxCorners[0],
            boxCorners[BoxCornerSpecifier::FRONT|BoxCornerSpecifier::TOP],
            boxCorners[BoxCornerSpecifier::TOP]
        }},

        // right faces
        AreaTriangle{.mPoints{
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::FRONT],
            boxCorners[BoxCornerSpecifier::RIGHT],
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::TOP]
        }},
        AreaTriangle{.mPoints{
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::FRONT],
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::TOP],
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::TOP|BoxCornerSpecifier::FRONT]
        }},

        // bottom faces
        AreaTriangle{.mPoints{
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::FRONT],
            boxCorners[BoxCornerSpecifier::FRONT],
            boxCorners[0]
        }},
        AreaTriangle{.mPoints{
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::FRONT],
            boxCorners[0],
            boxCorners[BoxCornerSpecifier::RIGHT]
        }},

        // top faces
        AreaTriangle{.mPoints{
            boxCorners[BoxCornerSpecifier::TOP],
            boxCorners[BoxCornerSpecifier::TOP|BoxCornerSpecifier::FRONT],
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::TOP|BoxCornerSpecifier::FRONT]
        }},
        AreaTriangle{.mPoints{
            boxCorners[BoxCornerSpecifier::TOP],
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::TOP|BoxCornerSpecifier::FRONT],
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::TOP]
        }},

        // back faces
        AreaTriangle{.mPoints{
            boxCorners[BoxCornerSpecifier::RIGHT],
            boxCorners[0],
            boxCorners[BoxCornerSpecifier::TOP]
        }},
        AreaTriangle{.mPoints{
            boxCorners[BoxCornerSpecifier::RIGHT],
            boxCorners[BoxCornerSpecifier::TOP],
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::TOP]
        }},

        // front faces
        AreaTriangle{.mPoints{
            boxCorners[BoxCornerSpecifier::FRONT],
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::FRONT],
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::TOP|BoxCornerSpecifier::FRONT]
        }},
        AreaTriangle{.mPoints{
            boxCorners[BoxCornerSpecifier::FRONT],
            boxCorners[BoxCornerSpecifier::RIGHT|BoxCornerSpecifier::TOP|BoxCornerSpecifier::FRONT],
            boxCorners[BoxCornerSpecifier::TOP|BoxCornerSpecifier::FRONT]
        }}
    }};
}

std::array<glm::vec3, 8> ObjectBounds::getVolumeRelativeBoxCorners() const {
    switch(mType) {
        case TrueVolumeType::BOX:
            return mTrueVolume.mBox.getVolumeRelativeBoxCorners();
        case TrueVolumeType::SPHERE:
            return mTrueVolume.mSphere.getVolumeRelativeBoxCorners();
        case TrueVolumeType::CAPSULE:
            return mTrueVolume.mCapsule.getVolumeRelativeBoxCorners();
        default:
            assert(false && "Unrecognized true volume type specified");
            return mTrueVolume.mBox.getVolumeRelativeBoxCorners();
    }
}

glm::vec3 ObjectBounds::getComputedWorldPosition() const {
    return mPosition + getWorldRotationTransform() * mPositionOffset;
}
glm::quat ObjectBounds::getComputedWorldOrientation() const {
    return glm::normalize(glm::quat_cast(getWorldRotationTransform() * getLocalRotationTransform()));
}

std::array<glm::vec3, 8> ObjectBounds::getLocalOrientedBoxCorners() const {
    std::array<glm::vec3, 8> orientedCorners { getVolumeRelativeBoxCorners() };
    for(glm::vec3& localCorner: orientedCorners) {
        localCorner = mPositionOffset + getLocalRotationTransform() * localCorner;
    }
    return orientedCorners;
}

std::array<glm::vec3, 8> ObjectBounds::getWorldOrientedBoxCorners() const {
    std::array<glm::vec3, 8> worldCorners { getLocalOrientedBoxCorners() };
    for(glm::vec3& orientedCorner: worldCorners) {
        orientedCorner = mPosition + getWorldRotationTransform() * orientedCorner;
    }
    return worldCorners;
}

bool ObjectBounds::isSensible() const {
    switch(mType) {
        case TrueVolumeType::BOX:
            return mTrueVolume.mBox.isSensible();
        case TrueVolumeType::SPHERE:
            return mTrueVolume.mSphere.isSensible();
        case TrueVolumeType::CAPSULE:
            return mTrueVolume.mCapsule.isSensible();
        default:
            assert(false && "Unrecognized true volume type specified");
            return mTrueVolume.mBox.isSensible();
    }
}

glm::vec3 AxisAlignedBounds::getSupportAlong(const glm::vec3& axis) const {
    // Reject invalid axes
    assert(squareDistance(axis) != 0 && "Axis must be non-zero vector");
    assert(isFinite(axis) && "Axis must be finite");

    // Degenerate shapes yield origin
    if(!isPositiveStrict()) {
        return getComputedWorldPosition();
    }

    return getSupportBox(axis, *this);
}

std::pair<float, float> ObjectBounds::getProjectionAlong(const glm::vec3& axis) const {
    assert(isSensible() && "Invalid bounds provided");
    assert(squareDistance(axis) != 0 && "Axis must be non-zero vector");
    assert(isFinite(axis) && "Axis must be finite");

    switch(mType) {
    case TrueVolumeType::BOX:
        return axisProjectBox(axis, *this);
    case TrueVolumeType::CAPSULE:
        return axisProjectCapsule(axis, *this);
    case TrueVolumeType::SPHERE:
        return axisProjectSphere(axis, *this);
    default:
        assert(false && "Unknown object type specified");
        return { std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };
    }
}

glm::vec3 ObjectBounds::getSupportAlong(const glm::vec3& axis) const {
    // Reject invalid axes
    assert(isSensible() && "Invalid bounds provided");
    assert(squareDistance(axis) != 0 && "Axis must be non-zero vector");
    assert(isFinite(axis) && "Axis must be finite");

    // Degenerate shapes yield origin
    if(!isPositiveStrict()) {
        return getComputedWorldPosition();
    }

    switch(mType) {
        case TrueVolumeType::BOX:
            return getSupportBox(axis, *this);
        case TrueVolumeType::SPHERE:
            return getSupportSphere(axis, *this);
        case TrueVolumeType::CAPSULE:
            return getSupportCapsule(axis, *this);
        default:
            assert(false && "Unrecognized true volume type specified");
            return glm::vec3(std::numeric_limits<float>::infinity());
    }
}

AxisAlignedBounds::AxisAlignedBounds(): 
mExtents { glm::vec3{0.f}, glm::vec3{0.f} }
{}

AxisAlignedBounds::AxisAlignedBounds(const ObjectBounds& objectBounds): AxisAlignedBounds{} {
    const std::pair<float, float> projectedOnX { objectBounds.getProjectionAlong(glm::vec3{ 1.f, 0.f, 0.f }) };
    const std::pair<float, float> projectedOnY { objectBounds.getProjectionAlong(glm::vec3{ 0.f, 1.f, 0.f }) };
    const std::pair<float, float> projectedOnZ { objectBounds.getProjectionAlong(glm::vec3{ 0.f, 0.f, 1.f }) };

    const Extents axisAlignedExtents {
        { projectedOnX.second, projectedOnY.second, projectedOnZ.second },
        {  projectedOnX.first,  projectedOnY.first,  projectedOnZ.first },
    };
    setByExtents(axisAlignedExtents);
}

AxisAlignedBounds::AxisAlignedBounds(const Extents& axisAlignedExtents): AxisAlignedBounds{} {
    setByExtents(axisAlignedExtents);
}

std::array<glm::vec3, 8> AxisAlignedBounds::getAxisAlignedBoxCorners() const {
    std::array<glm::vec3, 8> axisAlignedBoxCorners {};
    /** least significant 3 bits represent corners of a box, where
     * * 0th bit represents x, 1 is right, 0 is left
     * * 1st bit represents y, 1 is up, 0 is down
     * * 2nd bit represents z, 1 is front, 0 is back
     */
    for(uint8_t corner {0}; corner < 8; ++corner) {
        axisAlignedBoxCorners[corner].x = corner&BoxCornerSpecifier::RIGHT? mExtents.first.x: mExtents.second.x;
        axisAlignedBoxCorners[corner].y = corner&BoxCornerSpecifier::TOP? mExtents.first.y: mExtents.second.y;
        axisAlignedBoxCorners[corner].z = corner&BoxCornerSpecifier::FRONT? mExtents.first.z: mExtents.second.z;
    }
    return axisAlignedBoxCorners;
}

AxisAlignedBounds::Extents AxisAlignedBounds::getAxisAlignedBoxExtents() const {
    return mExtents;
}

AxisAlignedBounds AxisAlignedBounds::operator+(const AxisAlignedBounds& other) const {
    Extents ourExtents { getAxisAlignedBoxExtents() };
    const glm::vec3& bottomCorner { ourExtents.second };
    const glm::vec3& topCorner { ourExtents.first };

    Extents otherExtents { other.getAxisAlignedBoxExtents() };
    const glm::vec3& otherBottomCorner { otherExtents.second };
    const glm::vec3& otherTopCorner { otherExtents.first };

    Extents extents {
        {glm::max(topCorner, otherTopCorner)}, {glm::min(bottomCorner, otherBottomCorner)} 
    };

    return { extents };
}

void AxisAlignedBounds::setByExtents(const Extents& axisAlignedExtents) {
    assert(
        axisAlignedExtents.first.x >= axisAlignedExtents.second.x
        && axisAlignedExtents.first.y >= axisAlignedExtents.second.y
        && axisAlignedExtents.first.z >= axisAlignedExtents.second.z
        && "First of extents pair must be right top front corner, and second must be left back right corner"
    );
    mExtents = axisAlignedExtents;
}

std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> getBoxIntersections(const Ray& ray, const std::array<AreaTriangle, 12>& boxTriangles) {
    // Compute intersection between the ray and every box triangle.  If a triangle
    // on one face has an intersection, skip the other triangle on the
    // same face
    std::array<glm::vec3, 2> intersectionPoints { glm::vec3{std::numeric_limits<float>::infinity()}, glm::vec3{std::numeric_limits<float>::infinity()} };
    uint8_t nIntersections { 0 };
    for(uint8_t i{0}; i < 12 && nIntersections < 2; ++i) {
        const AreaTriangle& triangle { boxTriangles[i] };
        std::pair<bool, glm::vec3> possibleIntersection { computeIntersection(ray, triangle) };
        if(possibleIntersection.first) {
            intersectionPoints[nIntersections++] = possibleIntersection.second;
            // skip the next triangle on this face
            if(i % 2 == 0) {
                ++i;
            }
        }
    }

    // Sort the intersections by distance from ray origin
    if(nIntersections == 2) {
        const glm::vec3 pointOne { intersectionPoints[0] - ray.mStart };
        const glm::vec3 pointTwo { intersectionPoints[1] - ray.mStart };
        if(glm::dot(pointOne, pointOne) > glm::dot(pointTwo, pointTwo)) {
            std::swap(intersectionPoints[0], intersectionPoints[1]);
        }
    }

    return { nIntersections, std::pair<glm::vec3, glm::vec3>{
        intersectionPoints[0], intersectionPoints[1]
    } };
}

std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> getSphereIntersections(const Ray& ray, const ObjectBounds& sphere) {
    const glm::vec3 rayDirection { glm::normalize( ray.mDirection ) };

    // Hold our results here
    uint8_t nIntersections { 0 };
    std::array<glm::vec3, 2> intersectionPoints { glm::vec3{std::numeric_limits<float>::infinity()}, glm::vec3{std::numeric_limits<float>::infinity()} };

    // Solve ray-sphere intersection
    const glm::vec3 originDifference { ray.mStart - sphere.getComputedWorldPosition() };
    const float qdrtcA { 1.f };
    const float qdrtcB { 2.f * glm::dot(originDifference, rayDirection) };
    const float qdrtcC { glm::dot(originDifference, originDifference) - sphere.mTrueVolume.mSphere.mRadius * sphere.mTrueVolume.mSphere.mRadius };
    const float discriminant { qdrtcB * qdrtcB - 4.f * qdrtcA * qdrtcC };

    // Ignore single intersection and no intersection cases
    if(discriminant <= 0) {
        return { 0, { intersectionPoints[0], intersectionPoints[1] } };
    }

    const float solution0 { (-qdrtcB + glm::sqrt(discriminant)) / (2 * qdrtcA) };
    const float solution1 { (-qdrtcB - glm::sqrt(discriminant)) / (2 * qdrtcA) };

    // Ensure that solutions fall within the length of the ray
    if(solution0 <= ray.mLength && solution0 >= 0) {
        intersectionPoints[nIntersections++] = ray.mStart + solution0 * rayDirection;
    }
    if(solution1 <= ray.mLength && solution1 >= 0) {
        intersectionPoints[nIntersections++] = ray.mStart + solution1 * rayDirection;
    }

    // Sort the intersections by distance from ray origin
    if(nIntersections == 2) {
        const glm::vec3 pointOne { intersectionPoints[0] - ray.mStart };
        const glm::vec3 pointTwo { intersectionPoints[1] - ray.mStart };
        if(glm::dot(pointOne, pointOne) > glm::dot(pointTwo, pointTwo)) {
            std::swap(intersectionPoints[0], intersectionPoints[1]);
        }
    }

    return { nIntersections, { intersectionPoints[0], intersectionPoints[1] } };
}

std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> getCapsuleIntersections(const Ray& ray, const ObjectBounds& capsule) {
    const glm::vec3 rayDirection { glm::normalize( ray.mDirection ) };

    // Hold our results here
    uint8_t nIntersections { 0 };
    std::array<glm::vec3, 2> intersectionPoints { glm::vec3{std::numeric_limits<float>::infinity()}, glm::vec3{std::numeric_limits<float>::infinity()} };
    std::vector<glm::vec3> consideredIntersections {};

    // Cache some specifics about our capsule
    const glm::vec3 capsulePosition { capsule.getComputedWorldPosition() };
    const glm::mat3 capsuleRotation { glm::mat3_cast(capsule.getComputedWorldOrientation()) };
    const glm::vec3 capsuleAxis { capsuleRotation * glm::vec3 { 0.f, 1.f, 0.f } };
    const std::array<glm::vec3, 2> capsuleEnds {
        capsulePosition - capsule.mTrueVolume.mCapsule.mHeight * .5f * capsuleAxis,
        capsulePosition + capsule.mTrueVolume.mCapsule.mHeight * .5f * capsuleAxis
    };

    // Find intersections between the first end of the capsule and the ray, ...
    std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> currentIntersections {
        getSphereIntersections(
            ray,
            ObjectBounds::create(
                VolumeSphere { .mRadius {capsule.mTrueVolume.mCapsule.mRadius } },
                capsuleEnds[0],
                capsuleRotation
            )
        )
    };
    if(
        currentIntersections.first >= 1 
        // filter for points on the outer hemisphere
        && glm::dot( 
            currentIntersections.second.first - capsuleEnds[0], capsuleAxis
        ) * glm::dot(
            currentIntersections.second.first - capsuleEnds[1], capsuleAxis
        ) > 0
    ) {
        consideredIntersections.push_back(currentIntersections.second.first);
    }
    if(
        currentIntersections.first >= 2
        // filter for points on the outer hemisphere
        && glm::dot( 
            currentIntersections.second.second - capsuleEnds[0], capsuleAxis
        ) * glm::dot(
            currentIntersections.second.second - capsuleEnds[1], capsuleAxis
        ) > 0
    ) {
        consideredIntersections.push_back(currentIntersections.second.second);
    }

    // ... second end of capsule and ray, ...
    currentIntersections = getSphereIntersections(
        ray,
        ObjectBounds::create(
            VolumeSphere { .mRadius { capsule.mTrueVolume.mCapsule.mRadius } },
            capsuleEnds[1],
            capsuleRotation
        )
    );
    if(
        currentIntersections.first >= 1 
        && glm::dot( // filter for points on the outer hemisphere
            currentIntersections.second.first - capsuleEnds[0], capsuleAxis
        ) * glm::dot(
            currentIntersections.second.first - capsuleEnds[1], capsuleAxis
        ) > 0
    ) {
        consideredIntersections.push_back(currentIntersections.second.first);
    }
    if(
        currentIntersections.first >= 2
        // filter for points on the outer hemisphere
        && glm::dot( 
            currentIntersections.second.second - capsuleEnds[0], capsuleAxis
        ) * glm::dot(
            currentIntersections.second.second - capsuleEnds[1], capsuleAxis
        ) > 0
    ) {
        consideredIntersections.push_back(currentIntersections.second.second);
    }

    // ... and cylinder surface and ray
    currentIntersections = getCylinderIntersections(
        ray, 
        capsulePosition,
        capsuleAxis,
        capsule.mTrueVolume.mCapsule.mHeight,
        capsule.mTrueVolume.mCapsule.mRadius
    );
    if(currentIntersections.first >= 1) {
        consideredIntersections.push_back(currentIntersections.second.first);
    }
    if(currentIntersections.first >= 2) {
        consideredIntersections.push_back(currentIntersections.second.second);
    }

    if(consideredIntersections.empty()) {
        return { 0, {intersectionPoints[0], intersectionPoints[1]} };
    }

    // Walk through our list of considered intersections, storing the ones with the least and most
    // distance from the ray in our results
    for(const auto& intersection: consideredIntersections) {
        const glm::vec3 intersectionRelativeRayOrigin { intersection - ray.mStart };
        const float squareDistance { glm::dot(intersectionRelativeRayOrigin, intersectionRelativeRayOrigin) };
        if(nIntersections < 1) {
            intersectionPoints[nIntersections++] = intersection;
        } else if (nIntersections < 2) {
            const glm::vec3 previousIntersectionRelativeRayOrigin { intersectionPoints[0] - ray.mStart };
            const float previousSquareDistance { glm::dot(previousIntersectionRelativeRayOrigin, previousIntersectionRelativeRayOrigin) };
            if(previousSquareDistance > squareDistance) {
                intersectionPoints[nIntersections++] = intersectionPoints[0];
                intersectionPoints[0] = intersection;
            } else if (previousSquareDistance == squareDistance) { continue; }
            else {
                intersectionPoints[nIntersections++] = intersection;
            }
        } else {
            const glm::vec3 previousLeast { intersectionPoints[0] - ray.mStart };
            const glm::vec3 previousMost { intersectionPoints[1] - ray.mStart };
            const float previousLeastSquareDistance { glm::dot(previousLeast, previousLeast) };
            const float previousMostSquareDistance { glm::dot(previousMost, previousMost) };
            if(squareDistance < previousLeastSquareDistance) {
                intersectionPoints[0] = intersection;
            } else if(squareDistance > previousMostSquareDistance) {
                intersectionPoints[1] = intersection;
            }
        }
    }

    return { nIntersections, { intersectionPoints[0], intersectionPoints[1] } };
}

std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> getCylinderIntersections(
    const Ray& ray,
    const glm::vec3& cylinderOrigin,
    const glm::vec3& cylinderAxis,
    float cylinderHeight,
    float cylinderRadius
) {
    // Normalize ray and cylinder directions before moving on
    const glm::vec3 directionRay { glm::normalize( ray.mDirection ) };
    const glm::vec3 directionCylinder { glm::normalize(cylinderAxis) };

    // Hold our results here
    uint8_t nIntersections { 0 };
    std::array<glm::vec3, 2> intersectionPoints { glm::vec3{std::numeric_limits<float>::infinity()}, glm::vec3{std::numeric_limits<float>::infinity()} };

    // Compute and store reusable constants
    const glm::vec3 cylinderLowerBound { cylinderOrigin - cylinderHeight * .5f * directionCylinder };
    const glm::vec3 originRayRelativeCylinder { ray.mStart - cylinderLowerBound };
    const float dottedDirections { glm::dot(directionRay, directionCylinder) };
    const float dotRayOriginDirection { glm::dot(originRayRelativeCylinder, directionRay) };
    const float dotRayOriginCylinderAxis { glm::dot(originRayRelativeCylinder, directionCylinder) };

    // Either 0 or 2 means directions are aligned and no intersection possible
    const float qdrtcA { 1.f - dottedDirections * dottedDirections };
    if(qdrtcA == 0 || qdrtcA == 2) return { nIntersections, { intersectionPoints[0], intersectionPoints[1] } };

    const float qdrtcB { 2.f * (dotRayOriginDirection - dotRayOriginCylinderAxis * dottedDirections) };
    const float qdrtcC { 
        glm::dot(originRayRelativeCylinder, originRayRelativeCylinder)
        - dotRayOriginCylinderAxis * dotRayOriginCylinderAxis
        - cylinderRadius * cylinderRadius
    };

    const float discriminant { qdrtcB * qdrtcB - 4.f * qdrtcA * qdrtcC };
    if(discriminant <= 0) {
        return { nIntersections, { intersectionPoints[0], intersectionPoints[1] } };
    }

    const float solution0 { (-qdrtcB + glm::sqrt(discriminant)) / (2 * qdrtcA) };
    const float solution1 { (-qdrtcB - glm::sqrt(discriminant)) / (2 * qdrtcA) };

    // If solution 0 is somewhere along the length of the ray, test cylinder constraint
    if(
        solution0 >= 0 && solution0 <= ray.mLength
    ) {
        const glm::vec3 intersectionPoint { ray.mStart + solution0 * directionRay };
        const float dotRelativeHitCylinderBottom { glm::dot(intersectionPoint - cylinderLowerBound, directionCylinder) };
        const glm::vec3 intersectionPointSpine { 
            dotRelativeHitCylinderBottom * directionCylinder
            + cylinderLowerBound
        };
        const float hitHeight { glm::length(intersectionPointSpine - cylinderLowerBound) };
        if(hitHeight <= cylinderHeight && dotRelativeHitCylinderBottom >= 0) {
            intersectionPoints[nIntersections++] = ray.mStart + solution0 * directionRay;
        }
    }

    // If solution 1 is somewhere along the length of the ray, test cylinder constraint
    if(
        solution1 >= 0 && solution1 <= ray.mLength
    ) {
        const glm::vec3 intersectionPoint { ray.mStart + solution1 * directionRay };
        const float dotRelativeHitCylinderBottom { glm::dot(intersectionPoint - cylinderLowerBound, directionCylinder) };
        const glm::vec3 intersectionPointSpine { 
            dotRelativeHitCylinderBottom * directionCylinder
            + cylinderLowerBound
        };
        const float hitHeight { glm::length(intersectionPointSpine - cylinderLowerBound) };
        if(hitHeight <= cylinderHeight && dotRelativeHitCylinderBottom >= 0) {
            intersectionPoints[nIntersections++] = ray.mStart + solution1 * directionRay;
        }
    }

    // Sort the intersections by distance from ray origin
    if(nIntersections == 2) {
        const glm::vec3 pointOne { intersectionPoints[0] - ray.mStart };
        const glm::vec3 pointTwo { intersectionPoints[1] - ray.mStart };
        if(glm::dot(pointOne, pointOne) > glm::dot(pointTwo, pointTwo)) {
            std::swap(intersectionPoints[0], intersectionPoints[1]);
        }
    }

    return { nIntersections, { intersectionPoints[0], intersectionPoints[1] } };
}

bool checkContainsPointBox(const glm::vec3& point, const ObjectBounds& box) {
    const glm::quat boxOrientation { box.getComputedWorldOrientation() };
    const glm::vec3 boxPosition { box.getComputedWorldPosition() };
    const glm::mat4 boxTransformInverse {
        glm::inverse(buildModelMatrix(glm::vec4{boxPosition, 1.f}, boxOrientation))
    };
    const glm::vec4 point4 { point, 1.f };
    const glm::vec3 pointRelativeBox {
        static_cast<glm::vec3>(boxTransformInverse * point4)
    };
    return overlaps(pointRelativeBox, AxisAlignedBounds{
        glm::vec3{ 0.f },
        box.mTrueVolume.mBox.mDimensions
    });
}

bool checkContainsPointSphere(const glm::vec3& point, const ObjectBounds& sphere) {
    const glm::vec3 sphereCenterToPoint {
        point - sphere.getComputedWorldPosition()
    };
    const float sphereRadius { sphere.mTrueVolume.mSphere.mRadius };
    return (
        glm::dot(sphereCenterToPoint, sphereCenterToPoint) 
        < sphereRadius * sphereRadius
    );
}

bool checkContainsPointCapsule(const glm::vec3& point, const ObjectBounds& capsule) {
    // Cache some specifics about our capsule
    const glm::vec3 capsulePosition { capsule.getComputedWorldPosition() };
    const glm::mat3 capsuleRotation { glm::mat3_cast(capsule.getComputedWorldOrientation()) };
    const glm::vec3 capsuleAxis { capsuleRotation * glm::vec3 { 0.f, 1.f, 0.f } };
    const float capsuleRadius { capsule.mTrueVolume.mCapsule.mRadius };
    const float capsuleHeight { capsule.mTrueVolume.mCapsule.mHeight };
    const std::array<glm::vec3, 2> capsuleEnds {
        capsulePosition - capsule.mTrueVolume.mCapsule.mHeight * .5f * capsuleAxis,
        capsulePosition + capsule.mTrueVolume.mCapsule.mHeight * .5f * capsuleAxis
    };

    const ObjectBounds sphere0 { ObjectBounds::create(
        VolumeSphere { .mRadius { capsuleRadius } },
        capsuleEnds[0],
        capsuleRotation
    ) };
    const ObjectBounds sphere1 { ObjectBounds::create(
        VolumeSphere { .mRadius { capsuleRadius } },
        capsuleEnds[1],
        capsuleRotation
    ) };

    const glm::vec3 projectedAlong {
        glm::dot(point - capsulePosition, capsuleAxis) * capsuleAxis
    };
    const glm::vec3 projectedPerpendicular {
        point - capsulePosition - projectedAlong
    };

    const bool withinCylinderHeight { glm::dot(projectedAlong, projectedAlong) < capsuleHeight * capsuleHeight * 0.25 };
    const bool withinCylinderRadius { glm::dot(projectedPerpendicular, projectedPerpendicular) < capsuleRadius * capsuleRadius };

    return (
        (withinCylinderHeight && withinCylinderRadius)
        || checkContainsPointSphere(point, sphere0)
        || checkContainsPointSphere(point, sphere1)
    );
}

template <typename T, typename U>
inline bool gjkOverlaps(const T& one, const U& two) {
    glm::vec3 searchDirection { two.getComputedWorldPosition() - one.getComputedWorldPosition() };

    // Two non-degenerate objects share the same origin, obviously they overlap
    if(squareDistance(searchDirection) == 0.f) {
        return true;
    }

    std::array<glm::vec3, 4> simplex {};
    simplex[0] = minkowskiDifference(one, two, searchDirection);
    uint8_t nSimplexPoints { 1 };

    // we go towards the origin from the first point we found
    searchDirection = -simplex[0]; 
    bool found { false };
    while(true) {
        const glm::vec3 candidatePoint { minkowskiDifference(one, two, searchDirection) };

        // We couldn't find a point past the origin in this direction, so obviously there is no intersection
        if(glm::dot(candidatePoint, searchDirection) <= 0) {
            return false;
        }

        simplex[nSimplexPoints++] = candidatePoint;
        assert(nSimplexPoints <= 4 && nSimplexPoints >= 2 && "Within the loop, we must always have between 2 and 4 points in the simplex");

        searchDirection = doSimplex(simplex, nSimplexPoints, found);
        if(found) {
            return true;
        }
    }
}

std::pair<float, float> axisProjectBox(const glm::vec3& axis, const ObjectBounds& box) {
    const glm::vec3 axisNormalized { glm::normalize(axis) };
    float minimum { std::numeric_limits<float>::infinity() };
    float maximum { -std::numeric_limits<float>::infinity() };

    for(const glm::vec3& point : box.getWorldOrientedBoxCorners()) {
        const float projectedPoint { glm::dot(axisNormalized, point) };
        if(projectedPoint < minimum) {
            minimum = projectedPoint;
        }
        if(projectedPoint > maximum) {
            maximum = projectedPoint;
        }
    }

    return { minimum, maximum };
}

std::pair<float, float> axisProjectSphere(const glm::vec3& axis, const ObjectBounds& sphere) {
    const glm::vec3 axisNormalized { glm::normalize(axis) };

    const float pointOne { 
        glm::dot(axisNormalized, sphere.getComputedWorldPosition())
        + sphere.mTrueVolume.mSphere.mRadius
    };
    const float pointTwo {
        glm::dot(axisNormalized, sphere.getComputedWorldPosition())
        - sphere.mTrueVolume.mSphere.mRadius
    };

    if(pointOne > pointTwo) {
        return { pointTwo, pointOne };
    }
    return { pointOne, pointTwo };
}

std::pair<float, float> axisProjectCapsule(const glm::vec3& axis, const ObjectBounds& capsule) {
    const glm::vec3 capsulePosition { capsule.getComputedWorldPosition() };
    const glm::mat3 capsuleRotation { glm::mat3_cast(capsule.getComputedWorldOrientation()) };
    const glm::vec3 capsuleAxis { capsuleRotation * glm::vec3 { 0.f, 1.f, 0.f } };
    const std::array<glm::vec3, 2> capsuleEnds {
        capsulePosition - capsule.mTrueVolume.mCapsule.mHeight * .5f * capsuleAxis,
        capsulePosition + capsule.mTrueVolume.mCapsule.mHeight * .5f * capsuleAxis
    };

    const glm::vec3 axisNormalized { glm::normalize(axis) };
    float minimum { std::numeric_limits<float>::infinity() };
    float maximum { -std::numeric_limits<float>::infinity() };
    for(const auto& point: capsuleEnds) {
        const float projectedPoint { glm::dot(axisNormalized, point) };
        if(projectedPoint > maximum) {
            maximum = projectedPoint;
        }
        if(projectedPoint < minimum) {
            minimum = projectedPoint;
        }
    }

    return {
        minimum - capsule.mTrueVolume.mCapsule.mRadius,
        maximum + capsule.mTrueVolume.mCapsule.mRadius
    };
}

std::array<glm::vec3, 3> getBoxEdgeAxes(const ObjectBounds& box) {
    const glm::vec3 localForward { 0.f, 0.f, -1.f };
    const glm::vec3 localRight { 1.f, 0.f, 0.f };
    const glm::vec3 localUp { 0.f, 1.f, 0.f };

    const glm::mat3 boxOrientation { box.getComputedWorldOrientation() };
    std::array<glm::vec3, 3> boxAxes {{
        { boxOrientation * localForward },
        { boxOrientation * localRight },
        { boxOrientation * localUp }
    }};

    return boxAxes;
}

bool checkOverlaps1D(float shape1Min, float shape1Max, float shape2Min, float shape2Max) {
    return shape1Min < shape2Max && shape1Max > shape2Min;
}

bool checkContains1D(float shape1Min, float shape1Max, float shape2Min, float shape2Max) {
    return shape1Max <= shape2Max && shape1Min >= shape2Min;
}


glm::vec3 getClosestPointOnBox(const glm::vec3& point, const ObjectBounds& box) {
    const std::array<glm::vec3, 8> boxCorners { box.getWorldOrientedBoxCorners() };
    const std::array<glm::vec3, 3> boxAxes {{
        boxCorners[BoxCornerSpecifier::RIGHT] - boxCorners[0],
        boxCorners[BoxCornerSpecifier::TOP] - boxCorners[0],
        boxCorners[BoxCornerSpecifier::FRONT] - boxCorners[0]
    }};

    const glm::vec3 projectedPoint {
        glm::dot(point - boxCorners[0], boxAxes[0]) / squareDistance(boxAxes[0]),
        glm::dot(point - boxCorners[0], boxAxes[1]) / squareDistance(boxAxes[1]),
        glm::dot(point - boxCorners[0], boxAxes[2]) / squareDistance(boxAxes[2]),
    };

    const glm::vec3 saturatedPoint {
        (projectedPoint.x > 1.f ? 1.f : (projectedPoint.x < 0.f ? 0.f : projectedPoint.x)),
        (projectedPoint.y > 1.f ? 1.f : (projectedPoint.y < 0.f ? 0.f : projectedPoint.y)),
        (projectedPoint.z > 1.f ? 1.f : (projectedPoint.z < 0.f ? 0.f : projectedPoint.z)),
    };

    return boxCorners[0] + (
        saturatedPoint.x * boxAxes[0]
        + saturatedPoint.y * boxAxes[1]
        + saturatedPoint.z * boxAxes[2]
    );
}

glm::vec3 getSupportBox(const glm::vec3& axis, const AxisAlignedBounds& box) {
    const glm::vec3 origin { box.getComputedWorldPosition() };
    const std::array<glm::vec3, 8> corners { box.getAxisAlignedBoxCorners() };

    // the point we're looking for _must_ be one of the corners of the box
    std::size_t answer;
    float greatestDistance { -std::numeric_limits<float>::infinity() };
    for(std::size_t i { 0 }; i < 8; ++i) {
        const float newDistance { glm::dot(corners[i] - origin, axis) };
        if(newDistance > greatestDistance) {
            greatestDistance = newDistance;
            answer = i;
        }
    }

    return corners[answer];
}

glm::vec3 getSupportBox(const glm::vec3& axis, const ObjectBounds& box) {
    const glm::vec3 origin { box.getComputedWorldPosition() };
    const std::array<glm::vec3, 8> corners { box.getWorldOrientedBoxCorners() };

    // the point we're looking for _must_ be one of the corners of the box
    std::size_t answer;
    float greatestDistance { -std::numeric_limits<float>::infinity() };
    for(std::size_t i { 0 }; i < 8; ++i) {
        const float newDistance { glm::dot(corners[i] - origin, axis) };
        if(newDistance > greatestDistance) {
            greatestDistance = newDistance;
            answer = i;
        }
    }

    return corners[answer];
}

glm::vec3 getSupportSphere(const glm::vec3& axis, const ObjectBounds& sphere) {
    const glm::vec3 origin { sphere.getComputedWorldPosition() };
    const float radius { sphere.mTrueVolume.mSphere.mRadius };

    return origin + radius * glm::normalize(axis);
}

glm::vec3 getSupportCapsule(const glm::vec3& vector, const ObjectBounds& capsule) {
    const glm::vec3 origin { capsule.getComputedWorldPosition() };
    const glm::vec3 capsuleAxis { glm::normalize(
        capsule.getComputedWorldOrientation() * glm::vec3 { 0.f, 1.f, 0.f }
    ) };
    const float radius { capsule.mTrueVolume.mCapsule.mRadius };
    const float height { capsule.mTrueVolume.mCapsule.mHeight };

    // The point we're looking for is the same as the one on a sphere, but offset
    // towards one of the capsule's end caps
    const glm::vec3 equivalentSphereSupport { origin + radius * glm::normalize(vector) };
    const float dotAxisVector { glm::dot(capsuleAxis, vector) };
    const glm::vec3 offset {
        dotAxisVector == 0.f ? glm::vec3(0.f) : (
            dotAxisVector > 0.f ? .5f : -.5f
        ) * height * capsuleAxis
    };

    return origin + offset + equivalentSphereSupport;
}

glm::vec3 doSimplex(std::array<glm::vec3, 4>& simplex, uint8_t& nSimplexPoints, bool& found) {
    assert(!found && "What are we doing if we found our enclosing simplex already?");
    switch(nSimplexPoints) {
    case 2:
        return doSimplex2(simplex, nSimplexPoints);
    case 3:
        return doSimplex3(simplex, nSimplexPoints);
    case 4:
        return doSimplex4(simplex, nSimplexPoints, found);
    default:
        assert(false && "There is no reason whatsoever that a 3D simplex should have more than 4 or less than 2 points");
        return glm::vec3{ std::numeric_limits<float>::infinity() };
    }
}

glm::vec3 doSimplex2(std::array<glm::vec3, 4>& simplex, uint8_t& nSimplexPoints) {
    assert(nSimplexPoints == 2 && "This function should only be called when the simplex contains 2 points");

    const glm::vec3 pointA { simplex[1] };
    const glm::vec3 pointB { simplex[0] };

    // This constant shows up multiple times
    const glm::vec3 lineAB { pointB - pointA };

    // Note: we already know that A is beyond origin relative to B, so we can jump
    // straight to computing the perpendicular to AB

    // If B from A is the same direction as origin from A, go along
    // perpendicular to AB towards origin
    return glm::cross(glm::cross(lineAB, -pointA), lineAB);
}

glm::vec3 doSimplex3(std::array<glm::vec3, 4>& simplex, uint8_t& nSimplexPoints) {
    assert(nSimplexPoints == 3 && "This function should only be called when the simplex contains 3 points");

    const glm::vec3 pointA { simplex[2] };
    const glm::vec3 pointB { simplex[1] };
    const glm::vec3 pointC { simplex[0] };

    // A few constants which show up on multiple conditions
    const glm::vec3 lineAC { pointC - pointA };
    const glm::vec3 lineAB { pointB - pointA };
    const glm::vec3 crossABAC { glm::cross(lineAB, lineAC) };

    // See if our origin is past the triangle beyond lineAC
    if(glm::dot(glm::cross(crossABAC, lineAC), -pointA) > 0) {
        if(glm::dot(lineAC, -pointA) > 0) {
            nSimplexPoints = 2;
            simplex[0] = pointC;
            simplex[1] = pointA;
            return glm::cross(glm::cross(lineAC, -pointA), lineAC);

        // We now know we're closest to AB
        } else {
            nSimplexPoints = 2;
            simplex[0] = pointB;
            simplex[1] = pointA;
            return glm::cross(glm::cross(lineAB, -pointA), lineAB);
        }

    // See if we're beyond the triangle beyond lineAB
    } else if(glm::dot(glm::cross(lineAB, crossABAC), -pointA) > 0) {
        // We now know we're closest to AB (and by the first if we've already ruled out AC)
        nSimplexPoints = 2;
        simplex[0] = pointB;
        simplex[1] = pointA;
        return glm::cross(glm::cross(lineAB, -pointA), lineAB);

    // Test if origin is above the triangle
    } else if(glm::dot(crossABAC, -pointA) > 0) {
        return crossABAC;

    // We now know our origin is *below* the triangle
    } else {
        simplex[0] = pointB;
        simplex[1] = pointC;
        return -crossABAC;
    }
}

glm::vec3 doSimplex4(std::array<glm::vec3, 4>& simplex, uint8_t& nSimplexPoints, bool& found) {
    assert(!found && "What are we even doing here if we've already found an enclosing simplex?");
    assert(nSimplexPoints == 4 && "This function should only be called when the simplex contains 4 points");

    const glm::vec3 pointA { simplex[3] };
    const glm::vec3 pointB { simplex[2] };
    const glm::vec3 pointC { simplex[1] };
    const glm::vec3 pointD { simplex[0] };

    // A few constants which show up on multiple conditions
    const glm::vec3 lineAD { pointD - pointA };
    const glm::vec3 lineAC { pointC - pointA };
    const glm::vec3 lineAB { pointB - pointA };
    // Each cross below is a normal pointing outward from the triangle that forms a
    // face of the simplex tetrahedron
    const glm::vec3 crossABAC { glm::cross(lineAD, lineAB) };
    const glm::vec3 crossACAD { glm::cross(lineAC, lineAD) };
    const glm::vec3 crossADAB { glm::cross(lineAD, lineAB) };
    // note: we don't care about crossBCBD -- we already know we're above it

    // see if we're beyond face ABC
    if(glm::dot(crossABAC, -pointA) > 0) {
        // Are we closest to the region near line AC
        if(glm::dot(glm::cross(crossABAC, lineAC), -pointA) > 0) {
            // continue the search near line AC
            nSimplexPoints = 2;
            simplex[0] = pointC;
            simplex[1] = pointA;
            return glm::cross(glm::cross(lineAC, -pointA), lineAC);

        // Are we closest to the region near line AB?
        } else if(glm::dot(glm::cross(lineAB, crossABAC), -pointA) > 0) {
            nSimplexPoints = 2;
            simplex[0] = pointB;
            simplex[1] = pointA;
            return glm::cross(glm::cross(lineAB, -pointA), lineAB);
        }

        // We are directly above the face ABC of the tetrahedron
        nSimplexPoints = 3;
        simplex[0] = pointC;
        simplex[1] = pointB;
        simplex[2] = pointA;
        return crossABAC;

    // See if we're above the tetrahedron face ACD
    } else if(glm::dot(crossACAD, -pointA) > 0) {
        // Are we closest to line AD?
        if(glm::dot(glm::cross(crossACAD, lineAD), -pointA) > 0) {
            nSimplexPoints = 2;
            simplex[0] = pointD;
            simplex[1] = pointA;
            return glm::cross(glm::cross(lineAD, -pointA), lineAD);

        // Are we closest to line AC?
        } else if(glm::dot(glm::cross(lineAC, crossACAD), -pointA) > 0) {
            nSimplexPoints = 2;
            simplex[0] = pointC;
            simplex[1] = pointA;
            return glm::cross(glm::cross(lineAC, -pointA), lineAC);
        }

        // We are directly above face ACD of the tetrahedron
        nSimplexPoints = 3;
        simplex[0] = pointD;
        simplex[1] = pointC;
        simplex[2] = pointA;
        return crossACAD;

    // Finally check if we're above face ADB
    } else if(glm::dot(crossADAB, -pointA) > 0) {
        // Are we closest to line AB?
        if(glm::dot(glm::cross(crossADAB, lineAB), -pointA) > 0) {
            nSimplexPoints = 2;
            simplex[0] = pointB;
            simplex[1] = pointA;
            return glm::cross(glm::cross(lineAB, -pointA), lineAB);

        // ... or line AD?
        } else if(glm::dot(glm::cross(lineAD, crossADAB), -pointA) > 0) {
            nSimplexPoints = 2;
            simplex[0] = pointD;
            simplex[1] = pointA;
            return glm::cross(glm::cross(lineAD, -pointA), lineAD);
        }

        // We are certainly directly above face ADB of the tetrahedron
        nSimplexPoints = 3;
        simplex[0] = pointB;
        simplex[1] = pointD;
        simplex[2] = pointA;
        return crossADAB;
    }

    // We've exhausted every other option, we _know_ now
    // that we're in the tetrahedron
    found = true;
    return glm::vec3 { 0.f };
}

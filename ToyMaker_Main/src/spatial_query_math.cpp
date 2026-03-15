#include <cmath>
#include "toymaker/engine/spatial_query_math.hpp"

using namespace ToyMaker;

inline float squareDistance(const glm::vec3& vector) { return glm::dot(vector, vector); }

bool ToyMaker::ObjectBounds::isPositive() const {
    bool isBoundsNonZero;
    switch(mType) {
        case ToyMaker::ObjectBounds::TrueVolumeType::BOX:
            isBoundsNonZero = ToyMaker::isPositive(mTrueVolume.mBox.mDimensions);
            break;
        case ToyMaker::ObjectBounds::TrueVolumeType::CAPSULE:
            isBoundsNonZero = ToyMaker::isPositive(mTrueVolume.mCapsule.mRadius);
            break;
        case ToyMaker::ObjectBounds::TrueVolumeType::SPHERE:
            isBoundsNonZero = ToyMaker::isPositive(mTrueVolume.mSphere.mRadius);
            break;
        default:
            assert(false && "Unrecognized shape passed");
    }
    return isBoundsNonZero;
}

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

bool checkBoxContains(const glm::vec3& point, const ObjectBounds& box);
bool checkCapsuleContains(const glm::vec3& point, const ObjectBounds& capsule);
bool checkSphereContains(const glm::vec3& point, const ObjectBounds& sphere);

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
    if(!isPositive(bounds.getDimensions())) {
        return {false, {glm::vec3{std::numeric_limits<float>::infinity()}, glm::vec3{std::numeric_limits<float>::infinity()}}};
    }

    return getBoxIntersections(ray, bounds);
}

std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> ToyMaker::computeIntersections(const Ray& ray, const ObjectBounds& bounds) {
    assert(ray.isSensible() && "Invalid ray provided");
    assert(bounds.isSensible() && "Invalid bounds provided");

    // Bounds should be greater than zero, representing a real volume in the 
    // scene
    if(!bounds.isPositive()) {
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
    if(!isPositive(bounds.getDimensions())) {
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
    if(!bounds.isPositive()) {
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
        otherBottomCorner.x <= topCorner.x && otherTopCorner.x >= bottomCorner.x 
        // overlap in y
        && otherBottomCorner.y <= topCorner.y && otherTopCorner.y >= bottomCorner.y
        // overlap in z
        && otherBottomCorner.z <= topCorner.z && otherTopCorner.z >= bottomCorner.z
    );
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
        bottomCorner.x <= otherBottomCorner.x && topCorner.x >= otherTopCorner.x
        // contained in y
        && bottomCorner.y <= otherBottomCorner.y && topCorner.y >= otherTopCorner.y
        // contained in z
        && bottomCorner.z <= otherBottomCorner.z && topCorner.z >= otherTopCorner.z
    );
}

bool ToyMaker::contains(const glm::vec3& point, const ObjectBounds& bounds) {
    assert(isFinite(point) && "Invalid point provided");
    assert(bounds.isSensible() && "Invalid bounds provided");

    // Bounds should be greater than zero, representing a real volume in the 
    // scene
    if(!bounds.isPositive()) {
        return false;
    }

    switch(bounds.mType) {
        case ToyMaker::ObjectBounds::TrueVolumeType::BOX:
            return checkBoxContains(point, bounds);
        case ToyMaker::ObjectBounds::TrueVolumeType::CAPSULE:
            return checkCapsuleContains(point, bounds);
        case ToyMaker::ObjectBounds::TrueVolumeType::SPHERE:
            return checkSphereContains(point, bounds);
        default:
            assert(false && "Unrecognized shape passed");
    }

}

bool ToyMaker::contains(const Ray& ray, const ObjectBounds& bounds) {
    assert(ray.isSensible() && "Invalid ray provided");
    assert(bounds.isSensible() && "Invalid bounds provided");

    // Bounds should be greater than zero, representing a real volume in the 
    // scene
    if(!bounds.isPositive()) {
        return false;
    }

    // Infinite rays immediately fail containment in finite bounds
    if(!isFinite(ray.mLength)) return false;

    // Bounds contain a ray if it contains both the ray's start and end points
    const glm::vec3 rayStart { ray.mStart };
    const glm::vec3 rayEnd { rayStart + glm::normalize(ray.mDirection) * ray.mLength };
    return contains(ray.mStart, bounds) && contains(rayEnd, bounds);
}

ObjectBounds ObjectBounds::create(const VolumeBox& box, const glm::vec3& positionOffset, const glm::vec3& orientationOffset) {
    return ObjectBounds {
        .mType { TrueVolumeType::BOX },
        .mTrueVolume { .mBox { box } },
        .mPositionOffset { positionOffset },
        .mOrientationOffset{ orientationOffset },
    };
}

ObjectBounds ObjectBounds::create(const VolumeSphere& sphere, const glm::vec3& positionOffset, const glm::vec3& orientationOffset) {
    return ObjectBounds {
        .mType { TrueVolumeType::SPHERE },
        .mTrueVolume { .mSphere { sphere } },
        .mPositionOffset { positionOffset },
        .mOrientationOffset { orientationOffset },
    };
}

ObjectBounds ObjectBounds::create(const VolumeCapsule& capsule, const glm::vec3& positionOffset, const glm::vec3& orientationOffset) {
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

AxisAlignedBounds::AxisAlignedBounds(): 
mExtents { glm::vec3{0.f}, glm::vec3{0.f} }
{}

AxisAlignedBounds::AxisAlignedBounds(const ObjectBounds& objectBounds): AxisAlignedBounds{} {
    Extents axisAlignedExtents{
        glm::vec3{ -std::numeric_limits<float>::infinity() },
        glm::vec3{ std::numeric_limits<float>::infinity()  },
    };
    for(auto& corner: objectBounds.getWorldOrientedBoxCorners()) {
        axisAlignedExtents.first = glm::max(axisAlignedExtents.first, corner);
        axisAlignedExtents.second = glm::min(axisAlignedExtents.second, corner);
    }
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
    const float discriminant { qdrtcB * qdrtcB - 4.f * qdrtcA * qdrtcA };

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
                capsuleAxis
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
            capsuleAxis
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
            intersectionPoints[nIntersections++] = ray.mStart + solution0 * directionRay;
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

bool checkBoxContains(const glm::vec3& point, const ObjectBounds& box) {
    const glm::quat boxOrientation { box.getComputedWorldOrientation() };
    const glm::vec3 boxPosition { box.getComputedWorldPosition() };
    const glm::mat4 boxTransformInverse {
        glm::inverse(buildModelMatrix(glm::vec4{boxPosition, 1.f}, boxOrientation))
    };
    const glm::vec4 point4 { point, 1.f };
    const glm::vec3 pointRelativeBox {
        static_cast<glm::vec3>(boxTransformInverse * point4)
    };
    return overlaps(point, AxisAlignedBounds{
        glm::vec3{0.f, 0.f, 0.f},
        box.mTrueVolume.mBox.mDimensions
    });
}

bool checkSphereContains(const glm::vec3& point, const ObjectBounds& sphere) {
    const glm::vec3 sphereCenterToPoint {
        point - sphere.getComputedWorldPosition()
    };
    const float sphereRadius { sphere.mTrueVolume.mSphere.mRadius };
    return (
        glm::dot(sphereCenterToPoint, sphereCenterToPoint) 
        < sphereRadius * sphereRadius
    );
}

bool checkCapsuleContains(const glm::vec3& point, const ObjectBounds& capsule) {
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
        capsuleAxis
    ) };
    const ObjectBounds sphere1 { ObjectBounds::create(
        VolumeSphere { .mRadius { capsuleRadius } },
        capsuleEnds[1],
        capsuleAxis
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
        || checkSphereContains(point, sphere0)
        || checkSphereContains(point, sphere1)
    );
}

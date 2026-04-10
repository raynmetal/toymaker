#include <cmath>
#include "toymaker/engine/spatial_query/types.hpp"

using namespace ToyMaker;

// Gets the minimum and maximum points of a 3D box projected on an axis
std::pair<float, float> axisProjectBox(const glm::vec3& axis, const ObjectBounds& box);
// Gets the minimum and maximum points of a sphere projected on an axis
std::pair<float, float> axisProjectSphere(const glm::vec3& axis, const ObjectBounds& sphere);
// Gets the minimum and maximum points of a capsule projected on an axis
std::pair<float, float> axisProjectCapsule(const glm::vec3& axis, const ObjectBounds& capsule);

glm::vec3 getSupportBox(const glm::vec3& axis, const AxisAlignedBounds& box);
glm::vec3 getSupportBox(const glm::vec3& axis, const ObjectBounds& box);
glm::vec3 getSupportSphere(const glm::vec3& axis, const ObjectBounds& sphere);
glm::vec3 getSupportCapsule(const glm::vec3& axis, const ObjectBounds& capsule);

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
    assert(squareDistance(axis) && "Axis must be non-zero vector");
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

    return offset + equivalentSphereSupport;
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

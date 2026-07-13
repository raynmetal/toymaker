#include <set>

#include "toymaker/engine/util.hpp"
#include "toymaker/engine/spatial_query/types.hpp"

using namespace ToyMaker;

constexpr float kEPAEpsilon { 0.001 };


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

ObjectBounds ObjectBounds::create(const VolumeBox& box, const glm::vec3& positionOffset, const glm::quat& orientationOffset, InteractionLayerMask interactionLayers, InteractionLayerMask interactionMask) {
    return ObjectBounds {
        .mType { TrueVolumeType::BOX },
        .mTrueVolume { .mBox { box } },
        .mPositionWorld { positionOffset },
        .mPositionOffset { positionOffset },
        .mOrientationWorld { orientationOffset },
        .mOrientationOffset{ orientationOffset },
        .mInteractionLayers { interactionLayers },
        .mInteractionMask { interactionMask },
    };
}

ObjectBounds ObjectBounds::create(const VolumeSphere& sphere, const glm::vec3& positionOffset, const glm::quat& orientationOffset, InteractionLayerMask interactionLayers, InteractionLayerMask interactionMask) {
    return ObjectBounds {
        .mType { TrueVolumeType::SPHERE },
        .mTrueVolume { .mSphere { sphere } },
        .mPositionWorld { positionOffset },
        .mPositionOffset { positionOffset },
        .mOrientationWorld { orientationOffset },
        .mOrientationOffset { orientationOffset },
        .mInteractionLayers { interactionLayers },
        .mInteractionMask { interactionMask },
    };
}

ObjectBounds ObjectBounds::create(const VolumeCapsule& capsule, const glm::vec3& positionOffset, const glm::quat& orientationOffset, InteractionLayerMask interactionLayers, InteractionLayerMask interactionMask) {
    return ObjectBounds {
        .mType { TrueVolumeType::CAPSULE },
        .mTrueVolume { .mCapsule { capsule } },
        .mPositionWorld { positionOffset },
        .mPositionOffset { positionOffset },
        .mOrientationWorld { orientationOffset },
        .mOrientationOffset { orientationOffset },
        .mInteractionLayers { interactionLayers },
        .mInteractionMask { interactionMask },
    };
}

glm::vec3 Polytope::getNextSearch() const {
    const Face& triangle { mFaces.top() };

    const glm::vec3 crossTriangle { glm::cross(
        mPoints[triangle.mIndices[1]] - mPoints[triangle.mIndices[0]],
        mPoints[triangle.mIndices[2]] - mPoints[triangle.mIndices[0]]
    ) };

    return crossTriangle;
}

glm::vec3 Polytope::getClosestPoint() const {
    const Face& triangle { mFaces.top() };
    const glm::vec3 normTriangle { getClosestTriangleNormal() };
    return glm::dot(normTriangle, mPoints[triangle.mIndices[0]]) * normTriangle;
}

glm::vec3 Polytope::getClosestTriangleNormal() const {
    const Face& triangle { mFaces.top() };
    return { glm::normalize(glm::cross(
        mPoints[triangle.mIndices[1]] - mPoints[triangle.mIndices[0]],
        mPoints[triangle.mIndices[2]] - mPoints[triangle.mIndices[0]]
    )) };
}

AreaTriangle Polytope::getClosestTriangle() const {
    const Face& triangle { mFaces.top() };
    return { .mPoints {
        mPoints[triangle.mIndices[0]],
        mPoints[triangle.mIndices[1]],
        mPoints[triangle.mIndices[2]],
    } };
}

AreaTriangle Polytope::getClosestTriangleSupportA() const {
    const Face& triangle { mFaces.top() };
    return { .mPoints {
        mPointsSupportA[triangle.mIndices[0]],
        mPointsSupportA[triangle.mIndices[1]],
        mPointsSupportA[triangle.mIndices[2]],
    } };
}

AreaTriangle Polytope::getClosestTriangleSupportB() const {
    const Face& triangle { mFaces.top() };
    return { .mPoints {
        mPointsSupportB[triangle.mIndices[0]],
        mPointsSupportB[triangle.mIndices[1]],
        mPointsSupportB[triangle.mIndices[2]],
    } };
}

Polytope::Polytope(): mFaces([this](const Face& one, const Face& two) -> bool {
    const glm::vec3 normOne { glm::normalize(this->getTriangleCross(one)) };
    const glm::vec3 normTwo { glm::normalize(this->getTriangleCross(two)) };
    return glm::dot(normOne, mPoints[one.mIndices[0]]) > glm::dot(normTwo, mPoints[two.mIndices[0]]);
}) {}

Polytope::Polytope(const Simplex& simplex): Polytope{} {
    assert(simplex.mNPoints == 4 && "Polytope must be formed from tetrahedral simplex");

    for(uint8_t i { 0 }; i < 4; ++i) {
        mPoints.push_back(simplex.mPoints[i]);
        mPointsSupportA.push_back(simplex.mPointsSupportA[i]);
        mPointsSupportB.push_back(simplex.mPointsSupportB[i]);
    }

    // Store indices in counterclockwise winding so that triangle norms
    // point _away_ from origin
    mFaces.push(Face { .mIndices { 3, 2, 1 } });
    mFaces.push(Face { .mIndices { 3, 1, 0 } });
    mFaces.push(Face { .mIndices { 3, 0, 2 } });
    mFaces.push(Face { .mIndices { 1, 2, 0 } });
}

bool Polytope::append(const glm::vec3& newPoint, const glm::vec3& supportA, const glm::vec3& supportB) {
    assert(supportA - supportB == newPoint && "The difference of the supports should yield the new point");
    const glm::vec3 crossTriangle { getNextSearch() };
    const Face& oldTriangle { mFaces.top() };
    const float oldDistance { glm::dot(
        crossTriangle, mPoints[oldTriangle.mIndices[0]]
    ) };
    const float newDistance { glm::dot(crossTriangle, newPoint) };

    // Our new point is not further from the origin in this direction; there
    // is nothing to be done
    if(newDistance - oldDistance <= kEPAEpsilon) {
        return false;
    }

    // Add the new point to our list of points
    const uint16_t indexNew { static_cast<uint16_t>(mPoints.size()) };
    mPoints.push_back(newPoint);
    mPointsSupportA.push_back(supportA);
    mPointsSupportB.push_back(supportB);

    // Copy over old faces into a container that's easier to iterate over
    std::vector<Face> oldFaces {};
    while(!mFaces.empty()) {
        oldFaces.push_back(mFaces.top());
        mFaces.pop();
    }

    // Simultaneously build our edge list and cull faces seen by the
    // newly added point
    std::set<std::pair<uint16_t, uint16_t>> edgeList {};
    for(const Face& face: oldFaces) {
        // Faces that aren't seen by the new point survive
        if(glm::dot(getTriangleCross(face), newPoint - mPoints[face.mIndices[0]]) < 0) {
            mFaces.push(face);
            continue;
        }

        // Other faces have their edges added to the edge list
        for(uint8_t i { 0 }; i < 3; ++i) {
            const uint16_t one { face.mIndices[i] };
            const uint16_t two { face.mIndices[(i + 1)%3] };

            // This edge was already in the list (implying it's an 
            // internal edge).  Remove it, and move on to the next
            const auto found { edgeList.find(std::pair {two, one}) };
            if(found != edgeList.end()) {
                edgeList.erase(found);
                continue;
            }

            edgeList.insert(std::pair { one, two });
        }
    }

    // Construct triangles to add to the polytope based on the newly computed edge list
    for(const auto& edge: edgeList) {
        mFaces.push(Face { .mIndices { indexNew, edge.first, edge.second } });
    }
    return true;
}

void ObjectBounds::applyModelMatrix(const glm::mat4& modelMatrix) {
    mPositionOrigin = static_cast<glm::vec3>(modelMatrix * glm::vec4{0.f, 0.f, 0.f, 1.f});
    assert(isNumber(mPositionOrigin) && "Position update failed");
    mOrientationOrigin = glm::normalize(glm::quat_cast(getRotationMatrix(modelMatrix)));
    recomputeWorldPositionOrientation();
    assert(
        isNumber(mOrientationOrigin.w)
        && isNumber(glm::vec3 { mOrientationOrigin.x, mOrientationOrigin.y, mOrientationOrigin.z })
        && "Orientation update failed"
    );
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

glm::vec3 ObjectBounds::getPositionWorld() const {
    return mPositionWorld;
}

void ObjectBounds::setPositionWorld(const glm::vec3& newPosition) {
    mPositionWorld = newPosition;
    mPositionOrigin = newPosition - (
        glm::transpose(glm::mat3 { getRotationTransformOrigin() }) * mPositionOffset
    );
    assert(isNumber(mPositionOrigin) && "Position update failed");
    mTransformUpdateRequired = true;
}

glm::quat ObjectBounds::getOrientationWorld() const {
    return mOrientationWorld;
}

void ObjectBounds::recomputeWorldPositionOrientation() {
    mPositionWorld = mPositionOrigin + mOrientationOrigin * mPositionOffset;
    mOrientationWorld = mOrientationOrigin * mOrientationOffset;
}

void ObjectBounds::setOrientationWorld(const glm::quat& newOrientation) {
    const glm::quat orientationNormalized { glm::normalize(newOrientation) };
    mOrientationWorld = orientationNormalized;

    // object origin position depends on origin orientation
    const glm::vec3 worldPositionPrevious { getPositionWorld() };
    mOrientationOrigin = glm::normalize(glm::quat_cast(
        glm::mat3_cast(orientationNormalized) * glm::transpose(getRotationTransformLocal())
    ));
    assert(
        isNumber(mOrientationOrigin.w)
        && isNumber(glm::vec3{mOrientationOrigin.x, mOrientationOrigin.y, mOrientationOrigin.z})
        && "Orientation update failed"
    );
    // update world position such that it stays in-place by
    // updating the origin's position instead
    setPositionWorld(worldPositionPrevious);
    mTransformUpdateRequired = true;
}

std::array<glm::vec3, 8> ObjectBounds::getLocalOrientedBoxCorners() const {
    std::array<glm::vec3, 8> orientedCorners { getVolumeRelativeBoxCorners() };
    for(glm::vec3& localCorner: orientedCorners) {
        localCorner = mPositionOffset + mOrientationOffset * localCorner;
    }
    return orientedCorners;
}

std::array<glm::vec3, 8> ObjectBounds::getWorldOrientedBoxCorners() const {
    std::array<glm::vec3, 8> worldCorners { getLocalOrientedBoxCorners() };
    for(glm::vec3& orientedCorner: worldCorners) {
        orientedCorner = mPositionOrigin + mOrientationOrigin * orientedCorner;
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
        return getPositionWorld();
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
        return getPositionWorld();
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
    mInteractionLayers = objectBounds.mInteractionLayers;
    setByExtents(axisAlignedExtents);
}

AxisAlignedBounds::AxisAlignedBounds(const Extents& axisAlignedExtents, InteractionLayerMask interactionLayers): AxisAlignedBounds{} {
    setByExtents(axisAlignedExtents);
    mInteractionLayers = interactionLayers;
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

    return { extents, static_cast<InteractionLayerMask>(mInteractionLayers | other.mInteractionLayers) };
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
    const glm::vec3 origin { box.getPositionWorld() };
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
    const glm::vec3 origin { box.getPositionWorld() };
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
    const glm::vec3 origin { sphere.getPositionWorld() };
    const float radius { sphere.mTrueVolume.mSphere.mRadius };

    return origin + radius * glm::normalize(axis);
}

glm::vec3 getSupportCapsule(const glm::vec3& vector, const ObjectBounds& capsule) {
    const glm::vec3 origin { capsule.getPositionWorld() };
    const glm::vec3 capsuleAxis { glm::normalize(
        capsule.getOrientationWorld() * glm::vec3 { 0.f, 1.f, 0.f }
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
        glm::dot(axisNormalized, sphere.getPositionWorld())
        + sphere.mTrueVolume.mSphere.mRadius
    };
    const float pointTwo {
        glm::dot(axisNormalized, sphere.getPositionWorld())
        - sphere.mTrueVolume.mSphere.mRadius
    };

    if(pointOne > pointTwo) {
        return { pointTwo, pointOne };
    }
    return { pointOne, pointTwo };
}

std::pair<float, float> axisProjectCapsule(const glm::vec3& axis, const ObjectBounds& capsule) {
    const glm::vec3 capsulePosition { capsule.getPositionWorld() };
    const glm::mat3 capsuleRotation { glm::mat3_cast(capsule.getOrientationWorld()) };
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

std::pair<bool, glm::vec3> Simplex::evaluate() {
    switch(mNPoints) {
    case 2:
        return { false, Simplex::doSimplex2() };
    case 3:
        return { false, Simplex::doSimplex3() };
    case 4:
        return Simplex::doSimplex4();
    default:
        assert(false && "There is no reason whatsoever that a 3D simplex should have more than 4 or less than 2 points");
        return { false, glm::vec3{ std::numeric_limits<float>::infinity() } };
    }
}

glm::vec3 Simplex::doSimplex2() {
    assert(mNPoints == 2 && "This function should only be called when the simplex contains 2 points");

    const glm::vec3 pointA { mPoints[1] };
    const glm::vec3 pointB { mPoints[0] };

    // This constant shows up multiple times
    const glm::vec3 lineAB { pointB - pointA };

    // Note: we already know that A is beyond origin relative to B, so we can jump
    // straight to computing the perpendicular to AB

    // If B from A is the same direction as origin from A, go along
    // perpendicular to AB towards origin
    return glm::cross(glm::cross(lineAB, -pointA), lineAB);
}

glm::vec3 Simplex::doSimplex3() {
    assert(mNPoints == 3 && "This function should only be called when the simplex contains 3 points");

    const uint8_t indexA { 2 };
    const uint8_t indexB { 1 };
    const uint8_t indexC { 0 };

    const glm::vec3 pointA { mPoints[indexA] };
    const glm::vec3 pointB { mPoints[indexB] };
    const glm::vec3 pointC { mPoints[indexC] };

    // A few constants which show up on multiple conditions
    const glm::vec3 lineAC { pointC - pointA };
    const glm::vec3 lineAB { pointB - pointA };
    const glm::vec3 crossABAC { glm::cross(lineAB, lineAC) };

    // See if our origin is past the triangle beyond lineAC
    if(glm::dot(glm::cross(crossABAC, lineAC), -pointA) > 0) {
        if(glm::dot(lineAC, -pointA) > 0) {
            reorder({ indexC, indexA });
            return glm::cross(glm::cross(lineAC, -pointA), lineAC);

        // We now know we're closest to AB
        } else {
            reorder({ indexB, indexA });
            return glm::cross(glm::cross(lineAB, -pointA), lineAB);
        }

    // See if we're beyond the triangle beyond lineAB
    } else if(glm::dot(glm::cross(lineAB, crossABAC), -pointA) > 0) {
        // We now know we're closest to AB (and by the first if we've already ruled out AC)
        reorder({ indexB, indexA });
        return glm::cross(glm::cross(lineAB, -pointA), lineAB);

    // Test if origin is above the triangle
    } else if(glm::dot(crossABAC, -pointA) > 0) {
        return crossABAC;

    // We now know our origin is *below* the triangle
    } else {
        reorder({ indexB, indexC, indexA });
        return -crossABAC;
    }
}

std::pair<bool, glm::vec3> Simplex::doSimplex4() {
    assert(mNPoints == 4 && "This function should only be called when the simplex contains 4 points");

    const uint8_t indexA { 3 };
    const uint8_t indexB { 2 };
    const uint8_t indexC { 1 };
    const uint8_t indexD { 0 };

    const glm::vec3 pointA { mPoints[indexA] };
    const glm::vec3 pointB { mPoints[indexB] };
    const glm::vec3 pointC { mPoints[indexC] };
    const glm::vec3 pointD { mPoints[indexD] };

    // A few constants which show up on multiple conditions
    const glm::vec3 lineAD { pointD - pointA };
    const glm::vec3 lineAC { pointC - pointA };
    const glm::vec3 lineAB { pointB - pointA };
    // Each cross below is a normal pointing outward from the triangle that forms a
    // face of the simplex tetrahedron
    const glm::vec3 crossABAC { glm::cross(lineAB, lineAC) };
    const glm::vec3 crossACAD { glm::cross(lineAC, lineAD) };
    const glm::vec3 crossADAB { glm::cross(lineAD, lineAB) };
    // note: we don't care about crossBCBD -- we already know we're above it

    // see if we're beyond face ABC
    if(glm::dot(crossABAC, -pointA) > 0) {
        // Are we closest to the region near line AC
        if(glm::dot(glm::cross(crossABAC, lineAC), -pointA) > 0) {
            // continue the search near line AC
            reorder({ indexC, indexA });
            return { false, glm::cross(glm::cross(lineAC, -pointA), lineAC) };

        // Are we closest to the region near line AB?
        } else if(glm::dot(glm::cross(lineAB, crossABAC), -pointA) > 0) {
            reorder({ indexB, indexA });
            return { false, glm::cross(glm::cross(lineAB, -pointA), lineAB) };
        }

        // We are directly above the face ABC of the tetrahedron
        reorder({ indexC, indexB, indexA });
        return { false, crossABAC };

    // See if we're above the tetrahedron face ACD
    } else if(glm::dot(crossACAD, -pointA) > 0) {
        // Are we closest to line AD?
        if(glm::dot(glm::cross(crossACAD, lineAD), -pointA) > 0) {
            reorder({ indexD, indexA });
            return { false, glm::cross(glm::cross(lineAD, -pointA), lineAD) };

        // Are we closest to line AC?
        } else if(glm::dot(glm::cross(lineAC, crossACAD), -pointA) > 0) {
            reorder({ indexC, indexA });
            return { false, glm::cross(glm::cross(lineAC, -pointA), lineAC) };
        }

        // We are directly above face ACD of the tetrahedron
        reorder({ indexD, indexC, indexA });
        return { false, crossACAD };

    // Finally check if we're above face ADB
    } else if(glm::dot(crossADAB, -pointA) > 0) {
        // Are we closest to line AB?
        if(glm::dot(glm::cross(crossADAB, lineAB), -pointA) > 0) {
            reorder({ indexB, indexA });
            return { false, glm::cross(glm::cross(lineAB, -pointA), lineAB) };

        // ... or line AD?
        } else if(glm::dot(glm::cross(lineAD, crossADAB), -pointA) > 0) {
            reorder({ indexD, indexA });
            return { false, glm::cross(glm::cross(lineAD, -pointA), lineAD) };
        }

        // We are certainly directly above face ADB of the tetrahedron
        reorder({ indexB, indexD, indexA });
        return { false, crossADAB };
    }

    // We've exhausted every other option, we now _know_
    // that we're in the tetrahedron
    return { true, glm::vec3 { 0.f } };
}



#include "toymaker/engine/spatial_query/math.hpp"
#include "string_conversions.hpp"
#include <doctest/doctest.h>
#include <cmath>

namespace ToyMakerTests {

const float kFloatTolerance { 0.001f };

// Test some general characteristics about AABBs formed from spheres such
// as containment and correct sizing
void testAABBSphereEnclosure(const glm::vec3& position, const float inputRadius) {
    ToyMaker::ObjectBounds sphere {
        ToyMaker::ObjectBounds::create(
            ToyMaker::VolumeSphere {
                .mRadius { inputRadius },
            },
            position,
            glm::vec3 { 0.f }
        )
    };
    ToyMaker::AxisAlignedBounds enclosingAABB { sphere };
    const bool isSphereContained { ToyMaker::contains(sphere, enclosingAABB) };
    CHECK(isSphereContained);

    // Check whether the AABB just encloses the sphere
    const glm::vec3 aabbDimensions { enclosingAABB.getDimensions() };
    const float radius { sphere.mTrueVolume.mSphere.mRadius };
    CHECK(glm::abs(aabbDimensions.x - 2.f * radius) <= kFloatTolerance);
    CHECK(glm::abs(aabbDimensions.y - 2.f * radius) <= kFloatTolerance);
    CHECK(glm::abs(aabbDimensions.z - 2.f * radius) <= kFloatTolerance);   
}

// Test some general characteristics about AABBs formed from capsules such
// as containment and correct sizing
void testAABBCapsuleEnclosure(const glm::vec3& position, const glm::vec3& pitchYawRoll, const float inputHeight, const float inputRadius) {
    ToyMaker::ObjectBounds capsule {
        ToyMaker::ObjectBounds::create(
            ToyMaker::VolumeCapsule {
                .mHeight { inputHeight },
                .mRadius { inputRadius },
            },
            position,
            pitchYawRoll
        )
    };
    ToyMaker::AxisAlignedBounds enclosingAABB { capsule };
    const bool isCapsuleContained { ToyMaker::contains(capsule, enclosingAABB) };
    CHECK(isCapsuleContained);

    // Check whether the AABB just encloses the sphere
    const glm::vec3 aabbDimensions { enclosingAABB.getDimensions() };
    const std::pair<float, float> capsuleProjectedX { capsule.getProjectionAlong(glm::vec3{ 1.f, 0.f, 0.f }) };
    const std::pair<float, float> capsuleProjectedY { capsule.getProjectionAlong(glm::vec3{ 0.f, 1.f, 0.f }) };
    const std::pair<float, float> capsuleProjectedZ { capsule.getProjectionAlong(glm::vec3{ 0.f, 0.f, 1.f }) };
    CHECK(glm::abs(aabbDimensions.x - (capsuleProjectedX.second - capsuleProjectedX.first)) <= kFloatTolerance);
    CHECK(glm::abs(aabbDimensions.y - (capsuleProjectedY.second - capsuleProjectedY.first)) <= kFloatTolerance);
    CHECK(glm::abs(aabbDimensions.z - (capsuleProjectedZ.second - capsuleProjectedZ.first)) <= kFloatTolerance);
}

// Test some general characteristics about AABBs formed from object oriented boxes such
// as containment and correct sizing
void testAABBBoxEnclosure(const glm::vec3& position, const glm::vec3& pitchYawRoll, const glm::vec3 dimensions) {
    ToyMaker::ObjectBounds box {
        ToyMaker::ObjectBounds::create(
            ToyMaker::VolumeBox{
                .mDimensions { dimensions },
            },
            position,
            pitchYawRoll
        )
    };
    ToyMaker::AxisAlignedBounds enclosingAABB { box };
    const bool isBoxContained { ToyMaker::contains(box, enclosingAABB) };
    CHECK(isBoxContained);

    // Check whether the AABB just encloses the sphere
    const glm::vec3 aabbDimensions { enclosingAABB.getDimensions() };
    const std::pair<float, float> boxProjectedX { box.getProjectionAlong(glm::vec3{ 1.f, 0.f, 0.f }) };
    const std::pair<float, float> boxProjectedY { box.getProjectionAlong(glm::vec3{ 0.f, 1.f, 0.f }) };
    const std::pair<float, float> boxProjectedZ { box.getProjectionAlong(glm::vec3{ 0.f, 0.f, 1.f }) };
    CHECK(glm::abs(aabbDimensions.x - (boxProjectedX.second - boxProjectedX.first)) <= kFloatTolerance);
    CHECK(glm::abs(aabbDimensions.y - (boxProjectedY.second - boxProjectedY.first)) <= kFloatTolerance);
    CHECK(glm::abs(aabbDimensions.z - (boxProjectedZ.second - boxProjectedZ.first)) <= kFloatTolerance);
}



TEST_SUITE("Spatial Queries") {

TEST_CASE("AABB Object Bounds Encapsulation") {
    SUBCASE("Different spheres at different positions") {
        const std::vector<std::pair<glm::vec3, float>> positionRadiusPairs{{
            { glm::vec3{ 0.f }, 5.f },
            { glm::vec3{ 2.f }, 1.f },
            { glm::vec3{ -100.f, 20.f, 0.f }, 7.f },
            { glm::vec3{ 10000.f }, 100.f },
            { glm::vec3{ -100000.f, 30.f, 100.f }, 4.f },
            { glm::vec3{ 100000.f, 10000.f, 800.f }, .5f },
        }};
        for(const auto& element: positionRadiusPairs) {
            testAABBSphereEnclosure(element.first, element.second);
        }
    }
    SUBCASE("Different capsules at different positions") {
        const std::vector<std::tuple<glm::vec3, glm::vec3, float, float>> capsuleParams {{
            {
                glm::vec3 { 0.f },
                glm::vec3 { 0.f },
                5,
                2
            },
            {
                glm::vec3 { 10000.f },
                glm::vec3 {
                    glm::radians(90.f),
                    glm::radians(45.f),
                    glm::radians(12.f),
                },
                32,
                2
            },
            {
                glm::vec3 { 342.f, -32498, 43294 },
                glm::vec3 {
                    glm::radians(32.f),
                    glm::radians(98.f),
                    glm::radians(120.f),
                },
                4.3,
                3.2
            },
            {
                glm::vec3 { 22.f, 190, 0 },
                glm::vec3 {
                    glm::radians(83.f),
                    glm::radians(43.f),
                    glm::radians(20.f),
                },
                0,
                3.8
            },
            {
                glm::vec3 { 10000.f },
                glm::vec3 {
                    glm::radians(32.f),
                },
                0.5,
                0.2
            },
        }};
        for(const auto& element: capsuleParams) {
            testAABBCapsuleEnclosure(
                std::get<0>(element), // capsule position
                std::get<1>(element), // capsule pitch-yaw-roll
                std::get<2>(element), // capsule height
                std::get<3>(element)  // capsule radius
            );
        }
    }

    SUBCASE("Different object oriented boxes at different positions") {
        const std::vector<std::tuple<glm::vec3, glm::vec3, glm::vec3>> boxParams {{
            {
                glm::vec3{ 0.f },
                glm::vec3{ 0.f },
                glm::vec3{ 5.f }
            },
            {
                glm::vec3 { 10000.f },
                glm::vec3 {
                    glm::radians(120.f),
                    glm::radians(72.f),
                    glm::radians(100.f),
                },
                glm::vec3{ 1.f, 2.f, 3.f },
            },
            {
                glm::vec3 { 100.f, 0.f, 32.f },
                glm::vec3 {
                    glm::radians(43.f),
                    glm::radians(2.f),
                    glm::radians(20.f),
                },
                glm::vec3{ 1000.f, 200.f, 300.f },
            },
            {
                glm::vec3 { 74.f, -234.f, 2.f },
                glm::vec3 {
                    glm::radians(143.f),
                    glm::radians(52.f),
                    glm::radians(300.f),
                },
                glm::vec3{ 1000.f, 1000.f, 1000.f },
            },
            {
                glm::vec3 { -30000.f },
                glm::vec3 {
                    glm::radians(1423.f),
                    glm::radians(2423.f),
                    glm::radians(3324.f),
                },
                glm::vec3{ .4f, .3f, .2f },
            },
        }};
        for(const auto& element: boxParams) {
            testAABBBoxEnclosure(
                std::get<0>(element), // box position
                std::get<1>(element), // box pitch-yaw-roll
                std::get<2>(element)  // box dimensions
            );
        }
    }
}

}

}
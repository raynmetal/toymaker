#include <cmath>
#include <doctest/doctest.h>

#include "toymaker/engine/spatial_query/math.hpp"

#include "string_conversions.hpp"

namespace ToyMakerTests {

const float kFloatTolerance { 0.001f };

TEST_SUITE("Spatial Queries") {

TEST_CASE("Box-Box Overlap Detection") {
    // Start with two boxes positioned right next to each other
    ToyMaker::ObjectBounds boxOne { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeBox {
            .mDimensions { 5.f },
        },
        glm::vec3 { -2.5f, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };
    ToyMaker::ObjectBounds boxTwo { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeBox {
            .mDimensions { 5.f },
        },
        glm::vec3 { 2.5f, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };

    SUBCASE("Overlap fails when boxes only touch, but do not overlap") {
        const bool overlaps {
            ToyMaker::overlaps(boxOne, boxTwo)
        };
        CHECK(!overlaps);
    }

    SUBCASE("Minor rotation of either box causes overlap to succeed") {
        boxTwo.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(2.f) };
        const bool overlaps {
            ToyMaker::overlaps(boxOne, boxTwo)
        };
        CHECK(overlaps);
        const auto gjkResult { ToyMaker::gjkOverlaps(boxOne, boxTwo) };
        CHECK(gjkResult.second.mNPoints == 4);

        const ToyMaker::Polytope polytope { ToyMaker::buildPolytope(boxOne, boxTwo, gjkResult.second) };
        CHECK(polytope.getNumFaces() >= 4);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(boxOne, boxTwo) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(collision.mContactA.mPenetrationDepth <= .1f);
        CHECK(collision.mContactA.mPenetrationDepth > 0.f);
        CHECK(collision.mContactA.mPoint.y > 0.f); // right object clockwise on +Z, tops should touch
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);

    }

    SUBCASE("Either of the boxes being degenerate causes the overlap test to fail") {
        boxTwo.mPositionOffset = boxOne.mPositionOffset;
        boxTwo.mTrueVolume.mBox.mDimensions = glm::vec3 { 0.f };
        const bool overlaps { ToyMaker::overlaps(boxOne, boxTwo) };
        CHECK(!overlaps);
    }
}

TEST_CASE("Box-Sphere Overlap Detection") {
    // Start with two objects positioned right next to each other
    ToyMaker::ObjectBounds box { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeBox {
            .mDimensions { 5.f },
        },
        glm::vec3 { -2.5f, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };
    ToyMaker::ObjectBounds sphere { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeSphere {
            .mRadius { 2.5f }
        },
        glm::vec3 { 2.51f, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };

    SUBCASE("Overlap fails when objects only touch, but do not overlap") {
        const bool overlaps {
            ToyMaker::overlaps(box, sphere)
        };
        CHECK(!overlaps);
    }

    SUBCASE("Minor rotation of box causes overlap to succeed") {
        box.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(5.f) };
        const bool overlaps {
            ToyMaker::overlaps(box, sphere)
        };
        CHECK(overlaps);
        const auto gjkResult { ToyMaker::gjkOverlaps(box, sphere) };
        CHECK(gjkResult.second.mNPoints == 4);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(box, sphere) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(collision.mContactA.mPenetrationDepth <= .2f);
        CHECK(collision.mContactA.mPenetrationDepth > 0.f);
        CHECK(collision.mContactA.mPoint.y < 0.f); // left object clockwise on +Z, bottoms should touch
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);

    }

    SUBCASE("Either shape being degenerate causes overlap test to fail") {
        sphere.mPositionOffset = box.mPositionOffset;
        sphere.mTrueVolume.mSphere.mRadius = 0.f;
        const bool overlaps { ToyMaker::overlaps(box, sphere) };
        CHECK(!overlaps);
    }
}

TEST_CASE("Box-Capsule Overlap Detection") {
    // Start with two objects positioned right next to each other
    ToyMaker::ObjectBounds box { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeBox {
            .mDimensions { 5.f },
        },
        glm::vec3 { -2.5f, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };
    ToyMaker::ObjectBounds capsule { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeCapsule {
            .mHeight { 5.f },
            .mRadius { 2.5f },
        },
        glm::vec3 { 2.51f, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };

    SUBCASE("Overlap fails when objects only touch, but do not overlap") {
        const bool overlaps {
            ToyMaker::overlaps(box, capsule)
        };
        CHECK(!overlaps);
    }

    SUBCASE("Overlap succeeds when objects actually do intersect") {
        capsule.mPositionOffset = glm::vec3 { 2.f, 0.f, 0.f };
        const bool overlaps {
            ToyMaker::overlaps(box, capsule)
        };
        CHECK(overlaps);
        const auto gjkResult { ToyMaker::gjkOverlaps(box, capsule) };
        CHECK(gjkResult.second.mNPoints == 4);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(box, capsule) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(glm::abs(collision.mContactA.mPenetrationDepth - .5f) <= .1f);
        CHECK(collision.mContactA.mPenetrationDepth > 0.f);
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);

    }

    SUBCASE("Minor rotation of box causes overlap to succeed") {
        box.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(7.f) };
        const bool overlaps {
            ToyMaker::overlaps(box, capsule)
        };
        CHECK(overlaps);
        const auto gjkResult { ToyMaker::gjkOverlaps(box, capsule) };
        CHECK(gjkResult.second.mNPoints == 4);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(box, capsule) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(collision.mContactA.mPenetrationDepth <= .3f);
        CHECK(collision.mContactA.mPenetrationDepth > 0.f);
        CHECK(collision.mContactA.mPoint.y < 0.f); // left object clockwise on +Z, bottoms should touch
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);

    }

    SUBCASE("Minor rotation of capsule causes overlap to succeed") {
        capsule.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(5.f) };
        const bool overlaps {
            ToyMaker::overlaps(box, capsule)
        };
        CHECK(overlaps);
        const auto gjkResult { ToyMaker::gjkOverlaps(box, capsule) };
        CHECK(gjkResult.second.mNPoints == 4);
        CHECK(gjkResult.first == true);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(box, capsule) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(collision.mContactA.mPenetrationDepth <= .3f);
        CHECK(collision.mContactA.mPenetrationDepth > 0.f);
        CHECK(collision.mContactA.mPoint.y > 0.f); // right object clockwise on +Z, tops should touch
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);

    }

    SUBCASE("Either shape being degenerate causes overlap test to fail") {
        capsule.mPositionOffset = box.mPositionOffset;
        capsule.mTrueVolume.mCapsule.mRadius = 0.f;
        const bool overlaps { ToyMaker::overlaps(box, capsule) };
        CHECK(!overlaps);
    }

    SUBCASE("Capsule end overlaps predictably with box") {
        capsule.mOrientationOffset = glm::vec3{ 0.f, 0.f, glm::radians(90.f) };
        capsule.mPositionOffset = glm::vec3 { 
            .5f * capsule.mTrueVolume.mCapsule.mHeight + capsule.mTrueVolume.mCapsule.mRadius - 0.2f,
            0.f,
            0.f
        };

        const bool overlaps1 { ToyMaker::overlaps(box, capsule) };
        CHECK(overlaps1);
        auto gjkResult { ToyMaker::gjkOverlaps(box, capsule) };
        CHECK(gjkResult.second.mNPoints == 4);
        CHECK(gjkResult.first == true);

        capsule.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(-90.f) };
        const bool overlaps2 { ToyMaker::overlaps(box, capsule) };
        CHECK(overlaps2);
        gjkResult = ToyMaker::gjkOverlaps(box, capsule);
        CHECK(gjkResult.second.mNPoints == 4);
        CHECK(gjkResult.first == true);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(box, capsule) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(collision.mContactA.mPenetrationDepth <= .2f);
        CHECK(collision.mContactA.mPenetrationDepth > 0.f);
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);

    }
}

TEST_CASE("Capsule-Capsule Overlap Detection") {
    // Start with two objects positioned right next to each other
    ToyMaker::ObjectBounds capsuleOne { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeCapsule {
            .mHeight { 5.f },
            .mRadius { 2.5f }
        },
        glm::vec3 { -2.5f, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };
    ToyMaker::ObjectBounds capsuleTwo { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeCapsule {
            .mHeight { 5.f },
            .mRadius { 2.5f },
        },
        glm::vec3 { 2.51f, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };

    SUBCASE("Overlap fails when objects only touch, but do not overlap") {
        const bool overlaps {
            ToyMaker::overlaps(capsuleOne, capsuleTwo)
        };
        CHECK(!overlaps);
    }

    SUBCASE("Overlap succeeds when objects actually do intersect") {
        capsuleTwo.mPositionOffset = glm::vec3 { 2.f, 0.f, 0.f };
        const bool overlaps {
            ToyMaker::overlaps(capsuleOne, capsuleTwo)
        };
        CHECK(overlaps);
        const auto gjkResult { ToyMaker::gjkOverlaps(capsuleOne, capsuleTwo) };
        CHECK(gjkResult.second.mNPoints == 4);
        CHECK(gjkResult.first == true);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(capsuleOne, capsuleTwo) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(glm::abs(collision.mContactA.mPenetrationDepth - .5f) <= .1f);
        CHECK(collision.mContactA.mPenetrationDepth > 0.f);
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);
    }

    SUBCASE("Minor rotation of capsule one causes overlap to succeed") {
        capsuleOne.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(7.f) };
        const bool overlaps {
            ToyMaker::overlaps(capsuleOne, capsuleTwo)
        };
        CHECK(overlaps);
        const auto gjkResult { ToyMaker::gjkOverlaps(capsuleOne, capsuleTwo) };
        CHECK(gjkResult.second.mNPoints == 4);
        CHECK(gjkResult.first == true);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(capsuleOne, capsuleTwo) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(collision.mContactA.mPenetrationDepth <= .4f);
        CHECK(collision.mContactA.mPenetrationDepth > 0.f);
        CHECK(collision.mContactA.mPoint.y < 0.f); // left object clockwise on +Z, bottoms should touch
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);

    }

    SUBCASE("Minor rotation of capsule two causes overlap to succeed") {
        capsuleTwo.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(5.f) };
        const bool overlaps {
            ToyMaker::overlaps(capsuleOne, capsuleTwo)
        };
        CHECK(overlaps);
        const auto gjkResult { ToyMaker::gjkOverlaps(capsuleOne, capsuleTwo) };
        CHECK(gjkResult.second.mNPoints == 4);
        CHECK(gjkResult.first == true);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(capsuleOne, capsuleTwo) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(collision.mContactA.mPenetrationDepth <= .4f);
        CHECK(collision.mContactA.mPenetrationDepth > 0.f);
        CHECK(collision.mContactA.mPoint.y > 0.f); // right object clockwise on +Z, tops should touch
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);

    }

    SUBCASE("Either shape being degenerate causes overlap test to fail") {
        capsuleTwo.mPositionOffset = capsuleOne.mPositionOffset;
        capsuleTwo.mTrueVolume.mCapsule.mRadius = 0.f;
        const bool overlaps { ToyMaker::overlaps(capsuleOne, capsuleTwo) };
        CHECK(!overlaps);
    }

    SUBCASE("Perpendicular capsules intersect correctly") {
        capsuleTwo.mOrientationOffset = glm::vec3{ 0.f, 0.f, glm::radians(90.f) };
        capsuleTwo.mPositionOffset = glm::vec3 { 
            .5f * capsuleTwo.mTrueVolume.mCapsule.mHeight + capsuleTwo.mTrueVolume.mCapsule.mRadius - 0.2f,
            0.f,
            0.f
        };

        const bool overlaps1 { ToyMaker::overlaps(capsuleOne, capsuleTwo) };
        CHECK(overlaps1);
        const auto gjkResult1 { ToyMaker::gjkOverlaps(capsuleOne, capsuleTwo) };
        CHECK(gjkResult1.second.mNPoints == 4);
        CHECK(gjkResult1.first == true);

        capsuleTwo.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(-90.f) };
        const bool overlaps2 { ToyMaker::overlaps(capsuleOne, capsuleTwo) };
        CHECK(overlaps2);
        const auto gjkResult2 { ToyMaker::gjkOverlaps(capsuleOne, capsuleTwo) };
        CHECK(gjkResult2.second.mNPoints == 4);
        CHECK(gjkResult2.first == true);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(capsuleOne, capsuleTwo) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(glm::abs(collision.mContactA.mPenetrationDepth - .2f) <= .1f);
        CHECK(collision.mContactA.mPenetrationDepth > 0.f);
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);
    }
}

TEST_CASE("Capsule-Sphere Overlap Detection") {
    // Start with two objects positioned right next to each other
    ToyMaker::ObjectBounds capsule { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeCapsule {
            .mHeight { 5.f },
            .mRadius { 2.5f }
        },
        glm::vec3 { -2.5f, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };
    ToyMaker::ObjectBounds sphere { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeSphere {
            .mRadius { 2.5f },
        },
        glm::vec3 { 2.51f, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };

    SUBCASE("Overlap fails when objects only touch, but do not overlap") {
        const bool overlaps {
            ToyMaker::overlaps(capsule, sphere)
        };
        CHECK(!overlaps);
    }

    SUBCASE("Overlap succeeds when objects actually do intersect") {
        sphere.mPositionOffset = glm::vec3 { 2.f, 0.f, 0.f };
        const bool overlaps {
            ToyMaker::overlaps(capsule, sphere)
        };
        CHECK(overlaps);
        const auto gjkResult { ToyMaker::gjkOverlaps(capsule, sphere) };
        CHECK(gjkResult.second.mNPoints == 4);
        CHECK(gjkResult.first == true);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(capsule, sphere) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(glm::abs(collision.mContactA.mPenetrationDepth - .5f) <= .1f);
        CHECK(collision.mContactA.mPenetrationDepth > 0.f);
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);

    }

    SUBCASE("Minor rotation of capsule causes overlap to succeed") {
        capsule.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(7.f) };
        const bool overlaps {
            ToyMaker::overlaps(capsule, sphere)
        };
        CHECK(overlaps);
        const auto gjkResult { ToyMaker::gjkOverlaps(capsule, sphere) };
        CHECK(gjkResult.second.mNPoints == 4);
        CHECK(gjkResult.first == true);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(capsule, sphere) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(collision.mContactA.mPenetrationDepth <= .4f);
        CHECK(collision.mContactA.mPenetrationDepth > 0.f);
        CHECK(collision.mContactA.mPoint.y < 0.f); // left object clockwise on +Z, bottoms should touch
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);

    }

    SUBCASE("Either shape being degenerate causes overlap test to fail") {
        sphere.mPositionOffset = capsule.mPositionOffset;
        sphere.mTrueVolume.mSphere.mRadius = 0.f;
        const bool overlaps { ToyMaker::overlaps(capsule, sphere) };
        CHECK(!overlaps);
    }

    SUBCASE("Capsule ends intersect sphere correctly") {
        capsule.mOrientationOffset = glm::vec3{ 0.f, 0.f, glm::radians(90.f) };
        capsule.mPositionOffset = glm::vec3 {
            -(.5f * capsule.mTrueVolume.mCapsule.mHeight + capsule.mTrueVolume.mCapsule.mRadius - 0.2f),
            0.f,
            0.f
        };

        const bool overlaps1 { ToyMaker::overlaps(capsule, sphere) };
        CHECK(overlaps1);
        const auto gjkResult1 { ToyMaker::gjkOverlaps(capsule, sphere) };
        CHECK(gjkResult1.second.mNPoints == 4);
        CHECK(gjkResult1.first == true);

        capsule.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(-90.f) };
        const bool overlaps2 { ToyMaker::overlaps(capsule, sphere) };
        CHECK(overlaps2);
        const auto gjkResult2 { ToyMaker::gjkOverlaps(capsule, sphere) };
        CHECK(gjkResult2.second.mNPoints == 4);
        CHECK(gjkResult2.first == true);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(capsule, sphere) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(glm::abs(.2f - collision.mContactA.mPenetrationDepth) <= .07f);
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);

    }
}

TEST_CASE("Sphere-Sphere Overlap Detection") {
    // Start with two objects positioned right next to each other
    ToyMaker::ObjectBounds sphereOne { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeCapsule {
            .mRadius { 2.5f }
        },
        glm::vec3 { -2.5f, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };
    ToyMaker::ObjectBounds sphereTwo { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeSphere {
            .mRadius { 2.5f },
        },
        glm::vec3 { 2.51f, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };

    SUBCASE("Overlap fails when objects only touch, but do not overlap") {
        const bool overlaps {
            ToyMaker::overlaps(sphereOne, sphereTwo)
        };
        CHECK(!overlaps);
    }

    SUBCASE("Overlap succeeds when objects actually do intersect") {
        sphereTwo.mPositionOffset = glm::vec3 { 2.f, 0.f, 0.f };
        const bool overlaps {
            ToyMaker::overlaps(sphereOne, sphereTwo)
        };
        CHECK(overlaps);
        const auto gjkResult { ToyMaker::gjkOverlaps(sphereOne, sphereTwo) };
        CHECK(gjkResult.second.mNPoints == 4);
        CHECK(gjkResult.first == true);

        // Verify collision information
        const ToyMaker::Collision collision { ToyMaker::checkCollision(sphereOne, sphereTwo) };
        CHECK(collision.mCollided == true);
        CHECK(collision.mContactA.mPenetrationDepth == collision.mContactB.mPenetrationDepth);
        CHECK(glm::abs(collision.mContactA.mPenetrationDepth - .5f) <= 0.1f);
        CHECK(collision.mContactA.mNormal == -collision.mContactB.mNormal);
        CHECK(collision.mContactA.mTangent1 == -collision.mContactB.mTangent1);
        CHECK(collision.mContactA.mTangent2 == -collision.mContactB.mTangent2);

    }

    SUBCASE("Either shape being degenerate causes overlap test to fail") {
        sphereTwo.mPositionOffset = sphereOne.mPositionOffset;
        sphereTwo.mTrueVolume.mSphere.mRadius = 0.f;
        const bool overlaps { ToyMaker::overlaps(sphereOne, sphereTwo) };
        CHECK(!overlaps);
    }
}

}

}

#include "toymaker/engine/spatial_query_math.hpp"
#include "string_conversions.hpp"
#include <doctest/doctest.h>
#include <cmath>

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
    }

    SUBCASE("Minor rotation of box causes overlap to succeed") {
        box.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(7.f) };
        const bool overlaps {
            ToyMaker::overlaps(box, capsule)
        };
        CHECK(overlaps);
    }

    SUBCASE("Minor rotation of capsule causes overlap to succeed") {
        capsule.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(5.f) };
        const bool overlaps {
            ToyMaker::overlaps(box, capsule)
        };
        CHECK(overlaps);
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
        capsule.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(-90.f) };
        const bool overlaps2 { ToyMaker::overlaps(box, capsule) };
        CHECK(overlaps2);
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
    }

    SUBCASE("Minor rotation of capsule one causes overlap to succeed") {
        capsuleOne.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(7.f) };
        const bool overlaps {
            ToyMaker::overlaps(capsuleOne, capsuleTwo)
        };
        CHECK(overlaps);
    }

    SUBCASE("Minor rotation of capsule two causes overlap to succeed") {
        capsuleTwo.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(5.f) };
        const bool overlaps {
            ToyMaker::overlaps(capsuleOne, capsuleTwo)
        };
        CHECK(overlaps);
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
        capsuleTwo.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(-90.f) };
        const bool overlaps2 { ToyMaker::overlaps(capsuleOne, capsuleTwo) };
        CHECK(overlaps2);
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
    }

    SUBCASE("Minor rotation of capsule causes overlap to succeed") {
        capsule.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(7.f) };
        const bool overlaps {
            ToyMaker::overlaps(capsule, sphere)
        };
        CHECK(overlaps);
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
        capsule.mOrientationOffset = glm::vec3 { 0.f, 0.f, glm::radians(-90.f) };
        const bool overlaps2 { ToyMaker::overlaps(capsule, sphere) };
        CHECK(overlaps2);
    }
}
}

}

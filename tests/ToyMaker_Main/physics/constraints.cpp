#include <doctest/doctest.h>

#include "toymaker/engine/physics/types.hpp"
#include "toymaker/engine/spatial_query/math.hpp"

#include "string_conversions.hpp"

TEST_SUITE("Collision Constraints") {

TEST_CASE("Head-On") {
    ToyMaker::ObjectBounds object1 { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeSphere {
            .mRadius { 5.f },
        },
        glm::vec3 { -4.5, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };
    ToyMaker::ObjectBounds object2 { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeSphere {
            .mRadius { 5.f },
        },
        glm::vec3 { 4.5, 0.f, 0.f },
        glm::vec3 { 0.f }
    ) };
    ToyMaker::PhysicsLocal physics1 {
        .mForce { 0.f },
        .mTorque { 0.f },
        .mVelocity { 0.f },
        .mAngularVelocity { 0.f },
        .mRotationalInertia { 1.f },
        .mMass { 1.f }
    };
    ToyMaker::PhysicsLocal physics2 { physics1 };

    const std::unordered_map<
        ToyMaker::BaseConstraint::ParticipantID,
        std::pair<
            std::reference_wrapper<ToyMaker::ObjectBounds>,
            std::reference_wrapper<ToyMaker::PhysicsLocal>
        >
    > constraintTable {
        { 0, { object1, physics1 } },
        { 1, { object2, physics2 } },
    };

    SUBCASE("Equal Mass") {
        const bool overlaps { ToyMaker::overlaps(object1, object2) };
        const auto collision { ToyMaker::checkCollision(object1, object2) };
        REQUIRE(collision.mCollided);

        ToyMaker::CollisionConstraint collisionConstraint {
            collision,
            physics1,
            object1,
            physics2,
            object2
        };
        collisionConstraint.applyConstraint(constraintTable, 0.0001);

        CHECK(glm::abs(object1.getPositionWorld().x - (-5.f)) <= .001f);
        CHECK(glm::abs(object2.getPositionWorld().x - 5.f) <= .001f);
    }

    SUBCASE("Mass 1 > 2") {
        physics1.mMass = 2.f;
        const bool overlaps { ToyMaker::overlaps(object1, object2) };
        const auto collision { ToyMaker::checkCollision(object1, object2) };
        REQUIRE(collision.mCollided);

        ToyMaker::CollisionConstraint collisionConstraint {
            collision,
            physics1,
            object1,
            physics2,
            object2
        };
        collisionConstraint.applyConstraint(constraintTable, 0.0001);

        const float centerSeparation { glm::abs(object1.getPositionWorld().x - object2.getPositionWorld().x) };
        const auto collisionAfter { ToyMaker::checkCollision(object1, object2) };
        CHECK(glm::abs(centerSeparation - 10.f) <= .001f );
        CHECK(glm::abs(object1.getPositionWorld().x) < glm::abs(object2.getPositionWorld().x));
        CHECK(collisionAfter.mCollided == false);
    }

    SUBCASE("Mass 1 < 2") {
        physics2.mMass = 2.f;
        const bool overlaps { ToyMaker::overlaps(object1, object2) };
        const auto collision { ToyMaker::checkCollision(object1, object2) };
        REQUIRE(collision.mCollided);

        ToyMaker::CollisionConstraint collisionConstraint {
            collision,
            physics1,
            object1,
            physics2,
            object2
        };
        collisionConstraint.applyConstraint(constraintTable, 0.0001);

        const float centerSeparation { glm::abs(object1.getPositionWorld().x - object2.getPositionWorld().x) };
        const auto collisionAfter { ToyMaker::checkCollision(object1, object2) };
        CHECK(glm::abs(centerSeparation - 10.f) <= .001f );
        CHECK(glm::abs(object1.getPositionWorld().x) > glm::abs(object2.getPositionWorld().x));
        CHECK(collisionAfter.mCollided == false);
    }
}

TEST_CASE("Offset") {
    ToyMaker::ObjectBounds object1 { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeBox{
            .mDimensions { 10.f },
        },
        glm::vec3 { -4.5, 1.f, 0.f },
        glm::vec3 { 0.f }
    ) };
    ToyMaker::ObjectBounds object2 { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeBox {
            .mDimensions { 10.f },
        },
        glm::vec3 { 4.5, -1.f, 0.f },
        glm::vec3 { 0.f }
    ) };
    ToyMaker::PhysicsLocal physics1 {
        .mForce { 0.f },
        .mTorque { 0.f },
        .mVelocity { 0.f },
        .mAngularVelocity { 0.f },
        .mRotationalInertia { 1.f },
        .mMass { 1.f }
    };
    ToyMaker::PhysicsLocal physics2 { physics1 };

    const std::unordered_map<
        ToyMaker::BaseConstraint::ParticipantID,
        std::pair<
            std::reference_wrapper<ToyMaker::ObjectBounds>,
            std::reference_wrapper<ToyMaker::PhysicsLocal>
        >
    > constraintTable {
        { 0, { object1, physics1 } },
        { 1, { object2, physics2 } },
    };
}

}


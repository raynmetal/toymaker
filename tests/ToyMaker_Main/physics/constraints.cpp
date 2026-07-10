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
    ToyMaker::PhysicsState physics1 {
        .mForce { 0.f },
        .mTorque { 0.f },
        .mVelocity { 0.f },
        .mAngularVelocity { 0.f },
        .mRotationalInertiaInverse { 1.f },
        .mMassInverse { 1.f }
    };
    ToyMaker::PhysicsState physics2 { physics1 };

    const ToyMaker::BaseConstraint::ParticipantTable participantTable {
        { 0, { object1, physics1 } },
        { 1, { object2, physics2 } },
    };

    SUBCASE("Equal Mass") {
        const auto collision { ToyMaker::checkCollision(object1, object2) };
        REQUIRE(collision.mCollided);

        ToyMaker::CollisionConstraint collisionConstraint {};
        collisionConstraint.updateCollisionData(collision, physics1, object1, physics2, object2);
        collisionConstraint.applyConstraintPosition(participantTable, 0.0001);

        CHECK(glm::abs(object1.getPositionWorld().x - (-5.f)) <= .002f);
        CHECK(glm::abs(object2.getPositionWorld().x - 5.f) <= .002f);
    }

    SUBCASE("Mass 1 > 2") {
        physics1.setMass(2.f);
        const auto collision { ToyMaker::checkCollision(object1, object2) };
        REQUIRE(collision.mCollided);

        ToyMaker::CollisionConstraint collisionConstraint {};
        collisionConstraint.updateCollisionData(collision, physics1, object1, physics2, object2);
        collisionConstraint.applyConstraintPosition(participantTable, 0.0001);

        const float centerSeparation { glm::abs(object1.getPositionWorld().x - object2.getPositionWorld().x) };
        const auto collisionAfter { ToyMaker::checkCollision(object1, object2) };
        CHECK(glm::abs(centerSeparation - 10.f) <= .01f );
        CHECK(glm::abs(object1.getPositionWorld().x) < glm::abs(object2.getPositionWorld().x));
        CHECK(collisionAfter.mCollided == false);
    }

    SUBCASE("Mass 1 < 2") {
        physics2.setMass(2.f);
        const auto collision { ToyMaker::checkCollision(object1, object2) };
        REQUIRE(collision.mCollided);

        ToyMaker::CollisionConstraint collisionConstraint {};
        collisionConstraint.updateCollisionData(collision, physics1, object1, physics2, object2);
        collisionConstraint.applyConstraintPosition(participantTable, 0.0001);

        const float centerSeparation { glm::abs(object1.getPositionWorld().x - object2.getPositionWorld().x) };
        const auto collisionAfter { ToyMaker::checkCollision(object1, object2) };
        CHECK(glm::abs(centerSeparation - 10.f) <= .01f );
        CHECK(glm::abs(object1.getPositionWorld().x) > glm::abs(object2.getPositionWorld().x));
        CHECK(collisionAfter.mCollided == false);
    }
}

TEST_CASE("Offset") {
    ToyMaker::ObjectBounds object1 { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeBox{
            .mDimensions { 10.f },
        },
        glm::vec3 { -4.5, 4.5f, 0.f },
        glm::vec3 { 0.f }
    ) };
    ToyMaker::ObjectBounds object2 { ToyMaker::ObjectBounds::create(
        ToyMaker::VolumeBox {
            .mDimensions { 10.f },
        },
        glm::vec3 { 4.5, -4.5f, 0.f },
        glm::vec3 { 0.f }
    ) };
    ToyMaker::PhysicsState physics1 {
        .mForce { 0.f },
        .mTorque { 0.f },
        .mVelocity { 0.f },
        .mAngularVelocity { 0.f },
        .mRotationalInertiaInverse { 1.f },
        .mMassInverse { 1.f }
    };
    ToyMaker::PhysicsState physics2 { physics1 };
    const float originalCenterSeparation { glm::length(object1.getPositionWorld() - object2.getPositionWorld()) };

    const ToyMaker::BaseConstraint::ParticipantTable participantTable {
        { 0, { object1, physics1 } },
        { 1, { object2, physics2 } },
    };

    SUBCASE("Equal Inertia") {
        const glm::quat oldRotation1 { object1.getOrientationWorld() };
        const glm::quat oldRotation2 { object2.getOrientationWorld() };
        const auto collision { ToyMaker::checkCollision(object1, object2) };
        REQUIRE(collision.mCollided);

        ToyMaker::CollisionConstraint collisionConstraint {};
        collisionConstraint.updateCollisionData(collision, physics1, object1, physics2, object2);
        collisionConstraint.applyConstraintPosition(participantTable, 0.0001);

        // Check that both objects are no longer colliding, and are only barely touching
        const float centerSeparation { glm::length(object1.getPositionWorld() - object2.getPositionWorld()) };
        const auto collisionAfter { ToyMaker::checkCollision(object1, object2) };
        CHECK(glm::abs(centerSeparation - originalCenterSeparation) <= .5f);

        // Check that both objects rotated around (approximately) the same axis by about
        // (approximately) the same amount
        const glm::quat rotationDelta1 { object1.getOrientationWorld() * glm::inverse(oldRotation1) };
        const glm::quat rotationDelta2 { object2.getOrientationWorld() * glm::inverse(oldRotation2) };
        CHECK(glm::abs(glm::abs(rotationDelta1.w) - glm::abs(rotationDelta2.w)) <= .1f);
        CHECK(glm::abs(glm::abs(rotationDelta1.x) - glm::abs(rotationDelta2.x)) <= .1f);
        CHECK(glm::abs(glm::abs(rotationDelta1.y) - glm::abs(rotationDelta2.y)) <= .1f);
        CHECK(glm::abs(glm::abs(rotationDelta1.z) - glm::abs(rotationDelta2.z)) <= .1f);

        // Check that the rotations were both clockwise facing down the -z axis
        CHECK(rotationDelta1.z > 0.f);
        CHECK(rotationDelta2.z > 0.f);
    }

    SUBCASE("Inertia 1 > 2") {
        physics1.setRotationalInertia(glm::vec3{1.f, 1.f, 2.f});
        physics1.setMass(2.f);

        const glm::quat oldRotation1 { object1.getOrientationWorld() };
        const glm::quat oldRotation2 { object2.getOrientationWorld() };
        const auto collision { ToyMaker::checkCollision(object1, object2) };
        REQUIRE(collision.mCollided);

        ToyMaker::CollisionConstraint collisionConstraint {};
        collisionConstraint.updateCollisionData(collision, physics1, object1, physics2, object2);
        collisionConstraint.applyConstraintPosition(participantTable, 0.0001);

        // Check that both objects are no longer colliding, and are only barely touching
        const float centerSeparation { glm::length(object1.getPositionWorld() - object2.getPositionWorld()) };
        const auto collisionAfter { ToyMaker::checkCollision(object1, object2) };
        CHECK(glm::abs(centerSeparation - originalCenterSeparation) <= .5f);

        // Check that both objects rotated around (approximately) the same axis
        const glm::quat rotationDelta1 { object1.getOrientationWorld() * glm::inverse(oldRotation1) };
        const glm::quat rotationDelta2 { object2.getOrientationWorld() * glm::inverse(oldRotation2) };
        CHECK(glm::abs(glm::abs(rotationDelta1.x) - glm::abs(rotationDelta2.x)) <= .1f);
        CHECK(glm::abs(glm::abs(rotationDelta1.y) - glm::abs(rotationDelta2.y)) <= .1f);
        CHECK(glm::abs(glm::abs(rotationDelta1.z) - glm::abs(rotationDelta2.z)) <= .1f);

        // Check that the rotations were both clockwise
        CHECK(rotationDelta1.z > 0.f);
        CHECK(rotationDelta2.z > 0.f);

        // 2 should have experienced more rotation than 1
        CHECK(glm::abs(2.f*glm::acos(rotationDelta1.w)) < glm::abs(2.f*glm::acos(rotationDelta2.w)));
    }

    SUBCASE("Inertia 1 < 2") {
        physics2.setRotationalInertia({ 1.f, 1.f, 2.f });
        physics2.setMass(2.f);

        const glm::quat oldRotation1 { object1.getOrientationWorld() };
        const glm::quat oldRotation2 { object2.getOrientationWorld() };
        const auto collision { ToyMaker::checkCollision(object1, object2) };
        REQUIRE(collision.mCollided);

        ToyMaker::CollisionConstraint collisionConstraint {};
        collisionConstraint.updateCollisionData(collision, physics1, object1, physics2, object2);
        collisionConstraint.applyConstraintPosition(participantTable, 0.0001);

        // Check that both objects are no longer colliding, and are only barely touching
        const float centerSeparation { glm::length(object1.getPositionWorld() - object2.getPositionWorld()) };
        const auto collisionAfter { ToyMaker::checkCollision(object1, object2) };
        CHECK(glm::abs(centerSeparation - originalCenterSeparation) <= .5f);

        // Check that both objects rotated around (approximately) the same axis
        const glm::quat rotationDelta1 { object1.getOrientationWorld() * glm::inverse(oldRotation1) };
        const glm::quat rotationDelta2 { object2.getOrientationWorld() * glm::inverse(oldRotation2) };
        CHECK(glm::abs(glm::abs(rotationDelta1.x) - glm::abs(rotationDelta2.x)) <= .1f);
        CHECK(glm::abs(glm::abs(rotationDelta1.y) - glm::abs(rotationDelta2.y)) <= .1f);
        CHECK(glm::abs(glm::abs(rotationDelta1.z) - glm::abs(rotationDelta2.z)) <= .1f);

        // Check that the rotations were both clockwise facing down the -Z axis
        CHECK(rotationDelta1.z > 0.f);
        CHECK(rotationDelta2.z > 0.f);

        // 2 should have experienced more rotation than 1
        CHECK(glm::abs(2.f*glm::acos(rotationDelta1.w)) > glm::abs(2.f*glm::acos(rotationDelta2.w)));
    }
}

}


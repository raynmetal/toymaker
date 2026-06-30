#include <glm/gtc/quaternion.hpp>

#include "toymaker/engine/spatial_query/math.hpp"

#include "toymaker/engine/physics/types.hpp"

using namespace ToyMaker;


void PhysicsState::applyForceLocal(const glm::vec3& force, const glm::vec3& atPosition, const ObjectBounds& bounds) {
    const glm::vec3 position { bounds.getPositionWorld() };
    const glm::quat orientation { bounds.getOrientationWorld() };
    const glm::vec3 positionGlobal { position + orientation * atPosition };
    const glm::vec3 forceGlobal { orientation * force };
    applyForceGlobal(forceGlobal, positionGlobal, bounds);
}

void PhysicsState::applyForceGlobal(const glm::vec3& force, const glm::vec3& atPosition, const ObjectBounds& bounds) {
    // no force, nothing to do
    if(force == glm::vec3 { 0.f }) {
        return;
    }

    const glm::vec3 position { bounds.getPositionWorld() };
    const glm::vec3 forceOffset { atPosition - position };

    // calculate force being applied to the center of mass of the object
    const glm::vec3 toCenter { forceOffset != glm::vec3 { 0.f }?
        -glm::normalize(forceOffset) : glm::normalize(force)
    };
    const glm::vec3 centerForce { glm::dot(force, toCenter) * force };

    // calculate torque being applied tangentially
    const glm::vec3 axialTorque { forceOffset != glm::vec3 { 0.f }?
        glm::cross(forceOffset, force) : glm::vec3 { 0.f }
    };

    mForce += centerForce;
    mTorque += axialTorque;
}

void BaseConstraint::setCompliance(float newCompliance) {
    assert(newCompliance >= 0.f && "Compliance value must be non-negative");
    mCompliance = newCompliance;
}

float BaseConstraint::getCompliance() const {
    return mCompliance;
}

CollisionConstraint::CollisionConstraint(
    const Collision& collision,
    const PhysicsState& physicsA,
    const ObjectBounds& boundsA,
    const PhysicsState& physicsB,
    const ObjectBounds& boundsB
): Constraint<2> { 0.f } {
    updateCollisionData( collision,
        physicsA, boundsA,
        physicsB, boundsB
    );
}

void CollisionConstraint::updateCollisionData(
    const Collision& collision,
    const PhysicsState& physicsA,
    const ObjectBounds& boundsA,
    const PhysicsState& physicsB,
    const ObjectBounds& boundsB
) {
    // rotations from local to global frame for each body
    mCollided = collision.mCollided;
    mLastPointContactA = collision.mContactA.mPoint;
    mLastPointContactB = collision.mContactB.mPoint;
    mRelativePointContactA = (
        glm::inverse(boundsA.getOrientationWorld()) * (
            collision.mContactA.mPoint - boundsA.getPositionWorld()
        )
    );
    mRelativePointContactB = (
        glm::inverse(boundsB.getOrientationWorld()) * (
            collision.mContactB.mPoint - boundsB.getPositionWorld()
        )
    );
    mContactNormal = collision.mContactB.mNormal;

    // determine the speed at which the contact points were moving when the collision
    // took place
    const glm::vec3 pointVelocityA { physicsA.mVelocity + glm::cross(
        physicsA.mAngularVelocity,
        mLastPointContactA - boundsA.getPositionWorld()
    ) };
    const glm::vec3 pointVelocityB { physicsB.mVelocity + glm::cross(
        physicsB.mAngularVelocity,
        mLastPointContactB - boundsB.getPositionWorld()
    ) };
    const glm::vec3 pointVelocityAB {
        pointVelocityA - pointVelocityB
    };
    mCollisionVelocity = glm::dot(pointVelocityAB, mContactNormal);
}

void CollisionConstraint::applyConstraintPosition(
    const ParticipantTable& states,
    float substepSeconds
) {
    assert(states.size() == 2 && "Constraint accepts states belonging to exactly two participants");

    // fetch relevant data
    ObjectBounds& objectA { states.at(0).first.get() };
    ObjectBounds& objectB { states.at(1).first.get() };
    const PhysicsState& physicsA { states.at(0).second.get() };
    const PhysicsState& physicsB { states.at(1).second.get() };
    const Collision collision { checkCollision(objectA, objectB) };
    updateCollisionData(collision, physicsA, objectA, physicsB, objectB);

    // guard: no collision, so nothing to do
    if(!mCollided) {
        return;
    }

    const glm::vec3 positionA { objectA.getPositionWorld() };
    const glm::vec3 positionB { objectB.getPositionWorld() };

    // compute generalized inverse masses for A and B -- these will
    // be recomputed each substep, so there's no point in storing them
    const float generalizedInverseA { computeGeneralizedInverseMassPositional(
        objectA,
        physicsA.mMassInverse,
        physicsA.mRotationalInertiaInverse,
        collision.mContactA.mPoint,
        mContactNormal
    ) };
    const float generalizedInverseB { computeGeneralizedInverseMassPositional(
        objectB,
        physicsB.mMassInverse,
        physicsB.mRotationalInertiaInverse,
        collision.mContactB.mPoint,
        mContactNormal
    ) };

    // compute and update correction value
    const float alphaDerivative2 {
        getCompliance() / (substepSeconds * substepSeconds)
    };
    const float lagrangeCollision { getLagrange().at(0) };
    const float lagrangeDeltaCollision {
        -(
            collision.mContactA.mPenetrationDepth + alphaDerivative2 * lagrangeCollision
        ) / (
            generalizedInverseA + generalizedInverseB + alphaDerivative2
        )
    };
    assert(isNumber(lagrangeDeltaCollision) && "Lagrange delta calculation failed");
    applyLagrangeDelta(lagrangeDeltaCollision, 0);
    const glm::vec3 positionalImpulse {
        lagrangeDeltaCollision * mContactNormal
    };

    // cache old placement data
    const glm::quat orientationA { objectA.getOrientationWorld() };
    const glm::quat orientationB { objectB.getOrientationWorld() };

    // apply corrections
    objectA = impulseApplied(
        objectA,
        physicsA.mMassInverse,
        physicsA.mRotationalInertiaInverse,
        positionalImpulse,
        collision.mContactA.mPoint
    );
    objectB = impulseApplied(
        objectB,
        physicsB.mMassInverse,
        physicsB.mRotationalInertiaInverse,
        -positionalImpulse,
        collision.mContactB.mPoint
    );

    // retrieve new placement data
    const glm::vec3 positionANew { objectA.getPositionWorld() };
    const glm::vec3 positionBNew { objectB.getPositionWorld() };
    const glm::quat orientationANew { objectA.getOrientationWorld() };
    const glm::quat orientationBNew { objectB.getOrientationWorld() };

    // derive relative motion of point of contact
    const glm::vec3 pointContactANew { positionANew + orientationANew * mRelativePointContactA };
    const glm::vec3 pointContactBNew { positionBNew + orientationBNew * mRelativePointContactB };
    const glm::vec3 deltaA { pointContactANew - mLastPointContactA };
    const glm::vec3 deltaB { pointContactBNew - mLastPointContactB };
    const glm::vec3 deltaAB { deltaA - deltaB };
    const glm::vec3 deltaABTangent {
        deltaAB - glm::dot(deltaAB, mContactNormal) * mContactNormal
    };

    // determine whether a static friction correction need be applied
    // NOTE: collision and friction lagrange multipliers are directly proportional to forces in
    // the normal and tangential directions respectively, so we can use them quite conventiently
    // in this inequality
    const float combinedFrictionCoefficient {
        glm::min(physicsA.mCoefficientFrictionStatic, physicsB.mCoefficientFrictionStatic)
    };
    const float lagrangeFriction { getLagrange().at(1) };
    const float lagrangeDeltaFriction {
        -(
            glm::length(deltaABTangent) + alphaDerivative2 * lagrangeFriction
        ) / (
            generalizedInverseA + generalizedInverseB + alphaDerivative2
        )
    };
    if(glm::abs(lagrangeFriction + lagrangeDeltaFriction) >= glm::abs(combinedFrictionCoefficient * (lagrangeCollision + lagrangeDeltaCollision))) {
        return;
    }

    // apply static friction costraint
    assert(isNumber(lagrangeDeltaFriction) && "Lagrange delta calculation failed");
    applyLagrangeDelta(lagrangeDeltaFriction, 1);
    const glm::vec3 positionalImpulseFriction {
        lagrangeDeltaFriction * glm::normalize(deltaABTangent)
    };

    // apply corrections
    objectA = impulseApplied(
        objectA,
        physicsA.mMassInverse,
        physicsA.mRotationalInertiaInverse,
        positionalImpulseFriction,
        pointContactANew
    );
    objectB = impulseApplied(
        objectB,
        physicsB.mMassInverse,
        physicsB.mRotationalInertiaInverse,
        -positionalImpulseFriction,
        pointContactBNew
    );
}

void CollisionConstraint::applyConstraintVelocity(const ParticipantTable& states, float substepSeconds) {
    // guard: no collision, so nothing to do
    if(!mCollided) {
        return;
    }
    assert(states.size() == 2 && "Constraint accepts states belonging to exactly two participants");

    // fetch minimal data
    PhysicsState& physicsA { states.at(0).second.get() };
    PhysicsState& physicsB { states.at(1).second.get() };
    const ObjectBounds& objectA { states.at(0).first.get() };
    const ObjectBounds& objectB { states.at(1).first.get() };
    const glm::vec3 positionA { objectA.getPositionWorld() };
    const glm::vec3 positionB { objectB.getPositionWorld() };
    const glm::quat orientationA { objectA.getOrientationWorld() };
    const glm::quat orientationB { objectB.getOrientationWorld() };
    const glm::vec3 contactPositionA { positionA + orientationA * mRelativePointContactA };
    const glm::vec3 contactPositionB { positionB + orientationB * mRelativePointContactB };

    // compute generalized inverse masses for A and B -- these will
    // be recomputed each substep, so there's no point in storing them
    const float generalizedInverseA { computeGeneralizedInverseMassPositional(
        objectA,
        physicsA.mMassInverse,
        physicsA.mRotationalInertiaInverse,
        contactPositionA,
        mContactNormal
    ) };
    const float generalizedInverseB { computeGeneralizedInverseMassPositional(
        objectB,
        physicsB.mMassInverse,
        physicsB.mRotationalInertiaInverse,
        contactPositionB,
        mContactNormal
    ) };

    // Discover just how fast the contact points on each surface are moving
    // relative to each other
    const glm::vec3 contactVelocityA { physicsA.mVelocity + glm::cross(
        physicsA.mAngularVelocity,
        contactPositionA - positionA
    ) };
    const glm::vec3 contactVelocityB { physicsB.mVelocity + glm::cross(
        physicsB.mAngularVelocity,
        contactPositionB - positionB
    ) };
    const glm::vec3 contactVelocity { contactVelocityA - contactVelocityB };
    const float bounceVelocity { glm::dot(contactVelocity, mContactNormal) };

    // Apply static friction if required
    const float coefficientFrictionDynamic { glm::min(
        physicsA.mCoefficientFrictionDynamic,
        physicsB.mCoefficientFrictionDynamic
    ) };
    if(coefficientFrictionDynamic > 0.f) {
        const glm::vec3 tangentialVelocity {
            contactVelocity - bounceVelocity * mContactNormal
        };

        // derive the impulse required to fix our velocities
        const float lagrangeCollision { getLagrange().at(0) };
        const float forceNormal { lagrangeCollision / (substepSeconds * substepSeconds) };
        const glm::vec3 velocityCorrection {
            -glm::normalize(tangentialVelocity) * glm::min(
                glm::abs(substepSeconds * coefficientFrictionDynamic * forceNormal),
                glm::abs(glm::length(tangentialVelocity))
            )
        };
        const glm::vec3 impulseFriction {
            velocityCorrection / (generalizedInverseA + generalizedInverseB)
        };

        // apply the impulse
        physicsA = impulseApplied(
            objectA,
            physicsA,
            impulseFriction,
            contactPositionA
        );
        physicsB = impulseApplied(
            objectB,
            physicsB,
            -impulseFriction,
            contactPositionB
        );
    }
}

void PinConstraint::applyConstraintPosition(const ParticipantTable& states, float substepSeconds) {
    // fetch relevant data
    ObjectBounds& object { states.at(0).first.get() };
    const PhysicsState& physics { states.at(0).second.get() };
    const PinConstraintData constraint { getParameter(0) };
    const float alphaDerivative2 {
        getCompliance() / (substepSeconds * substepSeconds)
    };
    const glm::vec3 position { object.getPositionWorld() };
    const glm::quat orientation { object.getOrientationWorld() };

    // See if a positional correction is required, and apply it if it is
    const glm::vec3 correctionPositional { position - constraint.mOrigin };
    const float correctionPositionalSquared { squareDistance(correctionPositional) };
    if(correctionPositionalSquared) {
        const glm::vec3 correctionAxis { glm::normalize(correctionPositional) };
        const float generalizedInverseMass {
            computeGeneralizedInverseMassPositional(
                object,
                physics.mMassInverse,
                physics.mRotationalInertiaInverse,
                position,
                correctionAxis
            )
        };

        // discover and apply the required positional correction
        // compute and update correction value
        const float lagrangeDeltaPositional {
            -(
                std::sqrt(correctionPositionalSquared) + alphaDerivative2 * getLagrange().at(0)
            ) / (
                generalizedInverseMass + alphaDerivative2
            )
        };
        assert(isNumber(lagrangeDeltaPositional) && "Lagrange delta calculation failed");
        applyLagrangeDelta(lagrangeDeltaPositional, 0);
        const glm::vec3 impulsePositional { lagrangeDeltaPositional * correctionAxis };

        object = impulseApplied(
            object,
            physics.mMassInverse,
            physics.mRotationalInertiaInverse,
            impulsePositional,
            position
        );
    }

    // See if a rotational correction is required, then apply it if it is
    const glm::quat deltaOrientation {
        orientation * glm::inverse(constraint.mOrientation)
    };
    const glm::vec3 correctionRotational {
        2.f * glm::vec3{ deltaOrientation.x, deltaOrientation.y, deltaOrientation.z }
    };
    const float correctionRotationalSquared { squareDistance(correctionRotational) };
    if(correctionRotationalSquared) {
        const float generalizedInverseMass { computeGeneralizedInverseMassRotational(
            object, physics.mRotationalInertiaInverse, glm::normalize(correctionRotational)
        ) };

        // discover and apply the required rotational correction,  update correction value
        const float lagrangeDeltaRotational {
            -(
                std::sqrt(correctionRotationalSquared) + alphaDerivative2 * getLagrange().at(1)
            ) / (generalizedInverseMass + alphaDerivative2)
        };
        assert(isNumber(lagrangeDeltaRotational) && "Lagrange delta calculation failed");
        applyLagrangeDelta(lagrangeDeltaRotational, 1);
        const glm::vec3 impulseRotational {
            lagrangeDeltaRotational * glm::normalize(correctionRotational)
        };
        object = impulseApplied(
            object,
            physics.mRotationalInertiaInverse,
            impulseRotational
        );
    }
}

float ToyMaker::computeGeneralizedInverseMassPositional(
    const ObjectBounds& object,
    float inverseMass,
    const glm::vec3& inverseRotationalInertia,
    const glm::vec3& correctionPoint,
    const glm::vec3& correctionGradient
) {
    const glm::vec3 position { object.getPositionWorld() };
    const glm::vec3 correctionOffset { correctionPoint - position };
    const glm::vec3 correctionRotational { glm::cross(correctionOffset, correctionGradient) };
    const float generalizedInverseMass { inverseMass + (squareDistance(correctionRotational)?
        computeGeneralizedInverseMassRotational(
            object, inverseRotationalInertia, correctionRotational
        ) : 0.f)
    };
    return generalizedInverseMass;
}

float ToyMaker::computeGeneralizedInverseMassRotational(
    const ObjectBounds& object,
    const glm::vec3& inverseRotationalInertia,
    const glm::vec3& correction
) {
    const glm::quat orientation { object.getOrientationWorld() };
    const glm::vec3 correctionLocal { glm::inverse(orientation) * correction };
    return glm::dot(correctionLocal, inverseRotationalInertia * correctionLocal);
}

ObjectBounds ToyMaker::impulseApplied(
    ObjectBounds object,
    float massInverse,
    const glm::vec3& rotationalInertiaInverse,
    const glm::vec3& impulsePositional,
    const glm::vec3& impulsePoint
) {
    // apply positional corrections
    const glm::vec3 position { object.getPositionWorld() };
    const glm::vec3 positionNew { position + impulsePositional * massInverse };
    object.setPositionWorld(positionNew);

    // apply rotational corrections
    const glm::vec3 impulseRotation { glm::cross(
        impulsePoint - position, impulsePositional
    ) };
    object = impulseApplied(
        object, rotationalInertiaInverse, impulseRotation
    );
    return object;
}

PhysicsState ToyMaker::impulseApplied(
    const ObjectBounds& object,
    PhysicsState physics,
    const glm::vec3& impulsePositional,
    const glm::vec3& impulsePoint
) {
    const glm::vec3 deltaVelocity { impulsePositional * physics.mMassInverse };
    physics.mVelocity += deltaVelocity;
    const glm::vec3 impulseRotational { glm::cross(
        impulsePoint - object.getPositionWorld(),
        impulsePositional
    ) };
    physics = impulseApplied(
        object,
        physics,
        impulseRotational
    );
    return physics;
}

ObjectBounds ToyMaker::impulseApplied(
    ObjectBounds object,
    const glm::vec3& rotationalInertiaInverse,
    const glm::vec3& impulseRotational
) {
    const glm::quat orientation { object.getOrientationWorld() };
    const glm::vec3 impulseRotationalLocal { glm::inverse(orientation) * impulseRotational };
    const glm::vec3 deltaOrientation { orientation * (rotationalInertiaInverse * impulseRotationalLocal) };
    const glm::quat orientationNew { glm::normalize(
        orientation + .5f * glm::quat { 0.f, deltaOrientation.x, deltaOrientation.y, deltaOrientation.z } * orientation
    ) };
    object.setOrientationWorld(orientationNew);
    return object;
}

PhysicsState ToyMaker::impulseApplied(
    const ObjectBounds& object,
    PhysicsState physics,
    const glm::vec3& impulseRotational
) {
    const glm::quat orientation { object.getOrientationWorld() };
    const glm::vec3 impulseLocal { glm::inverse(orientation) * impulseRotational };
    const glm::vec3 deltaVelocityAngular { orientation * (physics.mRotationalInertiaInverse * impulseLocal) };
    physics.mAngularVelocity += deltaVelocityAngular;
    return physics;
}


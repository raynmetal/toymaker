
#include <glm/gtc/quaternion.hpp>

#include <toymaker/engine/physics/types.hpp>

using namespace ToyMaker;

float computeGeneralizedInverseMassPositional(
    float inverseMass,
    const glm::vec3& offsetWorld,
    const glm::mat3& rotationalInertia,
    const glm::vec3& contactDirection
);

glm::mat3 computeInertiaRotationalWorld(const glm::vec3& rotationalInertiaLocal, const glm::quat& orientation);

ObjectBounds impulseApplied(
    ObjectBounds object,
    float inertiaPositional,
    const glm::mat3& inertiaRotational,
    const glm::vec3& positionalImpulse,
    const glm::vec3& impulsePoint
);

void PhysicsLocal::applyForceLocal(const glm::vec3& force, const glm::vec3& atPosition) {
    // no force, nothing to do
    if(force == glm::vec3 { 0.f }) {
        return;
    }

    // calculate force being applied to the center of mass of the object
    const glm::vec3 centerForceDirection { atPosition != glm::vec3 { 0.f } ?
        -glm::normalize(atPosition) : glm::normalize(force)
    };
    const glm::vec3 centerForce { glm::dot(force, centerForceDirection) * centerForceDirection };

    // calculate torque being applied tangentially
    const glm::vec3 axialTorque { atPosition != glm::vec3 { 0.f }?
        glm::cross(atPosition, force) : glm::vec3 { 0.f }
    };

    mForce += centerForce;
    mTorque += axialTorque;
}

void PhysicsLocal::applyForceGlobal(const glm::vec3& force, const glm::vec3& atPosition, const ObjectBounds& bounds) {
    const glm::vec3 objectPosition { bounds.getPositionWorld() };
    const glm::quat objectRotationInverse { glm::inverse(bounds.getOrientationWorld()) };

    const glm::vec3 positionDelta { atPosition - objectPosition };

    const glm::vec3 positionLocal { objectRotationInverse * positionDelta };
    const glm::vec3 forceLocal { objectRotationInverse * force };

    applyForceLocal(forceLocal, positionLocal);
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
    const PhysicsLocal& physicsA,
    const ObjectBounds& boundsA,
    const PhysicsLocal& physicsB,
    const ObjectBounds& boundsB
): Constraint<CollisionConstraintData, 2> { 0.f } {

    updateCollisionData( collision,
        physicsA, boundsA,
        physicsB, boundsB
    );
}

void CollisionConstraint::updateCollisionData(
    const Collision& collision,
    const PhysicsLocal& physicsA,
    const ObjectBounds& boundsA,
    const PhysicsLocal& physicsB,
    const ObjectBounds& boundsB
) {
    // rotations from local to global frame for each body
    setParameter(0, {
        .mInverseMass { 1.f / physicsA.mMass },
        .mRotationalInertia { computeInertiaRotationalWorld(
            physicsA.mRotationalInertia, boundsA.getOrientationWorld()
        ) },
    });
    setParameter(1, {
        .mInverseMass { 1.f / physicsB.mMass },
        .mRotationalInertia { computeInertiaRotationalWorld(
            physicsA.mRotationalInertia, boundsA.getOrientationWorld()
        ) },
    });

    auto parameterA { getParameter(0) };
    auto parameterB { getParameter(1) };

    parameterA.mContact = collision.mContactA;
    parameterB.mContact = collision.mContactB;

    setParameter(0, parameterA);
    setParameter(1, parameterB);

    mCollided = collision.mCollided;
    mLastPointContactA = collision.mContactA.mPoint;
    mLastPointContactB = collision.mContactB.mPoint;
    mRelativePointContactA = glm::transpose(
        glm::mat3 { boundsA.getOrientationWorld() }
    ) * (
        collision.mContactA.mPoint - boundsA.getPositionWorld()
    );
    mRelativePointContactB = glm::transpose(
        glm::mat3 { boundsB.getOrientationWorld() }
    ) * (
        collision.mContactB.mPoint - boundsB.getPositionWorld()
    );
}

void CollisionConstraint::applyConstraint(
    const ParticipantTable& states,
    float substepSeconds
) {
    // guard: no collision, so nothing to do
    if(!mCollided) {
        return;
    }
    assert(states.size() == 2 && "Constraint accepts states belonging to exactly two participants");

    // fetch relevant data
    ObjectBounds& objectA { states.at(0).first.get() };
    ObjectBounds& objectB { states.at(1).first.get() };
    const PhysicsLocal& physicsA { states.at(0).second.get() };
    const PhysicsLocal& physicsB { states.at(0).second.get() };
    const CollisionConstraintData contactA { getParameter(0) };
    const CollisionConstraintData contactB { getParameter(1) };
    const glm::vec3& contactNormal { glm::normalize(contactB.mContact.mNormal) };

    const glm::vec3 positionA { objectA.getPositionWorld() };
    const glm::vec3 positionB { objectB.getPositionWorld() };

    // compute generalized inverse masses for A and B -- these will
    // be recomputed each substep, so there's no point in storing them
    const float generalizedInverseA {
        computeGeneralizedInverseMassPositional(
            contactA.mInverseMass,
            contactA.mContact.mPoint - positionA,
            contactA.mRotationalInertia,
            contactNormal
        )
    };
    const float generalizedInverseB {
        computeGeneralizedInverseMassPositional(
            contactB.mInverseMass,
            contactB.mContact.mPoint - positionB,
            contactB.mRotationalInertia,
            contactNormal
        )
    };

    // compute and update correction value
    const float alphaDerivative2 {
        getCompliance() / (substepSeconds * substepSeconds)
    };
    const float lagrangeDeltaCollision {
        -(
            contactA.mContact.mPenetrationDepth + alphaDerivative2 * getLagrange().at(0)
        ) / (
            generalizedInverseA + generalizedInverseB + alphaDerivative2
        )
    };
    assert(isNumber(lagrangeDeltaCollision) && "Lagrange delta calculation failed");
    applyLagrangeDelta(lagrangeDeltaCollision, 0);
    const glm::vec3 positionalImpulse {
        lagrangeDeltaCollision * contactNormal
    };

    // cache old placement data
    const glm::quat orientationA { objectA.getOrientationWorld() };
    const glm::quat orientationB { objectB.getOrientationWorld() };

    // apply corrections
    objectA = impulseApplied(
        objectA,
        contactA.mInverseMass,
        contactA.mRotationalInertia,
        positionalImpulse,
        contactA.mContact.mPoint
    );
    objectB = impulseApplied(
        objectB,
        contactB.mInverseMass,
        contactB.mRotationalInertia,
        -positionalImpulse,
        contactB.mContact.mPoint
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
        deltaAB - glm::dot(deltaAB, contactNormal) * contactNormal
    };

    // determine whether a static friction correction need be applied
    // NOTE: collision and friction lagrange multipliers are directly proportional to forces in
    // the normal and tangential directions respectively, so we can use them quite conventiently
    // in this inequality
    const float combinedFrictionCoefficient {
        glm::min(physicsA.mCoefficientFrictionStatic, physicsB.mCoefficientFrictionStatic)
    };
    const float lagrangeFriction { getLagrange().at(1) };
    const float lagrangeCollision { getLagrange().at(0) };
    if(glm::abs(lagrangeFriction) >= glm::abs(combinedFrictionCoefficient * lagrangeCollision)) {
        return;
    }

    // apply static friction costraint
    const float lagrangeDeltaFriction {
        -(
            glm::length(deltaABTangent) + alphaDerivative2 * lagrangeFriction
        ) / (
            generalizedInverseA + generalizedInverseB + alphaDerivative2
        )
    };
    assert(isNumber(lagrangeDeltaFriction) && "Lagrange delta calculation failed");
    applyLagrangeDelta(lagrangeDeltaFriction, 1);
    const glm::vec3 positionalImpulseFriction {
        lagrangeDeltaFriction * glm::normalize(deltaABTangent)
    };

    // apply corrections
    objectA = impulseApplied(
        objectA,
        contactA.mInverseMass,
        computeInertiaRotationalWorld(physicsA.mRotationalInertia, orientationANew),
        positionalImpulseFriction,
        pointContactANew
    );
    objectB = impulseApplied(
        objectB,
        contactB.mInverseMass,
        computeInertiaRotationalWorld(physicsA.mRotationalInertia, orientationANew),
        -positionalImpulseFriction,
        pointContactBNew
    );
}

float computeGeneralizedInverseMassPositional(
    float inverseMass,
    const glm::vec3& offset,
    const glm::mat3& rotationalInertia,
    const glm::vec3& contactDirection
) {
    const auto inertiaMatrixInverse {
        glm::inverse(rotationalInertia)
    };

    const auto crossOffsetContact {
        glm::cross(offset, contactDirection)
    };

    return inverseMass + glm::dot(
        crossOffsetContact,
        inertiaMatrixInverse * crossOffsetContact
    );
}

ObjectBounds impulseApplied(
    ObjectBounds object,
    float inertiaPositional,
    const glm::mat3& inertiaRotational,
    const glm::vec3& positionalImpulse,
    const glm::vec3& impulsePoint
) {
    // apply positional corrections
    const glm::vec3 position { object.getPositionWorld() };
    const glm::vec3 positionNew {
        position + positionalImpulse * inertiaPositional
    };
    object.setPositionWorld(positionNew);

    // apply rotational corrections
    const glm::vec3 impulseRotation {
        glm::inverse(inertiaRotational) * glm::cross(
            impulsePoint - position,
            positionalImpulse
        )
    };
    const glm::quat orientation { object.getOrientationWorld() };
    const glm::quat orientationNew {
        orientation
        + .5f * glm::quat{ impulseRotation } * orientation
    };
    object.setOrientationWorld(orientationNew);

    return object;
}

glm::mat3 computeInertiaRotationalWorld(const glm::vec3& rotationalInertiaLocal, const glm::quat& orientation) {
    const glm::mat3 matrixRotation { orientation };
    return { matrixRotation
        * glm::mat3 { glm::scale(glm::mat4 { 1.f }, rotationalInertiaLocal) }
        * glm::transpose(matrixRotation) // rotate vector to local frame
    };
}

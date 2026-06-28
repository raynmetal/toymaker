#include <glm/gtc/quaternion.hpp>

#include <toymaker/engine/physics/types.hpp>

using namespace ToyMaker;

/**
 * @brief Computes the generalized inverse mass used for positional corrections applied by
 * constraints.
 *
 * @param inverseMass 1 divided by the mass of the object.
 * @param offsetWorld Vector going from the center of mass of the object to the point
 * of application of force
 * @param rotationalInertia The rotational inertia of the object in the world frame.
 * @param correctionGradient The direction of the greatest increase in error with respect to
 * the desired object state
 *
 */
float computeGeneralizedInverseMassPositional(
    float inverseMass,
    const glm::vec3& offsetWorld,
    const glm::mat3& rotationalInertia,
    const glm::vec3& correctionGradient
);


/**
 * @brief Computes the generalized inverse mass used by constraints to apply strictly rotational
 * corrections
 *
 * @param inverseMass 1 divided by the mass of the object.
 * @param offsetWorld Vector going from the center of mass of the object to the point
 * of application of force
 * @param rotationalInertia The rotational inertia of the object in the world frame.
 * @param correctionGradient The direction of the greatest increase in error with respect to
 * the desired object state
 *
 */
float computeGeneralizedInverseMassRotational(
    const glm::mat3& rotationalInertia,
    const glm::vec3& correctionGradient
);



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
): Constraint<CollisionConstraintData, 2> { 0.f } {

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
    setParameter(0, {
        .mInverseMass { 1.f / physicsA.mMass },
        .mRotationalInertia { computeInertiaRotationalWorld(
            physicsA.mRotationalInertia, boundsA.getOrientationWorld()
        ) },
    });
    setParameter(1, {
        .mInverseMass { 1.f / physicsB.mMass },
        .mRotationalInertia { computeInertiaRotationalWorld(
            physicsB.mRotationalInertia, boundsB.getOrientationWorld()
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
}

void CollisionConstraint::applyConstraintPosition(
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
    const PhysicsState& physicsA { states.at(0).second.get() };
    const PhysicsState& physicsB { states.at(1).second.get() };
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
    const float lagrangeCollision { getLagrange().at(0) };
    const float lagrangeDeltaCollision {
        -(
            contactA.mContact.mPenetrationDepth + alphaDerivative2 * lagrangeCollision
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
        physicsA.mMass,
        contactA.mRotationalInertia,
        positionalImpulse,
        contactA.mContact.mPoint
    );
    objectB = impulseApplied(
        objectB,
        physicsB.mMass,
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
        physicsA.mMass,
        computeInertiaRotationalWorld(physicsA.mRotationalInertia, orientationANew),
        positionalImpulseFriction,
        pointContactANew
    );
    objectB = impulseApplied(
        objectB,
        physicsB.mMass,
        computeInertiaRotationalWorld(physicsB.mRotationalInertia, orientationBNew),
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
    const float coefficientFrictionDynamic { glm::min(
        physicsA.mCoefficientFrictionDynamic,
        physicsB.mCoefficientFrictionDynamic
    ) };

    // No friction, no problem, so exit early
    if(coefficientFrictionDynamic <= 0.f) {
        return;
    }

    // fetch other relevant data
    const ObjectBounds& objectA { states.at(0).first.get() };
    const ObjectBounds& objectB { states.at(1).first.get() };
    const CollisionConstraintData contactA { getParameter(0) };
    const CollisionConstraintData contactB { getParameter(1) };
    const glm::vec3& contactNormal { glm::normalize(contactB.mContact.mNormal) };
    const glm::vec3 positionA { objectA.getPositionWorld() };
    const glm::vec3 positionB { objectB.getPositionWorld() };
    const glm::quat orientationA { objectA.getOrientationWorld() };
    const glm::quat orientationB { objectB.getOrientationWorld() };

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

    // Discover just how fast the contact points on each surface are moving
    // relative to each other
    const glm::vec3 contactVelocityA {
        physicsA.mVelocity
        + glm::cross(
            physicsA.mAngularVelocity,
            contactA.mContact.mPoint - positionA
        )
    };
    const glm::vec3 contactVelocityB {
        physicsB.mVelocity
        + glm::cross(
            physicsB.mAngularVelocity,
            contactB.mContact.mPoint - positionB
        )
    };
    const glm::vec3 contactVelocity { contactVelocityA - contactVelocityB };
    const glm::vec3 tangentialVelocity {
        contactVelocity - glm::dot(contactVelocity, contactNormal) * contactNormal
    };

    // derive the impulse required to fix our velocities
    const float lagrangeCollision { getLagrange().at(0) };
    const float forceNormal {
        lagrangeCollision / (substepSeconds * substepSeconds)
    };
    const glm::vec3 velocityCorrection {
        -glm::normalize(tangentialVelocity) * glm::min(
            glm::abs(substepSeconds * coefficientFrictionDynamic * forceNormal),
            glm::abs(glm::length(tangentialVelocity))
        )
    };
    const glm::vec3 impulsePositional {
        velocityCorrection / (
            generalizedInverseA + generalizedInverseB
        )
    };

    // apply the impulse
    physicsA = impulseApplied(
        objectA,
        physicsA,
        impulsePositional,
        contactA.mContact.mPoint
    );
    physicsB = impulseApplied(
        objectB,
        physicsB,
        -impulsePositional,
        contactB.mContact.mPoint
    );
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
    const glm::mat3 rotationalInertiaWorld {
        computeInertiaRotationalWorld(physics.mRotationalInertia, orientation)
    };

    // See if a positional correction is required, and apply it if it is
    const glm::vec3 correctionPositional { position - constraint.mOrigin };
    const float correctionPositionalSquared { squareDistance(correctionPositional) };
    if(correctionPositionalSquared) {
        const glm::vec3 correctionAxis { glm::normalize(correctionPositional) };
        const float generalizedInverseMass {
            computeGeneralizedInverseMassPositional(
                1.f / physics.mMass,
                glm::vec3 { 0.f },
                rotationalInertiaWorld,
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
        const glm::vec3 impulsePositional {
            lagrangeDeltaPositional * correctionAxis
        };

        object = impulseApplied(
            object,
            physics.mMass,
            rotationalInertiaWorld,
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
                rotationalInertiaWorld, glm::normalize(correctionRotational)
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
            rotationalInertiaWorld,
            impulseRotational
        );
    }
}

float computeGeneralizedInverseMassPositional(
    float inverseMass,
    const glm::vec3& offset,
    const glm::mat3& rotationalInertia,
    const glm::vec3& correctionGradient
) {
    const auto inertiaMatrixInverse {
        glm::inverse(rotationalInertia)
    };

    const auto crossOffsetContact {
        glm::cross(offset, correctionGradient)
    };

    return inverseMass + glm::dot(
        crossOffsetContact,
        inertiaMatrixInverse * crossOffsetContact
    );
}

float computeGeneralizedInverseMassRotational(
    const glm::mat3& rotationalInertia,
    const glm::vec3& correctionGradient
) {
    return glm::dot(
        correctionGradient,
        glm::inverse(rotationalInertia) * correctionGradient
    );
}

ObjectBounds ToyMaker::impulseApplied(
    ObjectBounds object,
    float inertiaPositional,
    const glm::mat3& inertiaRotational,
    const glm::vec3& positionalImpulse,
    const glm::vec3& impulsePoint
) {
    // apply positional corrections
    const glm::vec3 position { object.getPositionWorld() };
    const glm::vec3 positionNew {
        position + positionalImpulse * (1.f / inertiaPositional)
    };
    object.setPositionWorld(positionNew);

    // apply rotational corrections
    const glm::vec3 impulseRotation { glm::cross(
        impulsePoint - position,
        positionalImpulse
    ) };
    object = impulseApplied(
        object,
        inertiaRotational,
        impulseRotation
    );
    return object;
}

PhysicsState ToyMaker::impulseApplied(
    const ObjectBounds& object,
    PhysicsState physics,
    const glm::vec3& impulsePositional,
    const glm::vec3& impulsePoint
) {
    const glm::vec3 deltaVelocity {
        impulsePositional / physics.mMass
    };
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
    const glm::mat3& inertiaRotational,
    const glm::vec3& impulseRotational
) {
    const glm::quat orientation { object.getOrientationWorld() };
    const glm::quat orientationNew { glm::normalize(
        orientation + (
            .5f * glm::quat {
                glm::inverse(inertiaRotational)
                * impulseRotational
            }
            * orientation
        )
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
    const glm::mat3 inertiaRotational {
        computeInertiaRotationalWorld(
            physics.mRotationalInertia,
            orientation
        )
    };
    const glm::vec3 deltaVelocityAngular {
        glm::inverse(inertiaRotational)
        * impulseRotational
    };
    physics.mAngularVelocity += deltaVelocityAngular;
    return physics;
}

glm::mat3 ToyMaker::computeInertiaRotationalWorld(const glm::vec3& rotationalInertiaLocal, const glm::quat& orientation) {
    const glm::mat3 matrixRotation { orientation };
    return { matrixRotation
        * glm::mat3 { glm::scale(glm::mat4 { 1.f }, rotationalInertiaLocal) }
        * glm::transpose(matrixRotation) // rotate vector to local frame
    };
}

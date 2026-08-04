#include <glm/gtc/quaternion.hpp>

#include "toymaker/engine/spatial_query/math.hpp"
#include "toymaker/engine/physics/types.hpp"

using namespace ToyMaker;

const float kPersistentThresholdSquared { 1.5e-6 };

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
    const glm::vec3 centerForce { glm::dot(force, toCenter) * glm::normalize(force) };

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

ConstraintContact::ConstraintContact(): Constraint<2> { 0.f } {}

void ConstraintContact::updateCollisionData(
    const Collision& collision,
    const PhysicsState& physicsA,
    const ObjectBounds& boundsA,
    const ObjectBounds& boundsAPrev,
    const PhysicsState& physicsB,
    const ObjectBounds& boundsB,
    const ObjectBounds& boundsBPrev
) {
    // capture the current state, both relative and absolute, and projected previous state, of our
    // contact points
    mCurrentA = collision.mContactA.mPoint;
    mCurrentB = collision.mContactB.mPoint;
    mRelativeA = (
        glm::inverse(boundsA.getOrientationWorld()) * (
            collision.mContactA.mPoint - boundsA.getPositionWorld()
        )
    );
    mRelativeB = (
        glm::inverse(boundsB.getOrientationWorld()) * (
            collision.mContactB.mPoint - boundsB.getPositionWorld()
        )
    );
    mPreviousA = (
        boundsAPrev.getPositionWorld()
        + boundsAPrev.getOrientationWorld() * mRelativeA
    );
    mPreviousB = (
        boundsBPrev.getPositionWorld()
        + boundsBPrev.getOrientationWorld() * mRelativeB
    );

    // store terms common to both participants
    mContactNormal = collision.mContactB.mNormal;
    mPenetration = collision.mContactB.mPenetrationDepth;

    // determine the speed at which the contact points were moving when the collision
    // took place
    const glm::vec3 pointVelocityA { physicsA.mVelocity + glm::cross(
        physicsA.mAngularVelocity,
        mCurrentA - boundsA.getPositionWorld()
    ) };
    const glm::vec3 pointVelocityB { physicsB.mVelocity + glm::cross(
        physicsB.mAngularVelocity,
        mCurrentB - boundsB.getPositionWorld()
    ) };
    const glm::vec3 pointVelocityAB {
        pointVelocityA - pointVelocityB
    };
    mCollisionVelocity = glm::dot(pointVelocityAB, mContactNormal);
}

void ConstraintContact::applyConstraintPosition(
    const ParticipantTable& states,
    float substepSeconds
) {
    assert(states.size() == 2 && "Constraint accepts states belonging to exactly two participants");
    const double oneBySubstep { 1.f / substepSeconds };

    // fetch physics state
    const PhysicsState& physicsA { states.at(0).second.get() };
    const PhysicsState& physicsB { states.at(1).second.get() };

    // guards:
    if(
        // at least one object must be dynamic for the collision solver to work
        (
            physicsA.getMode() != PhysicsState::MODE_DYNAMIC
            && physicsB.getMode() != PhysicsState::MODE_DYNAMIC
        // both objects must be configured to separate on collision
        ) || !(
            physicsA.separatesOnCollision()
            && physicsB.separatesOnCollision()
        )
    ) {
        return;
    }

    // compute current contact positions for A and B and determine whether to proceed with the
    // rest of the position correction
    ObjectBounds& objectA { states.at(0).first.get() };
    ObjectBounds& objectB { states.at(1).first.get() };
    const glm::vec3 positionA { objectA.getPositionWorld() };
    const glm::vec3 positionB { objectB.getPositionWorld() };
    const glm::quat orientationA { objectA.getOrientationWorld() };
    const glm::quat orientationB { objectB.getOrientationWorld() };
    const glm::vec3 currentA { positionA + orientationA * mRelativeA };
    const glm::vec3 currentB { positionB + orientationB * mRelativeB };

    // guard: contact points are currently separated, no correction needed
    const float currentPenetration { glm::dot(currentA - currentB, mContactNormal) };
    if(currentPenetration < 0.f) {
        return;
    }

    // compute generalized inverse masses for A and B -- these will
    // be recomputed each substep, so there's no point in storing them
    const float generalizedInverseA { computeGeneralizedInverseMassPositional(
        objectA,
        physicsA,
        currentA,
        mContactNormal
    ) };
    const float generalizedInverseB { computeGeneralizedInverseMassPositional(
        objectB,
        physicsB,
        currentB,
        mContactNormal
    ) };

    // compute and update correction value
    const float alphaDerivative2 {
        static_cast<float>(getCompliance() * oneBySubstep * oneBySubstep)
    };
    const float lagrangeCollision { getLagrange().at(0) };
    const float lagrangeDeltaCollision {
        -(
            mPenetration + alphaDerivative2 * lagrangeCollision
        ) / (
            generalizedInverseA + generalizedInverseB + alphaDerivative2
        )
    };
    assert(isNumber(lagrangeDeltaCollision) && "Lagrange delta calculation failed");
    applyLagrangeDelta(lagrangeDeltaCollision, 0);
    const glm::vec3 positionalImpulse {
        lagrangeDeltaCollision * mContactNormal
    };

    // apply corrections
    objectA = applyImpulseObject(
        objectA,
        physicsA,
        positionalImpulse,
        currentA
    );
    objectB = applyImpulseObject(
        objectB,
        physicsB,
        -positionalImpulse,
        currentB
    );

    // retrieve new placement data
    const glm::vec3 positionANew { objectA.getPositionWorld() };
    const glm::vec3 positionBNew { objectB.getPositionWorld() };
    const glm::quat orientationANew { objectA.getOrientationWorld() };
    const glm::quat orientationBNew { objectB.getOrientationWorld() };

    // derive relative motion of point of contact
    const glm::vec3 pointContactANew { positionANew + orientationANew * mRelativeA };
    const glm::vec3 pointContactBNew { positionBNew + orientationBNew * mRelativeB };
    const float separationAB { glm::dot(pointContactANew - pointContactBNew, mContactNormal) };

    const glm::vec3 deltaA { pointContactANew - mPreviousA };
    const glm::vec3 deltaB { pointContactBNew - mPreviousB };
    const glm::vec3 deltaAB { deltaA - deltaB };
    const glm::vec3 deltaABTangent {
        deltaAB - glm::dot(deltaAB, mContactNormal) * mContactNormal
    };

    // determine whether a static friction correction need be applied
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

    // NOTE: collision and friction lagrange multipliers are directly proportional to forces in
    // the normal and tangential directions respectively, so we can use them quite conventiently
    // in this inequality
    // guard: friction correction required, and friction force must be less than static
    // friction threshold per normal force
    if(
        !lagrangeDeltaFriction || (
            glm::abs(lagrangeFriction + lagrangeDeltaFriction)
            >= glm::abs(combinedFrictionCoefficient * (lagrangeCollision + lagrangeDeltaCollision))
        )
    ) {
        return;
    }

    // apply static friction costraint
    assert(isNumber(lagrangeDeltaFriction) && "Lagrange delta calculation failed");
    applyLagrangeDelta(lagrangeDeltaFriction, 1);
    const glm::vec3 positionalImpulseFriction {
        lagrangeDeltaFriction * glm::normalize(deltaABTangent)
    };

    // apply corrections
    objectA = applyImpulseObject(
        objectA,
        physicsA,
        positionalImpulseFriction,
        pointContactANew
    );
    objectB = applyImpulseObject(
        objectB,
        physicsB,
        -positionalImpulseFriction,
        pointContactBNew
    );
}

void ConstraintContact::applyConstraintVelocity(const ParticipantTable& states, float substepSeconds) {
    assert(states.size() == 2 && "Constraint accepts states belonging to exactly two participants");
    PhysicsState& physicsA { states.at(0).second.get() };
    PhysicsState& physicsB { states.at(1).second.get() };

    // guards:
    if(
        // at least one object must be dynamic for the collision solver to work
        (
            physicsA.getMode() != PhysicsState::MODE_DYNAMIC
            && physicsB.getMode() != PhysicsState::MODE_DYNAMIC
        // both objects must be configured to separate on collision
        ) || !(
            physicsA.separatesOnCollision()
            && physicsB.separatesOnCollision()
        )
    ) {
        return;
    }

    const double oneBySubstep { 1.f / substepSeconds };

    // cache position related stuff
    const ObjectBounds& objectA { states.at(0).first.get() };
    const ObjectBounds& objectB { states.at(1).first.get() };
    const glm::vec3 positionA { objectA.getPositionWorld() };
    const glm::vec3 positionB { objectB.getPositionWorld() };
    const glm::quat orientationA { objectA.getOrientationWorld() };
    const glm::quat orientationB { objectB.getOrientationWorld() };
    const glm::vec3 contactPositionA { positionA + orientationA * mRelativeA };
    const glm::vec3 contactPositionB { positionB + orientationB * mRelativeB };

    // cache physics related stuff
    const float generalizedInverseA { computeGeneralizedInverseMassPositional(
        objectA,
        physicsA,
        contactPositionA,
        mContactNormal
    ) };
    const float generalizedInverseB { computeGeneralizedInverseMassPositional(
        objectB,
        physicsB,
        contactPositionB,
        mContactNormal
    ) };

    // Discover just how fast the contact points on each surface are moving
    // relative to each other
    const glm::vec3 pointVelocityA { physicsA.mVelocity + glm::cross(
        physicsA.mAngularVelocity,
        contactPositionA - positionA
    ) };
    const glm::vec3 pointVelocityB { physicsB.mVelocity + glm::cross(
        physicsB.mAngularVelocity,
        contactPositionB - positionB
    ) };
    const glm::vec3 pointVelocityAB { pointVelocityA - pointVelocityB };
    const float bounceVelocity { glm::dot(pointVelocityAB, mContactNormal) };
    const float cutoffVelocity { physicsA.mVelocityCutoff + physicsB.mVelocityCutoff };

    // derive the current coefficient of restitution between this pair of objects, set
    // to 0 when small separation velocity detected
    const float coefficientRestitution { (glm::abs(bounceVelocity) <= cutoffVelocity)?
        0.f :
        glm::max(physicsA.mCoefficientRestitution, physicsB.mCoefficientRestitution)
    };
    const float bounceVelocityLimit { glm::min(-coefficientRestitution * mCollisionVelocity, 0.f) };
    const float bounceVelocityCorrection { glm::max(bounceVelocityLimit, bounceVelocity) - bounceVelocity };

    // attempt correction only when a bounce has taken place
    if(bounceVelocity < 0.f && mCollisionVelocity > 0.f && bounceVelocityCorrection) {
        const glm::vec3 correctionRestitution {
            mContactNormal * bounceVelocityCorrection
        };
        const double oneByInverseMasses {
            1.f / (generalizedInverseA + generalizedInverseB)
        };
        const glm::vec3 impulseRestitution {
            correctionRestitution / (generalizedInverseA + generalizedInverseB)
        };

        // apply the impulse
        physicsA = applyImpulsePhysics(
            objectA,
            physicsA,
            impulseRestitution,
            contactPositionA
        );
        physicsB = applyImpulsePhysics(
            objectB,
            physicsB,
            -impulseRestitution,
            contactPositionB
        );
    }

    // apply dynamic friction if required
    const float coefficientFrictionDynamic { glm::min(
        physicsA.mCoefficientFrictionDynamic,
        physicsB.mCoefficientFrictionDynamic
    ) };
    const glm::vec3 tangentialVelocity {
        pointVelocityAB - bounceVelocity * mContactNormal
    };
    if(coefficientFrictionDynamic > 0.f && squareDistance(tangentialVelocity)) {

        // derive the impulse required to fix our velocities
        const float lagrangeCollision { getLagrange().at(0) };
        const float forceNormal { static_cast<float>(lagrangeCollision * oneBySubstep * oneBySubstep) };
        const glm::vec3 velocityCorrection {
            -glm::normalize(tangentialVelocity) * glm::min(
                glm::abs(substepSeconds * coefficientFrictionDynamic * forceNormal),
                glm::length(tangentialVelocity)
            )
        };
        const double oneByInverseMassTotal { 1.f / (generalizedInverseA + generalizedInverseB) };
        const glm::vec3 impulseFriction {
            static_cast<glm::dvec3>(velocityCorrection) * oneByInverseMassTotal
        };

        // apply the impulse
        physicsA = applyImpulsePhysics(
            objectA,
            physicsA,
            impulseFriction,
            contactPositionA
        );
        physicsB = applyImpulsePhysics(
            objectB,
            physicsB,
            -impulseFriction,
            contactPositionB
        );
    }
}

void ConstraintContactManifold::resetLagrange() {
    for(auto i { 0 }; i < mNContacts; ++i) {
        mContacts[i].resetLagrange();
    }
}

void ConstraintContactManifold::applyConstraintVelocity(const ParticipantTable& states, float substepSeconds) {
    for(auto i { 0 }; i < mNContacts; ++i) {
        mContacts[i].applyConstraintVelocity(states, substepSeconds);
    }
}

void ConstraintContactManifold::applyConstraintPosition(const ParticipantTable& states, float substepSeconds) {
    for(auto i { 0 }; i < mNContacts; ++i) {
        mContacts[i].applyConstraintPosition(states, substepSeconds);
    }
}

void ConstraintContactManifold::addContact(const Collision& collision,
    const PhysicsState& physicsA, const ObjectBounds& boundsA, const ObjectBounds& boundsAPrev,
    const PhysicsState& physicsB, const ObjectBounds& boundsB, const ObjectBounds& boundsBPrev
) {
    trim(boundsA, boundsB);

    // guard: the contact being added should be real
    if(!collision.mCollided) {
        return;
    }

    // guard: see whether this contact is a repeat of one we've already seen
    for(std::size_t i { 0 }; i < 4; ++i) {
        const glm::vec3 deltaA { collision.mContactA.mPoint - mContacts[i].mCurrentA };
        const glm::vec3 deltaB { collision.mContactB.mPoint - mContacts[i].mCurrentB };
        const bool isDifferentA { squareDistance(deltaA) > kPersistentThresholdSquared };
        const bool isDifferentB { squareDistance(deltaB) > kPersistentThresholdSquared };
        if(!(isDifferentA || isDifferentB)) {
            return;
        }
    }

    mContacts[mNContacts++].updateCollisionData(collision,
        physicsA, boundsA, boundsAPrev,
        physicsB, boundsB, boundsBPrev
    );

    assert(mNContacts >= 1 && mNContacts <= 5 && "Invalid number of points in intermediate contact manifold");

    // 0th position for the deepest penetrating contact
    if(mNContacts >= 2) {
        std::size_t deepest { 0 };
        for(std::size_t i { 0 }; i < mNContacts; ++i) {
            if(mContacts[i].mPenetration > mContacts[deepest].mPenetration) {
                deepest = i;
            }
        }
        std::swap(mContacts[deepest], mContacts[0]);
    }

    // 1st position for the contact furthest from the deepest contact
    if(mNContacts >= 3) {
        std::size_t furthest { 1 };
        float furthestDistanceSquared {
            -std::numeric_limits<float>::infinity()
        };
        for(std::size_t i { 1 }; i < mNContacts; ++i) {
            const float distanceSquared {
                squareDistance(mContacts[i].mCurrentA - mContacts[0].mCurrentA)
            };
            if(distanceSquared > furthestDistanceSquared) {
                furthest = i;
                furthestDistanceSquared = distanceSquared;
            }
        }
        std::swap(mContacts[furthest], mContacts[1]);

        // if this point is colinear with the other two, discard it and all the points
        // after it
        //
        const float distanceSquared {
            squareDistance(mContacts[1].mCurrentA - mContacts[0].mCurrentA)
        };
        if(distanceSquared == 0.f) {
            mNContacts = 1;
            return;
        }
    }

    // 2nd position for the contact furthest (perpendicular distance) from line segment formed by
    // 0 and 1
    if(mNContacts >= 4) {
        std::size_t furthest { 2 };
        float furthestDistanceSquared {
            -std::numeric_limits<float>::infinity()
        };
        const glm::vec3 lineDir {
            glm::normalize(mContacts[1].mCurrentA - mContacts[0].mCurrentA)
        };
        for(std::size_t i { 2 }; i < mNContacts; ++i) {
            const float distanceSquared { squareDistance(
                mContacts[i].mCurrentA - mContacts[0].mCurrentA
                - glm::dot(
                    mContacts[i].mCurrentA - mContacts[0].mCurrentA,
                    lineDir
                ) * lineDir
            )};
            if(distanceSquared > furthestDistanceSquared) {
                furthest = i;
                furthestDistanceSquared = distanceSquared;
            }
        }
        std::swap(mContacts[furthest], mContacts[2]);
        const float distanceSquared { squareDistance(
            mContacts[2].mCurrentA - mContacts[0].mCurrentA
            - glm::dot(
                mContacts[2].mCurrentA - mContacts[0].mCurrentA,
                lineDir
            ) * lineDir
        )};

        // if this point is colinear with the other two, discard it and all the points
        // after it
        if(distanceSquared == 0.f) {
            mNContacts = 2;
            return;
        }
    }

    // 3rd position for furthest contact from triangle
    if(mNContacts >= 5) {
        std::size_t furthest { 3 };
        float furthestDistanceSquared {
            -std::numeric_limits<float>::infinity()
        };
        const glm::mat3 barycentricSolver { computeBarycentricSolver({
            .mPoints {{
                mContacts[0].mCurrentA,
                mContacts[1].mCurrentA,
                mContacts[2].mCurrentA,
            }}
        }) };
        const glm::mat3 barycentricToTrianglePoint {
            mContacts[0].mCurrentA, mContacts[1].mCurrentA, mContacts[2].mCurrentA
        };
        for(std::size_t i { 3 }; i < mNContacts; ++i) {
            const glm::vec3 barycentricCoordinates { glm::clamp(
                barycentricSolver * mContacts[i].mCurrentA,
                0.f, 1.f
            ) };
            const glm::vec3 closestPoint {
                barycentricToTrianglePoint * barycentricCoordinates
            };
            const float distanceSquared {
                squareDistance(mContacts[i].mCurrentA - closestPoint)
            };
            if(distanceSquared > furthestDistanceSquared) {
                furthest = i;
                furthestDistanceSquared = distanceSquared;
            }
        }
        std::swap(mContacts[furthest], mContacts[3]);
        const glm::vec3 barycentricCoordinates { glm::clamp(
            barycentricSolver * mContacts[3].mCurrentA,
            0.f, 1.f
        ) };

        // discard the point that ended up in the extra slot
        if(mNContacts == 5) {
            --mNContacts;
        }

        // discard the 4th point if it lies anywhere on the triangle
        if(isPositiveStrict(barycentricCoordinates)) {
            --mNContacts;
        }
    }
}

void ConstraintContactManifold::trim(const ObjectBounds& boundsA, const ObjectBounds& boundsB) {
    const glm::vec3 positionA { boundsA.getPositionWorld() };
    const glm::vec3 positionB { boundsB.getPositionWorld() };
    const glm::quat orientationA { boundsA.getOrientationWorld() };
    const glm::quat orientationB { boundsB.getOrientationWorld() };

    auto i { 0 };
    while(i < mNContacts) {
        const glm::vec3 newA { positionA + orientationA * mContacts[i].mRelativeA };
        const glm::vec3 newB { positionB + orientationB * mContacts[i].mRelativeB };
        const glm::vec3 newAB { positionA - positionB };

        const glm::vec3 deltaA { newA - mContacts[i].mCurrentA };
        const glm::vec3 deltaB { newB - mContacts[i].mCurrentB };

        const bool isSmallDeltaA { squareDistance(deltaA) <= kPersistentThresholdSquared };
        const bool isSmallDeltaB { squareDistance(deltaB) <= kPersistentThresholdSquared };

        // this contact can be kept, check the next one
        if(isSmallDeltaA && isSmallDeltaB) {
            // retained contacts (should) have no velocity
            mContacts[i++].mCollisionVelocity = 0.f;
            continue;
        }

        // backshift all of the contacts after this one
        for(auto j { i + 1 }; j < mNContacts; ++j) {
            mContacts[j - 1] = mContacts[j];
        }
        --mNContacts;
    }
}

void ConstraintDampingRigidbody::applyConstraintVelocity(const ParticipantTable& states, float substepSeconds) {
    // only dynamic objects have damping applied
    PhysicsState& physicsCurr { states.at(0).second.get() };
    if(physicsCurr.getMode() != PhysicsState::MODE_DYNAMIC) {
        return;
    }

    const float factorDamping { glm::min(physicsCurr.mVelocityBleed * substepSeconds, 1.f) };
    const float factorDampingAngular { glm::min(physicsCurr.mVelocityBleedAngular * substepSeconds, 1.f) };
    assert(factorDamping >= 0.f && "A negative damping value is invalid");
    assert(factorDampingAngular >= 0.f && "A negative damping value is invalid");
    assert(physicsCurr.mVelocityBleed <= 1.f && "A velocity bleed greater than one is invalid");
    assert(physicsCurr.mVelocityBleedAngular <= 1.f && "A velocity bleed greater than one is invalid");
    const float oneBySubstep { 1.f / substepSeconds };
    const float cutoffVelocity { physicsCurr.mVelocityCutoff };
    const float cutoffVelocityAngular { physicsCurr.mVelocityCutoffAngular };
    assert(cutoffVelocity >= 0.f && "A negative cutoff value is invalid");
    assert(cutoffVelocityAngular >= 0.f && "A negative cutoff value is invalid");

    const ObjectBounds& object { states.at(0).first.get() };
    const PhysicsState physicsPrev { getParameter(0) };

    // apply linear damping
    const glm::vec3 deltaVelocityLinear { physicsCurr.mVelocity - physicsPrev.mVelocity };
    const glm::vec3 correctionLinear { -factorDamping * deltaVelocityLinear };
    const glm::vec3 impulseLinear { correctionLinear * physicsCurr.getMass() };
    physicsCurr = applyImpulsePhysics(
        object,
        physicsCurr,
        impulseLinear,
        object.getPositionWorld()
    );
    if(squareDistance(physicsCurr.mVelocity) <= cutoffVelocity * cutoffVelocity) {
        physicsCurr.mVelocity = glm::vec3 { 0.f };
    }

    // apply angular damping
    const glm::quat orientation { object.getOrientationWorld() };
    const glm::vec3 deltaVelocityAngularLocal { glm::inverse(orientation) * (physicsCurr.mAngularVelocity - physicsPrev.mAngularVelocity) };
    const glm::vec3 correctionAngularLocal { -factorDampingAngular * deltaVelocityAngularLocal };
    const glm::vec3 impulseAngular { orientation * (correctionAngularLocal * physicsCurr.getRotationalInertia()) };
    physicsCurr = applyImpulsePhysics(
        object,
        physicsCurr,
        impulseAngular
    );
    if(squareDistance(physicsCurr.mAngularVelocity) <= cutoffVelocityAngular * cutoffVelocityAngular) {
        physicsCurr.mAngularVelocity = glm::vec3 { 0.f };
    }
}

void ConstraintRotation1D::applyConstraintPosition(const ParticipantTable& states, float substepSeconds) {
    assert(getConfig().isSensible(true) && "Invalid rotation constraint config");
    assert(states.find(0) != states.end() && "Entry for participant 0 not found");
    assert(states.find(1) != states.end() && "Entry for participant 1 not found");
    assert(getParameter(0).isSensible() && "Invalid rotation parameter for participant 0");
    assert(getParameter(1).isSensible() && "Invalid rotation parameter for participant 1");
    assert(squareDistance(getParameter(0).mVector) != 0.f && isNumber(getParameter(0).mVector) && isFinite(getParameter(0).mVector)  && "Invalid vector relative to participant 0 specified");
    assert(squareDistance(getParameter(1).mVector) != 0.f && isNumber(getParameter(1).mVector) && isFinite(getParameter(1).mVector)  && "Invalid vector relative to participant 1 specified");
    assert(glm::dot(getParameter(0).mVector, getConfig().mAxis) == 0.f && "Axis of rotation and participant 0 relative vector must be orthogonal");
    assert(glm::dot(getParameter(1).mVector, getConfig().mAxis) == 0.f && "Axis of rotation and participant 1 relative vector must be orthogonal");

    // guard: config must be active
    const Constraint1DOFConfig config { getConfig() };
    if(!config.isActive) {
        return;
    }

    // guard: at least _one_ of the participants must be dynamic, otherwise no corrections can
    // be made.
    const PhysicsState& physicsA { states.at(0).second.get() };
    const PhysicsState& physicsB { states.at(1).second.get() };
    if(
        physicsA.getMode() != PhysicsState::MODE_DYNAMIC
        && physicsB.getMode() != PhysicsState::MODE_DYNAMIC
    ) {
        return;
    }

    // find the globally oriented vectors of participants 0 and 1 and the axis of rotation
    ObjectBounds& boundsA { states.at(0).first.get() };
    ObjectBounds& boundsB { states.at(1).first.get() };
    const glm::quat orientationA { boundsA.getOrientationWorld() };
    const glm::quat orientationConstraintA { getParameter(0).mOrientation };
    const glm::quat orientationB { boundsB.getOrientationWorld() };
    const glm::quat orientationConstraintB { getParameter(1).mOrientation };
    const glm::vec3 axisLocal { config.mAxis };
    const glm::vec3 vectorLocalA { getParameter(0).mVector };
    const glm::vec3 vectorLocalB { getParameter(1).mVector };
    const glm::vec3 axis { glm::normalize(orientationA * orientationConstraintA * axisLocal) };
    const glm::vec3 vectorA { orientationA * orientationConstraintA * vectorLocalA };
    const glm::vec3 vectorB { orientationB * orientationConstraintB * vectorLocalB };

    // find the angle between our vectors in range [-Pi, Pi]
    const float angle { getAngle(vectorA, vectorB, axis) };

    // guard: only apply corrections when necessary
    if(angle >= config.mBoundLower && angle <= config.mBoundUpper) {
        return;
    }

    // find generalized inverse masses for both bodies
    const float generalizedInverseA { computeGeneralizedInverseMassRotational(
        boundsA, physicsA, axis
    ) };
    const float generalizedInverseB { computeGeneralizedInverseMassRotational(
        boundsB, physicsB, axis
    ) };

    // find correctional impulse
    const float oneBySubstep { 1.f / substepSeconds };
    const float alphaDerivative2 { getCompliance() * oneBySubstep * oneBySubstep };
    const float lagrangeDenominator {
        1.f / (generalizedInverseA + generalizedInverseB + alphaDerivative2)
    };
    const float error { -(angle - ((angle > config.mBoundUpper)?
        config.mBoundUpper : config.mBoundLower
    )) };
    const float lagrangePrevious { getLagrange().at(0) };
    const float lagrangeDelta { -(error + alphaDerivative2 * lagrangePrevious)
        * lagrangeDenominator
    };
    const glm::vec3 impulseCorrection {
        lagrangeDelta * axis
    };
    applyLagrangeDelta(lagrangeDelta, 0);

    // distribute correction amongst the bodies
    boundsA = applyImpulseObject(boundsA, physicsA, impulseCorrection);
    boundsB = applyImpulseObject(boundsB, physicsB, -impulseCorrection);
}

void ConstraintDistance1D::applyConstraintPosition(const ParticipantTable& states, float substepSeconds) {
    assert(getConfig().isSensible(false) && "Invalid position constraint config");
    assert(getParameter(0).isSensible() && "Invalid position parameter for participant 0");
    assert(getParameter(1).isSensible() && "Invalid position parameter for participant 1");
    assert(states.find(0) != states.end() && "Entry for participant 0 not found");
    assert(states.find(1) != states.end() && "Entry for participant 1 not found");
    assert(isNumber(getParameter(0).mVector) && isFinite(getParameter(0).mVector)  && "Invalid point relative to participant 0 specified");
    assert(isNumber(getParameter(1).mVector) && isFinite(getParameter(1).mVector)  && "Invalid point relative to participant 1 specified");

    // guard: config must be active
    const Constraint1DOFConfig config { getConfig() };
    if(!config.isActive) {
        return;
    }

    // guard: at least _one_ of the participants must be dynamic, otherwise no corrections can
    // be made.
    const PhysicsState& physicsA { states.at(0).second.get() };
    const PhysicsState& physicsB { states.at(1).second.get() };
    if(
        physicsA.getMode() != PhysicsState::MODE_DYNAMIC
        && physicsB.getMode() != PhysicsState::MODE_DYNAMIC
    ) {
        return;
    }

    // find the global position vectors of participants 0 and 1 and the globally oriented constrained axis
    ObjectBounds& boundsA { states.at(0).first.get() };
    ObjectBounds& boundsB { states.at(1).first.get() };
    const glm::quat orientationA { boundsA.getOrientationWorld() };
    const glm::quat orientationConstraintA { getParameter(0).mOrientation };
    const glm::quat orientationB { boundsB.getOrientationWorld() };
    const glm::quat orientationConstraintB { getParameter(1).mOrientation };
    const glm::vec3 positionA { boundsA.getPositionWorld() };
    const glm::vec3 positionB { boundsB.getPositionWorld() };
    const glm::vec3 axisLocal { config.mAxis };
    const glm::vec3 pointLocalA { getParameter(0).mVector };
    const glm::vec3 pointLocalB { getParameter(1).mVector };
    const glm::vec3 axis { glm::normalize(orientationA * orientationConstraintA * axisLocal) };
    const glm::vec3 pointA { positionA + orientationA * orientationConstraintA * pointLocalA };
    const glm::vec3 pointB { positionB + orientationB * orientationConstraintB * pointLocalB };

    // find the projected difference between A and B along the contrained axis
    const float projectedA { glm::dot(axis, pointA) };
    const float projectedB { glm::dot(axis, pointB) };
    const float deltaAB { projectedB - projectedA };

    // guard: apply correction only when configured limits are exceeded
    if(deltaAB <= config.mBoundUpper && deltaAB >= config.mBoundLower) {
        return;
    }

    // find generalized inverse masses of A and B
    const float generalizedInverseA { computeGeneralizedInverseMassPositional(
        boundsA, physicsA, pointA, axis
    ) };
    const float generalizedInverseB { computeGeneralizedInverseMassPositional(
        boundsB, physicsB, pointB, axis
    ) };

    // find and correctional impulse
    const float error { -(deltaAB - ((deltaAB > config.mBoundUpper)?
        config.mBoundUpper : config.mBoundLower
    )) };
    const float lagrangePrevious { getLagrange().at(0) };
    const float oneBySubstep { 1.f / substepSeconds };
    const float alphaDerivative2 { getCompliance() * oneBySubstep * oneBySubstep };
    const float lagrangeDenominator { 1.f / (
        generalizedInverseA + generalizedInverseB + alphaDerivative2
    ) };
    const float lagrangeDelta { -(error + alphaDerivative2 * lagrangePrevious)
        * lagrangeDenominator
    };
    const glm::vec3 impulseCorrection { lagrangeDelta * axis };
    applyLagrangeDelta(lagrangeDelta, 0);

    // apply the impulse
    boundsA = applyImpulseObject(boundsA, physicsA, impulseCorrection, pointA);
    boundsB = applyImpulseObject(boundsB, physicsB, -impulseCorrection, pointB);
}

float ToyMaker::computeGeneralizedInverseMassPositional(
    const ObjectBounds& object,
    const PhysicsState& physics,
    const glm::vec3& correctionPoint,
    const glm::vec3& correctionGradient
) {
    if(physics.getMode() != PhysicsState::MODE_DYNAMIC) {
        return 0.f;
    }
    const glm::vec3 position { object.getPositionWorld() };
    const glm::vec3 correctionOffset { correctionPoint - position };
    const glm::vec3 correctionRotational { glm::cross(correctionOffset, correctionGradient) };
    const float generalizedInverseMass { physics.mMassInverse + (squareDistance(correctionRotational)?
        computeGeneralizedInverseMassRotational(
            object, physics, correctionRotational
        ) : 0.f)
    };
    return generalizedInverseMass;
}

float ToyMaker::computeGeneralizedInverseMassRotational(
    const ObjectBounds& object,
    const PhysicsState& physics,
    const glm::vec3& correction
) {
    if(physics.getMode() != PhysicsState::MODE_DYNAMIC) {
        return 0.f;
    }
    const glm::quat orientation { object.getOrientationWorld() };
    const glm::vec3 correctionLocal { glm::inverse(orientation) * correction };
    return glm::dot(correctionLocal, physics.mRotationalInertiaInverse * correctionLocal);
}

ObjectBounds ToyMaker::applyImpulseObject(
    ObjectBounds object,
    const PhysicsState& physics,
    const glm::vec3& impulsePositional,
    const glm::vec3& impulsePoint
) {
    // guard: you can only apply an impulse to a dynamic physics object
    if(physics.getMode() != PhysicsState::MODE_DYNAMIC || squareDistance(impulsePositional) == 0.f) {
        return object;
    }

    // apply positional corrections
    const glm::vec3 position { object.getPositionWorld() };
    const glm::vec3 positionNew { position + impulsePositional * physics.mMassInverse };
    object.setPositionWorld(positionNew);

    // apply rotational corrections
    const glm::vec3 impulseRotation { glm::cross(
        impulsePoint - position, impulsePositional
    ) };
    object = applyImpulseObject(object, physics, impulseRotation);
    return object;
}

PhysicsState ToyMaker::applyImpulsePhysics(
    const ObjectBounds& object,
    PhysicsState physics,
    const glm::vec3& impulsePositional,
    const glm::vec3& impulsePoint
) {
    // guard: you can only apply an impulse to a dynamic physics object
    if(physics.getMode() != PhysicsState::MODE_DYNAMIC || squareDistance(impulsePositional) == 0.f) {
        return physics;
    }

    const glm::vec3 position { object.getPositionWorld() };
    const glm::vec3 impulsePointRelative { impulsePoint - object.getPositionWorld() };
    const glm::vec3 toCenter { impulsePointRelative != glm::vec3{ 0.f }?
        glm::normalize(-impulsePointRelative): glm::normalize(impulsePositional)
    };
    const glm::vec3 impulseLinear { glm::dot(impulsePositional, toCenter) * toCenter };
    const glm::vec3 deltaVelocity { impulseLinear * physics.mMassInverse };
    assert(isNumber(deltaVelocity) && "computed delta velocity must be a number");
    physics.mVelocity += deltaVelocity;
    assert(isNumber(physics.mVelocity) && "resulting object velocity must be a number");

    const glm::vec3 impulseRotational { impulsePointRelative != glm::vec3{ 0.f }?
        glm::cross(
            impulsePointRelative,
            impulsePositional
        ): glm::vec3 { 0.f }
    };
    physics = applyImpulsePhysics(object, physics, impulseRotational);

    return physics;
}

ObjectBounds ToyMaker::applyImpulseObject(
    ObjectBounds object,
    const PhysicsState& physics,
    const glm::vec3& impulseRotational
) {
    // guard: you can only apply an impulse to a dynamic physics object
    if(physics.getMode() != PhysicsState::MODE_DYNAMIC || squareDistance(impulseRotational) == 0.f ) {
        return object;
    }

    const glm::quat orientation { object.getOrientationWorld() };
    const glm::vec3 impulseRotationalLocal { glm::inverse(orientation) * impulseRotational };
    const glm::vec3 deltaOrientation { orientation * (physics.mRotationalInertiaInverse * impulseRotationalLocal) };
    const glm::quat orientationNew { glm::normalize(
        orientation + .5f * glm::quat(0.f, deltaOrientation.x, deltaOrientation.y, deltaOrientation.z) * orientation
    ) };
    object.setOrientationWorld(orientationNew);
    return object;
}

PhysicsState ToyMaker::applyImpulsePhysics(
    const ObjectBounds& object,
    PhysicsState physics,
    const glm::vec3& impulseRotational
) {
    // guard: you can only apply an impulse to a dynamic physics object
    if(physics.getMode() != PhysicsState::MODE_DYNAMIC || squareDistance(impulseRotational) == 0.f) {
        return physics;
    }

    const glm::quat orientation { object.getOrientationWorld() };
    const glm::vec3 impulseLocal { glm::inverse(orientation) * impulseRotational };
    const glm::vec3 deltaVelocityAngular { orientation * (physics.mRotationalInertiaInverse * impulseLocal) };
    assert(isNumber(deltaVelocityAngular) && "computed change in angular velocity must be a number");
    physics.mAngularVelocity += deltaVelocityAngular;
    assert(isNumber(physics.mAngularVelocity) && "resultant angular velocity must be a number");
    return physics;
}


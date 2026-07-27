#include <glm/gtc/quaternion.hpp>

#include "toymaker/engine/spatial_query/math.hpp"
#include "toymaker/engine/physics/types.hpp"

using namespace ToyMaker;

const float kPersistentThresholdSquared { 1e-6 };

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

ContactConstraint::ContactConstraint(): Constraint<2> { 0.f } {}

void ContactConstraint::updateCollisionData(
    const Collision& collision,
    const PhysicsState& physicsA,
    const ObjectBounds& boundsA,
    const ObjectBounds& boundsAPrev,
    const PhysicsState& physicsB,
    const ObjectBounds& boundsB,
    const ObjectBounds& boundsBPrev
) {
    mCollided = collision.mCollided;
    if(!mCollided) return;

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

void ContactConstraint::applyConstraintPosition(
    const ParticipantTable& states,
    float substepSeconds
) {
    assert(states.size() == 2 && "Constraint accepts states belonging to exactly two participants");

    // fetch physics state
    const PhysicsState& physicsA { states.at(0).second.get() };
    const PhysicsState& physicsB { states.at(1).second.get() };

    // guards:
    if(
        // no collision, so nothing to do
        !mCollided
        // at least one object must be dynamic for the collision solver to work
        || (
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

    ObjectBounds& objectA { states.at(0).first.get() };
    ObjectBounds& objectB { states.at(1).first.get() };
    const glm::vec3 positionA { objectA.getPositionWorld() };
    const glm::vec3 positionB { objectB.getPositionWorld() };

    // compute generalized inverse masses for A and B -- these will
    // be recomputed each substep, so there's no point in storing them
    const float generalizedInverseA { computeGeneralizedInverseMassPositional(
        objectA,
        physicsA,
        mCurrentA,
        mContactNormal
    ) };
    const float generalizedInverseB { computeGeneralizedInverseMassPositional(
        objectB,
        physicsB,
        mCurrentB,
        mContactNormal
    ) };

    // compute and update correction value
    const float alphaDerivative2 {
        getCompliance() / (substepSeconds * substepSeconds)
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

    // cache old placement data
    const glm::quat orientationA { objectA.getOrientationWorld() };
    const glm::quat orientationB { objectB.getOrientationWorld() };

    // apply corrections
    objectA = applyImpulseObject(
        objectA,
        physicsA,
        positionalImpulse,
        mCurrentA
    );
    objectB = applyImpulseObject(
        objectB,
        physicsB,
        -positionalImpulse,
        mCurrentB
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

void ContactConstraint::applyConstraintVelocity(const ParticipantTable& states, float substepSeconds) {
    // guard: no collision, so nothing to do
    if(!mCollided) {
        return;
    }
    assert(states.size() == 2 && "Constraint accepts states belonging to exactly two participants");

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
    PhysicsState& physicsA { states.at(0).second.get() };
    PhysicsState& physicsB { states.at(1).second.get() };
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

    // derive the current coefficient of restitution between this pair of objects, set
    // to 0 when small separation velocity detected
    const float coefficientRestitution { (glm::abs(bounceVelocity) <= 20.f * substepSeconds)?
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
    if(coefficientFrictionDynamic > 0.f) {
        const glm::vec3 tangentialVelocity {
            pointVelocityAB - bounceVelocity * mContactNormal
        };

        // derive the impulse required to fix our velocities
        const float lagrangeCollision { getLagrange().at(0) };
        const float forceNormal { lagrangeCollision / (substepSeconds * substepSeconds) };
        const glm::vec3 velocityCorrection {
            -glm::normalize(tangentialVelocity) * glm::min(
                glm::abs(substepSeconds * coefficientFrictionDynamic * forceNormal),
                glm::length(tangentialVelocity)
            )
        };
        const glm::vec3 impulseFriction {
            velocityCorrection / (generalizedInverseA + generalizedInverseB)
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

void ContactManifold::resetLagrange() {
    for(auto i { 0 }; i < mNContacts; ++i) {
        mContacts[i].resetLagrange();
    }
}

void ContactManifold::applyConstraintVelocity(const ParticipantTable& states, float substepSeconds) {
    for(auto i { 0 }; i < mNContacts; ++i) {
        mContacts[i].applyConstraintVelocity(states, substepSeconds);
    }
}

void ContactManifold::applyConstraintPosition(const ParticipantTable& states, float substepSeconds) {
    for(auto i { 0 }; i < mNContacts; ++i) {
        mContacts[i].applyConstraintPosition(states, substepSeconds);
    }
}

void ContactManifold::addContact(const Collision& collision,
    const PhysicsState& physicsA, const ObjectBounds& boundsA, const ObjectBounds& boundsAPrev,
    const PhysicsState& physicsB, const ObjectBounds& boundsB, const ObjectBounds& boundsBPrev
) {
    trim(boundsA, boundsB);

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

    mContacts[mNContacts++] = ContactConstraint();
    mContacts[mNContacts - 1].updateCollisionData(collision,
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
    }

    // 2nd position for the contact furthest from line segment formed by
    // 0 and 1
    if(mNContacts >= 4) {
        std::size_t furthest { 2 };
        float furthestDistanceSquared {
            -std::numeric_limits<float>::infinity()
        };
        const glm::vec3 lineVector {
            mContacts[1].mCurrentA - mContacts[0].mCurrentA
        };
        for(std::size_t i { 2 }; i < mNContacts; ++i) {
            const float t { std::clamp(
                glm::dot(
                    mContacts[i].mCurrentA - mContacts[0].mCurrentA,
                    lineVector
                ),
                0.f, 1.f
            ) };
            const glm::vec3 linePointClosest {
                mContacts[i].mCurrentA + t * lineVector
            };
            const float distanceSquared {
                squareDistance(mContacts[i].mCurrentA - linePointClosest)
            };
            if(distanceSquared > furthestDistanceSquared) {
                furthest = i;
                furthestDistanceSquared = distanceSquared;
            }
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
    }

    // discard the point that ended up in the extra slot
    if(mNContacts == 5) {
        --mNContacts;
    }

    // we have a triangle and a point -- discard the point if it lies
    // within the triangle
    if(mNContacts == 4) {
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
        const glm::vec3 barycentricCoordinates { glm::clamp(
            barycentricSolver * mContacts[3].mCurrentA,
            0.f, 1.f
        ) };

        // discard the 4th point if it lies anywhere on the triangle
        if(isPositiveStrict(barycentricCoordinates)) {
            --mNContacts;
        }
    }
}

void ContactManifold::trim(const ObjectBounds& boundsA, const ObjectBounds& boundsB) {
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

        const bool isPenetrating { glm::dot(mContacts[i].mContactNormal, newAB) > 0.f };
        const bool isSmallDeltaA { squareDistance(deltaA) < kPersistentThresholdSquared };
        const bool isSmallDeltaB { squareDistance(deltaB) < kPersistentThresholdSquared };

        // this contact can be kept, check the next one
        if(isPenetrating && isSmallDeltaA && isSmallDeltaB) {
            ++i;
            continue;
        }

        // backshift all of the contacts after this one
        for(auto j { i + 1 }; j < mNContacts; ++j) {
            mContacts[j - 1] = mContacts[j];
        }
        --mNContacts;
    }
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
    if(physics.getMode() != PhysicsState::MODE_DYNAMIC) {
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
    if(physics.getMode() != PhysicsState::MODE_DYNAMIC) {
        return physics;
    }

    const glm::vec3 position { object.getPositionWorld() };
    const glm::vec3 impulsePointRelative { impulsePoint - object.getPositionWorld() };
    const glm::vec3 toCenter { glm::normalize(-impulsePointRelative) };
    const glm::vec3 impulseLinear { glm::dot(impulsePositional, toCenter) * toCenter };
    const glm::vec3 deltaVelocity { impulseLinear * physics.mMassInverse };
    physics.mVelocity += deltaVelocity;

    const glm::vec3 impulseRotational { glm::cross(
        impulsePointRelative,
        impulsePositional
    ) };
    physics = applyImpulsePhysics(object, physics, impulseRotational);

    return physics;
}

ObjectBounds ToyMaker::applyImpulseObject(
    ObjectBounds object,
    const PhysicsState& physics,
    const glm::vec3& impulseRotational
) {
    // guard: you can only apply an impulse to a dynamic physics object
    if(physics.getMode() != PhysicsState::MODE_DYNAMIC) {
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
    if(physics.getMode() != PhysicsState::MODE_DYNAMIC) {
        return physics;
    }

    const glm::quat orientation { object.getOrientationWorld() };
    const glm::vec3 impulseLocal { glm::inverse(orientation) * impulseRotational };
    const glm::vec3 deltaVelocityAngular { orientation * (physics.mRotationalInertiaInverse * impulseLocal) };
    physics.mAngularVelocity += deltaVelocityAngular;
    return physics;
}


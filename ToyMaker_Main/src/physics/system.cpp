#include <map>
#include <unordered_map>

#include "toymaker/engine/spatial_query/system.hpp"
#include "toymaker/engine/spatial_query/math.hpp"

#include "toymaker/engine/physics/types.hpp"
#include "toymaker/engine/physics/system.hpp"

using namespace ToyMaker;

glm::vec3 computeRotationalInertiaBox(float mass, const ObjectBounds& cuboid);
glm::vec3 computeRotationalInertiaCapsule(float mass, const ObjectBounds& capsule);
glm::vec3 computeRotationalInertiaSphere(float mass, const ObjectBounds& sphere);

void PhysicsSystem::onSimulationActivated() {
    mRequiresInitialization = true;
}

void PhysicsSystem::onSimulationStep(uint32_t timestepMillis) {
    // if the physics system has just been woken up, all entities must
    // undergo initialization
    if(mRequiresInitialization) {
        for(EntityID entity: getEnabledEntities()) {
            updatePhysicsProperties(entity);
        }
        mRequiresInitialization = false;

    // if there's only a small collection of entities which haven't undergone
    // initialization, initialize them
    } else if(!mEntitiesUninitialized.empty()) {
        const std::set<EntityID> entitiesCopy { mEntitiesUninitialized };
        for(EntityID entity: entitiesCopy) {
            updatePhysicsProperties(entity);
        }
    }

    // do physics update for all eligible 
    std::unordered_map<EntityID, PhysicsStatePartial> previousStates {};
    const float substepInterval { (static_cast<float>(timestepMillis) / static_cast<float>(mSubsteps)) / static_cast<float>(1e3) };

    // clear lagrange multipliers in preparation for this physics update
    for(auto& constraint: mConstraints) {
        constraint.first->resetLagrange();
    }

    std::map<CollisionPair, std::unique_ptr<CollisionConstraint>> potentialCollisions {
        collectPotentialCollisions(substepInterval)
    };


    for(auto substep { 0 }; substep < mSubsteps; ++substep) {
        integrateForces(substepInterval, previousStates);

        applyPositionConstraints(potentialCollisions, substepInterval);

        deriveVelocities(substepInterval, previousStates);

        applyVelocityConstraints(potentialCollisions, substepInterval);
    }

    // clear forces (which are expected to be applied per frame)
    for(const auto entity: getEnabledEntities()) {
        auto physicsState { getComponent<PhysicsState>(entity) };
        physicsState.mForce = glm::vec3 { 0.f };
        physicsState.mTorque = glm::vec3 { 0.f };
        updateComponent(entity, physicsState);
    }
}

void PhysicsSystem::integrateForces(float substepSeconds, std::unordered_map<EntityID, PhysicsStatePartial>& previousStates) {
    for(const EntityID entity: getEnabledEntities()) {
        // filter out uninitialized entities
        if(mEntitiesUninitialized.find(entity) != mEntitiesUninitialized.end()) {
            continue;
        }

        // snapshot current object state
        PhysicsState physics { getComponent<PhysicsState>(entity) };
        ObjectBounds bounds { getComponent<ObjectBounds>(entity) };
        assert(
            bounds.mUpdatedFromTransform == false
            && "Object bounds controlled by the physics system cannot be updated through their transforms"
        );
        const PhysicsStatePartial physicsState {
            .mVelocity { physics.mVelocity },
            .mAngularVelocity { physics.mAngularVelocity },
            .mPosition { bounds.getPositionWorld() },
            .mOrientation  { bounds.getOrientationWorld() },
        };
        previousStates[entity] = physicsState;

        // update object state per positional derivatives
        if(physics.getMode() == PhysicsState::MODE_DYNAMIC) {
            physics.mVelocity += substepSeconds * physics.mForce * physics.mMassInverse;
            assert(isNumber(physics.mVelocity) && "Velocity computation failed");
        }
        if(physics.getMode() != PhysicsState::MODE_STATIC && squareDistance(physics.mVelocity) != 0.f) {
            bounds.setPositionWorld(
                physicsState.mPosition + substepSeconds * physics.mVelocity
            );
        }

        // update object state per rotational derivatives, see paper [Impulse based dynamic simulation
        // Appendix A.3.](https://people.eecs.berkeley.edu/~jfc/mirtich/impulse.html)
        if(physics.getMode() == PhysicsState::MODE_DYNAMIC) {
            const glm::quat toLocal { glm::inverse(physicsState.mOrientation) };
            const glm::vec3 rotationalInertiaLocal { physics.getRotationalInertia() };
            const glm::vec3 angularVelocityLocal { toLocal * physics.mAngularVelocity };
            const glm::vec3 torqueLocal { toLocal * physics.mTorque };
            const glm::vec3 deltaAngularVLocal { substepSeconds
                * physics.mRotationalInertiaInverse * (
                    torqueLocal - glm::cross(
                        angularVelocityLocal,
                        rotationalInertiaLocal * angularVelocityLocal
                    )
            ) };
            physics.mAngularVelocity += physicsState.mOrientation * deltaAngularVLocal;
        }

        // For sensible angular velocities, update orientation
        if(physics.getMode() != PhysicsState::MODE_STATIC && squareDistance(physics.mAngularVelocity) != 0.f) {
            const float deltaAngle {
                substepSeconds * glm::length(physics.mAngularVelocity)
            };
            const glm::vec3 axisAngularVelocity { glm::normalize(physics.mAngularVelocity) };
            const glm::quat orientationUpdate { glm::angleAxis(deltaAngle, axisAngularVelocity) };
            const auto newOrientation { glm::normalize(orientationUpdate * physicsState.mOrientation) };
            bounds.setOrientationWorld(newOrientation);
        }

        // push updates to component table
        updateComponent(entity, physics);
        updateComponent(entity, bounds);
    }
}

std::map<CollisionPair, std::unique_ptr<CollisionConstraint>> PhysicsSystem::collectPotentialCollisions(float substepSeconds) {
    std::map<CollisionPair, std::unique_ptr<CollisionConstraint>> potentialCollisions {};
    const std::set<EntityID>& enabledEntities { getEnabledEntities() };
    for(const EntityID entity: enabledEntities) {
        // find an AABB that can contain the object regardless of orientation, with some
        // margin
        const ObjectBounds objectBounds { getComponent<ObjectBounds>(entity) };
        const AxisAlignedBounds originalBounds { objectBounds };
        const float largestDimension {
            glm::max(originalBounds.getDimensions().x, glm::max(
                originalBounds.getDimensions().y, originalBounds.getDimensions().z
            ))
        };
        const float areaExpansionFactor { 2.f };
        const AxisAlignedBounds startSearch {
            originalBounds.getPositionWorld(),
            glm::vec3 { areaExpansionFactor * largestDimension },
            originalBounds.getInteractionLayers()
        };

        // expand AABB along object's current velocity
        const PhysicsState physics { getComponent<PhysicsState>(entity) };
        AxisAlignedBounds endSearch { startSearch };
        endSearch.setPosition(
            endSearch.getPositionWorld() + mSubsteps * substepSeconds * physics.mVelocity
        );
        const AxisAlignedBounds expandedSearch { startSearch + endSearch };

        // add links for all valid candidates found in the search
        const auto candidates { mWorld.lock()
            ->getSystem<SpatialQuerySystem>()
            ->findEntitiesOverlappingCoarse(
                expandedSearch,
                objectBounds.getInteractionMask()
            )
        };
        for(const std::pair<EntityID, AxisAlignedBounds>& candidate: candidates) {
            // self-collisions, collisions with non-physical objects are invalid
            if(
                candidate.first == entity
                || enabledEntities.find(candidate.first) == enabledEntities.end()
            ) {
                continue;
            }

            // skip already discovered collision constraints
            const CollisionPair link { entity, candidate.first };
            if(
                potentialCollisions.find(link) != potentialCollisions.end()
            ) {
                continue;
            }

            potentialCollisions.emplace(
                link,
                std::make_unique<CollisionConstraint> (
                    checkCollision(
                        objectBounds,
                        getComponent<ObjectBounds>(candidate.first)
                    ),
                    getComponent<PhysicsState>(entity),
                    objectBounds,
                    getComponent<PhysicsState>(candidate.first),
                    getComponent<ObjectBounds>(candidate.first)
                )
            );
        }
    }
    return potentialCollisions;
}

void PhysicsSystem::applyPositionCollisionConstraints(
    std::map<CollisionPair, std::unique_ptr<CollisionConstraint>>& constraints,
    float substepSeconds
) {
    for(auto& linkConstraint: constraints) {
        // retrieve data
        auto objectOne { getComponent<ObjectBounds>(linkConstraint.first.first()) };
        auto objectTwo { getComponent<ObjectBounds>(linkConstraint.first.second()) };
        auto physicsOne { getComponent<PhysicsState>(linkConstraint.first.first()) };
        auto physicsTwo { getComponent<PhysicsState>(linkConstraint.first.second()) };
        const auto collisionData { checkCollision(
            objectOne, objectTwo
        ) };
        const BaseConstraint::ParticipantTable participantTable {
            { 0, { objectOne, physicsOne }},
            { 1, { objectTwo, physicsTwo }}
        };

        // apply constraint
        linkConstraint.second->applyConstraintPosition(participantTable, substepSeconds);

        // push data to component arrays
        updateComponent<ObjectBounds>(linkConstraint.first.first(), objectOne);
        updateComponent<ObjectBounds>(linkConstraint.first.second(), objectTwo);
    }
}

void PhysicsSystem::applyPositionConstraints(std::map<CollisionPair, std::unique_ptr<CollisionConstraint>>& collisionConstraints, float substepSeconds) {
    for(ConstraintID constraint { 0 }; constraint < mConstraints.size(); ++constraint) {
        // skip deleted or inactive constraints
        if(
            mDeletedConstraints.find(constraint) != mDeletedConstraints.end()
            || mInactiveConstraints.find(constraint) != mDeletedConstraints.end()
        ) {
            continue;
        }

        // gather current entity states and prepare them for consumption by
        // constraint
        std::map<EntityID, std::pair<ObjectBounds, PhysicsState>> entities {};
        BaseConstraint::ParticipantTable participants {};
        std::unique_ptr<BaseConstraint>& solver { mConstraints[constraint].first };
        BaseConstraint::ParticipantID participantID {0};
        for(const EntityID entity: mConstraints[constraint].second) {
            entities.insert({
                entity,
                { getComponent<ObjectBounds>(entity), getComponent<PhysicsState>(entity) }
            });
            participants.insert({
                participantID++,
                {
                    entities[entity].first,
                    entities[entity].second
                }
            });
        }

        // apply constraints and update relevant entity component tables
        solver->applyConstraintPosition(participants, substepSeconds);
        for(const auto& entity: entities) {
            updateComponent(entity.first, entity.second.first);
            updateComponent(entity.first, entity.second.second);
        }
    }

    applyPositionCollisionConstraints(collisionConstraints, substepSeconds);
}

void PhysicsSystem::applyVelocityConstraints(std::map<CollisionPair, std::unique_ptr<CollisionConstraint>>& collisionConstraints, float substepSeconds) {
    applyVelocityCollisionConstraints(collisionConstraints, substepSeconds);
}

void PhysicsSystem::applyVelocityCollisionConstraints(std::map<CollisionPair, std::unique_ptr<CollisionConstraint>>& constraints, float substepSeconds) {
    for(auto& linkConstraint: constraints) {
        // retrieve data
        auto objectOne { getComponent<ObjectBounds>(linkConstraint.first.first()) };
        auto objectTwo { getComponent<ObjectBounds>(linkConstraint.first.second()) };
        auto physicsOne { getComponent<PhysicsState>(linkConstraint.first.first()) };
        auto physicsTwo { getComponent<PhysicsState>(linkConstraint.first.second()) };
        const auto collisionData { checkCollision(
            objectOne, objectTwo
        ) };
        const BaseConstraint::ParticipantTable participantTable {
            { 0, { objectOne, physicsOne }},
            { 1, { objectTwo, physicsTwo }}
        };

        // apply constraint
        linkConstraint.second->applyConstraintVelocity(participantTable, substepSeconds);

        // push data to component arrays
        updateComponent<PhysicsState>(linkConstraint.first.first(), physicsOne);
        updateComponent<PhysicsState>(linkConstraint.first.second(), physicsTwo);
    }
}

void PhysicsSystem::deriveVelocities(float substepSeconds, const std::unordered_map<EntityID, PhysicsStatePartial>& previousStates) {
    // derive actual physics properties post constraint solve
    for(const auto entity: getEnabledEntities()) {

        // filter out uninitialized entities
        if(mEntitiesUninitialized.find(entity) != mEntitiesUninitialized.end()) {
            continue;
        }

        // retrieve current and previous states
        const PhysicsStatePartial previousState { previousStates.at(entity) };
        auto physicsProps { getComponent<PhysicsState>(entity) };
        const auto bounds { getComponent<ObjectBounds>(entity) };
        const auto orientationInverse { glm::inverse(bounds.getOrientationWorld()) };

        // update linear terms
        physicsProps.mVelocity = (
            (bounds.getPositionWorld() - previousState.mPosition)
            / substepSeconds
        );

        // update angular terms
        const glm::quat deltaOrientation {
            (bounds.getOrientationWorld() * glm::inverse(previousState.mOrientation))
        };
        physicsProps.mAngularVelocity = (2.f * glm::vec3 {
            deltaOrientation.x, deltaOrientation.y, deltaOrientation.z
        } / substepSeconds);
        physicsProps.mAngularVelocity *= deltaOrientation.w >= 0.f ? 1.f : -1.f;

        // push updates to component table
        updateComponent(entity, physicsProps);
    }
}

void PhysicsSystem::onEntityEnabled(EntityID entityID) {
    updatePhysicsProperties(entityID);
}

void PhysicsSystem::onEntityDisabled(EntityID entityID) {
    mEntitiesUninitialized.erase(entityID);
    const auto foundEntity { mEntityConstraintMap.find(entityID) };
    if(foundEntity == mEntityConstraintMap.end()) {
        return;
    }
    for(const ConstraintID constraint: foundEntity->second) {
        mInactiveConstraints.insert(constraint);
    }
}

void PhysicsSystem::onEntityUpdated(EntityID entityID, ComponentType updatedComponent) {
    updatePhysicsProperties(entityID);
}

void PhysicsSystem::unregisterConstraint(ConstraintID constraint) {
    assert(constraint < mConstraints.size() && "Invalid constraint ID specified");
    for(const auto& entity: mConstraints.at(constraint).second) {
        mEntityConstraintMap.at(entity).erase(constraint);
    }
    mDeletedConstraints.insert(constraint);
}

void PhysicsSystem::refreshActiveConstraints() {
    mInactiveConstraints.clear();
    for(ConstraintID constraintID { 0 }; constraintID < mConstraints.size(); ++constraintID) {
        if(!isConstraintActive(constraintID)) {
            mInactiveConstraints.insert(constraintID);
        }
    }
}

bool PhysicsSystem::isConstraintActive(ConstraintID constraintID) const {
    const auto& enabledEntities { getEnabledEntities() };
    const auto& constraint { mConstraints.at(constraintID) };
    for(const auto& entity: constraint.second) {
        if(
            enabledEntities.find(entity) == enabledEntities.end()
            || mEntitiesUninitialized.find(entity) != enabledEntities.end()
        ) {
            return false;
        }
    }
    return true;
}

void PhysicsSystem::updatePhysicsProperties(EntityID entityID) {
    auto physicsProps { getComponent<PhysicsState>(entityID) };
    assert((
        isNonNegative(physicsProps.mMassInverse)
        && isFinite(physicsProps.mMassInverse)
    ) && "Entity must have non-zero positive mass");

    // senseless bounds usually mean that spatial query system has not
    // initialized bounds for this entity, wait for next timestep
    auto bounds { getComponent<ObjectBounds>(entityID) };
    if(
        !bounds.isSensible() || (
            bounds.mVolumeSystemComputed
            && bounds.mVolumeUpdateRequired
        )
    ) {
        mEntitiesUninitialized.insert(entityID);
        return;
    }
    bounds.mUpdatedFromTransform = false;
    updateComponent(entityID, bounds);
    mEntitiesUninitialized.erase(entityID);

    // enable any constraints that depend on this entity if possible
    if(mConstraintsInitialized) {
        const auto foundEntityConstraint { mEntityConstraintMap.find(entityID) };
        if(foundEntityConstraint != mEntityConstraintMap.end()) {
            for(const ConstraintID constraint: foundEntityConstraint->second) {
                if(isConstraintActive(constraint)) {
                    mInactiveConstraints.erase(constraint);
                }
            }
        }
    }

    switch(bounds.mType) {
        case ToyMaker::ObjectBounds::TrueVolumeType::BOX:
            physicsProps.setRotationalInertia(
                computeRotationalInertiaBox(physicsProps.getMass(), bounds)
            );
            break;
        case ToyMaker::ObjectBounds::TrueVolumeType::CAPSULE:
            physicsProps.setRotationalInertia(
                computeRotationalInertiaCapsule(
                    physicsProps.getMass(), bounds
                )
            );
            break;
        case ToyMaker::ObjectBounds::TrueVolumeType::SPHERE:
            physicsProps.setRotationalInertia(
                computeRotationalInertiaSphere(
                    physicsProps.getMass(), bounds
                )
            );
            break;
        default:
            assert(false && "Unrecognized bounds type");
            break;
    }

    updateComponent(entityID, physicsProps);
}

glm::vec3 computeRotationalInertiaBox(float mass, const ObjectBounds& cuboid) {
    const glm::vec3& dimensions { cuboid.mTrueVolume.mBox.mDimensions };
    const float mult { mass / 12.f };
    return glm::vec3 {
        mult * (
            dimensions.y * dimensions.y + dimensions.z * dimensions.z
        ),
        mult * (
            dimensions.x * dimensions.x + dimensions.z * dimensions.z
        ),
        mult * (
            dimensions.x * dimensions.x + dimensions.y * dimensions.y
        ),
    };
}

glm::vec3 computeRotationalInertiaCapsule(float mass, const ObjectBounds& capsule) {

    const float radius { capsule.mTrueVolume.mCapsule.mRadius };
    const float height { capsule.mTrueVolume.mCapsule.mHeight };

    const float volumeCylinder { glm::pi<float>() * radius * radius * height };
    const float volumeHemisphere { 2.f * glm::pi<float>() * radius * radius * radius / 3.f };

    const float massCylinder { mass * volumeCylinder / (volumeCylinder + 2.f * volumeHemisphere) };
    const float massHemisphere { (mass - massCylinder) * .5f };

    const float inertiaOffAxis {
        // cylinder inertia
        massCylinder * (height * height + 3.f * radius * radius) / 12.f

        // hemisphere inertia
        + 4.f * massHemisphere * radius * radius / 5.f

        // hemisphere origin offset inertia
        + massHemisphere * (
            16.f * height * height
            + 9.f * radius * radius
            + 12.f * height * radius
        ) / 32.f
    };

    return glm::vec3 {
        inertiaOffAxis,
        (5.f * massCylinder + 8.f * massHemisphere)
        * radius * radius
        / 10.f,
        inertiaOffAxis
    };
}

glm::vec3 computeRotationalInertiaSphere(float mass, const ObjectBounds& sphere) {
    const float radius { sphere.mTrueVolume.mSphere.mRadius };
    const float inertia { 2.f * mass * radius * radius / 5.f };
    return glm::vec3 { inertia, inertia, inertia };
}


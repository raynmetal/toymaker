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
            updateProperties(entity);
        }
        mRequiresInitialization = false;

    // if there's only a small collection of entities which haven't undergone
    // initialization, initialize them
    } else if(!mEntitiesUninitialized.empty()) {
        const std::set<EntityID> entitiesCopy { mEntitiesUninitialized };
        for(EntityID entity: entitiesCopy) {
            updateProperties(entity);
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
    std::queue<CollisionReport> collisionReports {};

    for(auto substep { 0 }; substep < mSubsteps; ++substep) {
        integrateForces(substepInterval, previousStates);

        updateCollisionEventQueue(potentialCollisions, collisionReports);

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

    reportCollisions(collisionReports, mCollisionSignallers);
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
            mConstraintsDeleted.find(constraint) != mConstraintsDeleted.end()
            || mConstraintsInactive.find(constraint) != mConstraintsInactive.end()
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

void PhysicsSystem::updateCollisionEventQueue(
    const std::map<CollisionPair, std::unique_ptr<CollisionConstraint>>& potentialCollisions,
    std::queue<CollisionReport>& queuedReports
) {
    for(const auto& linkConstraint: potentialCollisions) {
        // skip objects that aren't configured to report collisions
        auto physicsOne { getComponent<PhysicsState>(linkConstraint.first.first()) };
        auto physicsTwo { getComponent<PhysicsState>(linkConstraint.first.second()) };
        if(!(physicsOne.signalsOnCollision() || physicsTwo.signalsOnCollision())) {
            continue;
        }

        // at least one of these objects does report collisions
        auto objectOne { getComponent<ObjectBounds>(linkConstraint.first.first()) };
        auto objectTwo { getComponent<ObjectBounds>(linkConstraint.first.second()) };
        const auto collisionData { checkCollision(
            objectOne, objectTwo
        ) };

        // update report queue based on whether a collision or separation has taken place
        if(collisionData.mCollided) {
            onCollided(linkConstraint.first, collisionData, queuedReports);
        } else {
            onSeparated(linkConstraint.first, queuedReports);
        }
    }
}

void PhysicsSystem::onEntityEnabled(EntityID entityID) {
    updateProperties(entityID);
}

void PhysicsSystem::onEntityDisabled(EntityID entityID) {
    mEntitiesUninitialized.erase(entityID);
    const auto foundEntity { mEntityConstraintMap.find(entityID) };
    if(foundEntity == mEntityConstraintMap.end()) {
        return;
    }
    for(const ConstraintID constraint: foundEntity->second) {
        mConstraintsInactive.insert(constraint);
    }
}

void PhysicsSystem::onEntityUpdated(EntityID entityID, ComponentType updatedComponent) {
    updateProperties(entityID);
}

void PhysicsSystem::unregisterConstraint(ConstraintID constraint) {
    assert(constraint < mConstraints.size() && "Invalid constraint ID specified");
    for(const auto& entity: mConstraints.at(constraint).second) {
        mEntityConstraintMap.at(entity).erase(constraint);
    }
    mConstraintsDeleted.insert(constraint);
}

void PhysicsSystem::refreshActiveConstraints() {
    mConstraintsInactive.clear();
    for(ConstraintID constraintID { 0 }; constraintID < mConstraints.size(); ++constraintID) {
        if(!isConstraintActive(constraintID)) {
            mConstraintsInactive.insert(constraintID);
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

void PhysicsSystem::updateProperties(EntityID entityID) {
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
        unregisterCollisionSignal(entityID);
        mEntitiesUninitialized.insert(entityID);
        return;
    }
    // the physics system is the One True Source for updates
    bounds.mUpdatedFromTransform = false;
    updateComponent(entityID, bounds);

    mEntitiesUninitialized.erase(entityID);

    // enable any constraints that depend on this entity if possible
    if(mConstraintsInitialized) {
        const auto foundEntityConstraint { mEntityConstraintMap.find(entityID) };
        if(foundEntityConstraint != mEntityConstraintMap.end()) {
            for(const ConstraintID constraint: foundEntityConstraint->second) {
                if(isConstraintActive(constraint)) {
                    mConstraintsInactive.erase(constraint);
                }
            }
        }
    }

    // register collision signals for this entity if required
    if(physicsProps.signalsOnCollision()) {
        registerCollisionSignal(entityID);
    } else {
        unregisterCollisionSignal(entityID);
    }

    // compute rotational inertia based on shape and mass
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

void PhysicsSystem::unregisterCollisionSignal(EntityID entity) {
    mColliding.erase(entity);
    mCollisionSignallers.erase(entity);
}

void PhysicsSystem::registerCollisionSignal(EntityID entity) {
    // already registered, nothing to do
    if(mCollisionSignallers.find(entity) != mCollisionSignallers.end()) {
        return;
    }

    SignalCollided sigCollided { std::make_shared<Signal<SignalCollidedData>>(
        *this, SignalCollidedPrefix + std::to_string(entity)
    ) };
    SignalSeparated sigSeparated { std::make_shared<Signal<SignalSeparatedData>>(
        *this, SignalSeparatedPrefix + std::to_string(entity)
    ) };

    mCollisionSignallers.insert({entity, { sigCollided, sigSeparated } });
}

bool PhysicsSystem::wasColliding(const CollisionPair& pair) const {
    const auto& collidingA { mColliding.find(pair.first()) };
    const auto& collidingB { mColliding.find(pair.second()) };

    if(collidingA != mColliding.cend()) {
        return collidingA->second.find(pair) != collidingA->second.cend();
    }
    if(collidingB != mColliding.cend()) {
        return collidingB->second.find(pair) != collidingB->second.cend();
    }

    return false;
}


void PhysicsSystem::onCollided(
    const CollisionPair& pair,
    const Collision& collision,
    std::queue<CollisionReport>& queuedReports
) {
    // skip collision reports for pairs that already were colliding
    if(wasColliding(pair)) {
        return;
    }

    // Update list of collisions known to the system
    if(mCollisionSignallers.find(pair.first()) != mCollisionSignallers.end()) {
        mColliding[pair.first()].insert(pair);
    }
    if(mCollisionSignallers.find(pair.second()) != mCollisionSignallers.end()) {
        mColliding[pair.second()].insert(pair);
    }

    // Add the collision report to the queue
    const CollisionReport newReport {
        .mCollided { true },
        .mEvent { .mCollided { pair, collision } }
    };
    queuedReports.push(newReport);
}

void PhysicsSystem::onSeparated(
    const CollisionPair& pair,
    std::queue<CollisionReport>& queuedReports
) {
    // skip collision reports for pairs that weren't colliding to begin with
    if(!wasColliding(pair)) {
        return;
    }

    // remove this pair from lists of known collisions
    if(mCollisionSignallers.find(pair.first()) != mCollisionSignallers.end()) {
        mColliding[pair.first()].erase(pair);
    }
    if(mCollisionSignallers.find(pair.second()) != mCollisionSignallers.end()) {
        mColliding[pair.second()].erase(pair);
    }

    // Add the collision report to the queue
    const CollisionReport newReport {
        .mCollided { false },
        .mEvent { .mSeparated { pair } }
    };
    queuedReports.push(newReport);
}

void PhysicsSystem::reportCollisions(
    std::queue<CollisionReport>& reports,
    std::unordered_map<EntityID, std::pair<SignalCollided, SignalSeparated>>
) const {
    while(!reports.empty()) {
        const CollisionReport report { reports.front() };
        reports.pop();
        // TODO: The first-second-first-second nonsense is terribly confusing, make simpler
        if(report.mCollided) {
            auto signalA { mCollisionSignallers.find(report.mEvent.mCollided.first.first()) };
            auto signalB { mCollisionSignallers.find(report.mEvent.mCollided.first.second()) };
            if(signalA != mCollisionSignallers.end()) {
                signalA->second.first->emit(report.mEvent.mCollided);
            }
            if(signalB != mCollisionSignallers.end()) {
                signalB->second.first->emit(report.mEvent.mCollided);
            }
        } else {
            auto signalA { mCollisionSignallers.find(report.mEvent.mSeparated.first()) };
            auto signalB { mCollisionSignallers.find(report.mEvent.mSeparated.second()) };
            if(signalA != mCollisionSignallers.end()) {
                signalA->second.second->emit(report.mEvent.mSeparated);
            }
            if(signalB != mCollisionSignallers.end()) {
                signalB->second.second->emit(report.mEvent.mSeparated);
            }
        }
    }
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


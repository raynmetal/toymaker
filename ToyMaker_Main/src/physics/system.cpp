#define GLM_ENABLE_EXPERIMENTAL

#include <map>

#include <glm/gtx/string_cast.hpp>

#include <unordered_map>

#include "toymaker/engine/spatial_query/system.hpp"
#include "toymaker/engine/spatial_query/math.hpp"

#include <toymaker/engine/physics/system.hpp>


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

    // do physics update for all eligible entities
    std::unordered_map<EntityID, PhysicsStatePartial> previousStates {};
    const float substepInterval { (static_cast<float>(timestepMillis) / static_cast<float>(mSubsteps)) / static_cast<float>(1e3) };

    std::map<ConstraintLink, CollisionConstraint> potentialCollisions {
        collectPotentialCollisions(substepInterval)
    };

    for(auto substep { 0 }; substep < mSubsteps; ++substep) {
        integrateForces(substepInterval, previousStates);

        applyCollisionConstraints(potentialCollisions, substepInterval);
        // TODO: Implement other contraint solvers

        deriveVelocities(substepInterval, previousStates);
    }

    // clear forces (which are expected to be applied per frame)
    for(const auto entity: getEnabledEntities()) {
        auto physicsState { getComponent<PhysicsLocal>(entity) };
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
        PhysicsLocal physics { getComponent<PhysicsLocal>(entity) };
        ObjectBounds bounds { getComponent<ObjectBounds>(entity) };
        const PhysicsStatePartial physicsState {
            .mVelocity { physics.mVelocity },
            .mAngularVelocity { physics.mAngularVelocity },
            .mPosition { bounds.getPositionWorld() },
            .mOrientation  { bounds.getOrientationWorld() },
        };
        previousStates[entity] = physicsState;

        // update object state per forces - linear
        physics.mVelocity += substepSeconds * physics.mForce / physics.mMass;
        bounds.setPositionWorld(
            physicsState.mPosition
            // velocity is relative to local frame, and must be rotated
            + substepSeconds * (physicsState.mOrientation * physics.mVelocity)
        );

        // update object state per forces - angular
        const glm::mat3 inertiaTensor {
            glm::mat3 { glm::scale(glm::mat4{ 1.f }, physics.mRotationalInertia) }
        };
        physics.mAngularVelocity += substepSeconds * (
            glm::inverse(inertiaTensor) * (
                physics.mTorque - glm::cross(
                    physics.mAngularVelocity,
                    inertiaTensor * physics.mAngularVelocity
                )
            )
        );
        const float deltaTheta {
            substepSeconds * glm::length(physics.mAngularVelocity)
        };
        // only for sensible angular velocities
        if(glm::length(physics.mAngularVelocity) != 0.f) {
            const glm::vec3 normalizedAngularVelocity {
                physicsState.mOrientation * glm::normalize(physics.mAngularVelocity)
            };
            const glm::quat orientationUpdate {
                glm::angleAxis(deltaTheta, normalizedAngularVelocity)
            };
            const auto newOrientation {
                glm::normalize(orientationUpdate * physicsState.mOrientation)
            };
            bounds.setOrientationWorld(
                newOrientation
            );
        }

        // push updates to component table
        updateComponent(entity, physics);
        updateComponent(entity, bounds);
    }
}

std::map<ConstraintLink, CollisionConstraint> PhysicsSystem::collectPotentialCollisions(float substepSeconds) {
    std::map<ConstraintLink, CollisionConstraint> potentialCollisions {};
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
        const PhysicsLocal physicsLocal { getComponent<PhysicsLocal>(entity) };
        const glm::vec3 globalVelocity { objectBounds.getOrientationWorld() * physicsLocal.mVelocity };
        AxisAlignedBounds endSearch { startSearch };
        endSearch.setPosition(
            endSearch.getPositionWorld() + substepSeconds * globalVelocity
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
        for(const auto& candidate: candidates) {
            // self-collisions, collisions with non-physical objects are invalid
            if(
                candidate.first == entity
                || enabledEntities.find(candidate.first) == enabledEntities.end()
            ) {
                continue;
            }

            // skip already discovered collision constraints
            const ConstraintLink link { entity, candidate.first };
            if(
                potentialCollisions.find(link) != potentialCollisions.end()
            ) {
                continue;
            }

            potentialCollisions.emplace(
                link,
                CollisionConstraint {
                    checkCollision(
                        objectBounds,
                        getComponent<ObjectBounds>(candidate.first)
                    ),
                    getComponent<PhysicsLocal>(entity),
                    objectBounds,
                    getComponent<PhysicsLocal>(candidate.first),
                    getComponent<ObjectBounds>(candidate.first)
                }
            );
        }
    }
    return potentialCollisions;
}

void PhysicsSystem::applyCollisionConstraints(
    std::map<ConstraintLink, CollisionConstraint>& constraints,
    float substepSeconds
) {
    for(auto linkConstraint: constraints) {
        // retrieve data
        auto objectOne { getComponent<ObjectBounds>(linkConstraint.first.first()) };
        auto objectTwo { getComponent<ObjectBounds>(linkConstraint.first.second()) };
        auto physicsOne { getComponent<PhysicsLocal>(linkConstraint.first.first()) };
        auto physicsTwo { getComponent<PhysicsLocal>(linkConstraint.first.second()) };
        const auto collisionData { checkCollision(
            objectOne, objectTwo
        ) };
        const BaseConstraint::ParticipantTable participantTable {
            { 0, { objectOne, physicsOne }},
            { 1, { objectTwo, physicsTwo }}
        };

        // apply constraint
        linkConstraint.second.updateCollisionData(
            collisionData,
            physicsOne, objectOne,
            physicsTwo, objectTwo
        );
        linkConstraint.second.applyConstraint(participantTable, substepSeconds);

        // push data to component arrays
        updateComponent<ObjectBounds>(linkConstraint.first.first(), objectOne);
        updateComponent<ObjectBounds>(linkConstraint.first.second(), objectTwo);
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
        auto physicsProps { getComponent<PhysicsLocal>(entity) };
        const auto bounds { getComponent<ObjectBounds>(entity) };
        const auto orientationInverse { glm::inverse(bounds.getOrientationWorld()) };
        // update linear terms
        physicsProps.mVelocity = (
            orientationInverse // in local frame
            * (bounds.getPositionWorld() - previousState.mPosition)
            / substepSeconds
        );

        // update angular terms
        const glm::quat deltaOrientation {
            (bounds.getOrientationWorld() * glm::inverse(previousState.mOrientation))
        };
        physicsProps.mAngularVelocity = orientationInverse * (
            2.f * glm::vec3 {
                deltaOrientation.x, deltaOrientation.y, deltaOrientation.z
            } / substepSeconds
        );
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
}

void PhysicsSystem::onEntityUpdated(EntityID entityID, ComponentType updatedComponent) {
    updatePhysicsProperties(entityID);
}

void PhysicsSystem::updatePhysicsProperties(EntityID entityID) {
    auto physicsProps { getComponent<PhysicsLocal>(entityID) };
    assert((
        isPositiveStrict(physicsProps.mMass)
    ) && "Entity must have non-zero positive mass");

    // senseless bounds usually mean that spatial query system has not
    // initialized bounds for this entity, wait for next timestep
    const auto bounds { getComponent<ObjectBounds>(entityID) };
    if(!bounds.isSensible()) {
        mEntitiesUninitialized.insert(entityID);
        return;
    }
    mEntitiesUninitialized.erase(entityID);

    switch(bounds.mType) {
        case ToyMaker::ObjectBounds::TrueVolumeType::BOX:
            physicsProps.mRotationalInertia = computeRotationalInertiaBox(
                physicsProps.mMass, bounds
            );
            break;
        case ToyMaker::ObjectBounds::TrueVolumeType::CAPSULE:
            physicsProps.mRotationalInertia = computeRotationalInertiaCapsule(
                physicsProps.mMass, bounds
            );
            break;
        case ToyMaker::ObjectBounds::TrueVolumeType::SPHERE:
            physicsProps.mRotationalInertia = computeRotationalInertiaSphere(
                physicsProps.mMass, bounds
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


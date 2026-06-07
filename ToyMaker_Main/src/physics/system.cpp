#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <unordered_map>

#include <toymaker/engine/physics/system.hpp>

using namespace ToyMaker;

glm::vec3 computeRotationalInertiaBox(float mass, const ObjectBounds& cuboid);
glm::vec3 computeRotationalInertiaCapsule(float mass, const ObjectBounds& capsule);
glm::vec3 computeRotationalInertiaSphere(float mass, const ObjectBounds& sphere);

void PhysicsSystem::onSimulationActivated() {
    mRequiresInitialization = true;
}

// storage for intermediate physics state
struct PhysicsStatePartial {
    glm::vec3 mVelocity;
    glm::vec3 mAngularVelocity;
    glm::vec3 mPosition;
    glm::quat mOrientation;
};

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
    for(auto substep { 0 }; substep < mSubsteps; ++substep) {
        for(const EntityID entity: getEnabledEntities()) {

            // filter out uninitialized entities
            if(mEntitiesUninitialized.find(entity) != mEntitiesUninitialized.end()) {
                continue;
            }

            // snapshot current object state
            PhysicsState physics { getComponent<PhysicsState>(entity) };
            ObjectBounds bounds { getComponent<ObjectBounds>(entity) };
            const PhysicsStatePartial physicsState {
                .mVelocity { physics.mVelocity },
                .mAngularVelocity { physics.mAngularVelocity },
                .mPosition { bounds.getPositionWorld() },
                .mOrientation  { bounds.getOrientationWorld() },
            };
            previousStates[entity] = physicsState;


            // update object state per forces - linear
            physics.mVelocity += substepInterval * physics.mForce / physics.mMass;
            bounds.setPositionWorld(
                physicsState.mPosition
                // velocity is relative to local frame, and must be rotated
                + substepInterval * (physicsState.mOrientation * physics.mVelocity)
            );

            // update object state per forces - angular
            const glm::mat3 inertiaTensor {
                glm::mat3 { glm::scale(glm::mat4{ 1.f }, physics.mRotationalInertia) }
            };
            physics.mAngularVelocity += substepInterval * (
                glm::inverse(inertiaTensor) * (
                    physics.mTorque - glm::cross(
                        physics.mAngularVelocity,
                        inertiaTensor * physics.mAngularVelocity
                    )
                )
            );
            const float deltaTheta {
                substepInterval * glm::length(physics.mAngularVelocity)
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

        // TODO: Implement contraint solvers

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
                orientationInverse // in local frame
                * (bounds.getPositionWorld() - previousState.mPosition)
                / substepInterval
            );

            // update angular terms
            const glm::quat deltaOrientation {
                (bounds.getOrientationWorld() * glm::inverse(previousState.mOrientation))
            };
            physicsProps.mAngularVelocity = orientationInverse * (
                2.f * glm::vec3 {
                    deltaOrientation.x, deltaOrientation.y, deltaOrientation.z
                } / substepInterval
            );
            physicsProps.mAngularVelocity *= deltaOrientation.w >= 0.f ? 1.f : -1.f;

            // push updates to component table
            updateComponent(entity, physicsProps);
        }
    }

    // clear forces (which are expected to be applied per frame)
    for(const auto entity: getEnabledEntities()) {
        auto physicsState { getComponent<PhysicsState>(entity) };
        physicsState.mForce = glm::vec3 { 0.f };
        physicsState.mTorque = glm::vec3 { 0.f };
        updateComponent(entity, physicsState);
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
    auto physicsProps { getComponent<PhysicsState>(entityID) };
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


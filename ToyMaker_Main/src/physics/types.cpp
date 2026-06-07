
#include <toymaker/engine/physics/types.hpp>

using namespace ToyMaker;

void PhysicsState::applyForceLocal(const glm::vec3& force, const glm::vec3& atPosition) {
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

void PhysicsState::applyForceGlobal(const glm::vec3& force, const glm::vec3& atPosition, const ObjectBounds& bounds) {
    const glm::vec3 objectPosition { bounds.getPositionWorld() };
    const glm::quat objectRotationInverse { glm::inverse(bounds.getOrientationWorld()) };

    const glm::vec3 positionDelta { atPosition - objectPosition };

    const glm::vec3 positionLocal { objectRotationInverse * positionDelta };
    const glm::vec3 forceLocal { objectRotationInverse * force };

    applyForceLocal(forceLocal, positionLocal);
}


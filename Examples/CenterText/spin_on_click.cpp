#include <toymaker/engine/spatial_query/types.hpp>
#include <toymaker/engine/physics/types.hpp>

#include "spin_on_click.hpp"

std::shared_ptr<ToyMaker::BaseSimObjectAspect> SpinOnClick::create(const nlohmann::json& jsonAspectProperties) {
    (void) jsonAspectProperties; // prevent unused parameter warning
    return std::shared_ptr<SpinOnClick>(new SpinOnClick{});
}

bool SpinOnClick::onPointerLeftClick(glm::vec4 clickLocation) {
    const auto currentBounds { getComponent<ToyMaker::ObjectBounds>() };
    auto currentState { getComponent<ToyMaker::PhysicsState>() };

    currentState.applyForceGlobal(glm::vec3 { 0.f, 0.f, -1.f }, glm::vec3 { clickLocation }, currentBounds);

    // keep only the rotational component ...
    currentState.mForce = glm::vec3 { 0.f };

    // ... around the y axis
    currentState.mTorque.x = 0.f;
    currentState.mTorque.z = 0.f;

    updateComponent(currentState);
    return true;
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> SpinOnClick::clone() const {
    return std::shared_ptr<SpinOnClick>(new SpinOnClick{});
}


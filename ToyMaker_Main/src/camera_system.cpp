#include <algorithm>
#include <set>

#include "toymaker/engine/scene_system.hpp"
#include "toymaker/engine/camera_system.hpp"

using namespace ToyMaker;

const float MAX_FOV {90.f};
const float MIN_FOV {40.f};

void CameraSystem::updateActiveCameraMatrices() {
    std::set<EntityID> enabledEntities { getEnabledEntities() };

    // Get and clear list of cameras awaiting a view or projection update
    std::set<EntityID> requiresProjectionUpdate{};
    std::set<EntityID> requiresViewUpdate{};
    std::swap(requiresProjectionUpdate, mProjectionUpdateQueue);
    std::swap(requiresViewUpdate, mViewUpdateQueue);

    // Apply pending updates
    for(EntityID entity: requiresProjectionUpdate) {
        CameraProperties cameraProperties { getComponent<CameraProperties>(entity) };
        switch(cameraProperties.mProjectionType) {
            case CameraProperties::ProjectionType::FRUSTUM:
                assert(cameraProperties.mNearFarPlanes.x > 0.f && cameraProperties.mNearFarPlanes.y > cameraProperties.mNearFarPlanes.x
                    && "For frustum cameras, near and far planes must both be greater than 0, with the \
                    far plane having a higher value than the near plane");
                cameraProperties.mProjectionMatrix = glm::perspective(
                    glm::radians(glm::clamp(cameraProperties.mFov, MIN_FOV, MAX_FOV)), 
                    cameraProperties.mAspect,
                    cameraProperties.mNearFarPlanes.x, cameraProperties.mNearFarPlanes.y
                );
            break;
            case CameraProperties::ProjectionType::ORTHOGRAPHIC:
                cameraProperties.mProjectionMatrix = glm::ortho(
                    -cameraProperties.mOrthographicDimensions.x/2.f, cameraProperties.mOrthographicDimensions.x/2.f,
                    -cameraProperties.mOrthographicDimensions.y/2.f, cameraProperties.mOrthographicDimensions.y/2.f,
                    cameraProperties.mNearFarPlanes.x, cameraProperties.mNearFarPlanes.y
                );
            break;
        }
        updateComponent<CameraProperties>(entity, cameraProperties);
    }
    for(EntityID entity: requiresViewUpdate) {
        CameraProperties cameraProperties { getComponent<CameraProperties>(entity) };
        cameraProperties.mViewMatrix = glm::inverse(getComponent<Transform>(entity).mModelMatrix);
        updateComponent<CameraProperties>(entity, cameraProperties);
    }
}

void CameraSystem::onEntityEnabled(EntityID entity) {
    mViewUpdateQueue.insert(entity);
    mProjectionUpdateQueue.insert(entity);
}
void CameraSystem::onEntityDisabled(EntityID entity) {
    mViewUpdateQueue.erase(entity);
    mProjectionUpdateQueue.erase(entity);
}

void CameraSystem::onEntityUpdated(EntityID entity)  {
    mViewUpdateQueue.insert(entity);
    mProjectionUpdateQueue.insert(entity);
}

void CameraSystem::onPreRenderStep(float simulationProgress) {
    (void)simulationProgress; // prevent unused parameter warnings
    updateActiveCameraMatrices();
}

void CameraSystem::onSimulationActivated() {
    for(EntityID entity : getEnabledEntities()) {
        mViewUpdateQueue.insert(entity);
        mProjectionUpdateQueue.insert(entity);
    }
    updateActiveCameraMatrices();
}

#include <limits>

#include "toymaker/engine/spatial_query/system.hpp"

using namespace ToyMaker;

std::vector<std::shared_ptr<SceneNodeCore>> SpatialQuerySystem::findNodesOverlappingCoarse(const Ray& searchRay, InteractionLayerMask interactionMask) {
    const std::vector<std::pair<EntityID, AxisAlignedBounds>> intersectingEntityIDs { findEntitiesOverlappingCoarse(searchRay, interactionMask) };

    const std::size_t resultListLength { intersectingEntityIDs.size() };
    std::vector<UniversalEntityID> resultNodeQuery { resultListLength };

    for(std::size_t resultIndex {0}; resultIndex < resultListLength; ++resultIndex) {
        resultNodeQuery[resultIndex] = { mWorld.lock()->getID(), intersectingEntityIDs[resultIndex].first };
    }

    return mWorld.lock()->getSystem<SceneSystem>()->getNodesByID(resultNodeQuery);
}

std::vector<std::shared_ptr<SceneNodeCore>> SpatialQuerySystem::findNodesOverlappingCoarse(const AxisAlignedBounds& searchBounds, InteractionLayerMask interactionMask) {
    const std::vector<std::pair<EntityID, AxisAlignedBounds>> intersectingEntityIDs { findEntitiesOverlappingCoarse(searchBounds, interactionMask) };

    const std::size_t resultListLength { intersectingEntityIDs.size() };
    std::vector<UniversalEntityID> resultNodeQuery { resultListLength };

    for(std::size_t resultIndex {0}; resultIndex < resultListLength; ++resultIndex) {
        resultNodeQuery[resultIndex] = { mWorld.lock()->getID(), intersectingEntityIDs[resultIndex].first };
    }

    return mWorld.lock()->getSystem<SceneSystem>()->getNodesByID(resultNodeQuery);
}

std::vector<std::pair<EntityID, AxisAlignedBounds>> SpatialQuerySystem::findEntitiesOverlappingCoarse(const AxisAlignedBounds& searchBounds, InteractionLayerMask interactionMask) const {
    if(mRequiresInitialization) {
        return std::vector<std::pair<EntityID, AxisAlignedBounds>>{};
    }
    return mOctree->findEntitiesOverlappingCoarse(searchBounds, interactionMask);
}

std::vector<std::pair<EntityID, AxisAlignedBounds>> SpatialQuerySystem::findEntitiesOverlappingCoarse(const Ray& searchRay, InteractionLayerMask interactionMask) const {
    if(mRequiresInitialization) {
        return std::vector<std::pair<EntityID, AxisAlignedBounds>>{};
    }
    return mOctree->findEntitiesOverlappingCoarse(searchRay, interactionMask);
}

void SpatialQuerySystem::StaticModelBoundsComputeSystem::onEntityEnabled(EntityID entityID) {
    const ObjectBounds bounds { getComponent<ObjectBounds>(entityID) };
    if(!bounds.mVolumeSystemComputed || !bounds.mVolumeUpdateRequired) {
        return;
    }
    recomputeObjectBounds(entityID);
}

void SpatialQuerySystem::StaticModelBoundsComputeSystem::onEntityUpdated(EntityID entityID, ComponentType updatedComponent) {
    const ObjectBounds bounds { getComponent<ObjectBounds>(entityID) };
    // TODO: Logic for object bound recomputation is scattered, find a way to simplify
    if(
        !bounds.mVolumeSystemComputed
        // only update bounding volume when requested, or when a related component has been updated
        || !(
            bounds.mVolumeUpdateRequired
            || updatedComponent != mWorld.lock()->getComponentType<ObjectBounds>()
        )
    ) {
        return;
    }
    recomputeObjectBounds(entityID);
}

void SpatialQuerySystem::StaticModelBoundsComputeSystem::recomputeObjectBounds(EntityID entityID) {
    ObjectBounds objectBounds { getComponent<ObjectBounds>(entityID) };
    const glm::mat3 objectScale { getComponent<Transform>(entityID).getMatrixScale() };
    const std::shared_ptr<StaticModel> model { getComponent<std::shared_ptr<StaticModel>>(entityID) };

    objectBounds.mVolumeUpdateRequired = false;
    objectBounds.mInteractionLayers = 1 << (sizeof(InteractionLayerMask) * 4 - 1);
    objectBounds.mInteractionMask = 0;

    // Only the transform has been updated -- simply resize OOBB accordingly
    if(!model->boundsNeedRecompute() && objectBounds.mUpdatedFromTransform) {
        const ObjectBounds modelBounds { model->getBounds() };
        objectBounds.mTrueVolume.mBox.mDimensions = objectScale * modelBounds.mTrueVolume.mBox.mDimensions;
        objectBounds.setPositionOffset(objectScale * modelBounds.mPositionOffset);
        updateComponent<ObjectBounds>(entityID, objectBounds);
        return;
    }

    glm::vec3 minCorner {std::numeric_limits<float>::infinity()};
    glm::vec3 maxCorner {-std::numeric_limits<float>::infinity()};
    std::vector<std::shared_ptr<StaticMesh>> meshHandles { model->getMeshHandles() };
    if(meshHandles.empty()) {
        minCorner = glm::vec3 { 0.f };
        maxCorner = glm::vec3 { 0.f };

    } else {
        for(auto meshHandle: model->getMeshHandles()) {
            assert(meshHandle->getVertexListBegin() != meshHandle->getVertexListEnd() && "Cannot compute bounding volume of an empty mesh");
            for(
                auto vertexIter { meshHandle->getVertexListBegin()};
                vertexIter != meshHandle->getVertexListEnd();
                ++vertexIter
            ) {
                minCorner = glm::min(static_cast<glm::vec3>(vertexIter->mPosition), minCorner);
                maxCorner = glm::max(static_cast<glm::vec3>(vertexIter->mPosition), maxCorner);
            }
        }
    }

    VolumeBox box { .mDimensions{ maxCorner - minCorner } };

    // Make sure that objects with 1 or 2 dimensions still get bounding boxes with 
    // 3 dimensions
    if(glm::length(box.mDimensions) > 0.f && (
        box.mDimensions.x == 0.f
        || box.mDimensions.y == 0.f
        || box.mDimensions.z == 0.f
    )) {
        const glm::vec3 epsilonOffsets {
            maxCorner.x > minCorner.x? 0.f: std::numeric_limits<float>::epsilon() * (maxCorner.x? glm::abs(maxCorner.x): 1.f),
            maxCorner.y > minCorner.y? 0.f: std::numeric_limits<float>::epsilon() * (maxCorner.y? glm::abs(maxCorner.y): 1.f),
            maxCorner.z > minCorner.z? 0.f: std::numeric_limits<float>::epsilon() * (maxCorner.z? glm::abs(maxCorner.z): 1.f),
        };
        maxCorner += epsilonOffsets;
        minCorner -= epsilonOffsets;
        assert(
            maxCorner.x > minCorner.x && maxCorner.y > minCorner.y && maxCorner.z > minCorner.z 
            && "We should expect max corner to be greater than min corner in \
            every direction with this adjustment"
        );
        box.mDimensions = maxCorner - minCorner;
    }

    // cache untransformed bounds in the static model itself, for later use
    objectBounds.mTrueVolume.mBox = box;
    objectBounds.setPositionOffset(minCorner + .5f * box.mDimensions);
    objectBounds.mType = ObjectBounds::TrueVolumeType::BOX;
    model->setBounds(objectBounds);

    // apply the object's current scale to the model before setting the bounds on the entity
    if(objectBounds.mUpdatedFromTransform) {
        objectBounds.mTrueVolume.mBox.mDimensions = objectScale * objectBounds.mTrueVolume.mBox.mDimensions;
        objectBounds.setPositionOffset(objectScale * objectBounds.mPositionOffset);
    }

    objectBounds.recomputeWorldPositionOrientation();
    updateComponent<ObjectBounds>(entityID, objectBounds);
}

void SpatialQuerySystem::LightBoundsComputeSystem::onEntityEnabled(EntityID entityID) {
    const ObjectBounds bounds { getComponent<ObjectBounds>(entityID) };
    if(!bounds.mVolumeSystemComputed) {
        return;
    }
    recomputeObjectBounds(entityID);
}

void SpatialQuerySystem::LightBoundsComputeSystem::onEntityUpdated(EntityID entityID, ComponentType updatedComponent) {
    const ObjectBounds bounds { getComponent<ObjectBounds>(entityID) };
    if(
        !bounds.mVolumeSystemComputed
        // Restrict bounds updates to when requested by spatial query system, or when a related component
        // has been updated
        || !(
            bounds.mVolumeUpdateRequired
            || updatedComponent != mWorld.lock()->getComponentType<ObjectBounds>()
        )
    ) {
        return;
    }
    recomputeObjectBounds(entityID);
}

void SpatialQuerySystem::LightBoundsComputeSystem::recomputeObjectBounds(EntityID entityID) {
    auto objectBounds { getComponent<ObjectBounds>(entityID) };
    auto lightEmissionData { getComponent<LightEmissionData>(entityID) };
    objectBounds.mVolumeUpdateRequired = false;
    objectBounds.mTrueVolume.mSphere.mRadius = lightEmissionData.mType == LightEmissionData::LightType::directional?
        0.f:
        lightEmissionData.mRadius;
    objectBounds.mType = ObjectBounds::TrueVolumeType::SPHERE;
    objectBounds.setOrientationOffset(glm::vec3{ 0.f });
    objectBounds.setPositionOffset(glm::vec3{ 0.f });
    objectBounds.mInteractionLayers = 1 << (sizeof(InteractionLayerMask) * 4 - 1);
    objectBounds.mInteractionMask = 0;
    updateComponent(entityID, objectBounds);
}

void SpatialQuerySystem::updateBounds(EntityID entity, bool forceTransformUpdate) {
    // Compute new object position based on its transform, if necessary
    ObjectBounds objectBounds { getComponent<ObjectBounds>(entity) };
    if(objectBounds.mUpdatedFromTransform || forceTransformUpdate) {
        const glm::mat4 modelMatrix { getComponent<Transform>(entity).mModelMatrix };
        objectBounds.applyModelMatrix(modelMatrix);
        updateComponent<ObjectBounds>(entity, objectBounds);
    }

    // Compute axis aligned bounds based on object bounds
    const AxisAlignedBounds axisAlignedBounds { objectBounds };

    // Apply updates
    updateComponent<AxisAlignedBounds>(entity, axisAlignedBounds);
}

void SpatialQuerySystem::updateTransform(EntityID entity) {
    ObjectBounds updatedBounds { getComponent<ObjectBounds>(entity) };
    if(!updatedBounds.mTransformUpdateRequired) {
        return;
    }
    updatedBounds.mTransformUpdateRequired = false;

    Transform newTransform { getComponent<Transform>(entity) };
    // entities whose transforms are set by their object bounds should
    // only have their translation and rotation inherited
    newTransform.mInheritedComponents = (TRANSFORMCOMPONENT_TRANSLATION | TRANSFORMCOMPONENT_ROTATION);
    newTransform.mModelMatrix = updatedBounds.getTranslationTransformOrigin() * updatedBounds.getRotationTransformOrigin();
    updateComponent(entity, newTransform);
    updateComponent(entity, updatedBounds);
}

void SpatialQuerySystem::rebuildOctree() {
    // one pass to transform all object positions and orientations, and simultaneously compute
    // axis aligned bounding boxes
    AxisAlignedBounds regionToEncompass {glm::vec3{0.f}, glm::vec3{0.f}};
    bool firstObject { true };
    for(EntityID entity: getEnabledEntities()) {
        updateBounds(entity, mRequiresInitialization);
        if(firstObject) {
            regionToEncompass.setPosition(getComponent<ObjectBounds>(entity).getPositionWorld());
            firstObject = false;
        }
        regionToEncompass = regionToEncompass + getComponent<AxisAlignedBounds>(entity);
    }
    assert(isFinite(regionToEncompass.getPositionWorld()) && "Start position must be finite");
    assert(isFinite(regionToEncompass.getDimensions()) && "Region to encompass is too large to be bound in an octree");
    if(!regionToEncompass.isPositiveStrict()) {
        regionToEncompass.setDimensions(glm::vec3{1.f});
    }

    // another pass to create and populate our octree
    mOctree = std::make_unique<Octree>(8, regionToEncompass);
    for(EntityID entity: getEnabledEntities()) {
        mOctree->insertEntity(entity, getComponent<AxisAlignedBounds>(entity));
    }
}

void SpatialQuerySystem::onEntityEnabled(EntityID entityID) {
    mUpdateQueueAABB.insert(entityID);
}

void SpatialQuerySystem::onEntityDisabled(EntityID entityID) {
    mUpdateQueueAABB.erase(entityID);
    if(!mRequiresInitialization) { mOctree->removeEntity(entityID); }
}

void SpatialQuerySystem::onEntityUpdated(EntityID entityID, ComponentType updatedComponent) {
    mUpdateQueueAABB.insert(entityID);

    if(updatedComponent == mWorld.lock()->getComponentType<ObjectBounds>()) {
        mUpdateQueueTransform.insert(entityID);
    }
}

void SpatialQuerySystem::onSimulationActivated() {
    mRequiresInitialization = true;
}

void SpatialQuerySystem::onSimulationPostStep(uint32_t timestepMillis) {
    (void)timestepMillis; // prevent unused parameter warning
    if(mRequiresInitialization)  {
        mUpdateQueueAABB.clear();
        rebuildOctree();
        mRequiresInitialization = false;
        return;
    }

    for(EntityID entity: mUpdateQueueTransform) {
        updateTransform(entity);
    }
    mUpdateQueueTransform.clear();
}

void SpatialQuerySystem::onPostTransformUpdate(uint32_t timestepMillis) {
    (void)timestepMillis; // prevent unused parameter warning
    if(mRequiresInitialization)  {
        mUpdateQueueAABB.clear();
        rebuildOctree();
        mRequiresInitialization = false;
        return;
    }

    for(EntityID entity: mUpdateQueueAABB) {
        mOctree->removeEntity(entity);
        updateBounds(entity);
        mOctree->insertEntity(entity, getComponent<AxisAlignedBounds>(entity));
    }
    mUpdateQueueAABB.clear();
}

void SpatialQuerySystem::updateBoundingVolume(
    EntityID entity,
    ObjectBounds::TrueVolumeType volumeType,
    const ObjectBounds::TrueVolume& volume,
    const glm::vec3& positionOffset,
    const glm::quat& orientationOffset
) {
    const auto& enabledEntities { getEnabledEntities() };
    assert(enabledEntities.find(entity) != enabledEntities.end() && "This entity is not managed by this system");
    switch(volumeType) {
        case ObjectBounds::TrueVolumeType::BOX:
            assert(volume.mBox.isSensible() && "Invalid box dimensions specified");
            break;
        case ObjectBounds::TrueVolumeType::CAPSULE:
            assert(volume.mCapsule.isSensible() && "Invalid capsule parameters specified");
            break;
        case ObjectBounds::TrueVolumeType::SPHERE:
            assert(volume.mCapsule.isSensible() && "Invalid sphere parameters specified");
            break;
        default:
            assert(false && "Unrecognized volume type specified");
    }
    assert(
        isNumber(positionOffset) && isFinite(positionOffset)
        && "Invalid position offset specified"
    );
    assert(
        isNumber(orientationOffset.w) && isFinite(orientationOffset.w)
        && isNumber(orientationOffset.x) && isFinite(orientationOffset.x)
        && isNumber(orientationOffset.y) && isFinite(orientationOffset.y)
        && isNumber(orientationOffset.z) && isFinite(orientationOffset.z)
        && (
            orientationOffset.w != 0.f
            || orientationOffset.x != 0.f
            || orientationOffset.y != 0.f
            || orientationOffset.z != 0.f
       ) && "Invalid orientation offset specified"
    );

    ObjectBounds bounds { getComponent<ObjectBounds>(entity) };

    bounds.mTrueVolume = volume;
    bounds.mType = volumeType;
    bounds.setPositionOffset(positionOffset);
    bounds.setOrientationOffset(orientationOffset);
    bounds.mVolumeSystemComputed = false;

    updateComponent<ObjectBounds>(entity, bounds);
}

void SpatialQuerySystem::resetBoundsOverride(EntityID entity) {
    const auto& enabledEntities { getEnabledEntities() };
    assert(enabledEntities.find(entity) != enabledEntities.end() && "This entity is not managed by this system");

    ObjectBounds bounds { getComponent<ObjectBounds>(entity) };
    if(bounds.mVolumeSystemComputed) {
        return;
    }

    bounds.mVolumeSystemComputed = true;
    bounds.mVolumeUpdateRequired = true;
    updateComponent(entity, bounds);
}



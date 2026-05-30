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
    recomputeObjectBounds(entityID);
}

void SpatialQuerySystem::StaticModelBoundsComputeSystem::onEntityUpdated(EntityID entityID) {
    recomputeObjectBounds(entityID);
}

void SpatialQuerySystem::StaticModelBoundsComputeSystem::recomputeObjectBounds(EntityID entityID) {
    ObjectBounds objectBounds { getComponent<ObjectBounds>(entityID) };
    const glm::mat3 objectScale { getComponent<Transform>(entityID).getMatrixScale() };

    const std::shared_ptr<StaticModel> model { getComponent<std::shared_ptr<StaticModel>>(entityID) };

    // Only the transform has been updated -- simply resize OOBB accordingly
    if(!model->boundsNeedRecompute()) {
        objectBounds.mTrueVolume.mBox.mDimensions = objectScale * model->getBounds().mTrueVolume.mBox.mDimensions;
        updateComponent<ObjectBounds>(entityID, objectBounds);
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
    objectBounds = ObjectBounds::create(
        box,
        minCorner + .5f * box.mDimensions,
        glm::vec3{0.f, 0.f, 0.f}
    );
    model->setBounds(objectBounds);

    // apply the object's current scale to the model before setting the bounds on the entity
    objectBounds.mTrueVolume.mBox.mDimensions = objectScale * model->getBounds().mTrueVolume.mBox.mDimensions;
    updateComponent<ObjectBounds>(entityID, objectBounds);
}

void SpatialQuerySystem::LightBoundsComputeSystem::onEntityEnabled(EntityID entityID) {
    recomputeObjectBounds(entityID);
}

void SpatialQuerySystem::LightBoundsComputeSystem::onEntityUpdated(EntityID entityID) {
    recomputeObjectBounds(entityID);
}

void SpatialQuerySystem::LightBoundsComputeSystem::recomputeObjectBounds(EntityID entityID) {
    ObjectBounds objectBounds { getComponent<ObjectBounds>(entityID) };

    LightEmissionData lightEmissionData { getComponent<LightEmissionData>(entityID) };
    objectBounds.mTrueVolume.mSphere.mRadius = lightEmissionData.mType == LightEmissionData::LightType::directional?
        0.f:
        lightEmissionData.mRadius;
    objectBounds.mType = ObjectBounds::TrueVolumeType::SPHERE;
    objectBounds.mOrientationOffset = glm::vec3{0.f};
    objectBounds.mPositionOffset = glm::vec3{0.f};

    updateComponent<ObjectBounds>(entityID, objectBounds);
}


void SpatialQuerySystem::updateBounds(EntityID entity) {
    // Compute new object position based on its transform
    const glm::mat4 modelMatrix { getComponent<Transform>(entity).mModelMatrix };
    ObjectBounds objectBounds { getComponent<ObjectBounds>(entity) };
    objectBounds.applyModelMatrix(modelMatrix);

    // Compute axis aligned bounds based on object bounds
    const AxisAlignedBounds axisAlignedBounds { objectBounds };

    // Apply updates
    updateComponent<ObjectBounds>(entity, objectBounds);
    updateComponent<AxisAlignedBounds>(entity, axisAlignedBounds);   
}

void SpatialQuerySystem::rebuildOctree() {
    // one pass to transform all object positions and orientations, and simultaneously compute 
    // axis aligned bounding boxes
    AxisAlignedBounds regionToEncompass {glm::vec3{0.f}, glm::vec3{0.f}};
    bool firstObject { true };
    for(EntityID entity: getEnabledEntities()) {
        updateBounds(entity);
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

void SpatialQuerySystem::onEntityUpdated(EntityID entityID) {
    mUpdateQueueAABB.insert(entityID);
}

void SpatialQuerySystem::onSimulationActivated() {
    mRequiresInitialization = true;
}

void SpatialQuerySystem::onSimulationStep(uint32_t timestepMillis) {
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


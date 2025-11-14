#include <limits>
#include <cmath>

#include "toymaker/engine/spatial_query_system.hpp"

using namespace ToyMaker;

std::vector<std::shared_ptr<SceneNodeCore>> SpatialQuerySystem::findNodesOverlapping(const Ray& searchRay) {
    const std::vector<std::pair<EntityID, AxisAlignedBounds>> intersectingEntityIDs { findEntitiesOverlapping(searchRay) };

    const std::size_t resultListLength { intersectingEntityIDs.size() };
    std::vector<UniversalEntityID> resultNodeQuery { resultListLength };

    for(std::size_t resultIndex {0}; resultIndex < resultListLength; ++resultIndex) {
        resultNodeQuery[resultIndex] = { mWorld.lock()->getID(), intersectingEntityIDs[resultIndex].first };
    }

    return mWorld.lock()->getSystem<SceneSystem>()->getNodesByID(resultNodeQuery);
}

std::vector<std::shared_ptr<SceneNodeCore>> SpatialQuerySystem::findNodesOverlapping(const AxisAlignedBounds& searchBounds) {
    const std::vector<std::pair<EntityID, AxisAlignedBounds>> intersectingEntityIDs { findEntitiesOverlapping(searchBounds) };

    const std::size_t resultListLength { intersectingEntityIDs.size() };
    std::vector<UniversalEntityID> resultNodeQuery { resultListLength };

    for(std::size_t resultIndex {0}; resultIndex < resultListLength; ++resultIndex) {
        resultNodeQuery[resultIndex] = { mWorld.lock()->getID(), intersectingEntityIDs[resultIndex].first };
    }

    return mWorld.lock()->getSystem<SceneSystem>()->getNodesByID(resultNodeQuery);
}


std::vector<std::pair<EntityID, AxisAlignedBounds>> SpatialQuerySystem::findEntitiesOverlapping(const AxisAlignedBounds& searchBounds) const {
    if(mRequiresInitialization) {
        return std::vector<std::pair<EntityID, AxisAlignedBounds>>{};
    }

    return mOctree->findEntitiesOverlapping(searchBounds);
}

std::vector<std::pair<EntityID, AxisAlignedBounds>> SpatialQuerySystem::findEntitiesOverlapping(const Ray& searchRay) const {
    if(mRequiresInitialization) {
        return std::vector<std::pair<EntityID, AxisAlignedBounds>>{};
    }

    return mOctree->findEntitiesOverlapping(searchRay);
}

void SpatialQuerySystem::StaticModelBoundsComputeSystem::onEntityEnabled(EntityID entityID) {
    recomputeObjectBounds(entityID);
}

void SpatialQuerySystem::StaticModelBoundsComputeSystem::onEntityUpdated(EntityID entityID) {
    recomputeObjectBounds(entityID);
}

void SpatialQuerySystem::StaticModelBoundsComputeSystem::recomputeObjectBounds(EntityID entityID) {
    ObjectBounds objectBounds { getComponent<ObjectBounds>(entityID) };

    const std::shared_ptr<StaticModel> model { getComponent<std::shared_ptr<StaticModel>>(entityID) };
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
    if(box.mDimensions.length() > 0.f && (
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

    objectBounds = ObjectBounds::create(
        box,
        minCorner + .5f * box.mDimensions,
        glm::vec3{0.f, 0.f, 0.f}
    );

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
            regionToEncompass.setPosition(getComponent<ObjectBounds>(entity).getComputedWorldPosition());
            firstObject = false;
        }
        regionToEncompass = regionToEncompass + getComponent<AxisAlignedBounds>(entity);
    }
    assert(isFinite(regionToEncompass.getPosition()) && "Start position must be finite");
    assert(isFinite(regionToEncompass.getDimensions()) && "Region to encompass is too large to be bound in an octree");
    if(!isPositive(regionToEncompass.getDimensions())) {
        regionToEncompass.setDimensions(glm::vec3{1.f});
    }

    // another pass to create and populate our octree
    mOctree = std::make_unique<Octree>(8, regionToEncompass);
    for(EntityID entity: getEnabledEntities()) {
        mOctree->insertEntity(entity, getComponent<AxisAlignedBounds>(entity));
    }
}

void SpatialQuerySystem::onEntityEnabled(EntityID entityID) {
    mComputeQueue.insert(entityID);
}

void SpatialQuerySystem::onEntityDisabled(EntityID entityID) {
    mComputeQueue.erase(entityID);
    if(!mRequiresInitialization) { mOctree->removeEntity(entityID); }
}

void SpatialQuerySystem::onEntityUpdated(EntityID entityID) {
    mComputeQueue.insert(entityID);
}

void SpatialQuerySystem::onSimulationActivated() {
    mRequiresInitialization = true;
}

void SpatialQuerySystem::onSimulationStep(uint32_t timestepMillis) {
    (void)timestepMillis; // prevent unused parameter warning
    if(mRequiresInitialization)  {
        mComputeQueue.clear();
        rebuildOctree();
        mRequiresInitialization = false;
        return;
    }

    for(EntityID entity: mComputeQueue) {
        mOctree->removeEntity(entity);
        updateBounds(entity);
        mOctree->insertEntity(entity, getComponent<AxisAlignedBounds>(entity));
    }

    mComputeQueue.clear();
}

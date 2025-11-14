#include <vector>
#include <cassert>
#include <cstdint>

#include "toymaker/engine/core/ecs_world.hpp"

using namespace ToyMaker;

WorldID ECSWorld::s_nextWorld { 0 };

Entity::~Entity() {
    mWorld.lock()->destroyEntity(mID);
}

Entity::Entity(const Entity& other): Entity { other.mWorld.lock()->privateCreateEntity() } {
    mWorld.lock()->copyComponents(mID, other.mID, *other.mWorld.lock());
}

Entity::Entity(Entity&& other) noexcept: mID{ other.mID }, mWorld{ other.mWorld } {
    other.mID = kMaxEntities;
}

void Entity::disableSystems() {
    mWorld.lock()->disableEntity(mID);
}

void Entity::enableSystems(Signature systemMask) {
    mWorld.lock()->enableEntity(mID, systemMask);
}

Entity& Entity::operator=(const Entity& other) {
    if(&other == this) return *this;
    assert(mID < kMaxEntities && "This entity does not have a valid entity ID and cannot be copied to");

    // Clear existing components
    mWorld.lock()->removeComponentsAll(mID);

    // Copy components from other, if it is a valid entity
    if(other.mID != kMaxEntities) {
        mWorld.lock()->copyComponents(mID, other.mID, *other.mWorld.lock());
    }
    return *this;
}

Entity& Entity::operator=(Entity&& other) noexcept {
    if(&other == this) return *this;

    mWorld.lock()->destroyEntity(mID);

    mWorld = other.mWorld;
    mID = other.mID;
    other.mID = kMaxEntities;

    return *this;
}

void Entity::joinWorld(ECSWorld& world) {
    world.relocateEntity(*this);
}

void ECSWorld::relocateEntity(Entity& entity) {
    if(this == entity.mWorld.lock().get()) return; // nothing to be done, it's the same world

    assert((mNextEntity < kMaxEntities || !mDeletedIDs.empty()) && "Max number of entities reached");
    EntityID nextID;
    if(!mDeletedIDs.empty()){
        nextID = mDeletedIDs.back();
        mDeletedIDs.pop_back();
    } else {
        nextID = mNextEntity++;
    }

    copyComponents(nextID, entity.mID, *entity.mWorld.lock());
    entity.mWorld.lock()->destroyEntity(entity.mID);

    entity.mID = nextID;
    entity.mWorld = shared_from_this();
}

void BaseSystem::enableEntity(EntityID entityID) {
    if(isSingleton()) return;
    assert(
        (
            mEnabledEntities.find(entityID) != mEnabledEntities.end()
            || mDisabledEntities.find(entityID) != mDisabledEntities.end()
        ) && "This system does not apply to this entity"
    );
    mDisabledEntities.erase(entityID);
    mEnabledEntities.insert(entityID);
}

void Entity::copy(const Entity& other) {
    assert(mID <= kMaxEntities && "This entity does not have a valid entity ID and cannot be copied to");
    if(other.mID == kMaxEntities) return;

    mWorld.lock()->copyComponents(mID, other.mID, *other.mWorld.lock());
}

void BaseSystem::disableEntity(EntityID entityID) {
    if(isSingleton()) return;
    assert(
        (
            mEnabledEntities.find(entityID) != mEnabledEntities.end()
            || mDisabledEntities.find(entityID) != mDisabledEntities.end()
        ) && "This system does not apply to this entity"
    );
    mEnabledEntities.erase(entityID);
    mDisabledEntities.insert(entityID);
}

void BaseSystem::addEntity(EntityID entityID, bool enabled) {
    if(
        isSingleton() 
        || (
            mEnabledEntities.find(entityID) != mEnabledEntities.end()
            || mDisabledEntities.find(entityID) != mDisabledEntities.end()
        )
    ) return;

    if(enabled){
        mEnabledEntities.insert(entityID);
    } else {
        mDisabledEntities.insert(entityID);
    }
}

void BaseSystem::removeEntity(EntityID entityID) {
    mEnabledEntities.erase(entityID);
    mDisabledEntities.erase(entityID);
}

const std::set<EntityID>& BaseSystem::getEnabledEntities() {
    return mEnabledEntities;
}

bool BaseSystem::isEnabled(EntityID entityID) const {
    return !isSingleton() && mEnabledEntities.find(entityID) != mEnabledEntities.end();
}

bool BaseSystem::isRegistered(EntityID entityID) const {
    return (
        !isSingleton() && (
            mEnabledEntities.find(entityID) != mEnabledEntities.end()
            || mDisabledEntities.find(entityID) != mDisabledEntities.end()
        )
    );
}

void SystemManager::handleInitialize() {
    for(auto& pair: mNameToSystem) {
        if(pair.second->isSingleton()) { continue; }
        pair.second->onInitialize();
    }
}
void SystemManager::handleSimulationActivated() {
    for(auto& pair: mNameToSystem) {
        if(pair.second->isSingleton()) { continue; }
        pair.second->onSimulationActivated();
    }
}
void SystemManager::handleSimulationPreStep(uint32_t simStepMillis) {
    for(auto& pair: mNameToSystem) {
        if(pair.second->isSingleton()) { continue; }
        pair.second->onSimulationPreStep(simStepMillis);
    }
}
void SystemManager::handleSimulationStep(uint32_t simStepMillis) {
    for(auto& pair: mNameToSystem) {
        if(pair.second->isSingleton()) { continue; }
        pair.second->onSimulationStep(simStepMillis);
    }
}
void SystemManager::handleSimulationPostStep(uint32_t simStepMillis) {
    for(auto& pair: mNameToSystem) {
        if(pair.second->isSingleton()) { continue; }
        pair.second->onSimulationPostStep(simStepMillis);
    }
}
void SystemManager::handlePostTransformUpdate(uint32_t timestepMillis) {
    for(auto& pair: mNameToSystem) {
        if(pair.second->isSingleton()) { continue; }
        pair.second->onPostTransformUpdate(timestepMillis);
    }
}
void SystemManager::handleVariableStep(float simulationProgress, uint32_t variableStepMillis) {
    for(auto& pair: mNameToSystem) {
        if(pair.second->isSingleton()) { continue; }
        pair.second->onVariableStep(simulationProgress, variableStepMillis);
    }
}
void SystemManager::handlePreRenderStep(float simulationProgress) {
    for(auto& pair: mNameToSystem) {
        if(pair.second->isSingleton()) { continue; }
        pair.second->onPreRenderStep(simulationProgress);
    }
}
void SystemManager::handlePostRenderStep(float simulationProgress) {
    for(auto& pair: mNameToSystem) {
        if(pair.second->isSingleton()) { continue; }
        pair.second->onPostRenderStep(simulationProgress);
    }
}
void SystemManager::handleSimulationDeactivated() {
    for(auto& pair: mNameToSystem) {
        if(pair.second->isSingleton()) { continue; }
        pair.second->onSimulationDeactivated();
    }
}

SystemManager SystemManager::instantiate(std::weak_ptr<ECSWorld> world) const {
    SystemManager newSystemManager { world };
    newSystemManager.mNameToSignature = mNameToSignature;
    newSystemManager.mNameToListenedForComponents = mNameToListenedForComponents;
    newSystemManager.mNameToSystemType = mNameToSystemType;
    for(auto pair: mNameToSystem) {
        newSystemManager.mNameToSystem.insert({
            pair.first,
            pair.second->instantiate(world)
        });
    }
    return newSystemManager;
}

void SystemManager::enableEntity(EntityID entityID, Signature entitySignature, Signature systemMask) {
    for(const auto& signaturePair: mNameToSignature) {
        auto& system { mNameToSystem[signaturePair.first] };
        if(
            // entity and system signatures match
            (signaturePair.second&entitySignature) == signaturePair.second
            // mask allows this system to be enabled for this entity
            && systemMask.test(mNameToSystemType[signaturePair.first])
            // entity isn't already enabled for this system
            && (!system->isSingleton() && !system->isEnabled(entityID))
        ) {
            system->enableEntity(entityID);
            system->onEntityEnabled(entityID);
        }
    }
}

void SystemManager::disableEntity(EntityID entityID, Signature entitySignature) {
    // disable all systems which match this entity's signature
    for(const auto& signaturePair: mNameToSignature) {
        if(
            // signatures match
            (signaturePair.second&entitySignature) == signaturePair.second
            // and entity is enabled
            && (
                !mNameToSystem[signaturePair.first]->isSingleton() 
                && mNameToSystem[signaturePair.first]->isEnabled(entityID)
            )
        ) {
            mNameToSystem[signaturePair.first]->disableEntity(entityID);
            mNameToSystem[signaturePair.first]->onEntityDisabled(entityID);
        }
    }
}

void SystemManager::handleEntitySignatureChanged(EntityID entityID, Signature signature) {
    for(auto& pair: mNameToSignature) {
        const std::string systemTypeName { pair.first };
        const Signature& systemSignature { pair.second };
        BaseSystem& system { *(mNameToSystem[systemTypeName].get()) };
        if(system.isSingleton()) continue;

        if(
            (signature&systemSignature) == systemSignature 
            && !system.isRegistered(entityID)
        ) {
            system.addEntity(entityID, false);

        } else if(
            (signature&systemSignature) != systemSignature 
            && system.isRegistered(entityID) 
        ) {
            system.disableEntity(entityID);
            system.onEntityDisabled(entityID);
            system.removeEntity(entityID);
        }
    }
}

void SystemManager::handleEntityDestroyed(EntityID entityID) {
    for(auto& pair: mNameToSystem) {
        if(!pair.second->isSingleton() && pair.second->isRegistered(entityID)) {
            if(pair.second->isEnabled(entityID)){
                pair.second->disableEntity(entityID);
                pair.second->onEntityDisabled(entityID);
            }
            pair.second->removeEntity(entityID);
        }
    }
}

void SystemManager::handleEntityUpdated(EntityID entityID, Signature signature, ComponentType updatedComponent) {
    for(auto& pair: mNameToSignature) {
        // see if the updated entity's signature matches that of the system
        Signature& systemSignature { pair.second };
        if((systemSignature&signature) != systemSignature) continue;

        // see if the system is listening for updates to this component
        if(!mNameToListenedForComponents[pair.first].test(updatedComponent)) continue;

        // ignore disabled and singleton systems
        BaseSystem& system { *(mNameToSystem[pair.first]).get() };
        if(system.isSingleton() || !system.isEnabled(entityID)) continue;

        // apply update
        system.onEntityUpdated(entityID);
    }
}

ComponentManager ComponentManager::instantiate(std::weak_ptr<ECSWorld> world) const {
    ComponentManager newComponentManager {world};
    newComponentManager.mNameToComponentHash = mNameToComponentHash;
    newComponentManager.mHashToComponentType = mHashToComponentType;
    for(auto pair: mHashToComponentArray) {
        newComponentManager.mHashToComponentArray.insert({
            pair.first,
            pair.second->instantiate(world)
        });
    }
    return newComponentManager;
}

void ComponentManager::addComponent(EntityID entityID, const nlohmann::json& jsonComponent) {
    const std::string componentTypeName { jsonComponent.at("type").get<std::string>() };
    const std::size_t componentHash { mNameToComponentHash.at(componentTypeName) };
    const ComponentType componentType { mHashToComponentType.at(componentHash) };
    mHashToComponentArray.at(componentHash)->addComponent(entityID, jsonComponent);
    mEntityToSignature.at(entityID).set(componentType, true);
}

void ComponentManager::updateComponent(EntityID entityID, const nlohmann::json& jsonComponent) {
    const std::string componentTypeName { jsonComponent.at("type").get<std::string>() };
    const std::size_t componentHash { mNameToComponentHash.at(componentTypeName) };
    mHashToComponentArray.at(componentHash)->updateComponent(entityID, jsonComponent);
}

bool ComponentManager::hasComponent(EntityID entityID, const std::string& type) {
    return getComponentArray(type)->hasComponent(entityID);
}

void ComponentManager::removeComponent(EntityID entityID, const std::string& type) {
    getComponentArray(type)->removeComponent(entityID);
}

Signature ComponentManager::getSignature(EntityID entityID) {
    return mEntityToSignature[entityID];
}

ComponentType ComponentManager::getComponentType(const std::string& typeName) const {
    const std::size_t componentHash { mNameToComponentHash.at(typeName) };
    return mHashToComponentType.at(componentHash);
}

void ComponentManager::copyComponents(EntityID to, EntityID from) {
    copyComponents(to, from, *this);
}

void ComponentManager::copyComponents(EntityID to, EntityID from, ComponentManager& other) {
    for(auto& pair: mHashToComponentArray) {
        pair.second->copyComponent(to, from, *other.mHashToComponentArray[pair.first]);
    }
    mEntityToSignature[to] |= other.mEntityToSignature[from];
}


void ComponentManager::handleEntityDestroyed(EntityID entityID) {
    const Signature entitySignature { mEntityToSignature[entityID] };

    for(const auto& pair: mHashToComponentType) {
        const std::size_t componentHash { pair.first };
        const ComponentType componentType { pair.second };

        if(entitySignature.test(componentType)) {
            mHashToComponentArray[componentHash]->handleEntityDestroyed(entityID);
        }
    }

    mEntityToSignature.erase(entityID);
}

void SystemManager::unregisterAll() {
    for(auto& pair: mNameToSystem) {
        pair.second->onDestroyed();
    }
    mNameToSystem.clear();
    mNameToSignature.clear();
    mNameToListenedForComponents.clear();
}

void ComponentManager::unregisterAll() {
    mNameToComponentHash.clear();
    mHashToComponentArray.clear();
    mHashToComponentType.clear();
    mEntityToSignature.clear();
}

void ECSWorld::addComponent(EntityID entityID, const nlohmann::json& jsonComponent) {
    assert(entityID < kMaxEntities && "Cannot add a component to an entity that does not exist");
    mComponentManager->addComponent(entityID, jsonComponent);
    Signature signature { mComponentManager->getSignature(entityID) };
    mSystemManager->handleEntitySignatureChanged(entityID, signature);
}
bool ECSWorld::hasComponent(EntityID entityID, const std::string& typeName) const {
    return mComponentManager->hasComponent(entityID, typeName);
}
void ECSWorld::updateComponent(EntityID entityID, const nlohmann::json& jsonComponent) {
    mComponentManager->updateComponent(entityID, jsonComponent);
    const Signature signature { mComponentManager->getSignature(entityID) };
    mSystemManager->handleEntityUpdated(entityID, signature, mComponentManager->getComponentType(jsonComponent.at("type")));
}
void ECSWorld::removeComponent(EntityID entityID, const std::string& typeName) {
    mComponentManager->removeComponent(entityID, typeName);
    Signature signature { mComponentManager->getSignature(entityID) };
    mSystemManager->handleEntitySignatureChanged(entityID, signature);
}

void Entity::addComponent(const nlohmann::json& jsonComponent) {
    mWorld.lock()->addComponent(mID, jsonComponent);
}
bool Entity::hasComponent(const std::string& typeName) const {
    return mWorld.lock()->hasComponent(mID, typeName);
}
void Entity::updateComponent(const nlohmann::json& jsonComponent) {
    mWorld.lock()->updateComponent(mID, jsonComponent);
}
void Entity::removeComponent(const std::string& typeName) {
    mWorld.lock()->removeComponent(mID, typeName);
}

std::weak_ptr<const ECSWorld> ECSWorld::getPrototype() {
    return getInstance();
}

std::shared_ptr<ECSWorld> ECSWorld::createWorld() {
    std::shared_ptr<ECSWorld> world { new ECSWorld{} };
    world->mSystemManager = std::make_unique<SystemManager>(world);
    world->mComponentManager = std::make_unique<ComponentManager>(world);
    return world;
}

std::weak_ptr<ECSWorld> ECSWorld::getInstance() {
    static std::shared_ptr<ECSWorld> instance { createWorld() };
    return instance;
}

std::shared_ptr<ECSWorld> ECSWorld::instantiate() const {
    std::shared_ptr<ECSWorld> newWorld{ createWorld() };
    newWorld->mID = ++s_nextWorld;
    newWorld->mSystemManager = std::make_unique<SystemManager>(mSystemManager->instantiate(newWorld));
    newWorld->mComponentManager = std::make_unique<ComponentManager>(mComponentManager->instantiate(newWorld));
    return newWorld;
}

void ECSWorld::disableEntity(EntityID entityID) {
    Signature entitySignature {mComponentManager->getSignature(entityID)};
    mSystemManager->disableEntity(entityID, entitySignature);
}
void ECSWorld::enableEntity(EntityID entityID, Signature systemMask) {
    Signature entitySignature {mComponentManager->getSignature(entityID)};
    mSystemManager->enableEntity(entityID, entitySignature, systemMask);
}

void ECSWorld::removeComponentsAll(EntityID entityID) {
    mSystemManager->handleEntityDestroyed(entityID);
    mComponentManager->handleEntityDestroyed(entityID);
}

void ECSWorld::destroyEntity(EntityID entityID) {
    if(entityID == kMaxEntities) return;

    removeComponentsAll(entityID);
    mDeletedIDs.push_back(entityID);
}

void ComponentManager::handlePreSimulationStep() {
    for(auto& pair: mHashToComponentArray) {
        // copy "Next" buffer into "Previous" buffer
        pair.second->handlePreSimulationStep();
    }
}

void ECSWorld::copyComponents(EntityID to, EntityID from) {
    copyComponents(to, from, *this);
}

void ECSWorld::copyComponents(EntityID to, EntityID from, ECSWorld& other) {
    if(from == kMaxEntities) return;
    mComponentManager->copyComponents(to, from, *other.mComponentManager);
    const Signature& signature { mComponentManager->getSignature(to) };
    mSystemManager->handleEntitySignatureChanged(to, signature);
}

void ECSWorld::initialize() {
    mSystemManager->handleInitialize();
}
void ECSWorld::activateSimulation() {
    mSystemManager->handleSimulationActivated();
}
void ECSWorld::deactivateSimulation() {
    mSystemManager->handleSimulationDeactivated();
}


/*******************************
 * TODO: Would these not be better handled by assigning some sort of per-step priority value
 * to each system, and storing references to them in a priority queue?
 * 
 * Also, we might decide whether to run this step for this system based on if the system 
 * provides an override for it.
 *******************************/

void ECSWorld::simulationPreStep(uint32_t simStepMillis) {
    mComponentManager->handlePreSimulationStep();
    mSystemManager->handleSimulationPreStep(simStepMillis);
}
void ECSWorld::simulationStep(uint32_t simStepMillis) {
    mSystemManager->handleSimulationStep(simStepMillis);
}
void ECSWorld::simulationPostStep(uint32_t simStepMillis) {
    mSystemManager->handleSimulationPostStep(simStepMillis);
}

void ECSWorld::postTransformUpdate(uint32_t timestepMillis) {
    mSystemManager->handlePostTransformUpdate(timestepMillis);
}
void ECSWorld::variableStep(float simulationProgress, uint32_t variableStepMillis) {
    mSystemManager->handleVariableStep(simulationProgress, variableStepMillis);
}
void ECSWorld::preRenderStep(float simulationProgress) {
    mSystemManager->handlePreRenderStep(simulationProgress);
}
void ECSWorld::postRenderStep(float simulationProgress) {
    mSystemManager->handlePostRenderStep(simulationProgress);
}

void ECSWorld::cleanup() {
    mComponentManager->unregisterAll();
    mSystemManager->unregisterAll();
}

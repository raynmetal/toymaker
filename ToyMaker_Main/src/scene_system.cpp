#include <vector>
#include <set>
#include <cassert>
#include <memory>

#include "toymaker/engine/util.hpp"
#include "toymaker/engine/render_system.hpp"
#include "toymaker/engine/core/ecs_world.hpp"
#include "toymaker/engine/scene_system.hpp"

using namespace ToyMaker;

const std::string ToyMaker::kSceneRootName { "" };

void SceneNodeCore::SceneNodeCore_del_(SceneNodeCore* sceneNode) {
    if(!sceneNode) return;
    sceneNode->onDestroyed();
    delete sceneNode;
}

void SceneNodeCore::onCreated(){}
void SceneNodeCore::onActivated(){};
void SceneNodeCore::onDeactivated(){};
void SceneNodeCore::onDestroyed(){};

std::shared_ptr<SceneNodeCore> SceneNodeCore::copy(const std::shared_ptr<const SceneNodeCore> other) {
    if(!other) return nullptr;
    std::shared_ptr<SceneNodeCore> newSceneNode{ other->clone() };
    newSceneNode->copyDescendants(*other);
    return newSceneNode;
}


std::shared_ptr<SceneNode> SceneNode::create(const nlohmann::json& sceneNodeDescription) {
    return BaseSceneNode<SceneNode>::create(sceneNodeDescription);
}

std::shared_ptr<SceneNode> SceneNode::copy(const std::shared_ptr<const SceneNode> sceneNode) {
    return BaseSceneNode<SceneNode>::copy(sceneNode);
}

std::shared_ptr<SceneNodeCore> SceneNodeCore::clone() const {
    // construct a new scene node with the same components as this one
    std::shared_ptr<SceneNodeCore> newSceneNode{ new SceneNodeCore{*this}, &SceneNodeCore_del_ };
    return newSceneNode;
}


SceneNodeCore::SceneNodeCore(const nlohmann::json& sceneNodeDescription) {
    validateName(sceneNodeDescription.at("name").get<std::string>());
    mName = sceneNodeDescription.at("name").get<std::string>();
    mEntity = std::make_shared<Entity>(
        ECSWorld::createEntityPrototype()
    );
    // bypass own implementation of addComponent. We shouldn't trigger methods that
    // require a shared pointer to this to be present
    addComponent<Transform>({}, true);
    addComponent<SceneHierarchyData>({}, true);
    addComponent<AxisAlignedBounds>({}, true);
    addComponent<ObjectBounds>({}, true);
    for(const nlohmann::json& componentDescription: sceneNodeDescription.at("components")) {
        addComponent(componentDescription, true);
    }
    assert(hasComponent<Placement>() && "scene nodes must define a placement component");
}

SceneNodeCore::SceneNodeCore(const SceneNodeCore& other): enable_shared_from_this{}
{
    mEntity = std::make_shared<Entity>(
        ECSWorld::createEntityPrototype()
    );
    copyAndReplaceAttributes(other);
}

void SceneNodeCore::recomputeChildNameIndexMapping() {
    mChildNameToNode.clear();
    for(std::size_t i{0}; i < mChildren.size(); ++i) {
        mChildNameToNode[mChildren[i]->mName] = i;
    }
}

void SceneNodeCore::copyAndReplaceAttributes(const SceneNodeCore& other) {
    // copy the other entity and its components
    std::shared_ptr<Entity> newEntity { 
        std::make_shared<Entity>(mEntity->getWorld().lock()->createEntity())
    };
    newEntity->copy(*(other.mEntity));
    mEntity = newEntity;
    mStateFlags = (other.mStateFlags&StateFlags::ENABLED);
    mChildren.clear();
    mChildNameToNode.clear();
    mName = other.mName;
    mParent.reset();
    mParentViewport.reset();
    mSystemMask = other.mSystemMask;
}

void SceneNodeCore::copyDescendants(const SceneNodeCore& other) {
    // copy descendant nodes, attach them to self
    for(auto& child: other.mChildren) {
        mChildNameToNode[child->mName] = mChildren.size();
        mChildren.push_back(SceneNodeCore::copy(child));

        // TODO : somehow make this whole thing less delicate. Shared
        // from this depends on the existence of a shared pointer
        // to the current object
        mChildren.back()->mParent = shared_from_this();
        setParentViewport(mChildren.back(), getLocalViewport());
    }
}

void SceneNodeCore::addComponent(const nlohmann::json& jsonComponent, const bool bypassSceneActivityCheck) {
    mEntity->addComponent(jsonComponent);

    // NOTE: required because even though this node's entity's signature changes, it
    // is disabled by default on any systems it is eligible for. We need to activate
    // the node according to its system mask
    if(!bypassSceneActivityCheck && isActive()) {
        mEntity->enableSystems(mSystemMask);
    }
}

bool SceneNodeCore::hasComponent(const std::string& typeName) const {
    return mEntity->hasComponent(typeName);
}

void SceneNodeCore::updateComponent(const nlohmann::json& jsonComponent) {
    mEntity->updateComponent(jsonComponent);
}

void SceneNodeCore::addOrUpdateComponent(const nlohmann::json& jsonComponent, const bool bypassSceneActivityCheck) {
    if(!hasComponent(jsonComponent.at("type"))) {
        addComponent(jsonComponent, bypassSceneActivityCheck);
        return;
    }
    updateComponent(jsonComponent);
}


bool SceneNodeCore::detectCycle(std::shared_ptr<SceneNodeCore> node) {
    if(!node) return false;

    std::shared_ptr<SceneNodeCore> slow { node };
    std::shared_ptr<SceneNodeCore> fast { slow->mParent.lock() };
    while(fast != nullptr && !fast->mParent.expired()  && slow != fast) {
        slow = slow->mParent.lock();
        fast = fast->mParent.lock()->mParent.lock();
    }

    if(slow == fast) return true;
    return false;
}

bool SceneNodeCore::inScene() const {
    return mStateFlags&StateFlags::ENABLED;
}

bool SceneNodeCore::isActive() const {
    return mStateFlags&StateFlags::ACTIVE;
}

bool SceneNodeCore::isAncestorOf(std::shared_ptr<const SceneNodeCore> sceneNode) const {
    if(!sceneNode || sceneNode.get() == this) return false;

    std::shared_ptr<const SceneNodeCore> currentNode { sceneNode };
    while(currentNode != nullptr && currentNode.get() != this) {
        currentNode = currentNode->mParent.lock();
    }
    return static_cast<bool>(currentNode);
}

void SceneNodeCore::setName(const std::string& name) {
    validateName(name);
    mName = name;
}

std::string SceneNodeCore::getName() const {
    return mName;
}

std::string SceneNodeCore::getViewportLocalPath() const {
    return getPathFromAncestor(getLocalViewport());
}

std::tuple<std::string, std::string> SceneNodeCore::nextInPath(const std::string& where) {
    // Search for beginning and end of the name of the next node in the specified path
    std::string::const_iterator nextBegin{ where.begin() };
    ++nextBegin;
    std::string::const_iterator nextEnd{ nextBegin };
    for(; nextEnd != where.end() && (*nextEnd) != '/'; ++nextEnd);
    assert(nextEnd != where.end() && "Invalid path not ending in '/' specified");
    assert(nextBegin != nextEnd && "Incomplete path, every node name in the path must be specified separated with '/'");

    // Form the name of the next node, as well as the remaining path left
    // to be traversed
    const std::string nextNodeName { nextBegin, nextEnd };
    const std::string remainingWhere { nextEnd, where.end() };
    return std::tuple<std::string, std::string>{nextNodeName, remainingWhere};
}

bool SceneNodeCore::hasNode(const std::string& pathToChild) const {
    if(pathToChild == "/") {
        return true;
    }

    assert(pathToChild != "" && "Path to child cannot be an empty string");
    const std::tuple<std::string, std::string> nextPair { nextInPath(pathToChild) };
    const std::string& nextNodeName { std::get<0>(nextPair) };
    const std::string& remainingWhere { std::get<1>(nextPair) };
    if(mChildNameToNode.find(nextNodeName) == mChildNameToNode.end()) {
        return false;
    }

    return mChildren[mChildNameToNode.at(nextNodeName)]->hasNode(remainingWhere);
}


void SceneNodeCore::addNode(std::shared_ptr<SceneNodeCore> node, const std::string& where) {
    assert(node && "Must be a non null pointer to a valid scene node");
    assert(node->mParent.expired() && "Node must not have a parent");
    if(where == "/") {
        assert(mChildNameToNode.find(node->mName) == mChildNameToNode.end() && "A node with this name already exists at this location");
        mChildNameToNode[node->mName] = mChildren.size();
        mChildren.push_back(node);
        node->mParent = shared_from_this();
        setParentViewport(node, getLocalViewport());
        assert(!detectCycle(node) && "Cycle detected, ancestor node added as child to its descendant.");
        getWorld().lock()->getSystem<SceneSystem>()->nodeAdded(node);
        return;
    }

    // descend to the next node in the path
    std::tuple<std::string, std::string> nextPair{ nextInPath(where) };
    const std::string& nextNodeName { std::get<0>(nextPair) };
    const std::string& remainingWhere { std::get<1>(nextPair) };
    assert(mChildNameToNode.find(nextNodeName) != mChildNameToNode.end() && "No child node with this name is known");

    mChildren[mChildNameToNode.at(nextNodeName)]->addNode(node, remainingWhere);
}

std::vector<std::shared_ptr<SceneNodeCore>> SceneNodeCore::getChildren() {
    return std::vector<std::shared_ptr<SceneNodeCore>>{ mChildren };
}
std::vector<std::shared_ptr<const SceneNodeCore>> SceneNodeCore::getChildren() const {
    std::vector<std::shared_ptr<const SceneNodeCore>> children {};
    for(auto& child: mChildren) {
        children.push_back(child);
    }
    return children;
}

std::shared_ptr<SceneNodeCore> SceneNodeCore::getNode(const std::string& where) {
    if(where=="/") {
        return shared_from_this();
    }

    // descend to the next node in the path
    std::tuple<std::string, std::string> nextPair{ nextInPath(where) };
    const std::string nextNodeName { std::get<0>(nextPair) };
    const std::string remainingWhere { std::get<1>(nextPair) };
    assert(mChildNameToNode.find(nextNodeName) != mChildNameToNode.end() && "No child node with this name is known");

    return mChildren[mChildNameToNode.at(nextNodeName)]->getNode(remainingWhere);
}

void SceneNodeCore::setParentViewport(std::shared_ptr<SceneNodeCore> node, std::shared_ptr<ViewportNode> newViewport)  {
    assert(node && "Cannot modify the parent viewport of a nonexistent node");

    // If the node whose viewport is being set is a viewport itself,
    if(auto nodeAsViewport = std::dynamic_pointer_cast<ViewportNode>(node)) {
        // remove it from its previous parent viewport's list of children,
        if(auto previousParentViewport = node->mParentViewport.lock()) {
            previousParentViewport->mChildViewports.erase(nodeAsViewport);
        }
        // and add it to the new parent viewport's list of children
        if(newViewport) {
            nodeAsViewport->mViewportLoadOrdinal = newViewport->mNLifetimeChildrenAdded++;
            newViewport->mChildViewports.insert(nodeAsViewport);
        }
    }

    // Otherwise, if this is a camera node,
    else if(node->mEntity->isRegistered<CameraSystem>()) {
        // remove it from its previous viewport's domain camera list,
        if(auto previousParentViewport = node->getLocalViewport()) {
            previousParentViewport->unregisterDomainCamera(node);
        }
        // add it to the new viewport domain camera list
        if(newViewport) {
            // NOTE: ADDITION viewports may only take on other viewports as children
            newViewport->registerDomainCamera(node);
        }
    }

    // finally set the parent viewport property
    node->mParentViewport = newViewport;

    // propagate parent viewport changes to descendants
    for(auto& child: node->getChildren()) {
        setParentViewport(child, node->getLocalViewport());
    }
}

std::shared_ptr<ViewportNode> SceneNodeCore::getLocalViewport() {
    return mParentViewport.lock();
}
std::shared_ptr<const ViewportNode> SceneNodeCore::getLocalViewport() const {
    return std::const_pointer_cast<ViewportNode>(mParentViewport.lock());
}

std::shared_ptr<SceneNodeCore> SceneNodeCore::getParentNode() {
    // TODO: Find a more efficient way to prevent access to the scene root
    // Guard against indirect access to scene root owned by the scene system via
    // its descendants.
    std::shared_ptr<SceneNodeCore> parent { mParent };
    if(parent) assert(parent->getName() != kSceneRootName && "Cannot retrieve reference to root node of the scene");
    return parent;
}

std::shared_ptr<const SceneNodeCore> SceneNodeCore::getParentNode() const {
    std::shared_ptr<SceneNodeCore> parent { mParent };
    return parent;
}

std::shared_ptr<SceneNodeCore> SceneNodeCore::disconnectNode(std::shared_ptr<SceneNodeCore> node) {
    // let system know that this node is being disconnected, in case it was part
    // of the scene tree
    node->mEntity->getWorld().lock()->getSystem<SceneSystem>()->nodeRemoved(node);

    //disconnect this node from its parent
    if(std::shared_ptr<SceneNodeCore> parent = node->mParent.lock()) {
        parent->mChildren.erase(std::next(parent->mChildren.begin(), parent->mChildNameToNode[node->mName]));
        parent->recomputeChildNameIndexMapping();
    }

    setParentViewport(node, nullptr);
    node->mParent.reset();
    return node;
}

std::shared_ptr<SceneNodeCore> SceneNodeCore::removeNode(const std::string& where) {
    if(where == "/") {
        assert(mName != kSceneRootName && "Cannot remove the hidden scene root node");
        return disconnectNode(shared_from_this());
    }

    // descend to the next node in the path
    std::tuple<std::string, std::string> nextPair{ nextInPath(where) };
    const std::string nextNodeName { std::get<0>(nextPair) };
    const std::string remainingWhere { std::get<1>(nextPair) };
    assert(mChildNameToNode.find(nextNodeName) != mChildNameToNode.end() && "No child node with this name is known");

    return mChildren[mChildNameToNode.at(nextNodeName)]->removeNode(remainingWhere);
}

std::vector<std::shared_ptr<SceneNodeCore>> SceneNodeCore::removeChildren() {
    std::vector<std::shared_ptr<SceneNodeCore>> children { getChildren() };
    for(auto& child: children) {
        disconnectNode(child);
    }
    return children;
}



std::vector<std::shared_ptr<SceneNodeCore>> SceneNodeCore::getDescendants() {
    std::vector<std::shared_ptr<SceneNodeCore>> descendants {};
    for(auto& child: mChildren) {
        descendants.push_back(child);
        std::vector<std::shared_ptr<SceneNodeCore>> childDescendants {child->getDescendants()};
        descendants.insert(descendants.end(), childDescendants.begin(), childDescendants.end());
    }
    return descendants;
}

std::string SceneNodeCore::getPathFromAncestor(std::shared_ptr<const SceneNodeCore> ancestor) const {
    assert(ancestor->isAncestorOf(shared_from_this()) && "The node in the argument is not an ancestor of this node");

    std::shared_ptr<const SceneNodeCore> currentNode { shared_from_this() };
    std::string path {"/"};
    while(currentNode != ancestor) {
        path = "/" + currentNode->mName + path;
        currentNode = currentNode->getParentNode();
    }

    return path;
}

EntityID SceneNodeCore::getEntityID() const {
    return mEntity->getID();
}
WorldID SceneNodeCore::getWorldID() const {
    return mEntity->getWorld().lock()->getID();
}
UniversalEntityID SceneNodeCore::getUniversalEntityID() const {
    return { mEntity->getWorld().lock()->getID(), mEntity->getID() };
}
std::weak_ptr<ECSWorld> SceneNodeCore::getWorld() const {
    return mEntity->getWorld();
}
void SceneNodeCore::joinWorld(ECSWorld& world) {
    mEntity->joinWorld(world);
}
void ViewportNode::joinWorld(ECSWorld& world) {
    if(!mOwnWorld) {
        getWorld().lock()->getSystem<RenderSystem>()->deleteRenderSet(mRenderSet);
    }

    SceneNodeCore::joinWorld(world);

    if(!mOwnWorld) {
        mRenderSet = getWorld().lock()->getSystem<RenderSystem>()->createRenderSet(mRenderConfiguration.mBaseDimensions, mRenderConfiguration.mBaseDimensions, {0, 0, mRenderConfiguration.mBaseDimensions.x, mRenderConfiguration.mBaseDimensions.y});
    }
}

void SceneNodeCore::validateName(const std::string& nodeName) {
    const bool containsValidCharacters{
        std::all_of(nodeName.begin(), nodeName.end(), 
            [](char c) { 
                return (std::isalnum(c) || c == '_');
            }
        )
    };
    assert(nodeName.size() > 0 && "Scene node must have a name");
    assert(containsValidCharacters && "Scene node name may contain only alphanumeric characters and underscores");
}

ViewportNode::~ViewportNode() {
    // Remove the render set associated with this node
    getWorld().lock()->getSystem<RenderSystem>()->deleteRenderSet(mRenderSet);

    /** 
     * NOTE: This works under the assumption that only this place has references to
     * its descendant nodes.  If the children are referenced elsewhere, they will be in an invalid 
     * state
     *
     * TODO: Make a more involved deletion that takes into account the possibility that 
     * a descendant viewport shares a world with this node, or that one of this node's 
     * children is being used elsewhere.
    */

    // Make sure all descendant nodes (and their respective entities) are deleted before
    // we destroy this node's world, if it has one.  Also remove our own entity from this 
    // world
    mActiveCamera = nullptr;
    mDomainCameras.clear();
    mChildViewports.clear();
    mChildren.clear();
    mChildNameToNode.clear();
    mEntity.reset();

    // Destroy this viewport's world
    if(mOwnWorld) {
        mOwnWorld->cleanup();
        mOwnWorld = nullptr;
    }
}


std::shared_ptr<ViewportNode> ViewportNode::create(const std::string& name, bool inheritsWorld, bool allowActionFlowthrough, const ViewportNode::RenderConfiguration& renderConfiguration, std::shared_ptr<Texture> skybox) {
    std::shared_ptr<ViewportNode> newViewport { BaseSceneNode<ViewportNode>::create(Placement{}, name) };
    if(!inheritsWorld) {
        newViewport->createAndJoinWorld();
    }
    newViewport->mRenderSet = newViewport->getWorld().lock()->getSystem<RenderSystem>()->createRenderSet(renderConfiguration.mBaseDimensions, renderConfiguration.mBaseDimensions, {0, 0, renderConfiguration.mBaseDimensions.x, renderConfiguration.mBaseDimensions.y});
    newViewport->setRenderConfiguration(renderConfiguration);
    if(!inheritsWorld) {
        newViewport->setSkybox(skybox);
    }
    newViewport->mActionFlowthrough = allowActionFlowthrough;
    return newViewport;
}

std::shared_ptr<ViewportNode> ViewportNode::create(const Key& key, const std::string& name, bool inheritsWorld, const RenderConfiguration& renderConfiguration, std::shared_ptr<Texture> skybox) {
    std::shared_ptr<ViewportNode> newViewport { BaseSceneNode<ViewportNode>::create(key, Placement{}, name) };
    if(!inheritsWorld) {
        newViewport->createAndJoinWorld();
    }
    newViewport->mRenderSet = newViewport->getWorld().lock()->getSystem<RenderSystem>()->createRenderSet(renderConfiguration.mBaseDimensions, renderConfiguration.mBaseDimensions, {0, 0, renderConfiguration.mBaseDimensions.x, renderConfiguration.mBaseDimensions.y}, renderConfiguration.mRenderType);
    newViewport->setRenderConfiguration(renderConfiguration);
    if(!inheritsWorld) {
        newViewport->setSkybox(skybox);
    }
    newViewport->mActionFlowthrough = true;
    return newViewport;
}

std::shared_ptr<ViewportNode> ViewportNode::create(const nlohmann::json& viewportNodeDescription) {
    std::shared_ptr<ViewportNode> newViewport {BaseSceneNode<ViewportNode>::create(viewportNodeDescription)};
    newViewport->updateComponent<Placement>(Placement {}); // override scene file placement

    assert(viewportNodeDescription.find("inherits_world") != viewportNodeDescription.end() && "Viewport descriptions must contain the \"inherits_world\" boolean attribute");
    if(viewportNodeDescription.at("inherits_world").get<bool>() == false) {
        newViewport->createAndJoinWorld();
    }

    assert(viewportNodeDescription.find("render_configuration") != viewportNodeDescription.end() && "Viewport description must contain a valid render configuration");
    viewportNodeDescription.at("render_configuration").get_to(newViewport->mRenderConfiguration);
    newViewport->mRenderSet = newViewport->getWorld().lock()->getSystem<RenderSystem>()->createRenderSet(newViewport->mRenderConfiguration.mBaseDimensions, newViewport->mRenderConfiguration.mBaseDimensions, {0, 0, newViewport->mRenderConfiguration.mBaseDimensions.x, newViewport->mRenderConfiguration.mBaseDimensions.y});
    newViewport->setRenderConfiguration(newViewport->mRenderConfiguration);

    if(viewportNodeDescription.at("inherits_world").get<bool>() == false && viewportNodeDescription.find("skybox_texture") != viewportNodeDescription.end()) {
        newViewport->setSkybox(
            ResourceDatabase::GetRegisteredResource<Texture>(viewportNodeDescription.at("skybox_texture").get<std::string>())
        );
    } else {
        assert(viewportNodeDescription.find("skybox_texture") == viewportNodeDescription.end() && "Viewports that don't own their respective worlds cannot specify a skybox texture");
    }

    if(newViewport->mRenderConfiguration.mRenderType == RenderConfiguration::RenderType::ADDITION) {
        assert(viewportNodeDescription.find("allow_action_flowthrough") != viewportNodeDescription.end() && "Addition viewports must set the allow action flowthrough property");
        viewportNodeDescription.at("allow_action_flowthrough").get_to(newViewport->mActionFlowthrough);
    }

    assert(viewportNodeDescription.find("prevent_handled_action_propagation") != viewportNodeDescription.end() && "Viewport must specify whether or not it allows handled actions to be passed to further viewports");
    viewportNodeDescription.at("prevent_handled_action_propagation").get_to(newViewport->mPreventHandledActionPropagation);

    return newViewport;
}

std::shared_ptr<ViewportNode> ViewportNode::copy(const std::shared_ptr<const ViewportNode> viewportNode) {
    std::shared_ptr<ViewportNode> newViewport {BaseSceneNode<ViewportNode>::copy(viewportNode)};
    return newViewport;
}

std::shared_ptr<SceneNodeCore> ViewportNode::clone() const {
    std::shared_ptr<SceneNodeCore> newSceneNode { new ViewportNode{*this}, &SceneNodeCore_del_ };
    std::shared_ptr<ViewportNode> newViewport { std::static_pointer_cast<ViewportNode>(newSceneNode) };
    if(mOwnWorld) {
        newViewport->createAndJoinWorld();
    }
    newViewport->mRenderSet = newViewport->getWorld().lock()->getSystem<RenderSystem>()->createRenderSet(mRenderConfiguration.mBaseDimensions, mRenderConfiguration.mBaseDimensions, {0, 0, mRenderConfiguration.mBaseDimensions.x, mRenderConfiguration.mBaseDimensions.y});
    newViewport->setRenderConfiguration(mRenderConfiguration);
    if(mOwnWorld) {
        newViewport->setSkybox(mOwnWorld->getSystem<RenderSystem>()->getSkybox());
    }
    return newSceneNode;
}

void ViewportNode::setRenderConfiguration(const ViewportNode::RenderConfiguration& renderConfiguration) {
    mRenderConfiguration = renderConfiguration;
    assert(mRenderConfiguration.mRequestedDimensions.x > 0 && mRenderConfiguration.mRequestedDimensions.y > 0 && "Cannot request negative dimensions for viewport renders");
    requestDimensions(mRenderConfiguration.mRequestedDimensions);
}

ViewportNode::RenderConfiguration ViewportNode::getRenderConfiguration() const {
    return mRenderConfiguration;
}

void ViewportNode::createAndJoinWorld() {
    mOwnWorld = std::shared_ptr<ECSWorld>{ECSWorld::getPrototype().lock()->instantiate() };
    joinWorld(*mOwnWorld);
    mOwnWorld->initialize();
}

void ViewportNode::onActivated() {
    if(
        !mActiveCamera 
        && !(mRenderConfiguration.mRenderType == RenderConfiguration::RenderType::ADDITION)
    ) {
        setActiveCamera(findFallbackCamera());
    }

    if(!(mRenderConfiguration.mRenderType == RenderConfiguration::RenderType::ADDITION)) {
        assert(mActiveCamera && "No cameras exist in this viewports domain, or none are enabled");
        assert(mActiveCamera->mEntity->isEnabled<CameraSystem>() && "The camera marked active for this viewport is not visible to the camera system");
    }

    if(!mOwnWorld) return;
    mOwnWorld->activateSimulation();
    mOwnWorld->simulationPreStep(0);
}

void ViewportNode::onDeactivated() {
    if(!mOwnWorld) return;
    mOwnWorld->deactivateSimulation();
}
void ViewportNode::setSkybox(std::shared_ptr<Texture> skybox) {
    assert(mOwnWorld && "Skybox may only be set for a viewport that has its own world");
    mOwnWorld->getSystem<RenderSystem>()->setSkybox(skybox);
}
void ViewportNode::viewNextDebugTexture() {
    std::shared_ptr<RenderSystem> renderSystem { getWorld().lock()->getSystem<RenderSystem>() };
    renderSystem->useRenderSet(mRenderSet);
    renderSystem->renderNextTexture();
}
void ViewportNode::updateExposure(float newExposure){
    std::shared_ptr<RenderSystem> renderSystem { getWorld().lock()->getSystem<RenderSystem>() };
    renderSystem->useRenderSet(mRenderSet);
    renderSystem->setExposure(newExposure);
}
void ViewportNode::updateGamma(float newGamma) {
    std::shared_ptr<RenderSystem> renderSystem { getWorld().lock()->getSystem<RenderSystem>() };
    renderSystem->useRenderSet(mRenderSet);
    renderSystem->setGamma(newGamma);
}

float ViewportNode::getExposure() {
    std::shared_ptr<RenderSystem> renderSystem { getWorld().lock()->getSystem<RenderSystem>() };
    renderSystem->useRenderSet(mRenderSet);
    return renderSystem->getExposure();
}

float ViewportNode::getGamma() {
    std::shared_ptr<RenderSystem> renderSystem { getWorld().lock()->getSystem<RenderSystem>() };
    renderSystem->useRenderSet(mRenderSet);
    return renderSystem->getGamma();
}

void ViewportNode::setActiveCamera(const std::string& cameraPath) {
    std::shared_ptr<SceneNodeCore> cameraNode { getByPath(cameraPath) };
    setActiveCamera(cameraNode);
}

void ViewportNode::setActiveCamera(std::shared_ptr<SceneNodeCore> cameraNode) {
    assert((cameraNode || !isActive() || mRenderConfiguration.mRenderType == RenderConfiguration::RenderType::ADDITION) && "Active camera may only be unset if this viewport is inactive");
    if(!cameraNode) { 
        mActiveCamera = nullptr;
        return;
    }

    assert(mDomainCameras.find(cameraNode) != mDomainCameras.end() && "This camera is under another viewport's domain, and cannot be used by this viewport");
    assert(cameraNode->mEntity->isRegistered<CameraSystem>() && "This node does not have all the required components to qualify as a camera");
    assert((!isActive() || cameraNode->isActive()) && "If a viewport is active, the camera it intends to use must also be active");

    mActiveCamera = cameraNode;
    getWorld().lock()->getSystem<RenderSystem>()->useRenderSet(mRenderSet);
    getWorld().lock()->getSystem<RenderSystem>()->setCamera(mActiveCamera->getEntityID());
}

void ViewportNode::requestDimensions(glm::u16vec2 requestDimensions) {
    assert(requestDimensions.x * requestDimensions.y > 0 && "Request dimensions cannot contain 0");
    std::shared_ptr<RenderSystem> renderSystem { getWorld().lock()->getSystem<RenderSystem>() };
    renderSystem->useRenderSet(mRenderSet);

    const float requestAspect { static_cast<float>(requestDimensions.x)/static_cast<float>(requestDimensions.y) };
    const float baseAspect { static_cast<float>(mRenderConfiguration.mBaseDimensions.x)/static_cast<float>(mRenderConfiguration.mBaseDimensions.y) };
    glm::vec2 requestToBaseRatio { 
        static_cast<float>(requestDimensions.x) / static_cast<float>(mRenderConfiguration.mBaseDimensions.x),
        static_cast<float>(requestDimensions.y) / static_cast<float>(mRenderConfiguration.mBaseDimensions.y)
    };

    mRenderConfiguration.mRequestedDimensions = requestDimensions;

    if(mRenderConfiguration.mResizeType != RenderConfiguration::ResizeType::OFF) {
        switch(mRenderConfiguration.mResizeMode)  {
            case RenderConfiguration::ResizeMode::FIXED_ASPECT:
                if(requestAspect > baseAspect) {
                    mRenderConfiguration.mComputedDimensions = { requestToBaseRatio.y * mRenderConfiguration.mBaseDimensions.x, mRenderConfiguration.mRequestedDimensions.y };

                } else {
                    mRenderConfiguration.mComputedDimensions = { mRenderConfiguration.mRequestedDimensions.x, requestToBaseRatio.x * mRenderConfiguration.mBaseDimensions.y };
                }

            break;
            case RenderConfiguration::ResizeMode::EXPAND_HORIZONTALLY:
                mRenderConfiguration.mComputedDimensions.x = mRenderConfiguration.mRequestedDimensions.x;

                // Bigger, so aspect ratio does not matter. Clamp Y
                if(requestToBaseRatio.y > 1.f && requestToBaseRatio.x > 1.f) { 
                    mRenderConfiguration.mComputedDimensions.y = mRenderConfiguration.mBaseDimensions.y;

                // Shorter but wider aspect. Y is full height of request
                } else if (requestToBaseRatio.y <= 1.f && requestAspect > baseAspect) {
                    mRenderConfiguration.mComputedDimensions.y = mRenderConfiguration.mRequestedDimensions.y;

                // Taller aspect, but narrower than base. Shrink Y in proportion to X, preserving aspect in render
                } else /*if (requestToBaseRatio.x <= 1.f && requestAspect <= baseAspect)*/ {
                    mRenderConfiguration.mComputedDimensions.y = requestToBaseRatio.x * mRenderConfiguration.mBaseDimensions.y;
                } 

            break;
            case RenderConfiguration::ResizeMode::EXPAND_VERTICALLY:
                mRenderConfiguration.mComputedDimensions.y = mRenderConfiguration.mRequestedDimensions.y;

                // Bigger, so aspect ratio does not matter. Clamp X
                if(requestToBaseRatio.x > 1.f && requestToBaseRatio.y > 1.f) {
                    mRenderConfiguration.mComputedDimensions.x = mRenderConfiguration.mBaseDimensions.x;

                // Narrower, but taller aspect. X is full width of the request.
                } else if (requestToBaseRatio.x <= 1.f && requestAspect <= baseAspect) {
                    mRenderConfiguration.mComputedDimensions.x = mRenderConfiguration.mRequestedDimensions.x;

                // Wider aspect, but shorter than base. Shrink X in proportion to Y, preserving aspect in render
                } else {
                    mRenderConfiguration.mComputedDimensions.x = requestToBaseRatio.y * mRenderConfiguration.mBaseDimensions.x;
                }
            break;
            case RenderConfiguration::ResizeMode::EXPAND_FILL:
                mRenderConfiguration.mComputedDimensions = mRenderConfiguration.mRequestedDimensions;
            break;
        }
    }

    switch(mRenderConfiguration.mResizeType) {
        case RenderConfiguration::ResizeType::OFF:
            mRenderConfiguration.mComputedDimensions = mRenderConfiguration.mBaseDimensions;
            renderSystem->setRenderProperties(
                mRenderConfiguration.mBaseDimensions, 
                requestDimensions,
                getCenteredViewportCoordinates(),
                mRenderConfiguration.mRenderType
            );
        break;
        case RenderConfiguration::ResizeType::TEXTURE_DIMENSIONS:
            renderSystem->setRenderProperties(
                glm::mat2{mRenderConfiguration.mRenderScale} * mRenderConfiguration.mBaseDimensions,
                requestDimensions,
                getCenteredViewportCoordinates(),
                mRenderConfiguration.mRenderType
            );
        break;
        case RenderConfiguration::ResizeType::VIEWPORT_DIMENSIONS:
            renderSystem->setRenderProperties(
                glm::mat2{mRenderConfiguration.mRenderScale} * mRenderConfiguration.mComputedDimensions, 
                requestDimensions,
                getCenteredViewportCoordinates(),
                mRenderConfiguration.mRenderType
            );
        break;
    }
    if(mRenderConfiguration.mRenderType == RenderConfiguration::RenderType::ADDITION) {
        for(auto& childViewport: mChildViewports) {
            if(childViewport->isActive()) {
                childViewport->requestDimensions(mRenderConfiguration.mComputedDimensions);
            }
        }
    }
}

void ViewportNode::setResizeType(RenderConfiguration::ResizeType resizeType) {
    if(mRenderConfiguration.mResizeType == resizeType) return;
    mRenderConfiguration.mResizeType = resizeType;
    requestDimensions(mRenderConfiguration.mRequestedDimensions);
}

void ViewportNode::setResizeMode(RenderConfiguration::ResizeMode resizeMode) {
    if(mRenderConfiguration.mResizeMode == resizeMode) return;
    mRenderConfiguration.mResizeMode = resizeMode;
    requestDimensions(mRenderConfiguration.mRequestedDimensions);
}

void ViewportNode::setUpdateMode(RenderConfiguration::UpdateMode updateMode) {
    if(mRenderConfiguration.mUpdateMode == updateMode) return;
    mRenderConfiguration.mUpdateMode = updateMode;
}

void ViewportNode::setFPSCap(float fpsCap) {
    assert(fpsCap> 0.f && "FPS cap cannot be negative or zero");
    mRenderConfiguration.mFPSCap = fpsCap;
}

void ViewportNode::setRenderScale(float renderScale) {
    assert(renderScale > 0.f && "Render scale cannot be negative or zero");
    mRenderConfiguration.mRenderScale = renderScale;
    requestDimensions(mRenderConfiguration.mRequestedDimensions);
}

void ViewportNode::registerDomainCamera(std::shared_ptr<SceneNodeCore> cameraNode) {
    assert(isAncestorOf(cameraNode) && "This node is not the camera node's ancestor");
    assert(mRenderConfiguration.mRenderType != RenderConfiguration::RenderType::ADDITION);
    mDomainCameras.insert(cameraNode);
}
void ViewportNode::unregisterDomainCamera(std::shared_ptr<SceneNodeCore> cameraNode) {
    mDomainCameras.erase(cameraNode);
    if(mActiveCamera == cameraNode) {
        setActiveCamera(findFallbackCamera());
    }
}
std::shared_ptr<SceneNodeCore> ViewportNode::findFallbackCamera() {
    assert(mRenderConfiguration.mRenderType != RenderConfiguration::RenderType::ADDITION);
    if(mDomainCameras.empty()) return nullptr;
    // pick a camera at random from our domain
    std::shared_ptr<SceneNodeCore> fallbackCamera {*mDomainCameras.begin()};
    for(std::shared_ptr<SceneNodeCore> domainCamera: mDomainCameras) {
        if(domainCamera->mEntity->isEnabled<CameraSystem>()) {
            fallbackCamera = domainCamera;
            break;
        }
    }
    return fallbackCamera;
}

std::shared_ptr<Texture> ViewportNode::fetchRenderResult(float simulationProgress) {
    assert(mRenderConfiguration.mFPSCap > 0.f && "FPS cannot be negative or zero");
    const uint32_t thresholdTime {static_cast<uint32_t>(1000 / mRenderConfiguration.mFPSCap)};
    bool renderOnce { false };

    switch (mRenderConfiguration.mUpdateMode) {
        case RenderConfiguration::UpdateMode::ONCE:
            mRenderConfiguration.mUpdateMode = RenderConfiguration::UpdateMode::NEVER;
            renderOnce = true;

            // fall through
        case RenderConfiguration::UpdateMode::ON_FETCH_CAP_FPS:
            if(!renderOnce && mTimeSinceLastRender < thresholdTime) {
                break;
            }

            // fall through
        case RenderConfiguration::UpdateMode::ON_FETCH:
            render_(simulationProgress);

            // fall through
        case RenderConfiguration::UpdateMode::ON_RENDER_CAP_FPS:
        case RenderConfiguration::UpdateMode::ON_RENDER:
        case RenderConfiguration::UpdateMode::NEVER:
        break;
    }

    getWorld().lock()->getSystem<RenderSystem>()->useRenderSet(mRenderSet);
    mTextureResult = getWorld().lock()->getSystem<RenderSystem>()->getCurrentScreenTexture();

    return mTextureResult;
}

uint32_t ViewportNode::render(float simulationProgress, uint32_t variableStep) {
    assert(mRenderConfiguration.mFPSCap > 0.f && "FPS cannot be negative or zero");
    const uint32_t thresholdTime {
        (
            mRenderConfiguration.mUpdateMode == RenderConfiguration::UpdateMode::ON_FETCH_CAP_FPS
            || mRenderConfiguration.mUpdateMode == RenderConfiguration::UpdateMode::ON_RENDER_CAP_FPS
        )? static_cast<uint32_t>(1000 / mRenderConfiguration.mFPSCap):
        0
    };

    mTimeSinceLastRender += variableStep;
    if(mTimeSinceLastRender > thresholdTime) {
        mTimeSinceLastRender = thresholdTime;
    }
    bool renderOnce { false };

    switch (mRenderConfiguration.mUpdateMode) {
        case RenderConfiguration::UpdateMode::ONCE:
            mRenderConfiguration.mUpdateMode = RenderConfiguration::UpdateMode::NEVER;
            renderOnce = true;

            // fall through
        case RenderConfiguration::UpdateMode::ON_RENDER_CAP_FPS:
            if(!renderOnce && mTimeSinceLastRender < thresholdTime) {
                break;
            }

            // fall through
        case RenderConfiguration::UpdateMode::ON_RENDER:
            render_(simulationProgress);

            // fall through
        case RenderConfiguration::UpdateMode::ON_FETCH:
        case RenderConfiguration::UpdateMode::ON_FETCH_CAP_FPS:
        case RenderConfiguration::UpdateMode::NEVER:
        break;
    }
    const uint32_t nextRenderTimeOffset { thresholdTime - mTimeSinceLastRender };
    return nextRenderTimeOffset;
}

void ViewportNode::render_(float simulationProgress) {

    mTimeSinceLastRender = 0;
    std::shared_ptr<ECSWorld> world = getWorld().lock();

    if(mOwnWorld) mOwnWorld->preRenderStep(simulationProgress);
    if(mRenderConfiguration.mRenderType == RenderConfiguration::RenderType::ADDITION) {
        // attach textures in reverse order of their appearance in the scene tree, so that viewports higher
        // up have their textures rendered on top of those lower down
        std::size_t childViewportIndex { 0 };
        for(auto childViewport { mChildViewports.rbegin() }; childViewport != mChildViewports.rend(); ++childViewport){
            if(std::shared_ptr<Texture> renderResult = (*childViewport)->fetchRenderResult(simulationProgress)) {
                /**
                  * NOTE: context change might have occurred because of fetchRenderResult. Make our own render set
                  * active once again
                  */
                world->getSystem<RenderSystem>()->useRenderSet(mRenderSet);
                world->getSystem<RenderSystem>()->addOrAssignRenderSource(
                    std::string("textureAddend_") + std::to_string(childViewportIndex++),
                    renderResult
                );
            }
        }
    }

    world->getSystem<RenderSystem>()->useRenderSet(mRenderSet);
    world->getSystem<RenderSystem>()->execute(simulationProgress);

    if(mRenderConfiguration.mRenderType == RenderConfiguration::RenderType::ADDITION) {
        for(std::size_t childViewportIndex { 0 }; childViewportIndex < mChildViewports.size(); ++childViewportIndex) {
            world->getSystem<RenderSystem>()->removeRenderSource(
                std::string("textureAddend_") + std::to_string(childViewportIndex)
            );
        }
    }
    if(mOwnWorld) mOwnWorld->postRenderStep(simulationProgress);
}

bool ViewportNode::handleAction(std::pair<ActionDefinition, ActionData> pendingAction) {
    // translate pointer input to coordinates that fall within the current viewport
    if(
        (pendingAction.first.mAttributes&InputAttributes::STATE_IS_LOCATION)
        && ((pendingAction.first.mAttributes&InputAttributes::N_AXES) == 2)
    ) {
        const SDL_Rect viewportCoordinates { getCenteredViewportCoordinates() };
        const glm::mat3 inputToViewportTransform {
            { static_cast<float>(mRenderConfiguration.mRequestedDimensions.x)/viewportCoordinates.w, 0.f, 0.f },
            { 0.f, static_cast<float>(mRenderConfiguration.mRequestedDimensions.y)/viewportCoordinates.h, 0.f },
            { 
                -static_cast<float>(viewportCoordinates.x)/viewportCoordinates.w,
                -static_cast<float>(viewportCoordinates.y)/viewportCoordinates.h,
                1.f
            },
        };
        pendingAction.second.mTwoAxisActionData.mValue = static_cast<glm::vec2>(inputToViewportTransform * glm::vec3{pendingAction.second.mTwoAxisActionData.mValue, 1.f});
    }

    bool actionHandled { false };
    actionHandled =  mActionDispatch.dispatchAction(pendingAction);

    // Prevent propagation to descendant viewports if flowthrough is disallowed.
    if(!mActionFlowthrough) return actionHandled;

    // Propagate action to descendants.
    for(std::shared_ptr<ViewportNode> child: mChildViewports) {
        if(child->isActive()) {
            bool childHandledAction { child->handleAction(pendingAction) };
            actionHandled = childHandledAction || actionHandled;

            if(childHandledAction && child->mPreventHandledActionPropagation) {
                break;
            }
        }
    }

    return actionHandled;
}

ActionDispatch& ViewportNode::getActionDispatch() {
    return mActionDispatch;
}

std::shared_ptr<ViewportNode> ViewportNode::getLocalViewport() {
    return std::static_pointer_cast<ViewportNode>(shared_from_this());
}

std::shared_ptr<const ViewportNode> ViewportNode::getLocalViewport() const {
    return std::static_pointer_cast<const ViewportNode>(std::const_pointer_cast<const SceneNodeCore>(shared_from_this()));
}

std::vector<std::shared_ptr<ViewportNode>> ViewportNode::getActiveDescendantViewports() {
    assert(isActive() && "Can only find active descendant viewports of an active viewport node");

    std::vector<std::shared_ptr<ViewportNode>> activeViewports {};
    for(auto child: mChildViewports) {
        if(!child->isActive()) continue;
        std::vector<std::shared_ptr<ViewportNode>> activeDescendantViewports { child->getActiveDescendantViewports() };
        activeViewports.insert(activeViewports.end(), activeDescendantViewports.begin(), activeDescendantViewports.end());
    }
    activeViewports.push_back(std::static_pointer_cast<ViewportNode>(shared_from_this()));

    return activeViewports;
}

std::vector<std::weak_ptr<ECSWorld>> ViewportNode::getActiveDescendantWorlds() {
    assert(isActive() && "Can only find active descendant worlds of an active viewportNode");

    std::vector<std::weak_ptr<ECSWorld>> activeWorlds {};
    if(mOwnWorld) activeWorlds.push_back(mOwnWorld);
    for(auto child: mChildViewports) {
        if(!child->isActive()) continue;

        std::vector<std::weak_ptr<ECSWorld>> activeDescendantWorlds { child->getActiveDescendantWorlds() };
        activeWorlds.insert(activeWorlds.end(), activeDescendantWorlds.begin(), activeDescendantWorlds.end());
    }

    return activeWorlds;
}

void SceneSystem::simulationStep(uint32_t simStepMillis, std::vector<std::pair<ActionDefinition, ActionData>> triggeredActions) {
    std::queue<std::shared_ptr<ViewportNode>> viewportsToVisit { {mRootNode} };
    while(!viewportsToVisit.empty()) {
        std::shared_ptr<ViewportNode> viewport { viewportsToVisit.front() };
        viewportsToVisit.pop();
        if(!viewport->isActive()) continue;

        if(viewport->mOwnWorld) {
            viewport->mOwnWorld->simulationPreStep(simStepMillis);
        }

        for(auto& childViewport: viewport->mChildViewports) {
            viewportsToVisit.push(childViewport);
        }
    }

    for(const auto& pendingAction: triggeredActions) {
        mRootNode->handleAction(pendingAction);
    }

    viewportsToVisit.push(mRootNode);
    while(!viewportsToVisit.empty()) {
        std::shared_ptr<ViewportNode> viewport { viewportsToVisit.front() };
        viewportsToVisit.pop();
        if(!viewport->isActive()) continue;

        if(viewport->mOwnWorld) {
            viewport->mOwnWorld->simulationStep(simStepMillis);
        }

        for(auto& childViewport: viewport->mChildViewports) {
            viewportsToVisit.push(childViewport);
        }
    }

    updateTransforms();
    viewportsToVisit.push(mRootNode);
    while(!viewportsToVisit.empty()) {
        std::shared_ptr<ViewportNode> viewport { viewportsToVisit.front() };
        viewportsToVisit.pop();

        if(!viewport->isActive()) continue;

        if(viewport->mOwnWorld) {
            viewport->mOwnWorld->postTransformUpdate(simStepMillis);
            viewport->mOwnWorld->simulationPostStep(simStepMillis);
        }

        for(auto& childViewport: viewport->mChildViewports) {
            viewportsToVisit.push(childViewport);
        }
    }
    updateTransforms();
}

void SceneSystem::variableStep(float simulationProgress, uint32_t simulationLagMillis, uint32_t variableStepMillis, std::vector<std::pair<ActionDefinition, ActionData>> triggeredActions) {
    for(const auto& pendingAction: triggeredActions) {
        mRootNode->handleAction(pendingAction);
    }

    std::queue<std::shared_ptr<ViewportNode>> viewportsToVisit { {mRootNode} };
    while(!viewportsToVisit.empty()) {
        std::shared_ptr<ViewportNode> viewport { viewportsToVisit.front() };
        viewportsToVisit.pop();
        if(!viewport->isActive()) continue;

        if(viewport->mOwnWorld) {
            viewport->mOwnWorld->variableStep(simulationProgress, variableStepMillis);
        }

        for(auto& childViewport: viewport->mChildViewports) {
            viewportsToVisit.push(childViewport);
        }
    }

    updateTransforms();

    viewportsToVisit.push({mRootNode});
    while(!viewportsToVisit.empty()) {
        std::shared_ptr<ViewportNode> viewport { viewportsToVisit.front() };
        viewportsToVisit.pop();

        if(!viewport->isActive()) continue;

        if(viewport->mOwnWorld) {
            viewport->mOwnWorld->postTransformUpdate(simulationLagMillis);
        }

        for(auto& childViewport: viewport->mChildViewports) {
            viewportsToVisit.push(childViewport);
        }
    }
    updateTransforms();
}

uint32_t SceneSystem::render(float simulationProgress, uint32_t variableStep) {
    uint32_t nextRenderTimeOffset { std::numeric_limits<uint32_t>::max() };
    std::vector<std::shared_ptr<ViewportNode>> activeViewports { getActiveViewports() };

    // render viewports in reverse order
    const std::size_t nActiveViewports { activeViewports.size() };
    for(std::size_t i {0}; i < nActiveViewports; ++i) {
        const uint32_t renderTimeOffset {
            activeViewports[i]->render(simulationProgress, variableStep)
        };
        nextRenderTimeOffset = glm::min(renderTimeOffset, nextRenderTimeOffset);
    }

    mRootNode->getWorld().lock()->getSystem<RenderSystem>()->useRenderSet(mRootNode->mRenderSet);
    mRootNode->getWorld().lock()->getSystem<RenderSystem>()->renderToScreen();

    return nextRenderTimeOffset;
}

void SceneSystem::onApplicationEnd() {
    nodeActivationChanged(mRootNode, false);
    mRootNode = nullptr;
}

bool SceneSystem::inScene(std::shared_ptr<const SceneNodeCore> sceneNode) const {
    return inScene(sceneNode->getUniversalEntityID());
}
bool SceneSystem::inScene(UniversalEntityID UniversalEntityID) const {
    return mEntityToNode.find(UniversalEntityID) != mEntityToNode.end();
}

bool SceneSystem::isActive(std::shared_ptr<const SceneNodeCore> sceneNode) const {
    return isActive(sceneNode->getUniversalEntityID());
}
bool SceneSystem::isActive(UniversalEntityID UniversalEntityID) const {
    return mActiveEntities.find(UniversalEntityID) != mActiveEntities.end();
}

std::shared_ptr<SceneNodeCore> SceneSystem::getNode(const std::string& where) {
    assert(where != "/" && "Cannot retrieve scene system's root node");
    return mRootNode->getNode(where);
}

std::shared_ptr<SceneNodeCore> SceneSystem::removeNode(const std::string& where) {
    assert(where != "/" && "Cannot remove scene system's root node");
    return mRootNode->removeNode(where);
}

std::weak_ptr<ECSWorld> SceneSystem::getRootWorld() const {
    return mRootNode->getWorld();
}

std::vector<std::shared_ptr<ViewportNode>> SceneSystem::getActiveViewports() {
    return mRootNode->getActiveDescendantViewports();
}
std::vector<std::weak_ptr<ECSWorld>> SceneSystem::getActiveWorlds() {
    return mRootNode->getActiveDescendantWorlds();
}

ViewportNode& SceneSystem::getRootViewport() const {
    return *mRootNode;
}

void SceneSystem::addNode(std::shared_ptr<SceneNodeCore> sceneNode, const std::string& where) {
    mRootNode->addNode(sceneNode, where);
}

void SceneSystem::updateHierarchyDataInsertion(std::shared_ptr<SceneNodeCore> insertedNode) {
    SceneHierarchyData insertedNodeHierarchyData {};

    std::shared_ptr<SceneNodeCore> parentNode { insertedNode->mParent.lock() };
    if(!parentNode || parentNode->getWorldID() != insertedNode->getWorldID()) {
        // a viewport with its own world or the root viewport of the scene is being added
        insertedNode->updateComponent(insertedNodeHierarchyData);
        return;
    }

    // find the end of the list of the inserted node's siblings
    WorldID parentWorld { parentNode->getWorldID() };
    std::shared_ptr<SceneNodeCore> siblingNode { 
        parentNode->getComponent<SceneHierarchyData>().mChild != kMaxEntities? 
        mEntityToNode[{parentWorld, parentNode->getComponent<SceneHierarchyData>().mChild}].lock():
        nullptr
    };

    for(/*pass*/;
        siblingNode != nullptr && siblingNode->getComponent<SceneHierarchyData>().mSibling != kMaxEntities;
        siblingNode = mEntityToNode[{parentWorld, siblingNode->getComponent<SceneHierarchyData>().mSibling}].lock()
    );

    // if current hierarchy node is that of a sibling, update the sibling link
    if(siblingNode != nullptr) {
        SceneHierarchyData siblingHierarchyData{ siblingNode->getComponent<SceneHierarchyData>() };
        siblingHierarchyData.mSibling = insertedNode->getEntityID();
        siblingNode->updateComponent<SceneHierarchyData>(siblingHierarchyData);

    // otherwise update the parent's child link
    } else {
        SceneHierarchyData parentHierarchyData { parentNode->getComponent<SceneHierarchyData>() };
        parentHierarchyData.mChild = insertedNode->getEntityID();
        parentNode->updateComponent<SceneHierarchyData>(parentHierarchyData);
    }

    // finally, update the scene node's own hierarchy data
    insertedNodeHierarchyData.mParent = parentNode->getEntityID();
    insertedNode->updateComponent<SceneHierarchyData>(insertedNodeHierarchyData);
}

void SceneSystem::updateHierarchyDataRemoval(std::shared_ptr<SceneNodeCore> removedNode) {
    SceneHierarchyData removedNodeHierarchyData { removedNode->getComponent<SceneHierarchyData>() };
    std::shared_ptr<SceneNodeCore> parentNode { removedNode->mParent.lock() };

    if(removedNodeHierarchyData.mParent == kMaxEntities || !parentNode) {
        // this node is a viewport node that owns its own world, or the root node of the scene system
        return;
    }

    // find the removed node's immediate sibling
    WorldID parentWorld { parentNode->getWorldID() };
    std::shared_ptr<SceneNodeCore> siblingNode { mEntityToNode[{parentWorld, parentNode->getComponent<SceneHierarchyData>().mChild}].lock() };
    EntityID removedNodeEntityID { removedNode->getEntityID() };

    for( /*pass*/;
        siblingNode->getComponent<SceneHierarchyData>().mSibling != removedNodeEntityID && siblingNode != removedNode;
        siblingNode = mEntityToNode[ {parentWorld, siblingNode->getComponent<SceneHierarchyData>().mSibling} ].lock()
    );

    // if current hierarchy data is that of a sibling, update the sibling link
    if(siblingNode != removedNode) {
        SceneHierarchyData siblingHierarchyData { siblingNode->getComponent<SceneHierarchyData>() };
        siblingHierarchyData.mSibling = removedNodeHierarchyData.mSibling;
        siblingNode->updateComponent(siblingHierarchyData);

    // otherwise update the parent's child link
    } else {
        SceneHierarchyData parentHierarchyData { parentNode->getComponent<SceneHierarchyData>() };
        parentHierarchyData.mChild = removedNodeHierarchyData.mSibling;
        parentNode->updateComponent(parentHierarchyData);
    }

    // nothing more to be done, the removed node's hierarchy data is irrelevant now that it
    // is being removed
}

void SceneSystem::nodeAdded(std::shared_ptr<SceneNodeCore> sceneNode) {
    if(!inScene(sceneNode->mParent.lock())) return;

    // Move this node to the world to which it belongs, where viewport nodes (may) mark 
    // the boundary between worlds
    {
        std::shared_ptr<ViewportNode> viewport { std::dynamic_pointer_cast<ViewportNode>(sceneNode) };
        if(viewport && viewport->mOwnWorld) {
            viewport->joinWorld(*viewport->mOwnWorld);
        } else {
            sceneNode->joinWorld(*sceneNode->mParent.lock()->getWorld().lock());
        }
        updateHierarchyDataInsertion(sceneNode);
    }
    mEntityToNode[sceneNode->getUniversalEntityID()] = sceneNode;


    // when a node is added to the scene, all its children should
    // be in the scene also, so have them registered. Also move every
    // node to its world, and switch worlds if a viewport 
    // node containing its own world is found
    for(auto& descendant: sceneNode->getDescendants()) {
        {
            std::shared_ptr<ViewportNode> viewport { std::dynamic_pointer_cast<ViewportNode>(descendant) };
            if(viewport && viewport->mOwnWorld) {
                viewport->joinWorld(*viewport->mOwnWorld);
            } else {
                descendant->joinWorld(*descendant->mParent.lock()->getWorld().lock());
            }
            updateHierarchyDataInsertion(descendant);
        }
        mEntityToNode[descendant->getUniversalEntityID()] = descendant;
    }

    // let the scene system activate systems on those nodes that
    // are enabled
    nodeActivationChanged(sceneNode, sceneNode->mStateFlags&SceneNodeCore::StateFlags::ENABLED);
}

void SceneSystem::nodeRemoved(std::shared_ptr<SceneNodeCore> sceneNode) {
    if(!inScene(sceneNode)) return;

    updateHierarchyDataRemoval(sceneNode);

    // disable the node and its children so that it is no
    // longer seen by any system
    nodeActivationChanged(sceneNode, false);

    // lose all references to the node and its descendants
    for(auto& descendant: sceneNode->getDescendants()) {
        mEntityToNode.erase(descendant->getUniversalEntityID());
    }
    mEntityToNode.erase(sceneNode->getUniversalEntityID());
}

void SceneSystem::nodeActivationChanged(std::shared_ptr<SceneNodeCore> sceneNode, bool state) {
    assert(sceneNode && "Null node reference cannot be enabled");

    // early exit if this node isn't in the scene, or if its parent
    // isn't active, or node is already in requested state
    if(
        !inScene(sceneNode)
        || (sceneNode->mName != kSceneRootName && !isActive(sceneNode->mParent.lock()))
        || (isActive(sceneNode) && state) 
        || (!isActive(sceneNode) && !state)
    ) { return; }

    if(state) activateSubtree(sceneNode);
    else deactivateSubtree(sceneNode);
}

void SceneSystem::activateSubtree(std::shared_ptr<SceneNodeCore> rootNode) {
    for(auto& childNode: rootNode->getChildren()) {
        if(childNode->mStateFlags&SceneNodeCore::StateFlags::ENABLED) activateSubtree(childNode);
    }

    rootNode->mStateFlags |= SceneNodeCore::StateFlags::ACTIVE;
    rootNode->mEntity->enableSystems(rootNode->mSystemMask);
    mActiveEntities.insert(rootNode->getUniversalEntityID());
    mComputeTransformQueue.insert(rootNode->getUniversalEntityID());

    rootNode->onActivated();
}

void SceneSystem::deactivateSubtree(std::shared_ptr<SceneNodeCore> rootNode) {
    rootNode->onDeactivated();

    rootNode->mEntity->disableSystems();
    mActiveEntities.erase(rootNode->getUniversalEntityID());
    mComputeTransformQueue.erase(rootNode->getUniversalEntityID());
    rootNode->mStateFlags &= ~SceneNodeCore::StateFlags::ACTIVE;

    for(auto& childNode: rootNode->getChildren()) {
        if(childNode->mStateFlags&SceneNodeCore::StateFlags::ENABLED) {
            deactivateSubtree(childNode);
        }
    }
}

void SceneSystem::updateTransforms() {
    // Prune the transform queue of nodes that will be
    // covered by their ancestor's update
    std::set<std::pair<WorldID, EntityID>> entitiesToIgnore {};
    for(std::pair<WorldID, EntityID> entityWorldPair: mComputeTransformQueue) {
        if(mEntityToNode.at(entityWorldPair).lock() == mRootNode) continue;
        std::shared_ptr<SceneNodeCore> sceneNode { mEntityToNode.at(entityWorldPair).lock()->mParent };
        while(sceneNode != nullptr) {
            if(mComputeTransformQueue.find(sceneNode->getUniversalEntityID()) != mComputeTransformQueue.end()) {
                entitiesToIgnore.insert(entityWorldPair);
                break;
            }
            sceneNode = sceneNode->mParent.lock();
        }
    }

    for(std::pair<WorldID, EntityID> entityWorldPair: entitiesToIgnore) {
        mComputeTransformQueue.erase(entityWorldPair);
    }

    // Apply transform updates to all subtrees present in the queue
    for(std::pair<WorldID, EntityID> entityWorldPair: mComputeTransformQueue) {
        std::vector<std::shared_ptr<SceneNodeCore>> toVisit { {mEntityToNode.at(entityWorldPair).lock()} };
        while(!toVisit.empty()) {
            std::shared_ptr<SceneNodeCore> currentNode { toVisit.back() };
            toVisit.pop_back();

            glm::mat4 localModelMatrix { getLocalTransform(currentNode).mModelMatrix };
            glm::mat4 worldMatrix { getCachedWorldTransform(currentNode->mParent.lock()).mModelMatrix };
            currentNode->updateComponent<Transform>({worldMatrix * localModelMatrix});

            for(std::shared_ptr<SceneNodeCore> child: currentNode->getChildren()) {
                toVisit.push_back(child);
            }
        }
    }
    mComputeTransformQueue.clear();
}

Transform SceneSystem::getLocalTransform(std::shared_ptr<const SceneNodeCore> sceneNode) const {
    constexpr Transform rootTransform {
        glm::mat4{1.f}
    };   
    if(!sceneNode) { return rootTransform; }
    const Placement nodePlacement { sceneNode->getComponent<Placement>() };
    return Transform{ buildModelMatrix (
        nodePlacement.mPosition,
        nodePlacement.mOrientation,
        nodePlacement.mScale
    )};
}

Transform SceneSystem::getCachedWorldTransform(std::shared_ptr<const SceneNodeCore> sceneNode) const {
    constexpr Transform rootTransform { glm::mat4{1.f} };
    if(!sceneNode) { return rootTransform; }
    return sceneNode->getComponent<Transform>();
}

void SceneSystem::markDirty(UniversalEntityID UniversalEntityID) {
    if(!isActive(UniversalEntityID)) return;
    mComputeTransformQueue.insert(UniversalEntityID);
}

void SceneSystem::onWorldEntityUpdate(UniversalEntityID UniversalEntityID) {
    markDirty(UniversalEntityID);
}

void SceneSystem::onApplicationInitialize(const ViewportNode::RenderConfiguration& rootViewportRenderConfiguration) {
    mRootNode = ViewportNode::create(SceneNodeCore::Key{}, kSceneRootName, false, rootViewportRenderConfiguration, nullptr);

    // Manual setup of root node, since it skips the normal activation procedure
    mRootNode->mStateFlags |= SceneNodeCore::StateFlags::ENABLED;
    mEntityToNode.insert({mRootNode->getUniversalEntityID(), mRootNode});
    mRootNode->updateComponent<Transform>({glm::mat4{1.f}});
}

void SceneSystem::onApplicationStart() {
    nodeActivationChanged(mRootNode, true);
    updateTransforms();
}

void SceneSystem::SceneSubworld::onEntityUpdated(EntityID entityID) {
    mWorld.lock()->getSystem<SceneSystem>()->onWorldEntityUpdate({mWorld.lock()->getID(), entityID});
}

std::shared_ptr<SceneNodeCore> SceneSystem::getNodeByID(const UniversalEntityID& universalEntityID) {
    const auto& foundNode { mEntityToNode.find(universalEntityID) };
    assert(foundNode != mEntityToNode.end() && "Could not find a node with this ID present in the tree");
    return foundNode->second.lock();
}

std::vector<std::shared_ptr<SceneNodeCore>> SceneSystem::getNodesByID(const std::vector<UniversalEntityID>& universalEntityIDs) {
    // allocate space for results
    const std::size_t resultListLength { universalEntityIDs.size() };
    std::vector<std::shared_ptr<SceneNodeCore>> resultNodes { resultListLength };

    // obtain node corresponding to each entity in list
    for(std::size_t nodeIndex{0}; nodeIndex < resultListLength; ++nodeIndex) {
        resultNodes[nodeIndex] = getNodeByID(universalEntityIDs[nodeIndex]);
    }

    return resultNodes;
}

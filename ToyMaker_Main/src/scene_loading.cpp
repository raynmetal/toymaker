#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "toymaker/engine/scene_loading.hpp"

using namespace ToyMaker;

std::shared_ptr<IResource> SceneFromFile::createResource(const nlohmann::json& sceneFileDescription) {
    std::string scenePath { sceneFileDescription.at("path").get<std::string>() };
    std::ifstream jsonFileStream;

    jsonFileStream.open(scenePath);
    nlohmann::json sceneDescription { nlohmann::json::parse(jsonFileStream) };
    jsonFileStream.close();
    
    std::cout << "Loading Resource: " << nlohmann::to_string(sceneDescription) << "\n";

    return ResourceDatabase::ConstructAnonymousResource<SceneNode>({
        { "type", SimObject::getResourceTypeName() },
        { "method", SceneFromDescription::getResourceConstructorName() },
        { "parameters", sceneDescription[0].get<nlohmann::json>() },
    });
}

std::shared_ptr<IResource> SceneFromDescription::createResource(const nlohmann::json& sceneDescription) {
    // load resources
    loadResources(sceneDescription.at("resources"));
    std::shared_ptr<SimObject> localRoot { loadSceneNodes(sceneDescription.at("nodes")) };
    loadConnections(sceneDescription.at("connections"), localRoot);

    return localRoot;
}

void SceneFromDescription::loadResources(const nlohmann::json& resourceList) {
    // Load resources described by this scene
    for(const nlohmann::json& resourceDescription: resourceList) {
        assert(
            resourceDescription.at("type").get<std::string>() != SceneNode::getResourceTypeName()
            && "Resource section cannot contain descriptions of scene nodes, only SimObjects loaded via files"
        );
        if(resourceDescription.at("type") == SimObject::getResourceTypeName()) {
            assert(
                resourceDescription.at("method") == SceneFromFile::getResourceConstructorName()
                && "Only scenes loaded from files may be defined in the resources section of a scene"
            );
        }
        ResourceDatabase::AddResourceDescription(resourceDescription);
    }
}

std::shared_ptr<SimObject> SceneFromDescription::loadSceneNodes(const nlohmann::json& nodeList) {
    assert(nodeList.is_array() && "A scene's \"nodes\" property must be an array of node descriptions");

    auto nodeDescriptionIterator { nodeList.cbegin() };
    const nlohmann::json& localRootDescription { nodeDescriptionIterator.value() };
    std::shared_ptr<SimObject> localRoot { nullptr };

    std::cout << "Loading root node : " << nlohmann::to_string(localRootDescription) << "\n";

    assert(localRootDescription.at("parent").get<std::string>() == "" 
        && "Root node must not have a parent"
    );
    assert(localRootDescription.at("type") == SimObject::getResourceTypeName() && "Scene's root must be a sim object");
    localRoot = ResourceDatabase::ConstructAnonymousResource<SimObject>({
        {"type", SimObject::getResourceTypeName()},
        {"method", SimObjectFromDescription::getResourceConstructorName()},
        {"parameters", localRootDescription},
    });

    for(++nodeDescriptionIterator; nodeDescriptionIterator != nodeList.cend(); ++nodeDescriptionIterator) {
        const nlohmann::json& nodeDescription { nodeDescriptionIterator.value() };
        std::shared_ptr<SceneNodeCore> node { nullptr };
        std::cout << "Loading node : " << nlohmann::to_string(nodeDescription) << "\n";

        assert(
            nodeDescription.at("parent").get<std::string>() != "" 
            && "All scene nodes besides the root must specify a parent node present in the same scene file"
        );
        // assert(
        //     localRoot->hasNode(nodeDescription.at("parent")) && "The path specified by the scene node does not exist"
        // );

        // construct scene node resource according to its type
        if(nodeDescription.at("type") == SimObject::getResourceTypeName()) {
            node = ResourceDatabase::ConstructAnonymousResource<SimObject>({
                {"type", SimObject::getResourceTypeName()},
                {"method", SimObjectFromDescription::getResourceConstructorName()},
                {"parameters", nodeDescription},
            });

        } else if(nodeDescription.at("type") == SceneNode::getResourceTypeName()) {
            node = ResourceDatabase::ConstructAnonymousResource<SceneNode>({
                {"type", SceneNode::getResourceTypeName()},
                {"method", SceneNodeFromDescription::getResourceConstructorName()},
                {"parameters", nodeDescription},
            });

        } else if(nodeDescription.at("type") == ViewportNode::getResourceTypeName()) {
            node = ResourceDatabase::ConstructAnonymousResource<ViewportNode>({
                {"type", ViewportNode::getResourceTypeName()},
                {"method", ViewportNodeFromDescription::getResourceConstructorName()},
                {"parameters", nodeDescription},
            });

        // TODO: make it so that certain resource constructors can define resource aliases
        // that refer to the same type (and by extension any tables associated with it)
        } else if(nodeDescription.at("type") == "Scene") {
            node = ResourceDatabase::GetRegisteredResource<SimObject>(
                nodeDescription.at("name").get<std::string>()
            );
            if(nodeDescription.at("copy").get<bool>()) {
                std::shared_ptr<SceneNodeCore> prototype { node };
                node = SimObject::copy(std::static_pointer_cast<SimObject>(prototype));
                // TODO: think of a safer way to accomplish this.
                node->setPrototype_(prototype);
            }
            if(nodeDescription.find("overrides") != nodeDescription.end()) {
                const nlohmann::json& overrides = nodeDescription.at("overrides");
                if(overrides.find("name") != overrides.end()) {
                    node->setName(overrides.at("name").get<std::string>());
                }
                if(overrides.find("components") != overrides.end()) {
                    overrideComponents(std::static_pointer_cast<ToyMaker::SimObject>(node), overrides.at("components"));
                }
                if(overrides.find("aspects") != overrides.end()) {
                    overrideAspects(std::static_pointer_cast<ToyMaker::SimObject>(node), overrides.at("aspects"));
                }
            }
        } else {
            assert(false && "Scene nodes in file must be scene nodes, sim objects, or reference to a scene node file resource");
        }

        localRoot->addNode(node, nodeDescription.at("parent").get<std::string>());
    }

    return localRoot;
}

void SceneFromDescription::loadConnections(const nlohmann::json& connectionList, std::shared_ptr<SceneNodeCore> localRoot) {
    for(const nlohmann::json& connection: connectionList) {
        BaseSimObjectAspect& signalFrom { localRoot->getByPath<BaseSimObjectAspect&>(connection.at("from").get<std::string>()) };
        BaseSimObjectAspect& signalTo { localRoot->getByPath<BaseSimObjectAspect&>(connection.at("to").get<std::string>()) };
        signalTo.connect(
            connection.at("signal").get<std::string>(),
            connection.at("observer").get<std::string>(),
            signalFrom
        );
    }
}
void SceneFromDescription::overrideComponents(std::shared_ptr<SimObject> node, const nlohmann::json& components) {
    for(const auto& component: components) {
        node->addOrUpdateComponent(component);
    }
}
void SceneFromDescription::overrideAspects(std::shared_ptr<SimObject> node, const nlohmann::json& aspects) {
    for(const auto& aspect: aspects) {
        node->addOrReplaceAspect(aspect);
    }
}

std::shared_ptr<IResource> SceneNodeFromDescription::createResource(const nlohmann::json& methodParams) {
    return SceneNode::create(methodParams);
}

std::shared_ptr<IResource> SimObjectFromDescription::createResource(const nlohmann::json& methodParameters) {
    return SimObject::create(methodParameters);
}

std::shared_ptr<IResource> ViewportNodeFromDescription::createResource(const nlohmann::json& methodParameters) {
    return ViewportNode::create(methodParameters);
}

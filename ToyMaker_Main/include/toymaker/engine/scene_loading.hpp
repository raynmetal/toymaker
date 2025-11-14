/**
 * @ingroup ToyMakerSerialization ToyMakerSceneSystem ToyMakerResourceDB
 * @file scene_loading.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief A collection of ResourceConstructor classes responsible for loading a scene into the engine.
 * @version 0.3.2
 * @date 2025-09-06
 * 
 */

#ifndef FOOLSENGINE_SCENELOADING_H
#define FOOLSENGINE_SCENELOADING_H

#include "core/resource_database.hpp"

#include "sim_system.hpp"
#include "scene_system.hpp"

namespace ToyMaker {
    class SceneFromDescription;
    class SceneNodeFromDescription;
    class SimObjectFromDescription;
    class ViewportNodeFromDescription;

    /**
     * @ingroup ToyMakerResourceDB ToyMakerSceneSystem
     * @brief Constructs a scene tree from a file containing its JSON description.
     * 
     * Its appearance in JSON might be as follows:
     * 
     * ```jsonc
     * 
     * {
     *     "name": "Ur_Button",
     *     "type": "SimObject",
     *     "method": "fromSceneFile",
     *     "parameters": {
     *         "path": "data/ur_button.json"
     *     }
     * }
     * 
     * ```
     * 
     * @see SceneFromDescription
     * 
     */
    class SceneFromFile: public ResourceConstructor<SimObject, SceneFromFile> {
    public:
        SceneFromFile():
        ResourceConstructor<SimObject, SceneFromFile> {0}
        {}

        /**
         * @brief The resource constructor type string associated with this constructor.
         * 
         * @return std::string This constructor's resource constructor type string.
         */
        static std::string getResourceConstructorName() { return "fromSceneFile"; }
    private:
        /**
         * @brief Creates a resource from its JSON description stored in a file at the path specified.
         * 
         * @param methodParams The parameters based on which the scene should be created.
         * @return std::shared_ptr<IResource> Handle to the resource base pointer of the root node of the newly created scene.
         */
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParams);
    };

    /**
     * @ingroup ToyMakerResourceDB ToyMakerSceneSystem
     * @brief Constructs a scene tree (separate from @em the scene tree) based on its description in JSON.
     * 
     * It will have, as its root, a SimObject which serves (or is intended to serve) as the interface between it and the scene it was imported into.
     * 
     * An example of such a json description is given below:
     * 
     * ```jsonc
     * 
     * {
     *     "resources": [],
     *     "nodes": [
     *         { 
     *             "name": "ur_button",
     *             "aspects": [
     *                 {
     *                     "type": "UIButton",
     *                     "anchor": [0.5, 0.5],
     *                     "scale": 1.0,
     *                     "value": "",
     *                     "text": "Default Text",
     *                     "font_resource_name": "Roboto_Mono_Regular_24",
     *                     "panel_active": "Bad_Panel",
     *                     "panel_inactive": "Bad_Panel",
     *                     "panel_hover": "Bad_Panel",
     *                     "panel_pressed": "Bad_Panel",
     *                     "has_highlight": false,
     *                     "highlight": "Bad_Panel",
     *                     "highlight_color": [0, 0, 0, 0]
     *                 }
     *             ],
     *             "components": [
     *                 {
     *                     "orientation": [ 1.0, 0.0, 0.0, 0.0 ],
     *                     "position": [ 0.0, 0.0, 0.0, 1.0 ],
     *                     "scale": [ 1.0, 1.0, 1.0 ],
     *                     "type": "Placement"
     *                 }
     *             ],
     *             "parent": "",
     *             "type": "SimObject"
     *         },
     *         {
     *             "name": "button_text",
     *             "parent": "/",
     *             "type": "SimObject",
     *             "components": [
     *                 {
     *                     "orientation": [ 1.0, 0.0, 0.0, 0.0 ],
     *                     "position": [ 0.0, 0.0, 0.0, 1.0 ],
     *                     "scale": [ 1.0, 1.0, 1.0 ],
     *                     "type": "Placement"
     *                 }
     *             ],
     *             "aspects": [
     *                 {
     *                     "type": "UIText",
     *                     "text": "Default Text",
     *                     "font_resource_name": "Roboto_Mono_Regular_24",
     *                     "scale": 1.0,
     *                     "anchor": [1.0, 1.0]
     *                 }
     *             ]
     *         },
     *         {
     *             "name": "highlight",
     *             "parent": "/",
     *             "type": "SceneNode",
     *             "components": [
     *                 {
     *                     "orientation": [1.0, 0.0, 0.0, 0.0],
     *                     "position": [ 0.0, 0.0, 0.0, 1.0 ],
     *                     "scale": [ 1.0, 1.0, 1.0 ],
     *                     "type": "Placement"
     *                 }
     *             ]
     *         }
     *     ],
     *     "connections": []
     * }
     * 
     * ```
     * 
     * @see SceneNodeFromDescription
     * @see SimObjectFromDescription
     * @see ViewportNodeFromDescription
     * @see SignalTracker
     */
    class SceneFromDescription: public ResourceConstructor<SimObject, SceneFromDescription> {
    public:
        SceneFromDescription(): 
        ResourceConstructor<SimObject, SceneFromDescription> {0}
        {}

        /**
         * @brief Gets this constructor's resource constructor type string.
         * 
         * @return std::string This constructor's resource constructor type string.
         */
        static std::string getResourceConstructorName() { return "fromSceneDescription"; }

    private:
        /**
         * @brief Loads the resources listed in the resource section of this scene description.
         * 
         * The resources used by this scene may have already been loaded by another scene, in which case they needn't be respecified here.
         * 
         * @param resourceList List of Resource descriptions to load into the ResourceDatabase.
         */
        void loadResources(const nlohmann::json& resourceList);

        /**
         * @brief Loads the scene nodes listed in the "nodes" section of the scene description.
         * 
         * @param nodeList A list of SimObjects, SceneNodes, and ViewportNodes, describing this scene.
         * @return std::shared_ptr<SimObject> The root of the newly constructed scene tree.
         */
        std::shared_ptr<SimObject> loadSceneNodes(const nlohmann::json& nodeList);

        /**
         * @brief Loads connections between nodes (functioning as SignalTrackers) within a scene.
         * 
         * @param connectionList A list of SimObjectAspect -> SimObjectAspect signal -> observer connections.
         * @param localRoot The root of the scene being constructed, relative to which scene paths are evaluated.
         */
        void loadConnections(const nlohmann::json& connectionList, std::shared_ptr<SceneNodeCore> localRoot);

        /**
         * @brief (When used as a node in another scene) a list of overrides to ECSWorld components to be applied to the root node of an imported scene.
         * 
         * @param node A node that was created from a scene file and imported into the present scene.
         * @param componentList A list of component overrides to be applied to the imported node.
         */
        void overrideComponents(std::shared_ptr<SimObject> node, const nlohmann::json& componentList);

        /**
         * @brief (When used as a node in another scene) a list of overrides to SimObjectAspects to be applied to the root node of an imported scene.
         * 
         * @param node A node that was created from a scene file and imported into the present scene.
         * @param aspectList A list of aspect overrides to be applied to the imported node.
         */
        void overrideAspects(std::shared_ptr<SimObject> node, const nlohmann::json& aspectList);

        /**
         * @brief Actually creates a scene tree out of its JSON description.
         * 
         * @param methodParams The scene's description in JSON.
         * @return std::shared_ptr<IResource> A handle to the base pointer of the root node of a newly created scene.
         */
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParams) override;
    };

    /**
     * @ingroup ToyMakerResourceDB ToyMakerSceneSystem
     * @brief Constructs a scene node based on its description in JSON.
     * 
     * @see SceneFromDescription
     * 
     */
    class SceneNodeFromDescription: public ResourceConstructor<SceneNode, SceneNodeFromDescription> {
    public:
        SceneNodeFromDescription(): 
        ResourceConstructor<SceneNode, SceneNodeFromDescription>{0}
        {}

        static std::string getResourceConstructorName() { return "fromNodeDescription"; }
    private:

        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParams) override;
    };

    /**
     * @ingroup ToyMakerResourceDB ToyMakerSceneSystem
     * @brief Constructs a SimObject from its description in JSON.
     * 
     * @see SceneFromDescription
     * 
     */
    class SimObjectFromDescription: public ResourceConstructor<SimObject, SimObjectFromDescription> {
    public:
        SimObjectFromDescription():
        ResourceConstructor<SimObject, SimObjectFromDescription> {0}
        {}

        static std::string getResourceConstructorName() { return "fromDescription"; }
    private:
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters);
    };

    /**
     * @ingroup ToyMakerResourceDB ToyMakerSceneSystem
     * @brief Constructs a ViewportNode from its description in JSON.
     * 
     * Its description might look as follows:
     * 
     * ```jsonc
     * 
     * {
     *     "name": "viewport_3D",
     *     "parent": "/",
     *     "type": "ViewportNode",
     *     "components": [
     *         {
     *             "type": "Placement",
     *             "orientation": [
     *                 1.0, 0.0, 0.0, 0.0
     *             ],
     *             "position": [
     *                 0.0, 0.0, 0.0, 1.0
     *             ],
     *             "scale": [
     *                 1.0, 1.0, 1.0
     *             ]
     *         }
     *     ],
     *     "inherits_world": false,
     *     "prevent_handled_action_propagation": false,
     *     "skybox_texture": "Skybox_Texture",
     *     "render_configuration": {
     *         "base_dimensions": [1366, 768],
     *         "update_mode": "on-render-cap-fps",
     *         "resize_type": "texture-dimensions",
     *         "resize_mode": "fixed-dimensions",
     *         "render_type": "basic-3d",
     *         "fps_cap": 60,
     *         "render_scale": 1.0
     *     }
     * },
     * 
     * ```
     * 
     */
    class ViewportNodeFromDescription: public ResourceConstructor<ViewportNode, ViewportNodeFromDescription> {
    public:
        ViewportNodeFromDescription():
        ResourceConstructor<ViewportNode, ViewportNodeFromDescription> {0}
        {}

        static std::string getResourceConstructorName() { return "fromDescription"; }

    private:
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters);
    };

}

#endif

# ToyMaker: Scene System

## What is it?

The ToyMaker::SceneSystem is responsible for maintaining and exposing the currently active scene.  It does so by holding a reference to a scene tree.

The scene tree is made up of scene nodes derived from ToyMaker::SceneNodeCore.  Each node is a wrapper over an ToyMaker::Entity.  This entity is guaranteed to have at least 3 components:

- ToyMaker::Placement -- The position, orientation, and scale of the node, relative to its parent (or not, depending on ToyMaker::RelativeTo)

- ToyMaker::Transform -- The cached matrix representing the position of this node relative to the world.

- ToyMaker::SceneHierarchyData -- A struct with information about this node's immediate neighbours in the hierarchy.

Additional components added to the entity, along with that entity's system configuration, determine its participation in any particular ToyMaker::System defined for this project.

The hierarchical organization of the nodes is left up to the developers of an application.  While scene nodes anywhere may be organized into a tree, only nodes attached to ToyMaker::SceneSystem's scene tree will be considered active and receive inputs and application updates.

The root node of the ToyMaker::SceneSystem's scene tree is a ToyMaker::ViewportNode, configurable via the `"root_viewport_render_configuration"` property of the application's `project.json` file.

The application's `project.json` also contains a `"root_scene_path"` property, pointing to the scene file loaded as the first scene in the application.

## Important API

- ToyMaker::SceneNodeCore -- The base class for all scene nodes, providing methods and hooks for querying and manipulating scene trees.

- ToyMaker::SceneSystem -- The scene system itself, through which nodes receive access to other ECS systems, user inputs, and application updates.  It is also responsible for keeping scene node ToyMaker::Transform components up-to-date.

- ToyMaker::SceneFromDescription and ToyMaker::SceneFromFile -- The resource constructors responsible for loading a scene into an application from its JSON representation.

## Why does it exist?

The scene system is to a game or application what the DOM tree is to a browser, or what a directory tree is to an OS' filesystem.  It allows its users to logically group objects together per the requirements of the application.

Scene nodes connected by a child-parent relationship also have a spatial relationship with each other.  The child's ToyMaker::Placement component specifies, by default, the child's placement relative to its parent's position, scale, and orientation, in the world.

This can be useful in a number of different scenarios: player character (parent) holding one swappable weapon (child), moving train (parent) carrying passengers (children), UI box (parent) containing UI text (child), and so on.

The enablement of the root node of a subtree also determines whether its descendants are active.  A dialog box, for example, can be displayed or hidden simply by enabling or disabling its root node.

## Scene files

In ToyMaker, a scene file is a JSON file adhering to a specific structure.  The scene file is divided into 3 sections: `"resources"`, `"nodes"`, and `"connections"`

### Resource list

This is a list of resources.  It is an array mapped to the scene file's root object's `"resources"` property.

```jsonc
// NOTE: Scene file json object
{
    "resources": [
        // NOTE: Resource definitions go here
    ]//,
    // ... other sections, ...
}
```

Each resource is an object necessarily containing the name of the resource, the type of resource it is, its resource constructor (called method), and the parameters the method uses.

```jsonc
{
    "resources": [
        {
            "name": "Bad_Panel_Texture", // NOTE: Resource name
            "type": "Texture", // NOTE: Resource type
            "method": "fromFile", // NOTE: Resource constructor name
            "parameters": { // NOTE: Resource constructor parameters
                "path": "data/textures/panel.png"
            }
        },


        // NOTE: Definition for a dependent resource
        {
            "name": "Bad_Panel",
            "type": "NineSlicePanel",
            "method": "fromDescription",
            "parameters": {
                "base_texture": "Bad_Panel_Texture", // NOTE: Resource dependency
                "content_region": [ 0.0235, 0.0235, 0.953, 0.953]
            }
        }
    ]//,
    // ... other sections, ...
}
```

Resources defined by an application at an earlier point may be used in scenes that are loaded at a later point.  The newly loaded scene needn't redefine the resource, and may proceed as though the resource definition is already present in the database.

Note, however, it is not permitted to define the same resource twice -- each resource can be defined only once.  It is up to the developer to ensure that when a scene loaded later in an application requires the same resource as a scene loaded earlier, that the resource definition is only present in the scene loaded earlier.

See classes derived from ToyMaker::ResourceConstructor for the parameters required by different resources and their constructors.

### Scene nodes

This is a list of nodes in the present scene.  It is an array mapped to the scene file's root object's `"nodes"` property.  Like `"resources"`, each node is also a JSON object in an array.

There are three types of nodes currently defined: [SceneNode](#scenenode), [SimObject](#simobject), and [ViewportNode](#viewportnode).  The root node of every scene file *must* be a ToyMaker::SimObject; every other node may be any one of the defined node types.

The root node must also have an empty string `""` as the value of its `"parent"` property.  Every other node must have a non-empty `"parent"` property, where `"/"` corresponds to the root node of this scene file, and where the parent path corresponds to a node defined earlier in this section.

#### SceneNode

ToyMaker::SceneNode is the most basic type of scene node, comprised of nothing more than its names and ToyMaker::ECSWorld components.  It has the following appearance in a scene file:

```jsonc
{

    "nodes" [
        // ... other nodes

        {
            // NOTE: the name of the node
            "name": "ui_light",

            // NOTE: the path to its parent node in this scene file, relative to the scene file's 
            // root, defined earlier in the same scene file
            "parent": "/viewport_UI/",

            // NOTE: the type of node this is
            "type": "SceneNode",

            // NOTE: a list of components and their initial values used by this node
            "components": [
                {
                    "orientation": [ 1.0, 0.0, 0.0, 0.0 ],
                    "position": [ 0.0, 0.0, 0.0, 1.0 ],
                    "scale": [ 1.0, 1.0, 1.0 ],
                    "type": "Placement"
                },
                {
                    "ambient": [ 0.9, 0.9, 0.9 ],
                    "diffuse": [ 0.0, 0.0, 0.0 ],
                    "specular": [ 0.0, 0.0, 0.0 ],
                    "lightType": "directional",
                    "type": "LightEmissionData"
                }
            ]
        }//,

        // ... other nodes
    ] //,

}
```

#### SimObject

The ToyMaker::SimObject is a type of scene node designed to provide a object-oriented and component-centric interface for application developers.  Like a regular scene node, it also has a name and ToyMaker::ECSWorld components, but has an additional `"aspects"` property.

A ToyMaker::SimObjectAspect is the base class of aspects used by ToyMaker::SimSystem.  Aspects encapsulate both data and behaviour related to the object they are attached to.  They play a similar role in ToyMaker that MonoBehaviours play in unity, or scripts in Godot, by providing a simple interface into scene node lifecycle events, application loop events, and the engine's signal and input systems.

As noted earlier, the root node of any scene file must be a SimObject.  The appearance of a SimObject in a scene file is given below:

```jsonc
{

    // ... other sections ...

    "nodes": [

        // ... other nodes ...

        {
            "name": "camera",
            "parent": "/viewport_UI/",
            "type": "SimObject",

            // NOTE: A list of aspects defined for this type
            "aspects":[
                {
                    "type": "QueryClick" // NOTE: At minimum, aspect def has the aspect type string
                }
            ],

            "components": [
                {
                    "fov": 45.0,
                    "aspect": 0.0,
                    "orthographic_dimensions": {
                        "horizontal": 1366,
                        "vertical": 768
                    },
                    "near_far_planes": {
                        "near": -1000,
                        "far": 1000
                    },
                    "projection_mode": "orthographic",
                    "type": "CameraProperties"
                },
                {
                    "orientation": [ 1.0, 0.0, 0.0, 0.0 ],
                    "position": [ 0.0, 0.0, 0.0, 1.0 ],
                    "scale": [ 1.0, 1.0, 1.0 ],
                    "type": "Placement"
                }
            ]
        }//,

        // ... other nodes ...

    ]//,

    // ... other sections ...

}
```

See classes derived from ToyMaker::SimObjectAspect for details on parameters required by specific aspects.

SimObjects have one more special feature in a scene file.  

Since the root node of a scene file is always guaranteed to be a SimObject, when one scene is imported into another scene, the import*ing* scene may specify overrides for the name, components, and aspects, of the import*ed* scene's root node, like so:

```jsonc
{
    "resources": [

        // ... other resources ...

        // NOTE: The imported scene
        {
            "name": "Ur_Button",
            "type": "SimObject",
            "method": "fromSceneFile",
            "parameters": {
                "path": "data/ur_button.json"
            }
        }//,

        // ... other resources ...

    ],

    "nodes": [

        // ... other nodes

        {
            "name": "Ur_Button",
            "parent": "/viewport_UI/",

            // NOTE: must be "Scene", even though technically it is a SimObject
            "type": "Scene",

            // NOTE: in the case of an asset, which may be used multiple times 
            // simultaneously, set to true.  For proper "scenes", this 
            // may be set to false.
            "copy": true,

            // NOTE: overrides to the imported scene's root node's properties
            "overrides":  {
                "name": "local_vs",
                "components": [
                    {
                        "type": "Placement",
                        "orientation": [ 1.0,  0.0, 0.0, 0.0 ],
                        "position": [ 0.0, 200.0, 0.0, 1.0 ],
                        "scale": [ 1.0, 1.0, 1.0 ]
                    }
                ],
                "aspects": [
                    {
                        "type": "UIButton",
                        "text": "Local VS",
                        "font_resource_name": "Roboto_Mono_Regular_24",
                        "color": [255, 255, 255, 255],
                        "scale": 1.0,
                        "anchor": [0.5, 0.5],
                        "value": "Game_Of_Ur_Local_VS",
                        "panel_active": "Bad_Button_Active_Panel",
                        "panel_inactive": "Bad_Button_Inactive_Panel",
                        "panel_hover": "Bad_Button_Hover_Panel",
                        "panel_pressed": "Bad_Button_Pressed_Panel",
                        "has_highlight": false
                    }
                ]
            }
        }//,

        // ... other nodes

    ]//,

    // ... other sections ...

}
```

#### ViewportNode

A ToyMaker::ViewportNode is one of the main interfaces between the developer and the ToyMaker::RenderSystem.  It is used to control the frequency of rendering updates to the target texture owned by this node, as well as the dimensions of that texture.

According to its configuration, it may also mark the boundary between the part of the scene tree corresponding to its parent viewport's ToyMaker::ECSWorld, and its own ECSWorld.

An example of where these properties are useful is if a game wants its UI to be rendered at the platform's native resolution, while rendering its 3D game world at a lower resolution.  The parent viewport and the UI viewport can have their render textures set to the size of the application window in native pixel dimensions.  The game world viewport has its resolution scaled down.  The parent viewport produces the final window texture by combining the results from the UI viewport and the game world viewport.

This also ensures that nodes in the UI world aren't affected by the systems active in the game world, and vice versa.

The appearance of the viewport node in a scene file is given below:

```jsonc
{

    // ... other sections ...

    "nodes": [

        // ... other nodes ...

        {
            "name": "viewport_3D",
            "parent": "/",
            "type": "ViewportNode",
            "components": [
                {
                    "type": "Placement",
                    "orientation": [
                        1.0, 0.0, 0.0, 0.0
                    ],
                    "position": [
                        0.0, 0.0, 0.0, 1.0
                    ],
                    "scale": [
                        1.0, 1.0, 1.0
                    ]
                }
            ],

            // NOTE: false when this viewport owns its own world.
            "inherits_world": false,

            // NOTE: whether the viewport sends an input to its remaining descendants after either it or 
            // an earlier descendant reports handling an action successfully.
            "prevent_handled_action_propagation": false,

            // NOTE: The skybox texture used by the skybox rendering stage of the rendering pipeline.
            "skybox_texture": "Skybox_Texture",

            // NOTE: The dimensions of the target texture, and the frequency with which it is updated.
            "render_configuration": {
                "base_dimensions": [1366, 768],
                "update_mode": "on-render-cap-fps",
                "resize_type": "texture-dimensions",
                "resize_mode": "fixed-dimensions",

                // NOTE: The type of rendering pipeline used for this viewport
                "render_type": "basic-3d",

                "fps_cap": 60,
                "render_scale": 1.0
            }
        },

        // ... other nodes ...

    ]//,

    // ... other sections ...
}
```

The root viewport of the ToyMaker::SceneSystem is also a viewport node, retrievable using ToyMaker::SceneSystem::getRootViewport().

### Signal connections

ToyMaker::SimObjectAspect, which is the engine's scripting interface, supports signals.  Any aspect subclass may declare its own signal or signal observer like so:

```c++
#include "toymaker/sim_system.hpp"

class MyAspect: public ToyMaker::SimObjectAspect<MyAspect> {
private:
    // ... other class members
    
    // NOTE: Handler method for some signal this aspect expects to
    // observe
    void someSignalHandler (int signalValue1, float signalValue2);
public:

    // ... other class interface members

    // NOTE: A signal member of this aspect
    Toymaker::Signal<> mSigMySignal {
        *this, // NOTE: Reference to SignalTracker-derived self
        "MySignal" // NOTE: The aspect's signal's name
    }

    // ... other class interface members

    // NOTE: A signal observer member of this aspect
    ToyMaker::SignalObserver<int, float> // NOTE: data types received
      mObserveSomeSignal {

        *this, // NOTE: ref to tracker
        "SomeSignalObserved"// NOTE: name of observer

        // NOTE: Lambda connecting this observer to the handler
        [this](int sig1, float sig2) {this->someSignalHandler(sig1, sig2);}
    };

    // ... other class interface members

};
```

Then, in the `"connections"` section of the scene file, use this to connect a signal from one node aspect to another:

```jsonc
{
    // ... other sections ...

    "connections": [

        // ... other connections ...

        {
            "to": "/path/to/my_node/@MyAspect",
            "observer": "SomeSignalObserved", // NOTE: name of our observer

            "from": "/path/to/other_node/@SomeSignalGenAspect",
            "signal": "SomeSignal"
        },

        {
            "from": "/path/to/my_node/@MyAspect",
            "signal": "MySignal", // our signal's name

            "from": "/path/to/yet/another_node/@SomeObserverAspect",
            "observer": "MySignalObserved"
        }

        // ... other connections ...
    ]
}
```

... where all paths are relative to the current scene file's root node.  Signals and observers from other scene files, provided they've been added to this scene's tree, may also be referenced.

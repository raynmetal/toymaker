/**
 * @ingroup ToyMakerSceneSystem
 * @file scene_system.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief System classes relating to the SceneSystem, which in some ways lies at the heart of the engine.
 * @version 0.3.2
 * @date 2025-09-05
 * 
 */

/**
 * @defgroup ToyMakerSceneSystem Scene System
 * @ingroup ToyMakerEngine
 * 
 */

#ifndef FOOLSENGINE_SCENESYSTEM_H
#define FOOLSENGINE_SCENESYSTEM_H

#include <vector>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <type_traits>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "core/ecs_world.hpp"
#include "core/resource_database.hpp"
#include "scene_components.hpp"
#include "spatial_query_math.hpp"
#include "render_system.hpp"
#include "texture.hpp"
#include "input_system/input_system.hpp"

namespace ToyMaker {

    class SceneNodeCore;
    class SceneNode;
    class ViewportNode;
    class SceneSystem;

    /**
     * @ingroup ToyMakerSceneSystem
     * @brief (Presently unused) A marker to indicate how transforms should be computed for a given scene node.
     * 
     * Currently all scene nodes are computed relative to their parent (even if they shouldn't be).
     * 
     * @todo Make this feature actually work.
     * 
     */
    enum class RelativeTo : uint8_t {
        PARENT=0, //< Compute relative to/on top of this node's parent's transform.
        // WORLD=1,
        // CAMERA=2,
    };

    /** @ingroup ToyMakerSerialization ToyMakerSceneSystem */
    NLOHMANN_JSON_SERIALIZE_ENUM(RelativeTo, {
        {RelativeTo::PARENT, "parent"},
    });

    /**
     * @ingroup ToyMakerSceneSystem
     * @brief (Perhaps unused) Special "reserved" entity IDs which the scene system might use.
     * 
     * @todo Figure out if there's actual any use for this or if my brain farted when I was writing the scene system.
     */
    enum SpecialEntity: EntityID {
        ENTITY_NULL = kMaxEntities,
    };

    /**
     * @ingroup ToyMakerSceneSystem
     * @brief Special name for the scene root, unusable by any other scene object.
     * 
     */
    extern const std::string kSceneRootName;

    /**
     * @ingroup ToyMakerSceneSystem
     * @brief The core of a node in the SceneSystem, a set of components and methods overridable or usable by all types of scene nodes.
     * 
     */
    class SceneNodeCore: public std::enable_shared_from_this<SceneNodeCore> {
    public:
        /**
         * @brief Deleter for a managed pointer to a scene node which ensures its onDestroyed virtual function gets called.
         * 
         * @param sceneNode The pointer to the scene node being deleted.
         * 
         * @todo See whether onDestroyed actually gets called, and whether this function is even necessary given that virtual destructors already exist.
         */
        static void SceneNodeCore_del_(SceneNodeCore* sceneNode);

        /**
         * @brief Destroys SceneNodeCore.
         * 
         * Mainly, marks this class and its derivatives' destructors as virtual destructors.
         * 
         */
        virtual ~SceneNodeCore()=default;

        /**
         * @brief Adds a component of type TComponent to the node.
         * 
         * @tparam TComponent The type of component being added.
         * @param component The value of the component added.
         * @param bypassSceneActivityCheck Prevents scene activity check when it is known that this node is not visible to the scene system.
         */
        template <typename TComponent>
        void addComponent(const TComponent& component, const bool bypassSceneActivityCheck=false);

        /**
         * @brief Adds a component to the node.
         * 
         * @param jsonComponent A description of the node, in JSON, along with its type and initial value.
         * @param bypassSceneActivityCheck Prevents scene activity check when it is known that this node is not visible to the scene system.
         */
        void addComponent(const nlohmann::json& jsonComponent, const bool bypassSceneActivityCheck=false);

        /**
         * @brief Retrieves a component belonging to this node.
         * 
         * @tparam TComponent The type of component being retrieved.
         * @param simulationProgress The progress towards the next simulation state after the previous simulation state as a number between 0 and 1.
         * @return TComponent The retrieved component value.
         * 
         */
        template <typename TComponent>
        TComponent getComponent(const float simulationProgress=1.f) const;

        /**
         * @brief Tests whether this node has a component of a specific type.
         * 
         * @tparam TComponent The type of component whose existence is being tested.
         * @retval true This type of component is present for this node;
         * @retval false This type of component is absent for this node;
         */
        template <typename TComponent>
        bool hasComponent() const;

        /**
         * @brief Tests whether this node has a component of a specific type.
         * 
         * @param type The component type string of the component whose existence is being tested.
         * @retval true This type of component is present for this node;
         * @retval false This type of component is absent for this node;
         */
        bool hasComponent(const std::string& type) const;

        /**
         * @brief Updates the value of a component of this node (to what it should be at the start of the next simulation step).
         * 
         * @tparam TComponent The type of component being updated.
         * @param component The component's new value.
         */
        template <typename TComponent>
        void updateComponent(const TComponent& component);

        /**
         * @brief Updates the value of a component of this node (to what it should be at the start of the next simulation step).
         * 
         * @param component A JSON description of the component's new value along with its type.
         */
        void updateComponent(const nlohmann::json& component);

        /**
         * @brief A method for adding a component or updating a component if that component is already present on this node.
         * 
         * @tparam TComponent The type of component being added or updated.
         * @param component The components new value, as of the start of the next simulation step.
         * @param bypassSceneActivityCheck Prevents scene activity check when it is known for certain that the node is not part of the scene system.
         */
        template <typename TComponent>
        void addOrUpdateComponent(const TComponent& component, const bool bypassSceneActivityCheck=false);

        /**
         * @brief A method for adding a component, or updating a component if the same type of component is already present on this node.
         * 
         * @param component The description of the component, including its new value and its type.
         * @param bypassSceneActivityCheck Prevents scene activity check when it is known for certain that the node is not part of the scene system.
         */
        void addOrUpdateComponent(const nlohmann::json& component, const bool bypassSceneActivityCheck=false);

        /**
         * @brief Removes a component present on this node.
         * 
         * @tparam TComponent The type of component being removed.
         */
        template <typename TComponent>
        void removeComponent();

        /**
         * @brief Sets whether or not a given system should be able to influence this scene object.
         * 
         * If the node hasn't already been made part of the scene, the activation will occur after it has been added to it.  Otherwise, the activation occurs right away.
         * 
         * @tparam TSystem The system being enabled or disabled for this node.
         * @param state Whether to enable or disable a system for this node.
         */
        template <typename TSystem>
        void setEnabled(bool state);

        /**
         * @brief Returns whether a particular system has been enabled for this node.
         * 
         * @tparam TSystem The system whose influence is being tested.
         * @retval true The system has been enabled, and will influence this node when it is a part of the scene;
         * @retval false The system has been disabled, and won't influence this node even when it is a part of the scene;
         */
        template <typename TSystem>
        bool getEnabled() const;

        /**
         * @brief Returns the entity id associated with these scene node.
         * 
         * @return EntityID The ID of the entity associated with this node.
         */
        EntityID getEntityID() const;

        /**
         * @brief Returns the ID of the ECSWorld this node belongs to.
         * 
         * @return WorldID The ID of the ECSWorld this node belongs to.
         */
        WorldID getWorldID() const;

        /**
         * @brief Gets the UniversalEntityID aka the world-entity-id pair associated with this node.
         * 
         * @return UniversalEntityID The world-entity-id pair associated with this node.
         */
        UniversalEntityID getUniversalEntityID() const;

        /**
         * @brief Gets a reference to the ECSWorld this node belongs to.
         * 
         * @return std::weak_ptr<ECSWorld> The ECSWorld this node belongs to.
         */
        std::weak_ptr<ECSWorld> getWorld() const;

        /**
         * @brief Returns whether this node is present as part of the SceneSystem's scene tree.
         * 
         * @retval true This node is part of the SceneSystem's scene tree.
         * @retval false This node is not part of the SceneSystem's scene tree.
         * 
         * @todo We can't be certain the value returned by this method and the one in the SceneSystem are in sync.  Remove this redundancy somehow.
         */
        bool inScene() const;

        /**
         * @brief Returns whether this node is present as part of the SceneSystem's scene tree, AND is active there as well.
         * 
         * @retval true This node is present and active in the SceneSystem's scene tree.
         * 
         * @retval false This node is not present or active in the SceneSystem's scene tree.
         * 
         * @todo We can't be certain the value returned by this method and the one in the SceneSystem are in sync.  Remove this redundancy somehow.
         * 
         */
        bool isActive() const;

        /**
         * @brief Tests whether a particular scene node is the ancestor of this one.
         * 
         * @param sceneNode The scene node being tested as an ancestor.
         * @retval true The argument node is this node's ancestor;
         * @retval false The argument node is not this node's ancestor;
         */
        bool isAncestorOf(std::shared_ptr<const SceneNodeCore> sceneNode) const;

        /**
         * @brief Tests whether a node specified by some path relative to this node is a real descendant of this node.
         * 
         * @param pathToChild The path to the descendant node whose existence is being tested.
         * 
         * @retval true There is a node present at the path specified;
         * @retval false There is no node present at the path specified;
         */
        bool hasNode(const std::string& pathToChild) const;
        
        /**
         * @brief Adds a node (or a tree of them) as a child of the node specified by the path in the argument.
         * 
         * @param node The node being added as a descendant of this node.
         * @param where The path to the node whose immediate child the new node will be.
         */
        void addNode(std::shared_ptr<SceneNodeCore> node, const std::string& where);

        /**
         * @brief Returns a list of all of this node's immediate children scene nodes.
         * 
         * @return std::vector<std::shared_ptr<SceneNodeCore>> A list of this node's immediate children scene nodes.
         */
        std::vector<std::shared_ptr<SceneNodeCore>> getChildren();

        /**
         * @brief Returns a list of all of this node's immediate children scene nodes.
         * 
         * @return std::vector<std::shared_ptr<const SceneNodeCore>> A list of (immutable) references to this node's immediate children scene nodes.
         */
        std::vector<std::shared_ptr<const SceneNodeCore>> getChildren() const;

        /**
         * @brief Gets all of the descendant nodes belonging to this scene node.
         * 
         * @return std::vector<std::shared_ptr<SceneNodeCore>> A list of descendant nodes belonging to this scene node.
         */
        std::vector<std::shared_ptr<SceneNodeCore>> getDescendants();

        /**
         * @brief Gets a reference to a node or related object by its path.
         * 
         * @tparam TObject The type of object being retrieved, by default a shared pointer to a SceneNode.
         * @param where The path to the object being retrieved.
         * @return TObject The object retrieved.
         */
        template <typename TObject=std::shared_ptr<SceneNode>>
        TObject getByPath(const std::string& where);

        /**
         * @brief Gets a pointer to a node by its EntityID, assuming that node and this one belong to the same ECSWorld.
         * 
         * @tparam TSceneNode The type of node being retrieved, by default a SceneNode.
         * @param entityID The id belonging to the entity associated with the node being retrieved.
         * @return std::shared_ptr<TSceneNode> A shared pointer to the node retrieved.
         */
        template <typename TSceneNode=SceneNode>
        std::shared_ptr<TSceneNode> getNodeByID(EntityID entityID);

        /**
         * @brief Gets the path from a node (assumed to be an ancestor) to this node.
         * 
         * @param ancestor A reference to a node which is an ancestor of this node.
         * @return std::string The path from the ancestor to this node.
         */
        std::string getPathFromAncestor(std::shared_ptr<const SceneNodeCore> ancestor) const;

        /**
         * @brief Returns the viewport node which is in the same ECSWorld as and is the closest ancestor of (or the same as) this node.
         * 
         * @return std::shared_ptr<ViewportNode> The local viewport node.
         */
        virtual std::shared_ptr<ViewportNode> getLocalViewport();

        /**
         * @brief Returns (a constant reference to) the viewport node which is in the same ECSWorld as and is the closes ancestor of (or the same as)  this node.
         * 
         * @return std::shared_ptr<const ViewportNode> The local viewport node.
         */
        virtual std::shared_ptr<const ViewportNode> getLocalViewport() const;

        /**
         * @brief Gets a reference to a scene node (of any valid type) based on its path relative to this node.
         * 
         * @param where The path to the node being retrieved.
         * @return std::shared_ptr<SceneNodeCore> The retrieved node.
         */
        std::shared_ptr<SceneNodeCore> getNode(const std::string& where);

        /**
         * @brief Gets the parent node of this node, if one is present.
         * 
         * @return std::shared_ptr<SceneNodeCore> This node's parent node.
         */
        std::shared_ptr<SceneNodeCore> getParentNode();

        /**
         * @brief Gets (a constant reference to) the parent node of this node, if one is present.
         * 
         * @return std::shared_ptr<const SceneNodeCore> This node's parent node.
         */
        std::shared_ptr<const SceneNodeCore> getParentNode() const;

        /**
         * @brief Removes a node from the tree present at the path specified.
         * 
         * @param where The path to the node being removed.
         * @return std::shared_ptr<SceneNodeCore> A reference to the removed node.
         */
        std::shared_ptr<SceneNodeCore> removeNode(const std::string& where);

        /**
         * @brief Disconnects and removes all the child nodes attached to this node.
         * 
         * @return std::vector<std::shared_ptr<SceneNodeCore>> A list of child nodes removed.
         */
        std::vector<std::shared_ptr<SceneNodeCore>> removeChildren();

        /**
         * @brief Returns the name string for this node.
         * 
         * @return std::string The name of this node.
         */
        std::string getName() const;

        /**
         * @brief Sets the name of this node.
         * 
         * @param name The new name for this node.
         */
        void setName(const std::string& name);

        /**
         * @brief Gets the path of this node relative to its local viewport node.
         * 
         * @return std::string The path to this node from its local viewport node.
         * 
         * @see getLocalViewport()
         */
        std::string getViewportLocalPath() const;

        /**
         * @brief A reference to the node which was used in order to construct this one.
         * 
         * May be useful in the future when there is more formal support for the notion of assets.
         * 
         * @param prototype The node that acted as the prototype for this node and its children.
         * 
         * @todo Think of a safer way to accomplish this.
         */
        inline void setPrototype_(std::shared_ptr<SceneNodeCore> prototype) { mPrototype=prototype; }

    protected:

        /**
         * @brief Removes this node's entity from its current ECSWorld and adds it to a new ECSWorld.
         * 
         * @param world The new ECSWorld this node's entity should join.
         */
        virtual void joinWorld(ECSWorld& world);

        /**
         * @brief Creates a new scene tree by copying another scene node and its descendants.
         * 
         * @param other The node used as the basis for the construction of a new tree.
         * 
         * @return std::shared_ptr<SceneNodeCore> The root node of the newly constructed scene tree.
         */
        static std::shared_ptr<SceneNodeCore> copy(const std::shared_ptr<const SceneNodeCore> other);

        /**
         * @brief Constructs a scene node object from essential and extra components.
         * 
         * @tparam TComponents A list of component types being added to a node in addition to its placement and transform components.
         * @param placement The placement component for the newly constructed node.
         * @param name The name to be given to this node.
         * @param components A list of component values to initialize this node's components with.
         */
        template<typename ...TComponents>
        SceneNodeCore(const Placement& placement, const std::string& name, TComponents...components);

        /**
         * @brief Constructs a node based on its description of JSON, later verifying that essential components are present.
         * 
         * @param jsonSceneNode A description for this node and the initial value of its components.
         * 
         */
        SceneNodeCore(const nlohmann::json& jsonSceneNode);

        /**
         * @brief Constructs a new node as a copy of another node.
         * 
         * @param sceneObject The node being copied.
         */
        SceneNodeCore(const SceneNodeCore& sceneObject);

        // /**
        //  * @brief Copy assignment operator.
        //  * 
        //  * @todo Sit down and figure out whether this operator will ever actually need to be used.
        //  */
        // BaseSceneNode& operator=(const BaseSceneNode& sceneObject);

        /**
         * @brief Scene node lifecycle hook for when a node is created.
         * 
         */
        virtual void onCreated();

        /**
         * @brief Scene node lifecycle hook for when a node is made an active part of the SceneSystem.
         * 
         */
        virtual void onActivated();

        /**
         * @brief Scene node lifecycle hook for when a node is deactivated on the SceneSystem.
         * 
         */
        virtual void onDeactivated();

        /**
         * @brief Scene node lifecycle hook for when a node (and possibly its descendants) are about to be destroyed.
         * 
         * @todo Figure out whether this is actually necessary, and why it was we couldn't just use the virtual destructor.
         * 
         */
        virtual void onDestroyed();

        /**
         * @brief Tests whether a given name is actually valid, throwing an error when it is not.
         * 
         * A node should contain letters, numbers, underscores, and should not match kSceneRootName.
         * 
         * @param nodeName The node name whose validity is being asserted.
         */
        static void validateName(const std::string& nodeName);

    private:
        /**
         * @brief A private struct to limit certain sensitive functions to this class and other closely coupled classes.
         * 
         */
        struct Key {};

        /**
         * @brief (Private) Constructs a new node based with essential components and some explicitly specified extra ones.
         * 
         * @tparam TComponents Additional component types being added to this node.
         * @param placement The placement value for this node.
         * @param name The name for this node.
         * @param components The initial values for extra components being added to this node.
         */
        template<typename ...TComponents>
        SceneNodeCore(const Key&, const Placement& placement, const std::string& name, TComponents...components);

        /**
         * @brief Flags that indicate whether this node is enabled and active for the SceneSystem.
         * 
         */
        enum StateFlags: uint8_t {
            ENABLED=0x1, //< This node is intended to be made active as soon as its added to the SceneSystem's scene tree.
            ACTIVE=0x2, //< This node is presently active on the SceneSystem's scene tree.
        };

        /**
         * @brief A helper intended to get scene nodes and related objects attached to the scene tree.
         * 
         * @tparam TObject The type of object this helper retrieves.
         * @tparam Enable Parameter to allow or prevent different specializations of this helper class.
         */
        template <typename TObject, typename Enable=void>
        struct getByPath_Helper {
            static TObject get(std::shared_ptr<SceneNodeCore> rootNode, const std::string& where);
            static constexpr bool s_valid { false };
        };

        /**
         * @brief Virtual method which each type of scene node with special members should implement (in lieu of copy constructors and copy assignment operators.)
         * 
         * @return std::shared_ptr<SceneNodeCore> A new node constructed based on this node.
         */
        virtual std::shared_ptr<SceneNodeCore> clone() const;

        /**
         * @brief Copies descendant nodes belonging to another node, attaches the copies to this node.
         * 
         * @param other The node whose descendants are being copied.
         * 
         */
        void copyDescendants(const SceneNodeCore& other);

        /**
         * @brief Sets a node as the parent viewport of another one.
         * 
         * For regular nodes, the side effect would be to move this node's entity and component's into the ECSWorld of its new parent viewport.
         * 
         * @param node The node whose parent viewport is being set.
         * @param newViewport The node's to-be parent viewport.
         */
        static void setParentViewport(std::shared_ptr<SceneNodeCore> node, std::shared_ptr<ViewportNode> newViewport);

        /**
         * @brief Disconnects this node from its parent node if it has one.
         * 
         * @param node The node whose relationship with its parent is being dissolved.
         * @return std::shared_ptr<SceneNodeCore> A reference to the disconnected node, the same one passed as an argument.
         */
        static std::shared_ptr<SceneNodeCore> disconnectNode(std::shared_ptr<SceneNodeCore> node);

        /**
         * @brief Tests whether there are any cycles in the path up to the node's oldest ancestor.
         * 
         * @param node The node whose path to its root is being tested for cycles.
         * @retval true There is a cycle present in the path to this node's root node.
         * @retval false There is no cycle present in the path to this node's root node.
         */
        static bool detectCycle(std::shared_ptr<SceneNodeCore> node);

        /**
         * @brief Strips the root-most part of the path to a node.
         * 
         * @param where The path before removing its prefix.
         * 
         * @return std::tuple<std::string, std::string> - .first -  The name of the node (presumably a child of the caller object) relative to which the stripped path is valid;  - .second -  The path with its prefix removed;
         * 
         * 
         */
        static std::tuple<std::string, std::string> nextInPath(const std::string& where);

        /**
         * @brief  Copies component values from another node and replaces the values on node's components with them.
         * 
         * @param other The node whose component values are being copied.
         */
        void copyAndReplaceAttributes(const SceneNodeCore& other);

        /**
         * @brief Utility function for updating SceneHierarchyData components belonging to a single ECSWorld in the SceneSystem.
         * 
         */
        void recomputeChildNameIndexMapping();

        /**
         * @brief The name of this scene node.
         * 
         */
        std::string mName {};

        /**
         * @brief Flags indicating the state of this scene node in the scene system.
         * 
         * @see StateFlags
         * 
         */
        uint8_t mStateFlags { 0x00 | StateFlags::ENABLED };

        /**
         * @brief A marker indicating how this node's transform component should be computed.
         * 
         */
        RelativeTo mRelativeTo{ RelativeTo::PARENT };

        /**
         * @brief The ECSWorld entity which this node is a wrapper over.
         * 
         */
        std::shared_ptr<Entity> mEntity { nullptr };

        /**
         * @brief A reference to this node's parent scene node.
         * 
         */
        std::weak_ptr<SceneNodeCore> mParent {};

        /**
         * @brief A reference to this node's parent viewport (whose meaning changes depending on whether this node is a camera, viewport, or other type of node)
         * 
         */
        std::weak_ptr<ViewportNode> mParentViewport {};

        /**
         * @brief A mapping of names of this node's child nodes to the indices of the nodes themselves.
         * 
         * @see mChildren
         * 
         */
        std::unordered_map<std::string, std::size_t> mChildNameToNode {};

        /**
         * @brief A list of this node's child nodes.
         * 
         */
        std::vector<std::shared_ptr<SceneNodeCore>> mChildren {};

        /**
         * @brief Allows a prototype scene node to be retained as a resource so long as this node is present in memory somewhere.
         * 
         */
        std::shared_ptr<SceneNodeCore> mPrototype { nullptr };

        /**
         * @brief A bitset, each position of which indicates whether a system should influence this node when it is part of the SceneSystem's scene tree.
         * 
         * @see SystemManager
         * 
         */
        Signature mSystemMask {Signature{}.set()};

    friend class SceneSystem;
    template<typename TSceneNode>
    friend class BaseSceneNode;
    friend class ViewportNode;
    };


    /**
     * @ingroup ToyMakerSceneSystem
     * @brief A CRTP template for all the scene node types present in the project.
     * 
     * @tparam TSceneNode A specialization and subclass of this class.
     */
    template <typename TSceneNode>
    class BaseSceneNode: public SceneNodeCore {
    public:

    protected:
        /**
         * @brief A (private) method for the creation of a new scene node for a particular type.
         * 
         * @tparam TComponents Component types of extra componennts to be added to this node.
         * @param placement The value of the placement component of this node.
         * @param name The name of this node.
         * @param components The initial values of extra components initialized for this node.
         * @return std::shared_ptr<TSceneNode> A shared pointer to the newly constructed node.
         * 
         * @remark An override for this static method is expected for all this class' subclasses.
         */
        template <typename ...TComponents>
        static std::shared_ptr<TSceneNode> create(const Key&, const Placement& placement, const std::string& name, TComponents...components);

        /**
         * @brief A method for creating a scene node of a specific type (TSceneNode)
         * 
         * @tparam TComponents Component types of extra components to be added to this node.
         * @param placement The value of the placement component of this node.
         * @param name The name of this node.
         * @param components The initial values of extra components initialized for this node.
         * @return std::shared_ptr<TSceneNode> A shared pointer to the newly constructed node.
         * 
         * @remark An override for this static method is expected for all this class' subclasses.
         */
        template <typename ...TComponents>
        static std::shared_ptr<TSceneNode> create(const Placement& placement, const std::string& name, TComponents...components);

        /**
         * @brief Creates a scene node of a type based on its description in JSON.
         * 
         * @param sceneNodeDescription A JSON description of the scene node being constructed.
         * @return std::shared_ptr<TSceneNode> The newly constructed scene node.
         * 
         * @remark An override for this static method is expected for all this class' subclasses.
         */
        static std::shared_ptr<TSceneNode> create(const nlohmann::json& sceneNodeDescription);


        /**
         * @brief Creates a scene node of a specific type based on another node of that type.
         * 
         * @param sceneNode The node being copied from.
         * 
         * @return std::shared_ptr<TSceneNode> The newly constructed node.
         * 
         * @remark An override for this static method is expected for all this class' subclasses.
         */
        static std::shared_ptr<TSceneNode> copy(const std::shared_ptr<const TSceneNode> sceneNode);
        template <typename...TComponents>

        /**
         * @brief Constructor for a single node of a subclass.
         * 
         * @param key Key only usable by this class' implementation and other tightly coupled classes and functions (allows certain scene system checks to be skipped).
         * @param placement The initial value of placement for this node.
         * @param name The name for this node.
         * @param components Initial values for various components of this node.
         */
        BaseSceneNode(const Key& key, const Placement& placement, const std::string& name, TComponents...components):
        SceneNodeCore{ key, placement, name, components... }
        {}

        /**
         * @brief General constructor for a single node of a subclass.
         * 
         * @tparam TComponents A list of component types to be initialized for this node.
         * @param placement The initial value for the placement component of this node.
         * @param name The name for the newly constructed node.
         * @param components Initial values for various components of this node.
         */
        template<typename ...TComponents>
        BaseSceneNode(const Placement& placement, const std::string& name, TComponents...components):
        SceneNodeCore{ placement, name, components... }
        {}

        /**
         * @brief Constructs a new node of a certain type based on its json description.
         * 
         * @param nodeDescription The description of the node and the types and initial values of its components.
         */
        BaseSceneNode(const nlohmann::json& nodeDescription) : SceneNodeCore { nodeDescription } {}

        /**
         * @brief Constructs a new node of a certain type as a copy of another node.
         * 
         * @param other The node being used as a template to construct this node.
         */
        BaseSceneNode(const SceneNodeCore& other): SceneNodeCore{ other } {}

    friend class SceneNodeCore;
    };

    /**
     * @ingroup ToyMakerSceneSystem
     * @brief The most basic vanilla flavour of scene node comprised of no more than a name and some components.
     * 
     * @see SceneNodeFromDescription
     * 
     */
    class SceneNode: public BaseSceneNode<SceneNode>, public Resource<SceneNode> { 
    public:
        template <typename ...TComponents>
        static std::shared_ptr<SceneNode> create(const Placement& placement, const std::string& name, TComponents...components);
        static std::shared_ptr<SceneNode> create(const nlohmann::json& sceneNodeDescription);
        static std::shared_ptr<SceneNode> copy(const std::shared_ptr<const SceneNode> other);

        static inline std::string getResourceTypeName() { return "SceneNode"; }

    protected:
        template<typename ...TComponents>
        SceneNode(const Placement& placement, const std::string& name, TComponents...components):
        BaseSceneNode<SceneNode>{placement, name, components...},
        Resource<SceneNode>{0}
        {}

        SceneNode(const nlohmann::json& jsonSceneNode):
        BaseSceneNode<SceneNode>{jsonSceneNode},
        Resource<SceneNode>{0}
        {}

        SceneNode(const SceneNode& sceneObject):
        BaseSceneNode<SceneNode>{sceneObject},
        Resource<SceneNode>{0}
        {}
    friend class BaseSceneNode<SceneNode>;
    };

    /**
     * @ingroup ToyMakerSceneSystem ToyMakerRenderSystem
     * @brief A type of node capable of and responsible for interacting sensibly with the engine's RenderSystem and ECSWorlds.
     * 
     * It is the only type of node (at present) to be able to create an ECSWorld of its own.  Any ECSWorld thus made will have its Entitys, Systems, and ComponentArrays isolated from those of any other ECSWorld.
     * 
     * It also provides an interface for modifying the behaviour and properties of the RenderSystem and target texture associated with this viewport.  Should serve as the primary interface between a game/application developer and the render system.
     * 
     * @remarks The root node of the SceneSystem is a ViewportNode.
     * 
     * @see ViewportNodeFromDescription
     * 
     */
    class ViewportNode: public BaseSceneNode<ViewportNode>, public Resource<ViewportNode> {
    public:
        /**
         * @brief A collection of data that specifies the behaviour and properties of the RenderSystem and target texture associated with this viewport.
         * 
         */
        struct RenderConfiguration {
            /**
             * @brief Different resize configurations available for this Viewport node that dictate how render textures (from the render pipeline proper) are mapped to this node's target texture.
             * 
             */
            enum class ResizeType: uint8_t {
                OFF=0, //< No resize, render texture is rendered as is with no scaling.
                VIEWPORT_DIMENSIONS, //< Viewport transform configured per stretch mode and requested dimensions
                TEXTURE_DIMENSIONS, //< Texture result rendered in base dimensions, and then warped to fit request dimensions
            };

            /**
             * @brief Determines which dimensions the end result of the viewport is allowed to expand on. 
             * 
             */
            enum class ResizeMode: uint8_t {
                FIXED_ASPECT=0, //< both, while retaining aspect ratio.
                EXPAND_VERTICALLY, //< Expand vertically if permitted by target dimensions, otherwise constrain to aspect.
                EXPAND_HORIZONTALLY, //< Expand horiontally if possible by target dimensions, otherwise constrain to aspect.
                EXPAND_FILL, //< No constraint in either dimension, expand to fill target dimensions always.
            };

            /**
             * @brief Configuration value determining when and how often render updates take place for this viewport.
             * 
             */
            enum class UpdateMode: uint8_t {
                NEVER=0, //< No rerender takes place even when render is called for this viewport.
                ONCE, //< Update on next render frame, then set to UpdateMode::NEVER
                ON_FETCH, //< Update whenever a request for the texture is made, where frequency is entirely dependent on caller.
                ON_FETCH_CAP_FPS, //< Update on request, but ignore requests exceeding FPS cap.  Final frequency is constrained to the FPS cap configured for this viewport, but may be lower than it.
                ON_RENDER, //< Update every render call with no constraint, so frequency depends entirely on the render frequency of the application loop.
                ON_RENDER_CAP_FPS, //< Update on render call when fps cap isn't exceeded.  Final FPS may be lower than specified as a cap.
            };

            /**
             * @brief Specifies the type of render pipeline requested by this viewport.
             * 
             */
            using RenderType = RenderSet::RenderType;

            /**
             * @brief The type of resizing/scaling behaviour from render->target texture for this viewport.
             * 
             */
            ResizeType mResizeType { ResizeType::VIEWPORT_DIMENSIONS };
            /**
             * @brief The resizing/scaling behaviour from render-> target texture for this viewport.
             * 
             */
            ResizeMode mResizeMode { ResizeMode::EXPAND_HORIZONTALLY };

            /**
             * @brief The type of render pipelien requested by this viewport.
             * 
             */
            RenderType mRenderType { RenderType::BASIC_3D };

            /**
             * @brief The design dimensions for this viewport, specified at the time of its development.
             * 
             */
            glm::u16vec2 mBaseDimensions { 800, 600 };

            /**
             * @brief The dimensions finally computed for this viewport, per request from other parts of the application.
             * 
             */
            glm::u16vec2 mComputedDimensions { 800, 600 };

            /**
             * @brief The dimensions requested by other parts of the application, to which this viewport's render texture may need to be resized.
             * 
             */
            glm::u16vec2 mRequestedDimensions { 800, 600 };

            /**
             * @brief A multiplier applied (in case resizing is enabled) determining multiplier to the base or computed dimensions used for the rendering texture.
             * 
             * - For texture resizing: `renderDimensions = renderScale * baseDimensions`.
             * 
             * - For viewport resizing: `renderDimensions = renderScale * computedDimensions`.
             * 
             * The target texture will always be exactly requested dimensions.
             * 
             */
            float mRenderScale { 1.f };

            /**
             * @brief The frequency of rendering updates in real time made on this viewport.
             * 
             */
            UpdateMode mUpdateMode { UpdateMode::ON_RENDER_CAP_FPS };

            /**
             * @brief If an FPS capped update mode is used, specifies the value of that cap.
             * 
             * @see mUpdateMode
             */
            float mFPSCap { 60.f };
        };

        /**
         * @brief Creates a viewport node with components essential to it.
         * 
         * @param name The name of the viewport.
         * @param inheritsWorld Whether it ought to use the same world as its parent viewport or create its own.
         * @param allowActionFlowThrough Whether actions generated by the InputManager are propagated to descendant viewports.
         * @param renderConfiguration The render configuration for this viewport.
         * @param skybox A skybox, if any, used as the background for this viewport's geometry.
         * @return std::shared_ptr<ViewportNode> The newly created viewport.
         */
        static std::shared_ptr<ViewportNode> create(const std::string& name, bool inheritsWorld, bool allowActionFlowThrough, const RenderConfiguration& renderConfiguration, std::shared_ptr<Texture> skybox);

        /**
         * @brief Creates a viewport node based on its JSON description.
         * 
         * @param sceneNodeDescription A description of the viewport's parameters and components in JSON.
         * @return std::shared_ptr<ViewportNode> The newly created viewport.
         * 
         * @see ViewportNodeFromDescription
         * 
         * @see create()
         * 
         */
        static std::shared_ptr<ViewportNode> create(const nlohmann::json& sceneNodeDescription);

        /**
         * @brief Copies the properties and components of another viewport and uses them to construct a new one.
         * 
         * @param other The viewport being used as the basis for a new viewport.
         * @return std::shared_ptr<ViewportNode> The new viewport created.
         */
        static std::shared_ptr<ViewportNode> copy(const std::shared_ptr<const ViewportNode> other);

        /**
         * @brief Gets the resource type string associated with the ViewportNode.
         * 
         * @return std::string This node's resource type string.
         */
        static inline std::string getResourceTypeName() { return "ViewportNode"; }

        /**
         * @brief Sets the next debug texture listed in this viewport's render set to the texture considered "active" by it.
         * 
         * @see RenderSystem
         */
        void viewNextDebugTexture();

        /**
         * @brief Updates the exposure of this viewport's RenderSet.
         * 
         * @param newExposure The new exposure value used by this viewport.
         * 
         * @see RenderSystem
         */
        void updateExposure(float newExposure);

        /**
         * @brief Updates the gamma value of this viewport's RenderSet.
         * 
         * @param newGamma The new gamma value used by this viewport.
         * 
         * @see RenderSystem
         * 
         */
        void updateGamma(float newGamma);

        /**
         * @brief Gets the exposure value used by this viewport's RenderSet.
         * 
         * @return float This viewport's exposure value.
         */
        float getExposure();

        /**
         * @brief Gets the gamma value used by this viewport's RenderSet.
         * 
         * @return float This viewport's gamma value.
         */
        float getGamma();

        /**
         * @brief Returns this viewport instead of base class return value.
         * 
         * @return std::shared_ptr<ViewportNode> A reference to this viewport.
         */
        std::shared_ptr<ViewportNode> getLocalViewport() override;

        /**
         * @brief Returns this viewport instead of base class return value.
         * 
         * @return std::shared_ptr<ViewportNode> A reference to this viewport.
         */
        virtual std::shared_ptr<const ViewportNode> getLocalViewport() const override;

        /**
         * @brief Fetches the render result for the most recently computed render frame.
         * 
         * @param simulationProgress Progress towards the next simulation state after the previous one, as a number between 0 and 1.
         * @return std::shared_ptr<Texture> The texture value returned by this function.
         */
        std::shared_ptr<Texture> fetchRenderResult(float simulationProgress);

        /**
         * @brief Sets the active camera for this viewport's RenderSet via path to the camera node.
         * 
         * @param cameraPath Path to an active scene node with a CameraProperties component in the same ECSWorld as this viewport.
         */
        void setActiveCamera(const std::string& cameraPath);

        /**
         * @brief Sets the active camera for this viewport's RenderSet via a reference to the camera node.
         * 
         * @param cameraNode Active scene node with a CameraProperties component in the same ECSWorld as this viewport.
         */
        void setActiveCamera(std::shared_ptr<SceneNodeCore> cameraNode);

        /**
         * @brief Gets the render configuration for this viewport.
         * 
         * @return RenderConfiguration Object representing this viewport's render configuration.
         */
        RenderConfiguration getRenderConfiguration() const;

        /**
         * @brief Sets the render configuration for this viewport.
         * 
         * @param renderConfiguration This viewport's new render configuration.
         */
        void setRenderConfiguration(const RenderConfiguration& renderConfiguration);

        /**
         * @brief Sets the skybox texture for this object's RenderSystem.
         * 
         * @param skybox The new skybox texture for this viewport's render system.
         * 
         * @see RenderSet
         * @see RenderSystem
         * 
         */
        void setSkybox(std::shared_ptr<Texture> skybox);

        /**
         * @brief Sets this viewport's behaviour when request dimensions (or target dimensions) are changed.
         * 
         * @param type The new type of behaviour to display on resize.
         */
        void setResizeType(RenderConfiguration::ResizeType type);

        /**
         * @brief When resize is enabled, determines how resized render dimensions are computed.
         * 
         * @param mode The new way for render resize dimensions to be computed.
         */
        void setResizeMode(RenderConfiguration::ResizeMode mode);

        /**
         * @brief Sets the scale relative to computed and design dimensions for the render pipeline target.
         * 
         * @param renderScale Multiplier for the render pipeline target (and not the viewport target.)
         */
        void setRenderScale(float renderScale);

        /**
         * @brief Sets the behaviour for frequency of render updates w.r.t render requests.
         * 
         * @param updateMode The render update behaviour.
         */
        void setUpdateMode(RenderConfiguration::UpdateMode updateMode);

        /**
         * @brief If an FPS capped update mode is selected, sets what that cap actually is.
         * 
         * @param fpsCap The new FPS cap for render updates to this viewport's render system.
         */
        void setFPSCap(float fpsCap);

        /**
         * @brief Target dimensions that another part of the program specifies for this viewport.
         * 
         * The target texture produced by this viewport is exactly the same as the dimensions specified in the request.
         * 
         * @param requestedDimensions The dimensions for this viewport's target texture.
         */
        void requestDimensions(glm::u16vec2 requestedDimensions);

        /**
         * @brief Gets the action dispatch object for this viewport, which is the central location from which all actions received by this viewport are distributed to their respective action handlers.
         * 
         * @return ActionDispatch& A reference to this viewport's action dispatch member.
         */
        ActionDispatch& getActionDispatch();

        /**
         * @brief Handles an action received by this viewport, generally by dispatching it to subscribed listeners and propagating the action down to descendant viewports and their listeners.
         * 
         * @param pendingAction The action definition-data pair describing the action.
         * @retval true The action was handled by a listener on this viewport;
         * @retval false The action was not handled by any listener on this viewport;
         */
        bool handleAction(std::pair<ActionDefinition, ActionData> pendingAction);

        /**
         * @brief Returns whether an action handled by one of this viewport's (high precedence) child viewports, should be sent to this viewports other children.
         * 
         * @retval true Propagation to further children is disallowed;
         * @retval false Propagation to further children is allowed;
         */
        bool disallowsHandledActionPropagation() const { return mPreventHandledActionPropagation; }

        /**
         * @brief Destroys this viewport object.
         * 
         */
        ~ViewportNode() override;

        /**
         * @brief (When this viewport is the immediate descendant of a RenderSet::RenderType::ADDITION viewport) The precedence of this viewport relative to other immediate descendants of its parent viewport.
         * 
         * Child viewports that are loaded first presently render over viewports that are loaded later on.
         * 
         * @return uint32_t The precedence of this viewport relative to other viewports.
         */
        inline uint32_t getViewportLoadOrdinal() const { return mViewportLoadOrdinal; }

    protected:
        ViewportNode(const Placement& placement, const std::string& name):
        BaseSceneNode<ViewportNode>{placement, name},
        Resource<ViewportNode>{0},
        mViewportLoadOrdinal { SDL_GetTicks() }
        {}
        ViewportNode(const nlohmann::json& jsonSceneNode):
        BaseSceneNode<ViewportNode>{jsonSceneNode},
        Resource<ViewportNode>{0},
        mViewportLoadOrdinal { SDL_GetTicks() }
        {}
        ViewportNode(const ViewportNode& sceneObject):
        BaseSceneNode<ViewportNode>{sceneObject},
        Resource<ViewportNode>{0},
        mViewportLoadOrdinal { SDL_GetTicks() }
        {}

        /**
         * @brief The ECS world owned by this viewport (if any), as well as the world this node is a member of.
         * 
         */
        std::shared_ptr<ECSWorld> mOwnWorld { nullptr };

        /**
         * @brief Override which for a viewport sets its active camera, and initializes the ECSWorld owned by this viewport.
         * 
         */
        void onActivated() override;

        /**
         * @brief Deactivates this viewports ECSWorld (if there is one) when this ViewportNode is retired.
         * 
         */
        void onDeactivated() override;

        /**
         * @brief Joins this node's entity to a world (either its own or its parent viewport's).
         * 
         * @param world The world this node's entity should join.
         */
        void joinWorld(ECSWorld& world) override;

    private:
        /**
         * @brief Identical to the other create() method, but a private version for use in coupled classes.
         * 
         * @param key A reference to a private struct available only to the scene system and related classes and functions.
         * @param name The name of this viewport.
         * @param inheritsWorld Whether this viewport joins its parent's ECSWorld or creates its own.
         * @param renderConfiguration The render configuration for this viewport.
         * @param skybox The skybox to be rendered in the background of this Viewport.
         * @return std::shared_ptr<ViewportNode> The newly constructed ViewportNode.
         */
        static std::shared_ptr<ViewportNode> create(const Key& key, const std::string& name, bool inheritsWorld, const RenderConfiguration& renderConfiguration, std::shared_ptr<Texture> skybox);

        /**
         * @brief Constructs a new ViewportNode using a simplified constructor.
         * 
         * @param key Private struct available to only the SceneSystem and closely coupled classes.
         * @param placement The position of the ViewportNode, as far as that goes.
         * @param name The name of this viewport.
         */
        ViewportNode(const Key& key, const Placement& placement, const std::string& name):
        BaseSceneNode<ViewportNode>{key, Placement{}, name},
        Resource<ViewportNode>{0}
        {(void)placement; /*prevent unused parameter warnings*/}

        /**
         * @brief Creates a new ViewportNode using itself as a template.
         * 
         * @return std::shared_ptr<SceneNodeCore> The newly constructed ViewportNode
         */
        std::shared_ptr<SceneNodeCore> clone() const override;

        /**
         * @brief Creates and joins its own ECSWorld.
         * 
         */
        void createAndJoinWorld();

        /**
         * @brief Registers a camera that belongs to this viewport, which is a descendant of it and not a descendant of any of its child viewports.
         * 
         * @param cameraNode The camera being registered as belonging to this viewport's domain.
         */
        void registerDomainCamera(std::shared_ptr<SceneNodeCore> cameraNode);

        /**
         * @brief Removes a camera from this viewport's domain.
         * 
         * @param cameraNode The camera being removed from this viewport's domain.
         */
        void unregisterDomainCamera(std::shared_ptr<SceneNodeCore> cameraNode);
        std::shared_ptr<SceneNodeCore> findFallbackCamera();

        /**
         * @brief Gets active descendant viewports (in DFS order) under this Viewport.
         * 
         * @return std::vector<std::shared_ptr<ViewportNode>> A list of descendant viewports, in DFS order.
         */
        std::vector<std::shared_ptr<ViewportNode>> getActiveDescendantViewports();

        /**
         * @brief Gets weak references to ECSWorlds belonging to descendant viewports.
         * 
         * @return std::vector<std::weak_ptr<ECSWorld>> A list of ECSWorlds belonging to this node and its descendant viewports (in DFS order).
         */
        std::vector<std::weak_ptr<ECSWorld>> getActiveDescendantWorlds();

        /**
         * @brief Requests execution of the render pipeline.
         * 
         * The request will be met according to the prerequisites outlined by this viewport's RenderConfiguration::mUpdateMode and RenderConfiguration::mFPSCap taken together.
         * 
         * @param simulationProgress The progress to the next simulation update from the end of the previous one as a number between 0 and 1.
         * @param variableStep The time since the last render request was made.
         * @return uint32_t Time until the next render frame can be computed, per this viewport's update configuration.
         */
        uint32_t render(float simulationProgress, uint32_t variableStep);

        /**
         * @brief Implementation responsible for actually computing a new render frame.
         * 
         * @param simulationProgress The progress to the next simulation update from the end of theprevious one as a number between 0 and 1.
         * 
         */
        void render_(float simulationProgress);

        /**
         * @brief Gets the region of this viewport's target texture that the rendered texture should be mapped to.
         * 
         * @return SDL_Rect The rendered-to sub-region of this viewport's target texture.
         * 
         */
        inline SDL_Rect getCenteredViewportCoordinates() const {
            return {
                mRenderConfiguration.mRequestedDimensions.x/2 - mRenderConfiguration.mComputedDimensions.x/2, mRenderConfiguration.mRequestedDimensions.y/2 - mRenderConfiguration.mComputedDimensions.y/2,
                mRenderConfiguration.mComputedDimensions.x, mRenderConfiguration.mComputedDimensions.y
            };
        }

        /**
         * @brief Comparator used for determining priority of descendant viewports owned by a RenderSet::RenderType::ADDITION viewport.
         * 
         */
        struct ViewportChildComp_ {
            bool operator() (const std::shared_ptr<ViewportNode>& one, const std::shared_ptr<ViewportNode>& two) const {
                return one->getViewportLoadOrdinal() < two->getViewportLoadOrdinal();
            }
        };

        /**
         * @brief Number dictating when this viewport should be computed relative to other viewports, especially when it is used by a RenderSet::RenderType::ADDITION viewport parent.
         * 
         */
        uint64_t mViewportLoadOrdinal { std::numeric_limits<uint64_t>::max() };

        /**
         * @brief A number that is incremented whenever a child viewport is added to this viewport, guaranteeing uniqueness in child viewport values.
         * 
         */
        uint32_t mNLifetimeChildrenAdded { 0 };

        /**
         * @brief Dispatcher for received actions to their action handlers within the domain of this viewport.
         * 
         */
        ActionDispatch mActionDispatch {};

        /**
         * @brief Whether or not handled actions are propagated to this viewport's descendant viewports.
         * 
         */
        bool mActionFlowthrough { false };

        /**
         * @brief When a child viewport handles an action, determines whether the action is sent along to this viewports other children.
         * 
         */
        bool mPreventHandledActionPropagation { true };

        /**
         * @brief This viewports children viewport, in the order they were added to this viewport.
         * 
         */
        std::set<std::shared_ptr<ViewportNode>, ViewportChildComp_> mChildViewports {};

        /**
         * @brief The active camera node associated with this viewport.
         * 
         */
        std::shared_ptr<SceneNodeCore> mActiveCamera { nullptr };

        /**
         * @brief The set of all active cameras that belong to the domain owned by this viewport.
         * 
         */
        std::set<std::shared_ptr<SceneNodeCore>, std::owner_less<std::shared_ptr<SceneNodeCore>>> mDomainCameras {};

        /**
         * @brief The ID of the RenderSet registered with this viewport's RenderSystem corresponding to this ViewportNode.
         * 
         */
        RenderSetID mRenderSet;

        /**
         * @brief The result of rendering from running the rendering pipeline associated with this viewport.
         * 
         * @warning Will be an empty pointer initially, and will be a new texture pointer every time this viewport's render configuration is changed.
         * 
         */
        std::shared_ptr<Texture> mTextureResult { nullptr };

        /**
         * @brief The render configuration associated with this viewport.
         * 
         */
        RenderConfiguration mRenderConfiguration {};

        /**
         * @brief The time, in milliseconds, since the last time a render request was honoured by this viewport.
         * 
         */
        uint32_t mTimeSinceLastRender { static_cast<uint32_t>(1000/mRenderConfiguration.mFPSCap) };

    friend class BaseSceneNode<ViewportNode>;
    friend class SceneNodeCore;
    friend class SceneSystem;
    };

    /**
     * @ingroup ToyMakerSceneSystem ToyMakerECSSystem
     * @brief The SceneSystem, a singleton System, responsible for tracking all objects in the scene, computing their Transforms, and maintaining hierarchical relationships between scene nodes.
     * 
     * In many ways this is the primary interface through which a game developer will manipulate and query the game world.  The scene tree is to games what a DOM tree is to browsers.
     * 
     */
    class SceneSystem: public System<SceneSystem, std::tuple<>, std::tuple<Placement, SceneHierarchyData, Transform>> {
    public:
        /**
         * @brief Constructs a new SceneSystem object.
         * 
         * @param world The world this SceneSystem will belong to, which is always the prototype ECSWorld.
         */
        explicit SceneSystem(std::weak_ptr<ECSWorld> world):
        System<SceneSystem, std::tuple<>, std::tuple<Placement, SceneHierarchyData, Transform>> { world }
        {}

        /**
         * @brief The system type string associated with the SceneSystem.
         * 
         * @return std::string This system's system type string.
         */
        static std::string getSystemTypeName() { return "SceneSystem"; }

        /**
         * @brief Informs this System's ECSWorld that the SceneSystem is a singleton, i.e., there should not be more than one instance of it in the entire project, regardless of how many ECSWorlds are present.
         * 
         * @retval true The SceneSystem is a singleton System;
         * 
         * @todo This is obviously stupid and I should do something about it.
         * 
         */
        bool isSingleton() const override { return true; }

        /**
         * @brief Gets an object of a specific type based on a valid path to that object belonging to the scene system from its root node.
         * 
         * @tparam TObject The object being retrieved, by default shared pointers to SceneNodes.
         * @param where The path to the object being retrieved.
         * @return TObject The object retrieved via this method.
         */
        template<typename TObject=std::shared_ptr<SceneNode>>
        TObject getByPath(const std::string& where);

        /**
         * @brief Gets a scene node by its world-entity ID pair.
         * 
         * @tparam TSceneNode The type of scene node reference deesired.
         * @param universalEntityID The world-entity ID pair of the node being retrieved.
         * @return std::shared_ptr<TSceneNode> The retrieved node.
         */
        template <typename TSceneNode>
        std::shared_ptr<TSceneNode> getNodeByID(const UniversalEntityID& universalEntityID);

        /**
         * @brief Gets nodes by their world-entity ID pair.
         * 
         * @param universalEntityIDs A list of world-entity ID pairs for which nodes are desired.
         * @return std::vector<std::shared_ptr<SceneNodeCore>> A list of nodes retrieved.
         */
        std::vector<std::shared_ptr<SceneNodeCore>> getNodesByID(const std::vector<UniversalEntityID>& universalEntityIDs);

        /**
         * @brief Gets a node by its scene node path.
         * 
         * @param where The path at which the node may be found relative to the SceneSystem's root node.
         * @return std::shared_ptr<SceneNodeCore> A pointer to the retrieved node.
         */
        std::shared_ptr<SceneNodeCore> getNode(const std::string& where);

        /**
         * @brief Removes a node present at the path specified in the call.
         * 
         * @param where The path to the node being removed.
         * @return std::shared_ptr<SceneNodeCore> A reference to the removed node.
         */
        std::shared_ptr<SceneNodeCore> removeNode(const std::string& where);

        /**
         * @brief Adds a node to the SceneSystem's scene tree as a child of the node specified by its path.
         * 
         * @param node The node being added to the SceneSystem's scene tree.
         * @param where The location of the to-be parent to the new node.
         */
        void addNode(std::shared_ptr<SceneNodeCore> node, const std::string& where);

        /**
         * @brief Gets the world associated with the root ViewportNode of the scene system.
         * 
         * @return std::weak_ptr<ECSWorld> A reference to the root node's ECSWorld.
         */
        std::weak_ptr<ECSWorld> getRootWorld() const;

        /**
         * @brief Gets a reference to the root viewport of the SceneSystem, usually used to adjust its rendering configuration.
         * 
         * @return ViewportNode& A reference to the root viewport of the SceneSystem.
         */
        ViewportNode& getRootViewport() const;

        /**
         * @brief A method intended to be used at the start of the application to configure the SceneSystem's root viewport.
         * 
         * @param rootViewportRenderConfiguration The render configuration for the SceneSystem's root ViewportNode.
         */
        void onApplicationInitialize(const ViewportNode::RenderConfiguration& rootViewportRenderConfiguration);

        /**
         * @brief Method to be called by main to initialize the SceneSystem as a whole.
         * 
         */
        void onApplicationStart();

        /**
         * @brief Clean up tasks the SceneSystem should perform before the application is terminated.
         * 
         */
        void onApplicationEnd();

        /**
         * @brief Runs a single step for the root viewport and its descendants, propagating any actions generated by the InputManager up until this point.
         * 
         * @param simStepMillis The time by which the simulation should be advanced.
         * @param triggeredActions Actions triggered up until the current point in the simulation.
         * 
         */
        void simulationStep(uint32_t simStepMillis, std::vector<std::pair<ActionDefinition, ActionData>> triggeredActions={});

        /**
         * @brief Runs the variable step for the SceneSystem's root viewport and its descendants.
         * 
         * This variable step may run multiple times between simulation steps, or once every handful of simulation steps, depending on the processing power and tasks being performed by the platform on which this application is running.
         * 
         * @param simulationProgress The progress to the next simulation step after the previous simulation step as a number between 0 and 1.
         * @param simulationLagMillis The time to the next simulation step at the present time, in milliseconds.
         * @param variableStepMillis The time since the computation of the last frame.
         * @param triggeredActions All unhandled actions triggered to the time this step is being run.
         */
        void variableStep(float simulationProgress, uint32_t simulationLagMillis, uint32_t variableStepMillis, std::vector<std::pair<ActionDefinition, ActionData>> triggeredActions={});

        /**
         * @brief Updates transforms of objects in the scene per changes in those object's Placement component.
         * 
         */
        void updateTransforms();

        /**
         * @brief Runs the render step for the SceneSystem's root viewport and its descendants.
         * 
         * @param simulationProgress Progress to the start of the next simulation step after the end of the previous one, as a number between 0 and 1.
         * @param variableStep The time since the last simulation step.
         * @return uint32_t The time to the next render execution, in milliseconds, based on the SceneSystem's root ViewportNode's configuration.
         */
        uint32_t render(float simulationProgress, uint32_t variableStep);

    private:
        /**
         * @ingroup ToyMakerECSSystem ToyMakerSceneSystem
         * @brief A subsystem of the SceneSystem which tracks, per world, which objects have had their Placement components updated.
         * 
         * These objects then have their UniversalEntityIDs sent to the SceneSystem, which schedules an update to their transforms as soon as possible.
         * 
         * This sub-system only listens for updates on an entity's Placement component, as seen in its base class.
         * 
         */
        class SceneSubworld: public System<SceneSubworld, std::tuple<Placement>, std::tuple<Transform, SceneHierarchyData>> {
        public:
            explicit SceneSubworld(std::weak_ptr<ECSWorld> world):
            System<SceneSubworld, std::tuple<Placement>, std::tuple<Transform, SceneHierarchyData>> { world }
            {}
            static std::string getSystemTypeName() { return "SceneSubworld"; }
        private:
            void onEntityUpdated(EntityID entityID) override;
        };

        /**
         * @brief Helper struct for retrieving nodes based on their UniversalEntityIDs.
         * 
         * The reason it is a struct and not a function is because functions don't permit partial specializations.
         * 
         * @tparam TSceneNode The type of scene node being retrieved.
         * @tparam Enable Template parameter governing whether a particular specialization is permitted or not.
         */
        template <typename TSceneNode, typename Enable=void>
        struct getNodeByID_Helper {
            static std::shared_ptr<TSceneNode> get(const UniversalEntityID& universalEntityID, SceneSystem& sceneSystem);
        };

        /**
         * @brief Returns whether a particular scene node is an active member of the SceneSystem's scene tree.
         * 
         * @param sceneNode The scene node whose activity is being queried.
         * @retval true The scene node is active.
         * @retval false The scene node is inactive.
         */
        bool isActive(std::shared_ptr<const SceneNodeCore> sceneNode) const;

        /**
         * @brief Returns whether a particular scene node is an active member of the SceneSystem's scene tree.
         * 
         * @param UniversalEntityID The id of the entity of the scene node whose activity is being queried.
         * @retval true The scene node is active.
         * @retval false The scene node is not active.
         */
        bool isActive(UniversalEntityID UniversalEntityID) const;

        /**
         * @brief Returns whether a particular scene node is in the SceneSystem's scene tree, even if it is inactive.
         * 
         * @param sceneNode The scene node whose membership in the scene tree is being tested.
         * @retval true The scene node is a member of the SceneSystem's scene tree.
         * @retval false The scene node is not a member of the SceneSystem's scene tree.
         */
        bool inScene(std::shared_ptr<const SceneNodeCore> sceneNode) const;

        /**
         * @brief Returns whether a particular scene node is in the SceneSystem's scene tree, even if it is inactive.
         * 
         * @param UniversalEntityID The ID of the entity of the node whose membership in the scene tree is being tested.
         * @retval true The scene node is a member of the SceneSystem's scene tree.
         * @retval false The scene node is not a member of the SceneSystem's scene tree.
         */
        bool inScene(UniversalEntityID UniversalEntityID) const;

        /**
         * @brief Marks a node as in need of a transform update based on its universal entity id.
         * 
         * @param UniversalEntityID The ID of the node needing a Transform update.
         */
        void markDirty(UniversalEntityID UniversalEntityID);

        /**
         * @brief Returns a list of active viewports including the root viewport of the scene tree.
         * 
         * @return std::vector<std::shared_ptr<ViewportNode>> A list of active viewports in the SceneSystem's scene tree.
         */
        std::vector<std::shared_ptr<ViewportNode>> getActiveViewports();

        /**
         * @brief Returns the ECSWorlds owned by ViewportNodes active in the scene tree.
         * 
         * @return std::vector<std::weak_ptr<ECSWorld>> A list of active ECSWorlds known by the scene system.
         */
        std::vector<std::weak_ptr<ECSWorld>> getActiveWorlds();

        /**
         * @brief Gets the transform of a node solely based on its Placement component and independent of its position in the scene hierarchy.
         * 
         * @param sceneNode The node whose Transform is being computed.
         * @return Transform The computed Transform.
         */
        Transform getLocalTransform(std::shared_ptr<const SceneNodeCore> sceneNode) const;

        /**
         * @brief Returns the Transform of a node based on both its local Placement and its hierarchical transforms.
         * 
         * @param sceneNode The node whose Transform is being retrieved.
         * @return Transform The computed Transform.
         */
        Transform getCachedWorldTransform(std::shared_ptr<const SceneNodeCore> sceneNode) const;

        /**
         * @brief Updates a node's scene hierarchy data per its location in the scene tree.
         * 
         * @param insertedNode The node just inserted into the SceneSystem's scene tree.
         */
        void updateHierarchyDataInsertion(std::shared_ptr<SceneNodeCore> insertedNode);

        /**
         * @brief Removes a node's scene hierarchy data before it's removed from the hierarchy.
         * 
         * @param removedNode The node removed from the SceneSystem's scene tree.
         */
        void updateHierarchyDataRemoval(std::shared_ptr<SceneNodeCore> removedNode);

        /**
         * @brief Plays any side effects associated with a node being added to the SceneSystem's scene tree.
         * 
         * @param sceneNode The node that was added.
         */
        void nodeAdded(std::shared_ptr<SceneNodeCore> sceneNode);

        /**
         * @brief Plays any side effects related to a node being removed from the SceneSystem's scene tree.
         * 
         * @param sceneNode The node that was removed.
         */
        void nodeRemoved(std::shared_ptr<SceneNodeCore> sceneNode);

        /**
         * @brief "Activates" or deactivates a node and its descendants on various ECS systems, per the node's system mask.
         * 
         * @param sceneNode The node whose activation is changing.
         * @param state Whether the node is active or not.
         */
        void nodeActivationChanged(std::shared_ptr<SceneNodeCore> sceneNode, bool state);

        /**
         * @brief Activates this node and its descendants.
         * 
         * @param sceneNode The node being activated.
         */
        void activateSubtree(std::shared_ptr<SceneNodeCore> sceneNode);

        /**
         * @brief Deactivates this node and its descendants.
         * 
         * @param sceneNode The node being deactivated.
         */
        void deactivateSubtree(std::shared_ptr<SceneNodeCore> sceneNode);

        /**
         * @brief A callback used by this system's subsystem to notify the SceneSystem that an entity has been updated.
         * 
         * @param UniversalEntityID The world-entity id pair associated with the updated active scene node.
         */
        void onWorldEntityUpdate(UniversalEntityID UniversalEntityID);

        /**
         * @brief Gets the scene node associated with an entity of a world-entity ID pair.
         * 
         * @param universalEntityID The world-entity id pair associated with a note.
         * @return std::shared_ptr<SceneNodeCore> The node associated with the ID passed as argument.
         */
        std::shared_ptr<SceneNodeCore> getNodeByID(const UniversalEntityID& universalEntityID);

        /**
         * @brief The root node of the SceneSystem, alive and active throughout the lifetime of the application.
         * 
         */
        std::shared_ptr<ViewportNode> mRootNode{ nullptr };

        /**
         * @brief A mapping from the world-entity IDs of active nodes in the SceneSystem to the nodes themselves.
         * 
         */
        std::map<UniversalEntityID, std::weak_ptr<SceneNodeCore>, std::less<UniversalEntityID>> mEntityToNode {};

        /**
         * @brief A list of world-entity IDs associated with all the active nodes known by the SceneSystem.
         * 
         */
        std::set<UniversalEntityID, std::less<UniversalEntityID>> mActiveEntities {};

        /**
         * @brief Nodes which were updated during this variable or simulation step scheduled for a Transform update.
         * 
         */
        std::set<UniversalEntityID, std::less<UniversalEntityID>> mComputeTransformQueue {};

    friend class SceneNodeCore;
    };


    template <typename TSceneNode>
    template <typename ...TComponents>
    std::shared_ptr<TSceneNode> BaseSceneNode<TSceneNode>::create(const Placement& placement, const std::string& name, TComponents...components) {
        std::shared_ptr<SceneNodeCore> newNode ( new TSceneNode(placement, name, components...), &SceneNodeCore_del_);
        newNode->onCreated();
        return std::static_pointer_cast<TSceneNode>(newNode);
    }

    template <typename TSceneNode>
    template <typename ...TComponents>
    std::shared_ptr<TSceneNode> BaseSceneNode<TSceneNode>::create(const Key& key, const Placement& placement, const std::string& name, TComponents...components) {
        std::shared_ptr<SceneNodeCore> newNode( new TSceneNode(key, placement, name, components...), &SceneNodeCore_del_);
        newNode->onCreated();
        return std::static_pointer_cast<TSceneNode>(newNode);
    }

    template <typename TSceneNode>
    std::shared_ptr<TSceneNode> BaseSceneNode<TSceneNode>::create(const nlohmann::json& sceneNodeDescription) {
        std::shared_ptr<SceneNodeCore> newNode{ new TSceneNode{ sceneNodeDescription }, &SceneNodeCore_del_};
        newNode->onCreated();
        return std::static_pointer_cast<TSceneNode>(newNode);
    }

    template <typename TSceneNode>
    std::shared_ptr<TSceneNode> BaseSceneNode<TSceneNode>::copy(const std::shared_ptr<const TSceneNode> sceneNode) {
        std::shared_ptr<SceneNodeCore> newNode { SceneNodeCore::copy(sceneNode) };
        newNode->onCreated();
        return std::static_pointer_cast<TSceneNode>(newNode);
    }

    template<typename ...TComponents>
    std::shared_ptr<SceneNode> SceneNode::create(const Placement& placement, const std::string& name,  TComponents...components) {
        return BaseSceneNode<SceneNode>::create<TComponents...>(placement, name, components...);
    }

    template <typename TObject>
    TObject SceneNodeCore::getByPath(const std::string& where) {
        return getByPath_Helper<TObject>::get(shared_from_this(), where);
    }

    template <typename TObject>
    TObject SceneSystem::getByPath(const std::string& where) {
        return mRootNode->getByPath<TObject>(where);
    }

    template <typename TSceneNode>
    inline std::shared_ptr<TSceneNode> SceneSystem::getNodeByID(const UniversalEntityID& universalEntityID) {
        return SceneSystem::getNodeByID_Helper<TSceneNode>(universalEntityID, *this);
    }

    // Fail retrieval in cases where no explicitly defined object by path
    // method exists
    template <typename TObject, typename Enable>
    TObject SceneNodeCore::getByPath_Helper<TObject, Enable>::get(std::shared_ptr<SceneNodeCore> rootNode, const std::string& where) {
        static_assert(false && "No Object-by-Path method for this type exists");
        return TObject{}; // this is just to shut the compiler up about no returned value
    }

    template <typename TSceneNode, typename Enable>
    std::shared_ptr<TSceneNode> SceneSystem::getNodeByID_Helper<TSceneNode, Enable>::get(const UniversalEntityID& universalEntityID, SceneSystem& sceneSystem) {
        static_assert(false && "No scene node of this type exists");
        return std::shared_ptr<TSceneNode>{};
    }

    template <typename TSceneNode>
    struct SceneSystem::getNodeByID_Helper<TSceneNode, typename std::enable_if_t<std::is_base_of<SceneNodeCore, TSceneNode>::value>> {
        std::shared_ptr<TSceneNode> get(const UniversalEntityID& universalEntityID, SceneSystem& sceneSystem) {
            return std::static_pointer_cast<TSceneNode>(sceneSystem.getNodeByID(universalEntityID));
        }
    };

    template <typename TObject>
    struct SceneNodeCore::getByPath_Helper<std::shared_ptr<TObject>, typename std::enable_if_t<std::is_base_of<SceneNodeCore, TObject>::value>> {
        static std::shared_ptr<TObject> get(std::shared_ptr<SceneNodeCore> rootNode, const std::string& where) {
            return std::static_pointer_cast<TObject>(rootNode->getNode(where));
        }
        static constexpr bool s_valid { true };
    };

    template <typename ...TComponents>
    SceneNodeCore::SceneNodeCore(const Placement& placement, const std::string& name, TComponents...components) {
        validateName(name);
        mName = name;
        mEntity = std::make_shared<Entity>(
            ECSWorld::createEntityPrototype<Placement, SceneHierarchyData, Transform, ObjectBounds, AxisAlignedBounds, TComponents...>(
                placement,
                SceneHierarchyData{},
                Transform{glm::mat4{1.f}},
                ObjectBounds {},
                AxisAlignedBounds {},
                components...
            )
        );
    }

    template <typename ...TComponents>
    SceneNodeCore::SceneNodeCore(const Key&, const Placement& placement, const std::string& name, TComponents...components) {
        mName = name;
        mEntity = std::make_shared<Entity>(
            ECSWorld::createEntityPrototype<Placement, SceneHierarchyData, Transform, ObjectBounds, AxisAlignedBounds, TComponents...>(
                placement,
                SceneHierarchyData{},
                Transform{glm::mat4{1.f}},
                ObjectBounds {},
                AxisAlignedBounds{},
                components...
            )
        );
    }

    template <typename TComponent>
    void SceneNodeCore::addComponent(const TComponent& component, bool bypassSceneActivityCheck) {
        mEntity->addComponent<TComponent>(component);

        // NOTE: required because even though this node's entity's signature changes, it
        // is disabled by default on any systems it is eligible for. We need to activate
        // the node according to its system mask
        if(!bypassSceneActivityCheck && isActive()) {
            mEntity->enableSystems(mSystemMask);
        }
        // NOTE: no removeComponent() equivalent required, as systems that depend on the removed
        // component will automatically have this entity removed from their list, and hence
        // be disabled
    }

    template <typename TComponent>
    TComponent SceneNodeCore::getComponent(const float simulationProgress) const {
        return mEntity->getComponent<TComponent>(simulationProgress);
    }

    template <typename TComponent>
    bool SceneNodeCore::hasComponent() const {
        return mEntity->hasComponent<TComponent>();
    }

    template <typename TComponent>
    void SceneNodeCore::updateComponent(const TComponent& component) {
        mEntity->updateComponent<TComponent>(component);
    }

    template <typename TComponent>
    void SceneNodeCore::addOrUpdateComponent(const TComponent& component, const bool bypassSceneActivityCheck) {
        if(!hasComponent<TComponent>()) {
            addComponent<TComponent>(component, bypassSceneActivityCheck);
            return;
        }
        updateComponent<TComponent>(component);
    }

    template <typename TComponent>
    void SceneNodeCore::removeComponent() {
        mEntity->removeComponent<TComponent>();
    }

    template <typename TSystem>
    bool SceneNodeCore::getEnabled() const {
        return mEntity->isEnabled<TSystem>();
    }

    template <typename TSystem>
    void SceneNodeCore::setEnabled(bool state) {
        const SystemType systemType { mEntity->getWorld().lock()->getSystemType<TSystem>() };
        mSystemMask.set(systemType, state);

        // since the system mask has been changed, we'll want the scene
        // to talk to ECS and make this node visible to the system that
        // was enabled, if eligible
        if(state == true && isActive()){
            mEntity->enableSystems(mSystemMask);
        }
    }

    // Specialization for when the scene system itself is marked
    // enabled or disabled
    template <>
    inline void SceneNodeCore::setEnabled<SceneSystem>(bool state) {
        const SystemType systemType { mEntity->getWorld().lock()->getSystemType<SceneSystem>() };
        // TODO: enabled entities are tracked in both SceneSystem's 
        // mActiveNodes and ECS getEnabledEntities, which is 
        // redundant and may eventually cause errors
        mSystemMask.set(systemType, state);
        //TODO: More redundancy. Why?
        mStateFlags = state? (mStateFlags | SceneNodeCore::StateFlags::ENABLED): (mStateFlags & ~SceneNodeCore::StateFlags::ENABLED);
        mEntity->getWorld().lock()->getSystem<SceneSystem>()->nodeActivationChanged(
            shared_from_this(),
            state
        );
    }

    // Prevent removal of components essential to a scene node
    template <>
    inline void SceneNodeCore::removeComponent<Placement>() {
        assert(false && "Cannot remove a scene node's Placement component");
    }
    template <>
    inline void SceneNodeCore::removeComponent<Transform>() {
        assert(false && "Cannot remove a scene node's Transform component");
    }

    template <typename TSceneNode>
    std::shared_ptr<TSceneNode> SceneNodeCore::getNodeByID(EntityID entityID) {
        return getWorld().lock()->getSystem<SceneSystem>()->getNodeByID<TSceneNode>({getWorldID(), entityID});
    }

    /** @ingroup ToyMakerRenderSystem ToyMakerSerialization */
    NLOHMANN_JSON_SERIALIZE_ENUM(ViewportNode::RenderConfiguration::ResizeType, {
        {ViewportNode::RenderConfiguration::ResizeType::OFF, "off"},
        {ViewportNode::RenderConfiguration::ResizeType::VIEWPORT_DIMENSIONS, "viewport-dimensions"},
        {ViewportNode::RenderConfiguration::ResizeType::TEXTURE_DIMENSIONS, "texture-dimensions"},
    });

    /** @ingroup ToyMakerRenderSystem ToyMakerSerialization */
    NLOHMANN_JSON_SERIALIZE_ENUM(ViewportNode::RenderConfiguration::ResizeMode, {
        {ViewportNode::RenderConfiguration::ResizeMode::FIXED_ASPECT,"fixed-aspect"},
        {ViewportNode::RenderConfiguration::ResizeMode::EXPAND_VERTICALLY, "expand-vertically"},
        {ViewportNode::RenderConfiguration::ResizeMode::EXPAND_HORIZONTALLY, "expand-horizontally"},
        {ViewportNode::RenderConfiguration::ResizeMode::EXPAND_FILL, "expand-fill"},
    });

    /** @ingroup ToyMakerRenderSystem ToyMakerSerialization */
    NLOHMANN_JSON_SERIALIZE_ENUM(ViewportNode::RenderConfiguration::UpdateMode, {
        {ViewportNode::RenderConfiguration::UpdateMode::NEVER, "never"},
        {ViewportNode::RenderConfiguration::UpdateMode::ONCE, "once"},
        {ViewportNode::RenderConfiguration::UpdateMode::ON_FETCH, "on-fetch"},
        {ViewportNode::RenderConfiguration::UpdateMode::ON_RENDER, "on-render"},
        {ViewportNode::RenderConfiguration::UpdateMode::ON_RENDER_CAP_FPS, "on-render-cap-fps"},
    });

    /** @ingroup ToyMakerRenderSystem ToyMakerSerialization */
    NLOHMANN_JSON_SERIALIZE_ENUM(ViewportNode::RenderConfiguration::RenderType, {
        {ViewportNode::RenderConfiguration::RenderType::BASIC_3D, "basic-3d"},
        {ViewportNode::RenderConfiguration::RenderType::ADDITION, "addition"},
    });

    /** @ingroup ToyMakerRenderSystem ToyMakerSerialization */
    inline void to_json(nlohmann::json& json, const ViewportNode::RenderConfiguration& renderConfiguration) {
        json = {
            {"base_dimensions", nlohmann::json::array({renderConfiguration.mBaseDimensions.x, renderConfiguration.mBaseDimensions.y})},
            {"update_mode", renderConfiguration.mUpdateMode},
            {"resize_type", renderConfiguration.mResizeType},
            {"resize_mode", renderConfiguration.mResizeMode},
            {"render_scale", renderConfiguration.mRenderScale},
            {"render_type", renderConfiguration.mRenderType},
            {"fps_cap", renderConfiguration.mFPSCap},
        };
    }

    /** @ingroup ToyMakerRenderSystem ToyMakerSerialization */
    inline void from_json(const nlohmann::json& json, ViewportNode::RenderConfiguration& renderConfiguration) {
        assert(json.find("base_dimensions") != json.end() && "Viewport descriptions must contain the \"base_dimensions\" size 2 array of Numbers attribute");
        json.at("base_dimensions")[0].get_to(renderConfiguration.mBaseDimensions.x);
        json.at("base_dimensions")[1].get_to(renderConfiguration.mBaseDimensions.y);
        json.at("render_type").get_to(renderConfiguration.mRenderType);
        renderConfiguration.mRequestedDimensions = renderConfiguration.mBaseDimensions;
        renderConfiguration.mComputedDimensions = renderConfiguration.mBaseDimensions;
        assert(renderConfiguration.mBaseDimensions.x > 0 && renderConfiguration.mBaseDimensions.y > 0 && "Base dimensions cannot include a 0 in either dimension");

        assert(json.find("update_mode") != json.end() && "Viewport render configuration must include the \"update_mode\" enum attribute");
        json.at("update_mode").get_to(renderConfiguration.mUpdateMode);

        assert(json.find("resize_type") != json.end() && "Viewport render configuration must include the \"resize_type\" enum attribute");
        json.at("resize_type").get_to(renderConfiguration.mResizeType);

        assert(json.find("resize_mode") != json.end() && "Viewport render configuration must include the \"resize_mode\" enum attribute");
        json.at("resize_mode").get_to(renderConfiguration.mResizeMode);

        assert(json.find("render_scale") != json.end() && "Viewport render configuration must include the \"render_scale\" float attribute");
        json.at("render_scale").get_to(renderConfiguration.mRenderScale);
        assert(renderConfiguration.mRenderScale > 0.f && "Render scale must be a positive non-zero decimal number");

        assert(json.find("fps_cap") != json.end() && "Viewport must include the \"fps_cap\" float attribute");
        json.at("fps_cap").get_to(renderConfiguration.mFPSCap);
        assert(renderConfiguration.mFPSCap > 0.f && "FPS cap must be a positive non-zero decimal number");
    }

}

#endif

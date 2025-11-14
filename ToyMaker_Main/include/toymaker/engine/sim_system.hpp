/**
 * @ingroup ToyMakerSimSystem ToyMakerSceneSystem
 * @file sim_system.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Classes and structs relating to the SimSystem, the system responsible for providing some level of general scriptability to scene nodes from a game developer's point-of-view.
 * @version 0.3.2
 * @date 2025-09-07
 * 
 * 
 */

/**
 * @defgroup ToyMakerSimSystem User Object-Oriented Logic
 * @ingroup ToyMakerEngine ToyMakerSceneSystem
 * 
 */

#ifndef FOOLSENGINE_SIMSYSTEM_H
#define FOOLSENGINE_SIMSYSTEM_H

#include <memory>
#include <set>
#include <cassert>
#include <functional>
#include <typeinfo>
#include <type_traits>

#include <nlohmann/json.hpp>

#include "core/resource_database.hpp"
#include "core/ecs_world.hpp"

#include "spatial_query_system.hpp"
#include "registrator.hpp"
#include "input_system/input_system.hpp"
#include "scene_system.hpp"
#include "signals.hpp"

namespace ToyMaker{

    class BaseSimObjectAspect;
    class SimObject;
    class SimSystem;

    /**
     * @ingroup ToyMakerECSComponent ToyMakerSimSystem
     * @brief The component associated with the SimSystem.
     * 
     * Provides a (raw, unmanaged) pointer to the SimObject it is a component of.
     * 
     * @see ECSWorld::registerComponentTypes()
     * 
     */
    struct SimCore {
        /**
         * @brief Gets the component type string for this class.
         * 
         * @return std::string The component type string for this class.
         */
        inline static std::string getComponentTypeName() { return "SimCore"; }

        /**
         * @brief Unmanaged pointer to the SimObject this is a component of.
         * 
         */
        SimObject* mSimObject;
    };

    // never used, so make empty definitions for these
    /** @ingroup ToyMakerSerialization ToyMakerSimSystem */
    inline void from_json(const nlohmann::json& json, SimCore& simCore) {
        (void)json; // prevent unused parameter warnings
        (void)simCore; // prevent unused parameter warnings
    }

    // never used, so make empty definitions for these
    /** @ingroup ToyMakerSerialization ToyMakerSimSystem */
    inline void to_json(nlohmann::json& json, const SimCore& simCore) {
        (void)json; // prevent unused parameter warnings
        (void)simCore; // prevent unused parameter warnings
    }

    /**
     * @ingroup ToyMakerSimSystem ToyMakerECSSystem
     * @brief The SimSystem is a system responsible for providing scriptability via SimObjects and SimObjectAspects.
     * 
     * A developer creates a subclass of SimObjectAspect, which can then be attached to a SimObject in the game either through the scene file or programmatically during the application's running.
     * 
     * The SimSystem then forwards scene object related lifecycle events and engine-loop events to active and interested SimObjectAspect subclasses. 
     * 
     */
    class SimSystem: public System<SimSystem, std::tuple<>, std::tuple<SimCore>> {
    public:
        explicit SimSystem(std::weak_ptr<ECSWorld> world):
        System<SimSystem, std::tuple<>, std::tuple<SimCore>>{world}
        {}

        /**
         * @brief Gets the system type string associated with this system.
         * 
         * @return std::string The system type string associated with this system.
         */
        static std::string getSystemTypeName() { return "SimSystem"; }

        /**
         * @brief Tests whether an aspect with a certain name is a valid SimObjectAspect type known by this application's SimSystem.
         * 
         * @param aspectName The name of the aspect.
         * @retval true This aspect has been registered with the SimSystem;
         * @retval false This aspect has not yet been registered with the SimSystem;
         */
        bool aspectRegistered(const std::string& aspectName) const;

    protected:
        virtual std::shared_ptr<BaseSystem> instantiate(std::weak_ptr<ECSWorld> world) override;

    private:
        
        /**
         * @brief Registers a new aspect (a derived class of SimObjectAspect) as an aspect known by the SimSystem.
         * 
         * @tparam TSimObjectAspect The type of the new aspect.
         */
        template <typename TSimObjectAspect>
        void registerAspect();

        /**
         * @brief Constructs a SimObjectAspect based on its description in JSON.
         * 
         * @param jsonAspectProperties The aspect's JSON description, including its type and initial value.
         * @return std::shared_ptr<BaseSimObjectAspect> [A base class pointer to] The newly constructed SimObjectAspect.
         */
        std::shared_ptr<BaseSimObjectAspect> constructAspect(const nlohmann::json& jsonAspectProperties);

        /**
         * @brief The method responsible for forwarding engine simulation step events to SimObjects and their SimObjectAspects.
         * 
         * @param simulationStepMillis The time by which the simulation should be advanced, in milliseconds.
         */
        void onSimulationStep(uint32_t simulationStepMillis) override;

        /**
         * @brief The method responsible for forwarding engine variable step events to SimObjects and their SimObjectAspects.
         * 
         * @param simulationProgress The progress towards the next simulation step after the end of the next simulation step as a number between 0 and 1.
         * @param variableStepMillis The time since the computation of the last variable step.
         */
        void onVariableStep(float simulationProgress, uint32_t variableStepMillis) override;

        /**
         * @brief A database for all constructors of SimObjectAspects, provided by the implementation of the aspects themselves.
         * 
         */
        std::unordered_map<std::string, std::shared_ptr<BaseSimObjectAspect> (*)(const nlohmann::json& jsonAspectProperties)> mAspectConstructors {};

    friend class BaseSimObjectAspect;
    friend class SimObject;
    };

    /**
     * @ingroup ToyMakerSimSystem ToyMakerSceneSystem
     * @brief A wrapper on entity that allows objects in the Scene to be scriptable.
     * 
     * This node will track (in addition to components owned by SceneNodeCore) any SimObjectAspects attached to it.  It will also ensure that such aspects receive scene object lifecycle and engine related events, which it itself receives from the SimSystem.
     * 
     * The purpose of this is to facilitate the same sort of object-oriented, component-oriented interfaces game developers may be accustomed to in other engines (like Monobehaviours in Unity, GDScript in Godot).
     * 
     * ## Usage:
     * 
     * An example JSON description of a SimObject, as would be seen in a scene file:
     * 
     * ```jsonc
     * {
     *     "aspects": [
     *         { "type": "QueryClick" }, 
     *         { "type": "UrLookAtBoard", "offset": [0.0, 1.0, 7.0] }
     *     ],
     *     "components": [
     *         {
     *             "fov": 55.5,
     *             "aspect": 1.77778,
     *             "orthographic_dimensions": {"horizontal": 0, "vertical": 0},
     *             "near_far_planes": {"near": 0.5, "far": 100},
     *             "projection_mode": "frustum",
     *             "type": "CameraProperties"
     *         },
     *         {
     *             "orientation": [
     *                 0.310491145,
     *                 -0.0720598251,
     *                 -0.923300385,
     *                 -0.214287385 
     *             ],
     *             "position": [
     *                 -6.0,
     *                 8.4,
     *                 6.0,
     *                 1.0
     *             ],
     *             "scale": [
     *                 1.0,
     *                 1.0,
     *                 1.0
     *             ],
     *             "type": "Placement"
     *         }
     *     ],
     *     "name": "camera",
     *     "parent": "/viewport_3D/",
     *     "type": "SimObject"
     * }
     * 
     * ```
     * 
     * @see SimObjectAspect
     * @see SimSystem
     * 
     */
    class SimObject: public BaseSceneNode<SimObject>, public Resource<SimObject> {
    public:
        /**
         * @brief Detaches all SimObjectAspects from this SimObject before allowing destruction to proceed.
         * 
         */
        ~SimObject() override;


        /**
         * @brief Gets the resource type string for the SimObject class.
         * 
         * @return std::string The resource type string for this class.
         */
        inline static std::string getResourceTypeName() { return "SimObject"; }

        /**
         * @brief Creates a new SimObject scene node initialized with some placement and component values.
         * 
         * @tparam TComponents The types of components being initialized for this node's entity.
         * @param placement This node's initial Placement value.
         * @param name The name for this node in the scene hierarchy.
         * @param components The values of other components being initialized for this node.
         * @return std::shared_ptr<SimObject> The newly created SimObjectAspect.
         * 
         */
        template <typename ...TComponents>
        static std::shared_ptr<SimObject> create(const Placement& placement, const std::string& name, TComponents...components);

        /**
         * @brief Creates a SimObject based on its description in JSON.
         * 
         * @param jsonSimObject A JSON description for this SimObject, including initial component values.
         * @return std::shared_ptr<SimObject> The newly constructed SimObject value.
         */
        static std::shared_ptr<SimObject> create(const nlohmann::json& jsonSimObject);

        /**
         * @brief Creates a new SimObject as a copy of another.
         * 
         * @param simObject The SimObject being copied.
         * @return std::shared_ptr<SimObject> The newly constructed SimObject.
         */
        static std::shared_ptr<SimObject> copy(const std::shared_ptr<const SimObject> simObject);

        /**
         * @brief Constructs and attaches a new SimObjectAspect to this node based on the aspect's description in JSON.
         * 
         * @param jsonAspectProperties The SimObjectAspect's JSON description.
         */
        void addAspect(const nlohmann::json& jsonAspectProperties);

        /**
         * @brief Constructs and attaches a new SimObjectAspect to this node which is a copy of the aspect passed as argument.
         * 
         * @param simObjectAspect The aspect being copied.
         */
        void addAspect(const BaseSimObjectAspect& simObjectAspect);

        /**
         * @brief Tests whether an aspect of a particular type is attached to this node.
         * 
         * @tparam TSimObjectAspect The type of aspect whose presence is being tested.
         * @retval true The aspect is present;
         * @retval false The aspect is absent;
         */
        template <typename TSimObjectAspect>
        bool hasAspect() const;

        /**
         * @brief Tests whether an aspect of a particular type is attached to this node.
         * 
         * @param aspectType The aspect type string of the aspect whose presence is being tested.
         * @retval true The aspect is present;
         * @retval false The aspect is absent;
         */
        bool hasAspect(const std::string& aspectType) const;

        /**
         * @brief Tests whether any aspects implementing some base class are present on this node.
         * 
         * @tparam TInterface The base class an aspect of this class, possibly.
         * @retval true An aspect implementing TInterface is present;
         * @retval false No aspect implementing TInterface is present;
         */
        template <typename TInterface>
        bool hasAspectWithInterface() const;

        /**
         * @brief Adds or replaces an aspect for this node.
         * 
         * @param simObjectAspect The aspect copied to construct a new aspect.
         */
        void addOrReplaceAspect(const BaseSimObjectAspect& simObjectAspect);

        /**
         * @brief Adds or replaces an aspect for this node.
         * 
         * @param jsonAspectProperties A JSON description for the aspect added to this node.
         */
        void addOrReplaceAspect(const nlohmann::json& jsonAspectProperties);

        /**
         * @brief Gets a reference to a specific aspect present on this node.
         * 
         * @tparam TSimObjectAspect The type of the aspect being fetched.
         * @return TSimObjectAspect& The fetched aspect.
         */
        template <typename TSimObjectAspect>
        TSimObjectAspect& getAspect();

        /**
         * @brief Gets (a base class reference to) an aspect present on this node.
         * 
         * @param aspectType The aspect type string for the asset being fetched.
         * @return BaseSimObjectAspect& The fetched aspect.
         */
        BaseSimObjectAspect& getAspect(const std::string& aspectType);

        /**
         * @brief Gets a list of aspects which subclass some base class.
         * 
         * @tparam TInterface The class implemented by fetched aspects.
         * @return std::vector<std::reference_wrapper<TInterface>> A list of aspects implementing TInterface, by their base class references.
         */
        template <typename TInterface>
        std::vector<std::reference_wrapper<TInterface>> getAspectsWithInterface();

        /**
         * @brief Removes an aspect of a certain type from this SimObject.
         * 
         * @tparam TSimObjectAspect The type of aspect being removed.
         */
        template <typename TSimObjectAspect>
        void removeAspect();

        /**
         * @brief Removes an aspect of a certain type from this SimObject.
         * 
         * @param aspectType The aspect type string of the asset being removed.
         */
        void removeAspect(const std::string& aspectType);

    protected:

        template <typename ...TComponents>
        SimObject(const Placement& placement, const std::string& name, TComponents...components);
        SimObject(const nlohmann::json& jsonSimObject);
        SimObject(const SimObject& other);

        // /**
        //  * @brief Copy assignment operator.
        //  * 
        //  * @todo Figure out whether this operator will actually ever be useful.
        //  * 
        //  */
        // SimObject& operator=(const SimObject& other);

    private:
        /**
         * @brief Calls aspect simulation update hooks for attached aspects.
         * 
         * @param simStepMillis The time by which the simulation is to be advanced.
         */
        void simulationUpdate(uint32_t simStepMillis);

        /**
         * @brief Calls aspect variable update hooks for attached aspects.
         * 
         * 
         * @param variableStepMillis The time since the computation of the last frame.
         */
        void variableUpdate(uint32_t variableStepMillis);

        /**
         * @brief Notifies all attached aspects that this scene node has become an active part of the scene.
         * 
         * @see BaseSimObjectAspect::onActivated()
         * 
         */
        void onActivated() override;

        /**
         * @brief Notifies all attached aspects that this scene node has stopped being an active part of the scene.
         * 
         * @see BaseSimObjectAspect::onDeactivated()
         * 
         */
        void onDeactivated() override;

        /**
         * @brief Copies all aspects present on another SimObject onto this one.
         * 
         * @param other An object whose aspects are being copied.
         */
        void copyAspects(const SimObject& other);

        /**
         * @brief Copies this scene object returning one containing all the same components and aspects as it.
         * 
         * @return std::shared_ptr<SceneNodeCore> The newly constructed SimObject, as a pointer to its base class.
         */
        std::shared_ptr<SceneNodeCore> clone() const override;

        /**
         * @brief Aspect name and pointer pairs for all aspects attached to this SimObject.
         * 
         */
        std::unordered_map<std::string, std::shared_ptr<BaseSimObjectAspect>> mSimObjectAspects {};

    friend class SimSystem;
    friend class BaseSimObjectAspect;
    friend class BaseSceneNode<SimObject>;
    };

    /**
     * @ingroup ToyMakerSimSystem ToyMakerInputSystem
     * @brief A class representing the connection between an Action generated by the InputManager, and a BaseSimObjectAspect method that is interested in handling the action.
     * 
     * @see BaseSimObjectAspect::declareFixedActionBinding()
     * @see ActionDefinition
     * @see ActionData
     * 
     */
    class FixedActionBinding {
    private:
        /**
         * @brief Calls the handler this binding holds a reference to with some newly received action data generated by the input system.
         * 
         * @param actionData The data of the generated action event.
         * @param actionDefinition The definition of the action.
         * @retval true The action was handled by this aspect;
         * @retval false The action was not handled by this aspect.
         */
        inline bool call(const ActionData& actionData, const ActionDefinition& actionDefinition) {
            return mHandler(actionData, actionDefinition);
        }

        /**
         * @brief Constructs a new binding object with the given action name and context, and a reference to the function interested in handling that action.
         * 
         * @param context The context name of the action.
         * @param name The name of the action itself.
         * @param handler The handler interested in receiving the action.
         */
        FixedActionBinding(const std::string& context, const std::string& name, std::function<bool(const ActionData&, const ActionDefinition&)> handler):
        mContext { context },
        mName { name },
        mHandler { handler }
        {}

        /**
         * @brief The name of the context owning the action.
         * 
         */
        std::string mContext;

        /**
         * @brief The name of the action itself.
         * 
         */
        std::string mName;

        /**
         * @brief A reference to the handler interested in receiving the action.
         * 
         * Such a handler must return a boolean value. `true` indicates that the handler was able to do something with the action, while `false` indicates that nothing was done with the action.
         * 
         */
        std::function<bool(const ActionData&, const ActionDefinition&)> mHandler;
    friend class BaseSimObjectAspect;
    };

    /**
     * @ingroup ToyMakerSimSystem ToyMakerSignals
     * @brief The base class for all aspects, providing an interface to its attached SimObject, and consequently, the engine's SceneSystem.
     * 
     * @see SimObjectAspect
     * 
     */
    class BaseSimObjectAspect : public std::enable_shared_from_this<BaseSimObjectAspect>, public SignalTracker, public IActionHandler {
    public:
        /**
         * @brief Destroys this object.
         * 
         */
        virtual ~BaseSimObjectAspect()=default;

        /**
         * @brief Overriding this allows an aspect to respond to simulation updates.
         * 
         * @param simStepMillis The time by which the simulation should be advanced, in milliseconds.
         * 
         * @see SceneSystem::simulationStep()
         */
        virtual void simulationUpdate(uint32_t simStepMillis) { (void)simStepMillis; /*prevent unused parameter warnings*/}

        /**
         * @brief Overriding this allows an aspect to respond to variable updates.
         * 
         * @param variableStepMillis The time since the execution of the last frame, in milliseconds.
         * 
         * @see SceneSystem::variableStep
         */
        virtual void variableUpdate(uint32_t variableStepMillis) {(void)variableStepMillis; /*prevent unused parameter warnings*/}

        /**
         * @brief Pipes an action received from the InputManager via our SimObject to all that action's handler methods on this aspect.
         * 
         * @param actionData The data describing the action event.
         * 
         * @param actionDefinition The definition of the action event.
         * 
         * @retval true The action was handled by this aspect;
         * 
         * @retval false The action was not handled by this aspect;
         * 
         * @todo Shouldn't this be private?  Will something bad happen if we make this private?
         * 
         */
        bool handleAction(const ActionData& actionData, const ActionDefinition& actionDefinition) override final;

        /**
         * @brief Returns the closest ancestor viewport to this node, if one exists (which it should, since this shouldn't be called until this aspect is attached to an active SimObject).
         * 
         * @return ViewportNode& A reference to this aspect's node's closest ancestor viewport.
         */
        ViewportNode& getLocalViewport();

    protected:
        BaseSimObjectAspect()=default;

        BaseSimObjectAspect(const BaseSimObjectAspect& other)=delete;
        BaseSimObjectAspect(BaseSimObjectAspect&& other)=delete;

        /**
         * @brief Registers an implementation of an aspect with the SimSystem.
         * 
         * This gives the sim system a pointer to the function responsible for the construction of an aspect, allowing the aspect to be constructed by its description in a JSON scene file.
         * 
         * @tparam TSimObjectAspectDerived The derived aspect's type.
         * 
         * @see SimSystem
         */
        template <typename TSimObjectAspectDerived>
        static inline void registerAspect() {
            // ensure registration of SimSystem before trying to register
            // this aspect
            auto& simSystemRegistrator { 
                Registrator<System<SimSystem, std::tuple<>, std::tuple<SimCore>>>::getRegistrator()
            };
            simSystemRegistrator.emptyFunc(); // (I think) prevent registrator from being optimized out.

            // Let SimSystem know that this type of aspect exists
            ECSWorld::getSystemPrototype<SimSystem>()->registerAspect<TSimObjectAspectDerived>();
        }

        /**
         * @brief Returns the sim object that this aspect is attached to.
         * 
         * @return SimObject& A reference to the sim object to which this aspect belongs.
         */
        SimObject& getSimObject();

        /**
         * @brief Adds a component of some type to the underlying entity.
         * 
         * @tparam TComponent The type of component being added.
         * @param component The value of the component when it is added.
         */
        template <typename TComponent>
        void addComponent(const TComponent& component);

        /**
         * @brief Tests whether a component of some specific type is present on the object.
         * 
         * @tparam TComponent The type of component whose existence is being tested.
         * @retval true The component exists;
         * @retval false The component does not exist;
         */
        template <typename TComponent>
        bool hasComponent();

        /**
         * @brief Updates the value of a component belonging to this object to a new one.
         * 
         * @tparam TComponent The type of component being updated.
         * @param component The new value of the component.
         */
        template <typename TComponent>
        void updateComponent(const TComponent& component);

        /**
         * @brief Gets the value of a component belonging to this object.
         * 
         * @tparam TComponent The type of component being retrieved.
         * @param simulationProgress The progress towards the next simulation step after the previous one represented as a value between 0 and 1.
         * @return TComponent The retrieved component value.
         */
        template <typename TComponent>
        TComponent getComponent(const float simulationProgress=1.f) const;

        /**
         * @brief Removes a component of some type belonging to the underlying SimObject.
         * 
         * @tparam TComponent The type of component being removed.
         */
        template <typename TComponent>
        void removeComponent();

        /**
         * @brief Adds a new aspect to the underlying SimObject constructed based on its properties in JSON.
         * 
         * @param jsonAspectProperties The properties of the aspect in JSON.
         * 
         */
        void addAspect(const nlohmann::json& jsonAspectProperties);

        /**
         * @brief Adds a new aspect to the underlying SimObject copied from an already existing aspect.
         * 
         * @param aspect The aspect being copied.
         */
        void addAspect(const BaseSimObjectAspect& aspect);

        /**
         * @brief Tests whether an aspect of a particular type is attached to the underlying SimObject.
         * 
         * @tparam TSimObjectAspect The type of aspect whose existence is being tested.
         * @retval true An aspect of the specified type exists;
         * @retval false No aspect of the type specified exists;
         */
        template <typename TSimObjectAspect>
        bool hasAspect() const;

        /**
         * @brief Tests whether an aspect of a particular type is attached to the underlying SimObject.
         * 
         * @param aspectType The aspect type name of the aspect.
         * @retval true An aspect of this type exists;
         * @retval false No aspect of this type exists;
         * 
         */
        bool hasAspect(const std::string& aspectType) const;

        /**
         * @brief Gets an aspect of a particular type belonging to the underlying SimObject.
         * 
         * @tparam TSimObjectAspect The type of the aspect being retrieved.
         * @return TSimObjectAspect& The retrieved aspect.
         */
        template <typename TSimObjectAspect>
        TSimObjectAspect& getAspect();

        /**
         * @brief Gets (a base class reference to) an aspect of a particular type belonging to the underlying SimObject.
         * 
         * @param aspectType The aspect type name of the aspect being retrieved.
         * @return BaseSimObjectAspect& The retrieved aspect.
         */
        BaseSimObjectAspect& getAspect(const std::string& aspectType);

        /**
         * @brief Removes an aspect from the underlying SimObject.
         * 
         * @tparam TSimObjectAspect The type of the aspect being removed.
         */
        template <typename TSimObjectAspect>
        void removeAspect();

        /**
         * @brief Adds or replaces an aspect on the underlying SimObject with a new aspect constructed as a copy of another.
         * 
         * @param aspect The aspect being copied.
         * 
         */
        void addOrReplaceAspect(const BaseSimObjectAspect& aspect);

        /**
         * @brief Adds or replaces an aspect on the underlying SimObject with a new aspect constructed from its JSON description.
         * 
         * @param jsonAspectProperties The JSON description of the aspect, including its type and value.
         */
        void addOrReplaceAspect(const nlohmann::json& jsonAspectProperties);

        /**
         * @brief Binds some method (or any function) present on this object to an action generated by the InputManager.
         * 
         * ## Usage
         * 
         * ```c++
         * 
         * // NOTE: Class inherited from SimObjectAspect
         * class QueryClick: public ToyMaker::SimObjectAspect<QueryClick>, public IUsePointer {
         * 
         *     // ...
         * 
         *     // NOTE: Handler for UI Tap actions
         *     bool onLeftClick(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition);
         * 
         *     // NOTE: Binding responsible for forwarding UI Tap actions to onLeftClick()
         *     std::weak_ptr<ToyMaker::FixedActionBinding> handlerLeftClick {
         *         declareFixedActionBinding(
         *             "UI",
         *             "Tap",
         *             [this](const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
         *                 return this->onLeftClick(actionData, actionDefinition);
         *             }
         *         )
         *     };    
         * 
         *     // ...
         * 
         * };
         * 
         * ```
         * 
         * @param context The name of the context of the bound action.
         * @param action The name of the action itself.
         * @return std::weak_ptr<FixedActionBinding> A weak pointer to the object representing the binding itself.
         */
        std::weak_ptr<FixedActionBinding> declareFixedActionBinding(const std::string& context, const std::string& action, std::function<bool(const ActionData&, const ActionDefinition&)>);

        /**
         * @brief Gets the ID of the ECSWorld Entity belonging to our SimObject.
         * 
         * @return EntityID This object's entity's ECS entity ID.
         */
        EntityID getEntityID() const;

        /**
         * @brief Gets a weak reference to the ECSWorld to which our SimObject's entity belongs.
         * 
         * @return std::weak_ptr<ECSWorld> The ECSWorld our SimObject's entity belongs to.
         */
        std::weak_ptr<ECSWorld> getWorld() const;

        /**
         * @brief Overridable function for fetching the aspect type string of an aspect.
         * 
         * @return std::string This aspect's aspect type string.
         */
        virtual std::string getAspectTypeName() const = 0;

    private:
        /**
         * @brief Called when an aspect has just been activated to bind its handler method to its associated action.
         * 
         */
        void activateFixedActionBindings();

        /**
         * @brief Called when an aspect's SimObject has been deactivated, retiring all currently active action bindings.
         * 
         */
        void deactivateFixedActionBindings();

        void onAttached_();
        void onDetached_();
        void onActivated_();
        void onDeactivated_();

        /**
         * @brief Callback for when an aspect has just been attached to an object (but which hasn't yet been activated).
         * 
         */
        virtual void onAttached(){}

        /**
         * @brief Callback for when an aspect is about to be removed from an object (after it has been deactivated).
         * 
         */
        virtual void onDetached(){}

        /**
         * @brief Callback for when the aspect is activated (after it is attached to an active SimObject, or if the SimObject it was attached to has just been activated)
         * 
         */
        virtual void onActivated() {}

        /**
         * @brief Callback for when the aspect is deactivated (just prior to being detached, or when its SimObject has been deactivated).
         * 
         */
        virtual void onDeactivated() {}

        /**
         * @brief Tests whether this aspect is attached to a SimObject.
         * 
         * @retval true This aspect is attached to a SimObject;
         * @retval false This aspect is untethered;
         */
        inline bool isAttached() { return mState&AspectState::ATTACHED; }

        /**
         * @brief Tests whether the SimObject this aspect is attached to is active on the SceneSystem and the SimSystem.
         * 
         * @retval true The underlying SimObject (and therefore this aspect) is active.
         * @retval false The underlying SimObject (and therefore this aspect) is inactive, or the SimObject does not exist.
         */
        inline bool isActive() { return mState&AspectState::ACTIVE; }

        /**
         * @brief Causes this aspect to be detached from its previous owner and to be attached to a new one.
         * 
         * @param owner The new owner of this aspect.
         */
        void attach(SimObject* owner);

        /**
         * @brief Causes this aspect to be disconnected from its current SimObject.
         * 
         */
        void detach();

        /**
         * @brief A method which must be overridden to specify how a new aspect should be constructed as a copy of this one.
         * 
         * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed aspect.
         */
        virtual std::shared_ptr<BaseSimObjectAspect> clone() const = 0;

        /**
         * @brief The set of action bindings owned by this aspect.
         * 
         * @see declareFixedActionBinding()
         */
        std::map<
            std::pair<std::string, std::string>,
            std::shared_ptr<FixedActionBinding>,
            std::less<std::pair<std::string, std::string>>
        > mFixedActionBindings {};

        /**
         * @brief The SimObject underlying this aspect.
         * 
         */
        SimObject* mSimObject { nullptr };

        /**
         * @brief Enum for mask values representing the readiness of an aspect.
         * 
         */
        enum AspectState : uint8_t {
            ATTACHED=1, //< Whether this aspect is attached to a SimObject.
            ACTIVE=2, //< Whether the attached SimObject is active.
        };

        /**
         * @brief Value representing the readiness of this aspect.
         * 
         * @see AspectState
         * 
         */
        uint8_t mState { 0x0 };

    friend class SimObject;
    };

    /**
     * @ingroup ToyMakerSimSystem
     * @brief An object containing closely related methods and data, and exposing object lifecycle and application event loops to a developer extending it.
     * 
     * Each SimObjectAspect implementation represents some behaviour and/or data representing the object it is attached to.
     * 
     * Implementations should strive to be orthogonal to other aspects in the project.  This does not mean, however, that two aspect implementations cannot be related to or dependent on each other.  But they should, as far as possible, be made so that the addition of one does not require the removal of another on the same object.
     * 
     * ## Usage:
     * 
     * Example aspect class header:
     * 
     * ```c++
     * 
     * //////// HEADER FILE
     * 
     * // NOTE: Inherit (using CRTP) SimObjectAspect.
     * class UrLookAtBoard: public ToyMaker::SimObjectAspect<UrLookAtBoard> {
     * public:
     * 
     *     // NOTE: Basic constructor, explicitly calling the constructor of the base class.
     *     UrLookAtBoard() : SimObjectAspect<UrLookAtBoard>{0} {}
     *     
     *     // NOTE: Aspect type string of this aspect.
     *     inline static std::string getSimObjectAspectTypeName() { return "UrLookAtBoard"; }
     * 
     *     // NOTE: Explicit static constructor function for this type from its JSON description.
     *     static std::shared_ptr<ToyMaker::BaseSimObjectAspect> create(const nlohmann::json& jsonAspectProperties);
     * 
     *     // NOTE: Override for its clone method for making valid copies of this aspect.
     *     std::shared_ptr<ToyMaker::BaseSimObjectAspect> clone() const override;
     *
     * private:
     *     // NOTE: Property belonging to this aspect describing it in some way.
     *     glm::vec3 mOffset { 0.f, 1.f, 2.f };
     * 
     *     // NOTE: (optional) override(s) for object lifecycle/engine event loop hooks.
     *     void onActivated() override;
     * };
     * 
     * ```
     *
     * Example aspect constructor implementation:
     * 
     * ```c++
     * 
     * //////// SOURCE FILE
     * 
     * // NOTE: manual mapping from JSON description to object properties
     * std::shared_ptr<ToyMaker::BaseSimObjectAspect> UrLookAtBoard::create(const nlohmann::json& jsonAspectProperties) {
     *     std::shared_ptr<UrLookAtBoard> lookAtBoardAspect { std::make_shared<UrLookAtBoard>() };
     *     lookAtBoardAspect->mOffset = glm::vec3{
     *         jsonAspectProperties.at("offset")[0].get<float>(),
     *         jsonAspectProperties.at("offset")[1].get<float>(),
     *         jsonAspectProperties.at("offset")[2].get<float>(),
     *     };
     *     return lookAtBoardAspect;
     * }
     * 
     * // NOTE: Custom value copy implementation
     * std::shared_ptr<ToyMaker::BaseSimObjectAspect> UrLookAtBoard::clone() const {
     *     std::shared_ptr<UrLookAtBoard> lookAtBoardAspect { std::make_shared<UrLookAtBoard>() };
     *     lookAtBoardAspect->mOffset = mOffset;
     *     return lookAtBoardAspect;
     * }
     * 
     * ```
     * 
     * The aspect's description in JSON:
     * 
     * ```
     * 
     * { "type": "UrLookAtBoard", "offset": [0.0, 1.0, 7.0] }
     * 
     * ```
     *
     */
    template <typename TSimObjectAspectDerived>
    class SimObjectAspect: public BaseSimObjectAspect {
    protected:
        SimObjectAspect(int explicitlyInitializeMe){
            (void)explicitlyInitializeMe; // prevent unused parameter warnings
            s_registrator.emptyFunc();
        }
    private:
        /**
         * @brief Returns the aspect type string associated with TSimObjectAspectDerived.
         * 
         * @return std::string The aspect type string of the aspect.
         */
        inline std::string getAspectTypeName() const override {
            return TSimObjectAspectDerived::getSimObjectAspectTypeName();
        }

        /**
         * @brief Registers the new aspect class and its constructor with the SimSystem.
         * 
         */
        static inline void registerSelf() {
            BaseSimObjectAspect::registerAspect<TSimObjectAspectDerived>();
        }

        /**
         * @brief The static helper class instance, responsible for making sure registerSelf() is called during the static initialization phase of the program.
         * 
         */
        static Registrator<SimObjectAspect<TSimObjectAspectDerived>>& s_registrator;
    friend class Registrator<SimObjectAspect<TSimObjectAspectDerived>>;
    friend class SimObject;
    };

    template<typename TSimObjectAspectDerived>
    Registrator<SimObjectAspect<TSimObjectAspectDerived>>& SimObjectAspect<TSimObjectAspectDerived>::s_registrator{ 
        Registrator<SimObjectAspect<TSimObjectAspectDerived>>::getRegistrator()
    };

    template <typename TSimObjectAspect>
    void SimSystem::registerAspect() {
        assert((std::is_base_of<BaseSimObjectAspect, TSimObjectAspect>::value) && "Type being registered must be a subclass of BaseSimObjectAspect");
        mAspectConstructors.insert_or_assign(
            TSimObjectAspect::getSimObjectAspectTypeName(),
            &(TSimObjectAspect::create)
        );
    }

    template <typename ...TComponents>
    std::shared_ptr<SimObject> SimObject::create(const Placement& placement, const std::string& name, TComponents...components) {
        return BaseSceneNode<SimObject>::create<SimObject, TComponents...>(placement, name, components...);
    }

    template <typename ...TComponents>
    SimObject::SimObject(const Placement& placement, const std::string& name, TComponents ... components) :
    BaseSceneNode<SimObject> { placement, name, SimCore{this}, components... },
    Resource<SimObject>{0}
    {}

    template <typename TComponent>
    void BaseSimObjectAspect::addComponent(const TComponent& component) {
        mSimObject->addComponent<TComponent>(component);
    }
    template <typename TComponent>
    bool BaseSimObjectAspect::hasComponent() {
        return mSimObject->hasComponent<TComponent>();
    }
    template <typename TComponent>
    void BaseSimObjectAspect::updateComponent(const TComponent& component) {
        mSimObject->updateComponent<TComponent>(component);
    }
    template <typename TComponent>
    TComponent BaseSimObjectAspect::getComponent(const float simulationProgress) const {
        return mSimObject->getComponent<TComponent>(simulationProgress);
    }
    template <typename TComponent>
    void BaseSimObjectAspect::removeComponent() {
        return mSimObject->removeComponent<TComponent>();
    }

    template <typename TSimObjectAspect>
    bool SimObject::hasAspect() const {
        return mSimObjectAspects.find(TSimObjectAspect::getSimObjectAspectTypeName()) != mSimObjectAspects.end();
    }

    template <typename TInterface>
    bool SimObject::hasAspectWithInterface() const {
        for(auto aspect: mSimObjectAspects) {
            if(auto interfaceReference = std::dynamic_pointer_cast<TInterface>(aspect.second)){
                return true;
            }
        }
        return false;
    }

    template <typename TSimObjectAspect>
    TSimObjectAspect& SimObject::getAspect() {
        return *(static_cast<TSimObjectAspect*>(mSimObjectAspects.at(TSimObjectAspect::getSimObjectAspectTypeName()).get()));
    }

    template <typename TInterface>
    std::vector<std::reference_wrapper<TInterface>> SimObject::getAspectsWithInterface() {
        std::vector<std::reference_wrapper<TInterface>> interfaceImplementations {};
        for(auto aspect: mSimObjectAspects) {
            if(auto interfaceReference = std::dynamic_pointer_cast<TInterface>(aspect.second)) {
                interfaceImplementations.push_back(*interfaceReference);
            }
        }
        return interfaceImplementations;
    }

    template <typename TSimObjectAspect>
    void SimObject::removeAspect() {
        mSimObjectAspects.erase(TSimObjectAspect::getSimObjectAspectTypeName());
    }

    template <typename TSimObjectAspect>
    void BaseSimObjectAspect::removeAspect() {
        mSimObject->removeAspect<TSimObjectAspect>();
    }

    template <typename TSimObjectAspect>
    TSimObjectAspect& BaseSimObjectAspect::getAspect() {
        return mSimObject->getAspect<TSimObjectAspect>();
    }

    template <typename TSimObjectAspect>
    bool BaseSimObjectAspect::hasAspect() const {
        return mSimObject->hasAspect<TSimObjectAspect>();
    }

    template <>
    struct SceneNodeCore::getByPath_Helper<BaseSimObjectAspect&> {
        static BaseSimObjectAspect& get(std::shared_ptr<SceneNodeCore> rootNode, const std::string& where) {
            std::string::const_iterator div {std::find(where.begin(), where.end(), '@')};
            assert(div != where.end() && "Must contain @ to be a valid path to a scene node's aspect");

            // extract the node path from full path
            const std::string nodePath { where.substr(0, div - where.begin()) };
            const std::string aspectName { where.substr(1 + (div - where.begin())) };
            assert(rootNode->getWorld().lock()->getSystem<SimSystem>()->aspectRegistered(aspectName) && "No aspect of this type has been registered with the Sim System");

            std::shared_ptr<SimObject> node { rootNode->getByPath<std::shared_ptr<SimObject>>(nodePath) };
            return node->getAspect(aspectName);
        }

        static constexpr bool s_valid { true };
    };

    template <typename TAspect>
    struct SceneNodeCore::getByPath_Helper<TAspect&, std::enable_if_t<std::is_base_of<BaseSimObjectAspect, TAspect>::value>> {
        static TAspect& get(std::shared_ptr<SceneNodeCore> rootNode, const std::string& where) {
            return static_cast<TAspect&>(SceneNodeCore::getByPath_Helper<BaseSimObjectAspect&>::get(rootNode, where));
        }
        static constexpr bool s_valid { true };
    };

    /** @ingroup ToyMakerECSComponent ToyMakerSimSystem */
    template<>
    inline SimCore Interpolator<SimCore>::operator() (const SimCore&, const SimCore& next, float) const {
        // Never return the previous state, as that is (supposed to be)
        // an invalidated reference to this node
        return next;
    }

    template <>
    inline void SceneNodeCore::removeComponent<SimCore>() {
        assert(false && "Cannot remove a sim object's sim core component.");
    }
}

#endif

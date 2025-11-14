/**
 * @ingroup ToyMakerECS
 * @file core/ecs_world.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief ToyMaker Engine's implementation of an ECS system
 * @version 0.3.2
 * @date 2025-08-28
 * 
 *     See Austin Morlan's implementation of a simple ECS, where entities are simple indices into various component arrays, each of which are kept tightly packed: 
 *      https://austinmorlan.com/posts/entity_component_system/
 * 
 */

#ifndef FOOLSENGINE_ECSWORLD_H
#define FOOLSENGINE_ECSWORLD_H

#include <cstdint>
#include <typeinfo>
#include <iostream>
#include <tuple>
#include <memory>
#include <vector>
#include <set>
#include <bitset>
#include <unordered_map>
#include <type_traits>
#include <set>

#include <nlohmann/json.hpp>

#include "../util.hpp"
#include "../registrator.hpp"

/**
 * @defgroup ToyMakerCore Core
 * @ingroup ToyMakerEngine
 * 
 */

/**
 * @defgroup ToyMakerECS Entity Component System
 * @ingroup ToyMakerCore
 * 
 */

/**
 * @defgroup ToyMakerECSSystem Systems
 * @ingroup ToyMakerECS
 * 
 */

/**
 * @defgroup ToyMakerECSComponent Components
 * @ingroup ToyMakerECS
 * 
 */

namespace ToyMaker {



    /**
     * @ingroup ToyMakerECS
     * @brief A single unsigned integer used as a name for an entity managed by an ECS system
     * 
     * All components belonging to an entity will also have the entity's id associated with it.  
     */
    using EntityID = std::uint64_t;

    /**
     * @ingroup ToyMakerECS
     * @brief An unsigned integer representing the name of an ECS world.  
     * 
     * A single application may contain multiple "Worlds" -- a set of tables containing entities and the components that describe them.  Each world is given an ID in turn.
     */
    using WorldID = std::uint64_t;

    /**
     * @ingroup ToyMakerECS
     * @brief An ID that uniquely identifies an entity.
     * 
     * This ID is made by appending the ID of the entity to the ID of the world the entity belongs to.  Together, they uniquely identify an entity within the entire application
     * 
     */
    using UniversalEntityID = std::pair<WorldID, EntityID>;

    /**
     * @ingroup ToyMakerECS
     * @brief A number tag used to represent components and systems.
     * 
     * @warning There can be no more than 254 different systems and types.
     */
    using ECSType = std::uint8_t;

    /**
     * @ingroup ToyMakerECSComponent
     * @brief An unsigned integer representing the type of a component.
     * 
     * Each type of component is mapped to one such number, and the same number doubles as an index into the entity's signature.
     * 
     * @see Signature
     */
    using ComponentType = ECSType;

    /**
     * @ingroup ToyMakerECSSystem
     * @brief An unsigned integer representing the type of a system
     * 
     * Each type of system is mapped to one such number, and the same number doubles as an index into an entity's signature.
     * 
     */
    using SystemType = ECSType;

    /**
     * @ingroup ToyMakerECS
     * @brief A user-set constant which limits the number of creatable entities in a single ECS system
     * 
     */
    constexpr EntityID kMaxEntities { 1000000 };

    /** 
     * @ingroup ToyMakerECS
     * @brief A constant used to restrict the number of definable system and component types in a project
     */
    constexpr ECSType kMaxECSTypes { 255 };

    /**
     * @ingroup ToyMakerECSComponent
     * @brief A constant that restricts the number of definable components in a project
     * @see kMaxECSTypes
     */
    constexpr ComponentType kMaxComponents { kMaxECSTypes };

    /**
     * @ingroup ToyMakerECSSystem
     * @brief A constant that restricts the number of definable systems in a project
     * @see kMaxECSTypes
     */
    constexpr SystemType kMaxSystems { kMaxECSTypes };

    /**
     * @ingroup ToyMakerECSSystem ToyMakerECSComponent
     * @brief A 255 bit number, where each enabled bit represents a relationship between an entity and some ECS related object
     * 
     * Some of its uses are described here:
     * 
     * - as a representation of the set of all components that compose an entity
     * - as a representation of the set of all systems the entity is eligible for
     * - as a representation of the set of all systems the entity is enabled for
     * 
     * @see ComponentType
     * @see SystemType
     */
    using Signature = std::bitset<kMaxComponents>;

    class BaseSystem;
    class SystemManager;
    class ComponentManager;
    class Entity;
    class ECSWorld;

    /**
     * @ingroup ToyMakerECSComponent
     * @brief An abstract base class for all ECS component arrays.
     * 
     * A component array is a table containing a mapping from an entity to its component, whose type is defined in a subclass of BaseComponentArray
     */
    class BaseComponentArray {
    public:

        /**
         * @brief Construct a new Base Component Array object
         * 
         * @param world a pointer to the this array will belong to once constructed
         */
        explicit BaseComponentArray(std::weak_ptr<ECSWorld> world): mWorld{world} {}

        /**
         * @brief Destroy the Base Component Array object
         * 
         */
        virtual ~BaseComponentArray()=default;

        /**
         * @brief A virtual function that handles the side-effect of destroying an entity, i.e., deleting its component in this array.
         * 
         * @param entityID the ID of the entity being destroyed
         */
        virtual void handleEntityDestroyed(EntityID entityID)=0;

        /**
         * @brief An unimplemented callback for a step that occurs before each simulation step.
         * 
         * For component arrays, this is when you can expect the component values from the `next` member array to be copied over into `previous`.
         * 
         */
        virtual void handlePreSimulationStep() = 0;

        /**
         * @brief Handles the copying of a component from one entity to another within the same array
         * 
         * @param to The entity which will receive the copied component
         * @param from The entity whose component is being copied
         */
        virtual void copyComponent(EntityID to, EntityID from)=0;

        /**
         * @brief Handles the copying of a component from one entity to another, where the other entity belongs to another ECS World
         * 
         * @param to The entity which will receive the copied component
         * @param from The entity whose component is being copied
         * @param other The source component array which the entity `from` belongs to
         */
        virtual void copyComponent(EntityID to, EntityID from, BaseComponentArray& other) = 0;

        /**
         * @brief Constructs and adds a component to an array based on its json description
         * 
         * @param to The entity to which the new component will be associated
         * @param jsonComponent The component's description in json
         */
        virtual void addComponent(EntityID to, const nlohmann::json& jsonComponent)=0;

        /**
         * @brief Updates the component associated with an entity based on a json description of a new component
         * 
         * @param to The entity whose component is updated
         * @param jsonComponent The component's description in json
         */
        virtual void updateComponent(EntityID to, const nlohmann::json& jsonComponent)=0;

        /**
         * @brief Tests whether this array has an entry for this entity
         * 
         * @param entityID The entity whose component is being queried
         * @retval true The component is present;
         * @retval false The component is absent;
         */
        virtual bool hasComponent(EntityID entityID) const=0;

        /**
         * @brief Removes the component associated with this entity, if present
         * 
         * @param entityID The entity whose component is being removed
         */
        virtual void removeComponent(EntityID entityID)=0;

        /**
         * @brief Creates a fresh, empty component array and associates it with a new World
         * 
         * @param world A pointer to the world the new component array will belong to
         * @return std::shared_ptr<BaseComponentArray> A new component array
         */
        virtual std::shared_ptr<BaseComponentArray> instantiate(std::weak_ptr<ECSWorld> world) const = 0;

    protected:

        /**
         * @brief A reference to the world to which this component array belongs
         * 
         */
        std::weak_ptr<ECSWorld> mWorld {};
    };

    /**
     * @ingroup ToyMakerECSComponent ToyMakerSerialization 
     * @brief A struct that describes how a JSON component description is turned into a component
     * 
     * 
     * Specialization example:
     * ```c++
     * template <typename TComponent>
     * struct ComponentFromJSON<
     *     std::shared_ptr<TComponent>,
     *     typename std::enable_if<
     *         std::is_base_of<BaseClass, TComponent>::value
     *     >::type> {
     *     
     *     static std::shared_ptr<TComponent> get(const nlohmann::json& jsonComponent) {
     *         // your custom way for constructing/retrieving a component
     *         // based on json
     *         return component;
     *     }
     *  };
     * ```
     * 
     */
    template <typename TComponent, typename Enable=void>
    struct ComponentFromJSON {
        /**
         * @brief Get method that automatically invokes a from_json function, found by nlohmann json
         * 
         * @param jsonComponent A description of the component in JSON
         * @return TComponent The type of the component the json component will be used to construct
         */
        static TComponent get(const nlohmann::json& jsonComponent){
            // in the regular case, just invoke the from_json method that the
            // author of the component has presumably implemented
            TComponent component = jsonComponent;
            return component;
        }
    };

    /**
     * @ingroup ToyMakerECSComponent ToyMakerSerialization
     * @brief A specialization of ComponentFromJSON that applies to components that are stored as shared pointers to components (as opposed to a value of the component type itself)
     * 
     * @tparam TComponent The type of the component created and stored in a shared pointer
     * @tparam Enable A parameter that enables or disables a particular specialization using SFINAE
     * 
     * @see template<typename TComponent, typename Enable=void> struct ComponentFromJSON
     * @see https://en.cppreference.com/w/cpp/language/sfinae.html
     */
    template <typename TComponent, typename Enable>
    struct ComponentFromJSON<std::shared_ptr<TComponent>, Enable> {
        static std::shared_ptr<TComponent> get(const nlohmann::json& jsonComponent) {
            // assume once again that the author of the component has provided
            // a from_json function that will be invoked here
            std::shared_ptr<TComponent> component { new TComponent{} = jsonComponent };
            return component;
        }
    };

    /**
     * @ingroup ToyMakerECSComponent
     * @brief A template class for interpolating components between simulation frames for various purposes.
     * 
     * Each simulation tick, two versions of a component are stored by each component array. The first version is the state of the component at T-0, and the next is the state of the component at T-<simulation step>.
     * 
     * Some systems tick at a different rate from the simulation system, eg., the rendering system, and as such may require a value between these two states to function as expected.
     * 
     * By default, the interpolator uses step interpolation which works with most types.  A specialization of this class can override this behaviour for a component, and implement a better suited variant (such as linear or spherical interpolation) if needed.
     * 
     * @tparam T The type of component being interpolated
     */
    template<typename T>
    class Interpolator {
    public:
        /**
         * @brief Returns an interpolated value for a component between two given states 
         * 
         * @param previousState The old value a component had, from the previous simulation tick
         * @param nextState The new value which will take effect at the next simulation tick
         * @param simulationProgress Progress towards the next simulation tick in the current moment, where `0` is the start and `1` is the end.
         * @return T An interpolated value
         */
        T operator() (const T& previousState, const T& nextState, float simulationProgress=1.f) const;

    private:
        /**
         * @brief a functor that performs the actual interpolation in the default case
         * 
         */
        RangeMapperLinear mProgressLimits {0.f, 1.f, 0.f, 1.f};
    };

    /**
     * @ingroup ToyMakerECSComponent
     * @brief A class that implements BaseComponentArray specializing it for a component of type `TComponent`.
     * 
     * @tparam TComponent The type of component the array is being made for.
     * 
     * @see ECSWorld::registerComponentTypes()
     */
    template<typename TComponent>
    class ComponentArray : public BaseComponentArray {
    public:
        /**
         * @brief Construct a new Component Array object
         * 
         * @param world The world to which this component array will belong.
         */
        explicit ComponentArray(std::weak_ptr<ECSWorld> world): BaseComponentArray{ world } {}

    private:

        /**
         * @brief Creates a new component array of the same type as this and associated with a new World
         * 
         * @param world The world to which the newly constructed ComponentArray will belong
         * @return std::shared_ptr<BaseComponentArray> A shared pointer to the new component array
         */
        std::shared_ptr<BaseComponentArray> instantiate(std::weak_ptr<ECSWorld> world) const override;

        /**
         * @brief Adds a component belonging to this entity to this array
         * 
         * @param entityID The entity to which the component will belong
         * @param component The value of the component
         */
        void addComponent(EntityID entityID, const TComponent& component);

        /**
         * @brief Adds a component belonging to this entity to this array based on its JSON description
         * 
         * @param entityID The entity to which the component will belong
         * @param componentJSON A JSON description of the value of the component
         */
        void addComponent(EntityID entityID, const nlohmann::json& componentJSON) override;

        /**
         * @brief Removes the component associated with a specific entity, maintaining packing but not order.
         * 
         * @param entityID The entity associated with the component being removed
         */
        void removeComponent(EntityID entityID) override;

        /**
         * @brief Get the component object
         * 
         * @param entityID The entity to whom the component belongs
         * @param simulationProgress Progress towards the next simulation tick.  By default 1.0, where the latest version of the component is retrieved.
         * @return TComponent The (interpolated) component
         */
        TComponent getComponent(EntityID entityID, float simulationProgress=1.f) const;

        /**
         * @brief Tests whether an entry for a component belonging to this entity is present
         * 
         * @param entityID The entity being tested.
         * 
         * @retval true Component belonging to the entity is present;
         * 
         * @retval false Component belonging to the entity is absent;
         */
        bool hasComponent(EntityID entityID) const override;

        /**
         * @brief Updates the value of the component belonging to this entity
         * 
         * @param entityID The ID of the entity whose component is being updated
         * @param newValue The value to which the component will be updated
         */
        void updateComponent(EntityID entityID, const TComponent& newValue);

        /**
         * @brief Updates the value of the component belonging to this entity based on a JSON description of the new value
         * 
         * @param entityID The ID of the entity whose component is being updated
         * @param value The value to which the component will be updated, described in JSON
         */
        void updateComponent(EntityID entityID, const nlohmann::json& value) override;

        /**
         * @brief A callback to handle the side effect of the destruction of an entity.  Here: deletion of the component associated with it.
         * 
         * @param entityID The entity being destroyed
         */
        virtual void handleEntityDestroyed(EntityID entityID) override;

        /**
         * @brief A callback for the start of a simulation step.
         * 
         * This is where the `next` member array is copied into the `previous` member array.
         * 
         */
        virtual void handlePreSimulationStep() override;

        /**
         * @brief Handles the copying of a component belonging to one entity to another
         * 
         * @param to The entity to which the copied component goes
         * @param from The entity from which the component is copied, belonging to the same ComponentArray<T>
         */
        virtual void copyComponent(EntityID to, EntityID from) override;

        /**
         * @brief Handles the copying of a component belonging to one entity to another
         * 
         * @param to The entity to which the copied component goes
         * @param from The entity from which the component is copied
         * @param other The ComponentArray<T> which holds the component belonging to `from`
         */
        virtual void copyComponent(EntityID to, EntityID from, BaseComponentArray& other) override;

        /**
         * @brief An array containing the state of each entity's component as will be seen at the start of the next simulation tick
         * 
         */
        std::vector<TComponent> mComponentsNext {};

        /**
         * @brief An array containing the state of each entity's component as seen in the last simulation tick.
         * 
         */
        std::vector<TComponent> mComponentsPrevious {};

        /**
         * @brief A mapping from the ID of an entity to the index in the component array that stores the entity's component
         * 
         * @see mComponentToEntity
         */
        std::unordered_map<EntityID, std::size_t> mEntityToComponentIndex {};

        /**
         * @brief A mapping from the index of a component to the ID of the  entity that owns that component.
         * 
         * @see mEntityToComponentIndex
         */
        std::unordered_map<std::size_t, EntityID> mComponentToEntity {};
    friend class ComponentManager;
    };


    /**
     * @ingroup ToyMakerECSComponent
     * 
     * @brief An object that stores and manages updates to all the component arrays instantiated for this ECS World.
     * 
     */
    class ComponentManager {
    public:
        /**
         * @brief Construct a new Component Manager object.
         * 
         * @param world The world to which this component manager belongs.
         */
        explicit ComponentManager(std::weak_ptr<ECSWorld> world): mWorld { world } {};
    private:

        /**
         * @brief Constructs a new component manager and associates it with the world passed in as argument.  Creates a fresh, but empty, set of component arrays.
         * 
         * @param world  The world to which the new component manager will belong.
         * @return ComponentManager The new component manager constructed, using this component manager as a template
         */
        ComponentManager instantiate(std::weak_ptr<ECSWorld> world) const;

        /**
         * @brief A method to allow a new component type to register itself with the ECS system for this project.
         * 
         * @tparam TComponent The type of component being registered.
         */
        template<typename TComponent> 
        void registerComponentArray();

        /**
         * @brief Helper function for retrieving the component type string defined as part of the component.
         * 
         * @tparam TComponent The type of component whose type string is being retrieved.
         */
        template <typename TComponent>
        struct getComponentTypeName {
            /**
             * @brief The method that retrieves the component type string.
             * 
             * @return std::string The type string for this component.
             */
            std::string operator()() {
                return TComponent::getComponentTypeName();
            }
        };

        /**
         * @brief A specialization of getComponentTypeName for components which are wrapped in shared pointers.
         * 
         * @tparam TComponent A shared_ptr wrapped component.
         * @see template <typename TComponent> struct getComponentTypeName.
         */
        template <typename TComponent>
        struct getComponentTypeName<std::shared_ptr<TComponent>> {
            std::string operator()() {
                return TComponent::getComponentTypeName();
            }
        };

        /**
         * @brief Get the (specialized) Component Array object.
         * 
         * @tparam TComponent The type of component whose array is being retrieved.
         * @return std::shared_ptr<ComponentArray<TComponent>> (Specialized) shared pointer to said component array.
         */
        template<typename TComponent>
        std::shared_ptr<ComponentArray<TComponent>> getComponentArray() const {
            const std::size_t componentHash { typeid(TComponent).hash_code() };
            assert(mHashToComponentType.find(componentHash) != mHashToComponentType.end() && "This component type has not been registered");
            return std::dynamic_pointer_cast<ComponentArray<TComponent>>(mHashToComponentArray.at(componentHash));
        }

        /**
         * @brief Get the Component Array object.
         * 
         * @param componentTypeName The component type string for the desired component array.
         * @return std::shared_ptr<BaseComponentArray> (Unspecialized) shared pointer to said component array.
         */
        std::shared_ptr<BaseComponentArray> getComponentArray(const std::string& componentTypeName) const {
            const std::size_t componentHash { mNameToComponentHash.at(componentTypeName) };
            return mHashToComponentArray.at(componentHash);
        }

        /**
         * @brief Get the component type ID for a given component type.
         * 
         * @tparam TComponent The type of the component type ID being retrieved.
         * @return ComponentType The ID of the component type.
         */
        template<typename TComponent>
        ComponentType getComponentType() const; 

        /**
         * @brief Get the component type ID for a given component type string.
         * 
         * @param typeName the component type string for the component type.
         * @return ComponentType The ID of the component type.
         */
        ComponentType getComponentType(const std::string& typeName) const;

        /**
         * @brief Get the component signature for a given entity.
         * 
         * The component signature describes which components are present for a given object.  The position of each bit in the signature corresponds to the ComponentType that that bit represents.
         * 
         * - `1` indicates that a component of that particular type is associated with the entity.
         * 
         * - `0` indicates that there is no component of that type associated with the entity.
         * 
         * @param entityID The entity whose component signature is being retrieved.
         * @return Signature A bitset representing the presence or absence of specific components for an entity.
         * 
         * @see ComponentType
         * @see mEntityToSignature
         */
        Signature getSignature(EntityID entityID);

        /**
         * @brief Adds a component entry for this entity in the array specified.
         * 
         * @tparam TComponent The type of component being added.
         * @param entityID The entity to which the component is being added.
         * @param component The value of the added component.
         */
        template<typename TComponent>
        void addComponent(EntityID entityID, const TComponent& component);

        /**
         * @brief Adds a component entry for this entity in the array specified (in the JSON description).
         * 
         * @param entityID The entity to which the component is being added.
         * @param jsonComponent The value of the added component, described in JSON.
         */
        void addComponent(EntityID entityID, const nlohmann::json& jsonComponent);

        /**
         * @brief Removes the component of the type specified from the entity.
         * 
         * @tparam TComponent The type of component being removed.
         * @param entityID The entity whose component is being removed.
         */
        template<typename TComponent>
        void removeComponent(EntityID entityID);

        /**
         * @brief Removes the component of the type specified from the entity.
         * 
         * @param entityID The entity whose component is being removed.
         * @param type The component type string of the component being removed.
         */
        void removeComponent(EntityID entityID, const std::string& type);

        /**
         * @brief Tests whether an entity has a component of a particular type.
         * 
         * @tparam TComponent The type of component whose existence is being tested.
         * @param entityID The entity being queried.
         * @retval true The component is present;
         * @retval false The component is absent;
         * 
         * @see bool hasComponent(EntityID entityID, const std::string& type)
         */
        template<typename TComponent>
        bool hasComponent(EntityID entityID) const;

        /**
         * @brief Tests whether an entity has a component of a particular type.
         * 
         * @param entityID The entity being queried.
         * @param type The component type string of the component whose existence is being tested.
         * @retval true The component is present;
         * @retval false The component is absent;
         * 
         * @see template <typename TComponent> bool hasComponent(EntityID entityID) const
         */
        bool hasComponent(EntityID entityID, const std::string& type);

        /**
         * @brief Get the component value for this entity.
         * 
         * @tparam TComponent The type of component value to retrieve.
         * @param entityID The entity whose component is being retrieved.
         * @param simulationProgress Progress towards the next simulation tick, a value between 0.0 and 1.0.
         * @return TComponent A copy of the entity's component.
         */
        template<typename TComponent>
        TComponent getComponent(EntityID entityID, float simulationProgress=1.f) const;

        /**
         * @brief Updates a component belonging to an entity with its new value.
         * 
         * @tparam TComponent The type of component being updated.
         * @param entityID The entity whose component is being updated.
         * @param newValue The new value for the component.
         * 
         * @see void updateComponent(EntityID entityID, const nlohmann::json& componentProperties)
         */
        template<typename TComponent>
        void updateComponent(EntityID entityID, const TComponent& newValue);

        /**
         * @brief Updates a component belonging to an entity with its new value(described in JSON).
         * 
         * @param entityID The entity whose component is being updated.
         * @param componentProperties The new value for the component, described in JSON.
         * 
         * @see void updateComponent<TComponent>(EntityID entityID, const TComponent& newValue)
         */
        void updateComponent(EntityID entityID, const nlohmann::json& componentProperties);

        /**
         * @brief Copies a component value from one component to another within the same component array.
         * 
         * @tparam TComponent The type of component being copied.
         * @param to The entity to which the component is copied.
         * @param from The entity the component is copied from.
         */
        template<typename TComponent>
        void copyComponent(EntityID to, EntityID from);

        /**
         * @brief Copies all components from one entity and updates or adds them to the other.
         * 
         * @param to The entity to which the copied components will be added or updated.
         * @param from The entity from which the components will be copied.
         */
        void copyComponents(EntityID to, EntityID from);

        /**
         * @brief Copies all components from one entity and updates or adds them to the other, where the source entity may belong to a different ECS World.
         * 
         * @param to The entity to which the copied components will be added or updated.
         * @param from The entity from which components will be copied.
         * @param other The component manager belonging to the same world as `from`.
         */
        void copyComponents(EntityID to, EntityID from, ComponentManager& other);

        /**
         * @brief Handles component arrays specific side effect of entity destruction.
         * 
         * Removes entry from every component array belonging to this entity.
         * 
         * @param entityID The entity being destroyed.
         */
        void handleEntityDestroyed(EntityID entityID);

        /**
         * @brief Callback for the start of a simulation step, where the contents of every component's `next` member is copied into its `previous` member.
         * 
         * @todo rename to handleSimulationPreStep
         */
        void handlePreSimulationStep();

        /**
         * @brief Unregisters all component arrays associated with this manager, as part of the destruction process for the world this component manager belongs to.
         * 
         */
        void unregisterAll();

        /**
         * @brief Maps each component type name to its corresponding hash.
         * 
         * @see mHashToComponentArray
         */
        std::unordered_map<std::string, std::size_t> mNameToComponentHash {};

        /**
         * @brief Maps a component's type hash to its ComponentType
         * 
         * The hash of a component is provided by the STL type_traits library.  The map allows us to retrieve the ECS Type ID (used in entity component signatures) for a type with a particular hash
         * 
         * @see ComponentType
         */
        std::unordered_map<std::size_t, ComponentType> mHashToComponentType {};

        /**
         * @brief Maps the hash of a component type to its corresponding ComponentArray
         * 
         */
        std::unordered_map<std::size_t, std::shared_ptr<BaseComponentArray>> mHashToComponentArray {};

        /**
         * @brief Stores the component signature of each entity.
         * 
         * The position of each bit in the signature corresponds to a ComponentType managed by the ComponentManager.  A given entity either has or doesn't have a particular component according as the corresponding bit in the signature is set or unset.
         * 
         * @see ComponentType
         * @see Signature
         * @see getSignature
         */
        std::unordered_map<EntityID, Signature> mEntityToSignature {};

        /**
         * @brief Holds a reference to the ECS world this component manager manages component arrays for.
         * 
         */
        std::weak_ptr<ECSWorld> mWorld;

    friend class ECSWorld;
    };

    /**
     * @ingroup ToyMakerECSSystem
     * @brief The base class that acts as the interface between the engine's ECS system and a particular built-in or user-defined System
     *
     */
    class BaseSystem : public std::enable_shared_from_this<BaseSystem> {
    public:
        /**
         * @brief Construct a new Base System object
         * 
         * @param world The world the System belongs to
         */
        BaseSystem(std::weak_ptr<ECSWorld> world): mWorld { world } {}

        /**
         * @brief Destroy the Base System object
         * 
         */
        virtual ~BaseSystem() = default;

        /**
         * @brief A method to query whether a particular System is a singleton, or is instantiated for each world in the project.
         * 
         * @retval true This system is a singleton, and manages entities across worlds;
         * @retval false This system belongs to the world it is a part of, and is instantiated once for each world;
         */
        virtual bool isSingleton() const { return false; }
    protected:

        /**
         * @brief Get a set of all entities that are influenced by this System.
         * 
         * @return const std::set<EntityID>& The active set of entities.
         */
        const std::set<EntityID>& getEnabledEntities();

        /**
         * @brief The actual implementation of getComponent for a system.
         * 
         * @tparam TComponent The type of component being retrieved.
         * @tparam TSystem The system for which the component is retrieved.
         * @param entityID The entity that owns the component.
         * @param progress The progress (0-1) between the previous sim tick and the next sim tick.
         * @return TComponent The (interpolated) state of the component.
         */
        template <typename TComponent, typename TSystem>
        TComponent getComponent_(EntityID entityID, float progress=1.f) const;

        /**
         * @brief The actual implementation of updateComponent for a system.
         * 
         * @tparam TComponent The type of component being updated.
         * @tparam TSystem The system triggering the update.
         * @param entityID The entity that owns the component.
         * @param component The new value of the component after the update.
         */
        template <typename TComponent, typename TSystem>
        void updateComponent_(EntityID entityID, const TComponent& component);

        /**
         * @brief Tests whether a particular entity is active for this system.
         * 
         * @param entityID The entity being queried.
         * @retval true Indicates that the entity is influenced by this system;
         * @retval false Indicates that the entity is not influenced by this system;
         */
        bool isEnabled(EntityID entityID) const;

        /**
         * @brief Tests whether a particular entity can be influenced by this system.
         * 
         * @param entityID The entity being queried.
         * @retval true Indicates that the entity's component signature matches this system's component signature;
         * @retval false Indicates that the component signatures do not match;
         */
        bool isRegistered(EntityID entityID) const;

        /**
         * @brief Creates a fresh copy of this system and associates it with a new ECS World, using this system as its template.
         * 
         * @param world The world the new system will be associated with.
         * @return std::shared_ptr<BaseSystem> The fresh copy of this system.
         */
        virtual std::shared_ptr<BaseSystem> instantiate(std::weak_ptr<ECSWorld> world) = 0;

        /**
         * @brief A reference to the world this system belongs to.
         * 
         */
        std::weak_ptr<ECSWorld> mWorld;
    private:

        /**
         * @brief Adds an entity to this system.
         * 
         * @param entityID The entity being added.
         * @param enabled Whether the entity should be influenced by this system after being added.
         */
        void addEntity(EntityID entityID, bool enabled=true);

        /**
         * @brief Removes an entity from this system.
         * 
         * @param entityID The entity being removed.
         */
        void removeEntity(EntityID entityID);

        /**
         * @brief Allows a registered entity to be influenced by this system.
         * 
         * @param entityID The entity being enabled for this system.
         */
        void enableEntity(EntityID entityID);
        /**
         * @brief Prevents the influencing of an entity by this system.
         * 
         * @param entityID The entity being disabled for this system.
         */
        void disableEntity(EntityID entityID);

        /**
         * @brief An overridable callback for when an entity has been enabled.
         * 
         * Useful if the system has related bookkeeping of its own that it would need to update in such an event.
         * 
         * @param entityID The enabled entity.
         */
        virtual void onEntityEnabled(EntityID entityID) {(void)entityID; /* prevent unused parameter warnings*/}

        /**
         * @brief An overridable callback for when an entity has been disabled.
         * 
         * Useful if the system has related bookkeeping of its own that it would need to update in such an event.
         * 
         * @param entityID The disabled entity.
         */
        virtual void onEntityDisabled(EntityID entityID) { (void)entityID; /* prevent unused parameter warnings*/}

        /**
         * @brief An overridable callback for when another system has updated a component shared by this system and an entity.
         * 
         * @param entityID The updated entity.
         */
        virtual void onEntityUpdated(EntityID entityID) { (void)entityID; /* prevent unused parameter warnings*/assert(false && "The base class version of onEntityUpdated should never be called"); }

        /**
         * @brief An overridable callback for right after an ECS world has just been created.
         * 
         */
        virtual void onInitialize() {}

        /**
         * @brief An overridable callback for right after the ECS World has been activated.
         * 
         */
        virtual void onSimulationActivated() {}

        /**
         * @brief An overridable callback called once at the beginning of every simulation step in the game loop.
         * 
         * @param simStepMillis The time by which the simulation must be moved forward.
         */
        virtual void onSimulationPreStep(uint32_t simStepMillis) {(void)simStepMillis;/*prevent unused parameter warnings*/}

        /**
         * @brief An overridable callback called once in the middle of every simulation step.
         * 
         * The simulation step is the same regardless of time that has elapsed since the last frame.  If a lot of time has passed, multiple simulation steps are computed in this frame, and conversely if very little time has passed, no simulation step is computed this frame.
         * 
         * @param simStepMillis The time by which the simulation must be moved forward.
         */
        virtual void onSimulationStep(uint32_t simStepMillis) {(void)simStepMillis;/*prevent unused parameter warnings*/}

        /**
         * @brief An overridable callback called once at the end of this simulation step, and after related transform updates are applied.
         * 
         * @param simStepMillis The time by which the simulation must be moved forward.
         */
        virtual void onSimulationPostStep(uint32_t simStepMillis) {(void)simStepMillis;/*prevent unused parameter warnings*/}

        /**
         * @brief An overridable callback called after all the transforms in the scene are updated by the scene system.
         * 
         * @param timeStepMillis The time by which the simulation must be moved forward.
         */
        virtual void onPostTransformUpdate(uint32_t timeStepMillis) {(void)timeStepMillis;/*prevent unused parameter warnings*/}

        /**
         * @brief Overridable callback called after all simulation updates (if any) for the current frame have been completed.
         * 
         * May be called multiple times between two simulation frames, or once every couple of simulation frames, depending on the cability of the current computer and operations being performed on it simultaneously.
         * 
         * @param simulationProgress The progress towards the next simulation step as a value between 0.0 and 1.0,  calculated using `time since last simulation step` / (`time of next simulation step` - `time of last simulation step`)
         * 
         * @param variableStepMillis The real time that has passed since the computation of the last frame (NOT the last simulation step).
         */
        virtual void onVariableStep(float simulationProgress, uint32_t variableStepMillis) {(void)simulationProgress; (void)variableStepMillis;/*prevent unused parameter warnings*/}

        /**
         * @brief Overridable callback called just before the render step takes place.
         * 
         * @param simulationProgress The progress towards the next simulation step as a value between 0.0 and 1.0, calculated using `time since last simulation step` / (`time of next simulation step` - `time of last simulation step`)
         */
        virtual void onPreRenderStep(float simulationProgress) {(void)simulationProgress;/*prevent unused parameter warnings*/}

        /**
         * @brief Overridable callback called just after the render step takes place.
         * 
         * @param simulationProgress The progress towards the next simulation step as a value between 0.0 and 1.0, calculated using `time since last simulation step` / (`time of next simulation step` - `time of last simulation step`)
         */
        virtual void onPostRenderStep(float simulationProgress) {(void)simulationProgress;/*prevent unused parameter warnings*/}

        /**
         * @brief Overridable callback called just after the ECS world owning this system has been deactivated.
         * 
         * Required if this system has some special bookkeeping it needs to perform before it is suspended.
         * 
         */
        virtual void onSimulationDeactivated() {}

        /**
         * @brief Overridable callback called just before this system is destroyed.
         * 
         * Useful if this system has some special bookkeeping it needs to perform before it is retired.
         * 
         */
        virtual void onDestroyed()  {}

        /**
         * @brief A set of all entities that are actively influenced by this system, managed by this system's ECS world.
         * 
         */
        std::set<EntityID> mEnabledEntities {};

        /**
         * @brief A set of all entities that are compatible with this system, but have not been enabled for it.
         * 
         */
        std::set<EntityID> mDisabledEntities {};

    friend class SystemManager;
    friend class ECSWorld;
    };

    /**
     * @ingroup ToyMakerECSSystem
     * @brief A system template that disables systems with this form of declaration.
     * 
     * @tparam TSystemDerived The class that is derived from this one, as in CRTP
     * @tparam TListenedForComponentsTuple A tuple of components that this system will listen for updates on.
     * @tparam TRequiredComponentsTuple A tuple of components that an entity requires in order to be eligible for this system.
     */
    template <typename TSystemDerived, typename TListenedForComponentsTuple, typename TRequiredComponentsTuple>
    class System{ static_assert(false && "Non specialized system cannot be declared"); };

    /**
     * @ingroup ToyMakerECSSystem
     * @brief The base class for any built-in or user-defined system that would like to be hooked to the engine's event cycle or have access to entities and components.
     * 
     * @tparam TSystemDerived The derived class to this base class, as in CRTP.
     * @tparam TListenedForComponents A tuple of components that this system will listen for updates on for its active entities.
     * @tparam TRequiredComponents A tuple of components that indicate what an entity must have in order to be eligible for this system.
     * 
     * @see ECSWorld::registerSystem()
     */
    template <typename TSystemDerived, typename ...TListenedForComponents, typename ...TRequiredComponents>
    class System<TSystemDerived, std::tuple<TListenedForComponents...>, std::tuple<TRequiredComponents...>>: public BaseSystem {

        /**
         * @brief Helper function called during static initialization to make this project's ECS system aware that this class is available.
         * 
         */
        static void registerSelf();

    protected:

        /**
         * @brief Construct a new System object
         * 
         * @warning The call to this class' registrator's empty func ensures (I think) that the registrator gets defined (and initialized), and isn't optimized out.
         * 
         * @param world The world this system will belong to, once created.
         */
        explicit System(std::weak_ptr<ECSWorld> world): BaseSystem { world } { s_registrator.emptyFunc(); }

        /**
         * @brief Gets the component belonging to the entity using this System's specialized version of getComponent.
         * 
         * @tparam TComponent The type of component being retrieved.
         * @param entityID The entity which owns the desired component.
         * @param progress A number between 0 and 1 representing the progress from the previous simulation step to the next one.
         * @return TComponent The retrieved component.
         */
        template<typename TComponent>
        TComponent getComponent(EntityID entityID, float progress=1.f) {
            assert(!isSingleton() && "Singletons cannot retrieve components by EntityID alone");
            return BaseSystem::getComponent_<TComponent, TSystemDerived>(entityID, progress);
        }

        /**
         * @brief Updates a component via this system's specialized version of updateComponent.
         * 
         * @tparam TComponent The type of component being updated.
         * @param entityID The entity whose component is being updated.
         * @param component The new component value.
         */
        template<typename TComponent>
        void updateComponent(EntityID entityID, const TComponent& component) {
            assert(!isSingleton() && "Singletons cannot retrieve components by EntityID alone");
            BaseSystem::updateComponent_<TComponent, TSystemDerived>(entityID, component);
        }

        /**
         * @brief Instantiates a fresh System using this one as a template, and associates it with a new world.
         * 
         * @param world The world the copied System will be associated with.
         * @return std::shared_ptr<BaseSystem> The new system.
         */
        std::shared_ptr<BaseSystem> instantiate(std::weak_ptr<ECSWorld> world) override;
    private:

        /**
         * @brief A specialization of Registrator<T> which ensures registerSelf() is called during this project's static initialization phase.
         * 
         */
        inline static Registrator<System<TSystemDerived, std::tuple<TListenedForComponents...>, std::tuple<TRequiredComponents...>>>& s_registrator {
            Registrator<System<TSystemDerived, std::tuple<TListenedForComponents...>, std::tuple<TRequiredComponents...>>>::getRegistrator()
        };

    friend class Registrator<System<TSystemDerived, std::tuple<TListenedForComponents...>, std::tuple<TRequiredComponents...>>>;
    };

    /**
     * @ingroup ToyMakerECSSystem
     * @brief Holds references to all the systems belonging to this manager's ECSWorld.
     * 
     * The system manager is responsible for keeping track of and passing messages to the various systems in this world.  It also maintains tables to track entity membership and activation for all the systems it manages.
     * 
     */
    class SystemManager {
    public:
        /**
         * @brief Construct a new System Manager object
         * 
         * @param world The world the system manager belongs to.
         */
        explicit SystemManager(std::weak_ptr<ECSWorld> world): mWorld{ world } {}
    private: 
        /**
         * @brief Creates a new system manager containing fresh copies of the same systems this manager uses, and associates it with the new world.
         * 
         * @param world The world to which the instantiated manager will be related.
         * @return SystemManager The new manager templated off the current one.
         */
        SystemManager instantiate(std::weak_ptr<ECSWorld> world) const;

        /**
         * @brief During static initialization, adds the new System type and its component signatures to its tables.
         * 
         * This allows the System to receive callbacks from this manager. It also allows the manager to determine if and when a particular system should receive component-related updates.
         * 
         * @tparam TSystem The system being registered against the system manager.
         * @param signature The component signature of the system, indicating which components an entity is required to have to be eligible for a particular system.
         * @param listenedForComponents The components this System would like to listen for updates for, when they occur on an entity enabled for it.
         */
        template<typename TSystem>
        void registerSystem(const Signature& signature, const Signature& listenedForComponents);

        /**
         * @brief Removes all references to managed systems, in preparation for destruction.
         * 
         */
        void unregisterAll();

        /**
         * @brief Gets an instance of a system belonging to this ECS World.
         * 
         * @tparam TSystem The type of system whose instance is being retrieved.
         * @return std::shared_ptr<TSystem> The retrieved System reference.
         */
        template<typename TSystem>
        std::shared_ptr<TSystem> getSystem();

        /**
         * @brief Enables the visibility of an entity to a particular system.
         * 
         * @tparam TSystem The system for which the entity is being enabled.
         * @param entityID The entity being enabled. 
         */
        template<typename TSystem>
        void enableEntity(EntityID entityID);

        /**
         * @brief Enables an entity for all eligible systems, per its component signature and the system mask.
         * 
         * The position of each bit on the entitySignature corresponds to the type of a system.  1 indicates that this system ought to be enabled for this entity if possible, while 0 indicates no change.
         * 
         * @param entityID The entity being enabled for multiple system.
         * @param entitySignature The component signature for this entity, which must match that of the system.
         * @param systemMask The systems for which this enabling should be performed.
         */
        void enableEntity(EntityID entityID, Signature entitySignature, Signature systemMask = Signature{}.set());

        /**
         * @brief Disables an entity for a particular system.
         * 
         * @tparam TSystem The system for which this entity will be disabled.
         * @param entityID The entity being disabled.
         */
        template<typename TSystem>
        void disableEntity(EntityID entityID);

        /**
         * @brief Disables all systems which match this entity's signature.
         * 
         * @param entityID The entity being disabled.
         * @param entitySignature The component signature of this entity.
         */
        void disableEntity(EntityID entityID, Signature entitySignature);

        /**
         * @brief Get the SystemType for a system.
         * 
         * The SystemType of a System corresponds to a position on the bitset of various System Masks, and its meaning varies according to context.
         * 
         * @tparam TSystem The System for which the SystemType id is being retrieved.
         * @return SystemType The SystemType id for a system.
         */
        template<typename TSystem>
        SystemType getSystemType() const;

        /**
         * @brief Tests whether an entity is enabled for a particular system.
         * 
         * @tparam TSystem The system being queried.
         * @param entityID The entity whose activation is being tested.
         * @retval true Indicates the entity is active;
         * @retval false Indicates the entity is inactive;
         */
        template<typename TSystem>
        bool isEnabled(EntityID entityID);

        /**
         * @brief Tests whether an entity is eligible for participation in a particular system.
         * 
         * @tparam TSystem The system being queried.
         * @param entityID The entity whose compatibility is being tested.
         * @retval true Indicates the entity is eligible;
         * @retval false Indicates the entity is ineligible;
         */
        template <typename TSystem>
        bool isRegistered(EntityID entityID);

        /**
         * @brief A callback for when components are added or removed from a particular entity, causing a corresponding change in its component signature, and addition or removal from various systems.
         * 
         * @param entityID The entity whose component signature has changed.
         * @param signature The entity's new component signature.
         */
        void handleEntitySignatureChanged(EntityID entityID, Signature signature);

        /**
         * @brief A callback for the destruction of an entity, passed on to systems interested in it.
         * 
         * @param entityID The entity being destroyed.
         */
        void handleEntityDestroyed(EntityID entityID);

        /**
         * @brief Handles an update to an entity by running a callback on all interested systems.
         * 
         * @param entityID The entity whose component was updated.
         * @param signature The component signature of the entity.
         * @param updatedComponent The ComponentType of the updated component.
         */
        void handleEntityUpdated(EntityID entityID, Signature signature, ComponentType updatedComponent);

        /**
         * @brief Handles an update to an entity by running a callback on all interested systems (except the one that triggered the update)
         * 
         * @tparam TSystem The system which caused the component update.
         * 
         * @param entityID The entity whose component was updated.
         * @param signature The component signature of the entity.
         * @param updatedComponent The ComponentType of the updated component.
         */
        template<typename TSystem>
        void handleEntityUpdatedBySystem(EntityID entityID, Signature signature, ComponentType updatedComponent);

        /**
         * @brief Runs the initialization callback of all systems that implement it.
         * 
         * @see BaseSystem::onInitialize()
         */
        void handleInitialize();

        /**
         * @brief Runs the activation callback of all systems that implement it.
         * 
         * @see BaseSystem::onSimulationActivated()
         */
        void handleSimulationActivated();

        /**
         * @brief Runs the sim prestep callback of all systems that implement it.
         * 
         * @param simStepMillis Time by which the simulation will advance this update, in milliseconds.
         * 
         * @see BaseSystem::onSimulationPreStep()
         */
        void handleSimulationPreStep(uint32_t simStepMillis);

        /**
         * @brief Runs the sim step callback for all systems that implement it.
         * 
         * @param simStepMillis Time by which the simulation will advance this frame, in milliseconds.
         * 
         * @see BaseSystem::onSimulationStep()
         */
        void handleSimulationStep(uint32_t simStepMillis);

        /**
         * @brief Runs the post simulation step callback for all systems that implement it.
         * 
         * @param simStepMillis Time by which the simulation will advance this frame, in milliseconds.
         * 
         * @see BaseSystem::onSimulationPostStep()
         */
        void handleSimulationPostStep(uint32_t simStepMillis);

        /**
         * @brief Runs the update callback that occurs after new transforms have been computed.
         * 
         * @param timeStepMillis Simulation time since the end of the last transform update.
         * 
         * @see BaseSystem::onPostTransformUpdate()
         */
        void handlePostTransformUpdate(uint32_t timeStepMillis);

        /**
         * @brief Runs the System callback that occurs once every frame, after any simulation steps computed that frame.
         * 
         * @param simulationProgress Progress since the last simulation step towards the next simulation step, a number between 0.0 and 1.0.
         * 
         * @param variableStepMillis Time (in milliseconds) since the last variable step
         * 
         * @see BaseSystem::onVariableStep()
         */
        void handleVariableStep(float simulationProgress, uint32_t variableStepMillis);

        /**
         * @brief Runs the System callback that occurs once before the scene in this ECSWorld is rendered. 
         * 
         * @param simulationProgress Progress since the last simulation step towards the next simulation step, a number between 0.0 and 1.0.
         * 
         * @see BaseSystem::onPreRenderStep()
         */
        void handlePreRenderStep(float simulationProgress);

        /**
         * @brief Runs the System callback after the scene in this ECSWorld has been rendered.
         * 
         * @param simulationProgress Progress since the last simulation step, a number between 0.0 and 1.0.
         */
        void handlePostRenderStep(float simulationProgress);

        /**
         * @brief Runs the System callback for deactivation, for any systems that have implemented it.
         * 
         */
        void handleSimulationDeactivated();

        /**
         * @brief Mapping from the system type name for a system to its component signature.
         * 
         * Only entities with Signatures that match a System's signature may be influenced by it.
         * 
         */
        std::unordered_map<std::string, Signature> mNameToSignature {};

        /**
         * @brief Mapping from the system type name for a system to its component listening signature.
         * 
         * When an entity enabled for a system has one of its components updated, and that component is marked in the systems listening signature, the corresponding system will be notified.
         * 
         */
        std::unordered_map<std::string, Signature> mNameToListenedForComponents {};

        /**
         * @brief Maps system type names to their corresponding SystemType values.
         * 
         * Each position on an entity's SystemMask (as provided in enableEntity()) corresponds to a value of SystemType, and indicates whether the entity is to be enabled or left alone for that System.
         */
        std::unordered_map<std::string, SystemType> mNameToSystemType {};

        /**
         * @brief Maps system type names to references to the actual in-memory Systems they represent.
         * 
         */
        std::unordered_map<std::string, std::shared_ptr<BaseSystem>> mNameToSystem {};

        /**
         * @brief The world this System manager belongs to.
         * 
         */
        std::weak_ptr<ECSWorld> mWorld;

    friend class ECSWorld;
    friend class BaseSystem;
    };

    /**
     * @ingroup ToyMakerECS
     * @brief A class that represents a set of systems, entities, and components, that are all interrelated, but isolated from other similar sets.
     * 
     * A single such set is called an ECSWorld.  Each ECSWorld is comprised of a list of entities, a system manager, and a component manager of its own.  Entities, systems, and components belonging to one world are not visible to systems in other worlds, or to any other ECSWorld.
     * 
     * @note Why would one want such a thing?  
     * 
     * @note A game has game elements (e.g., player avatar, enemies, obstacles, etc.,), and UI elements (health bar, options button, inventory, etc.,)
     * 
     * @note Both game and UI elements have visual components (meshes, textures, lights), interactive elements (collision volumes), placement components (position, rotation, scale), and more.
     * 
     * @note One expects that when the player moves, the game scene moves, but the UI scene stays in place.  One also expects the collision system of the UI to be independent of the game, and vice versa.  Certain systems, such as the physics system, should apply to game elements and not to UI elements.  The movement of a UI element should not influence game elements, and vice versa.
     * 
     * @note Self-contained, isolated ECS worlds are one convenient way to accomplish this separation.  The results of one simulation may still be displayed in the other (eg., screenshots on save files, or in-game UIs, or graphics settings) while keeping most of their operation independent.
     * 
     * @note It is then up to the developer of the project to specify when and where one world should "influence," or be exposed to, the other.
     * 
     */
    class ECSWorld: public std::enable_shared_from_this<ECSWorld> {
    public:
        /**
         * @brief Get the prototype ECSWorld
         * 
         * The prototype ECSWorld is the first ECSWorld created in a project.  All systems and components that register themselves to this module will be registered with this Prototype ECSWorld during the static initialization of the program.
         * 
         * The prototype is then used as a blueprint for instantiating more ECSWorlds, and populating them with the necessary component arrays and systems.
         * 
         * The prototype ECSWorld is also where a newly loaded scene node's components are stored, before it is made a member of the ECSWorld where it actually belongs.
         * 
         * @return std::weak_ptr<const ECSWorld> A constant reference to the prototype ECSWorld.
         */
        static std::weak_ptr<const ECSWorld> getPrototype();

        /**
         * @brief Creates a new ECSWorld using the systems and component arrays present in this one as a template.
         * 
         * @return std::shared_ptr<ECSWorld> The new ECSWorld
         */
        std::shared_ptr<ECSWorld> instantiate() const;

        /**
         * @brief Registers ComponentArrays for each type of component present in the component type list, if required.
         * 
         * @tparam TComponent The component type(s) being registered.
         * 
         * ## Usage:
         * 
         * ```c++
         * 
         * struct CameraProperties {
         *     // ...
         * 
         *     // NOTE: Static method returning the name of this component.
         *     static std::string getComponentTypeName() { return "CameraProperties"; }
         * 
         *     // ...
         * };
         * 
         * // ...
         * 
         * // NOTE: methods for converting this component to and from JSON, so that the component can be used/defined in a scene file.
         * inline void from_json(const nlohmann::json& json, CameraProperties& cameraProperties) {
         *     assert(json.at("type").get<std::string>() == CameraProperties::getComponentTypeName() && "Type mismatch, json must be of camera properties type");
         *     json.at("projection_mode").get_to(cameraProperties.mProjectionType);
         *     json.at("fov").get_to(cameraProperties.mFov);
         *     json.at("aspect").get_to(cameraProperties.mAspect);
         *     json.at("orthographic_dimensions")
         *         .at("horizontal")
         *         .get_to(cameraProperties.mOrthographicDimensions.x);
         *     json.at("orthographic_dimensions")
         *         .at("vertical")
         *         .get_to(cameraProperties.mOrthographicDimensions.y);
         *     json.at("near_far_planes").at("near").get_to(cameraProperties.mNearFarPlanes.x);
         *     json.at("near_far_planes").at("far").get_to(cameraProperties.mNearFarPlanes.y);
         * }
         * inline void to_json(nlohmann::json& json, const CameraProperties& cameraProperties) {
         *     json = {
         *         {"type", CameraProperties::getComponentTypeName()},
         *         {"projection_mode", cameraProperties.mProjectionType},
         *         {"fov", cameraProperties.mFov},
         *         {"aspect", cameraProperties.mAspect},
         *         {"orthographic_dimensions", {
         *             {"horizontal", cameraProperties.mOrthographicDimensions.x},
         *             {"vertical", cameraProperties.mOrthographicDimensions.y},
         *         }},
         *         {"near_far_planes", {
         *             {"near", cameraProperties.mNearFarPlanes.x},
         *             {"far", cameraProperties.mNearFarPlanes.y},
         *         }}
         *     };
         * }
         * 
         * ```
         * 
         * @see ComponentArray<TComponent>
         * 
         */
        template<typename ...TComponent>
        static void registerComponentTypes();

        /**
         * @brief Prevents the use of the unspecialized version of SystemRegistrationArgs.
         * 
         * @tparam TSystemDerived The new system which will derive System<TSystemDerived> as in CRTP.
         * @tparam TListenedForComponents A tuple of component types which establish what component updates a system is interested in.
         * @tparam TRequiredComponents A tuple of component types that an entity is required to have in order to be eligible for membership with a particular system.
         */
        template <typename TSystemDerived, typename TListenedForComponents, typename TRequiredComponents>
        struct SystemRegistrationArgs { static_assert(false && "Cannot create unspecialized instance of SystemRegistrationArgs"); };

        /**
         * @brief The partially specialized version of SystemRegistrationArgs, with its latter 2 template parameters being tuples of components.
         * 
         * @tparam TSystemDerived The new system which will derive System<TSystemDerived> as in CRTP.
         * 
         * @tparam TListenedForComponents A tuple of component types which establish what component updates a system is interested in.
         * 
         * @tparam TRequiredComponents A tuple of component types that an entity is required to have in order to be eligible for membership with a particular system.
         * 
         */
        template <typename TSystemDerived, typename ...TListenedForComponents, typename ...TRequiredComponents>
        struct SystemRegistrationArgs<TSystemDerived, std::tuple<TListenedForComponents...>, std::tuple<TRequiredComponents...>> {};


        /**
         * @brief Registers a System with the ECS System for this project.
         * 
         * This registration occurs at static initialization against the prototype ECSWorld.  All other ECSWorlds for a project are derived from the prototype.
         * 
         * Such a system has access to the project's ECS callbacks, and will be notified when any update to one of the entities it manages occurs.
         * 
         * ## Usage:
         * 
         * ```c++
         * 
         * // NOTE: TSystem inherits from System<TSystem, std::tuple<TListenedForComponent_1, TListenedForComponent_2, ...>, std::tuple<TOtherRequiredComponent_1, TOtherRequiredComponent_2, ...>>
         * class CameraSystem: public System<CameraSystem, std::tuple<Transform, CameraProperties>, std::tuple<>> {
         * public:
         *     // NOTE: implements static std::string getSystemTypeName()
         *     static std::string getSystemTypeName() { return "CameraSystem"; }
         *     
         *     // ...
         * 
         * private:
         * 
         *     // ...
         * 
         *     // NOTE: (optional) may override base class virtual functions
         *     void onEntityEnabled(EntityID entityID) override;
         *     void onEntityDisabled(EntityID entityID) override;
         *     void onEntityUpdated(EntityID entityID) override;
         *     void onSimulationActivated() override;
         *     void onPreRenderStep(float simulationProgress) override;
         * 
         *     // ...
         * 
         * };
         * 
         * ```
         * 
         * @tparam TSystemDerived The new System being registered.
         * 
         * @tparam TListenedForComponents A tuple of component types, indicating which component updates the system wants callbacks for.
         * 
         * @tparam TRequiredComponents A tuple of component types, indicating what components an entity needs to be eligible for management by this system.
         * 
         * @see template <typename TSystemDerived, typename ...TListenedForComponents, typename ...TRequiredComponents> class System<TSystemDerived, std::tuple<TListenedForComponents...>, std::tuple<TRequiredComponents...>>
         * 
         */
        template <typename TSystemDerived, typename ...TListenedForComponents, typename ...TRequiredComponents>
        static void registerSystem(SystemRegistrationArgs<TSystemDerived, std::tuple<TListenedForComponents...>, std::tuple<TRequiredComponents...>>);

        /**
         * @brief Get the system object of a specific type belonging to this ECSWorld.
         * 
         * @tparam TSystem The type of system being retrieved.
         * @return std::shared_ptr<TSystem> A reference to this ECSWorld's TSystem.
         */
        template<typename TSystem>
        std::shared_ptr<TSystem> getSystem();

        /**
         * @brief Get the system object of a specific type belonging to the prototype ECSWorld.
         * 
         * @tparam TSystem The type of system being retrieved.
         * @return std::shared_ptr<TSystem> A reference to the prototype ECSWorld's TSystem.
         * 
         * @see getPrototype()
         */
        template<typename TSystem>
        static std::shared_ptr<TSystem> getSystemPrototype();

        /**
         * @brief Get a system that has been marked as a singleton System.
         * 
         * A singleton System is a system that has no real entities or components of its own, and does not technically belong to any ECSWorld.  How it tracks active entities is up to the System's implementation.
         * 
         * Such a system can be assumed to preside over eligible entities across several world. The SceneSystem is one such system.
         * 
         * @tparam TSingletonSystem The type of the singleton System being retrieved.
         * @return std::shared_ptr<TSingletonSystem> A reference to the retrieved System.
         */
        template <typename TSingletonSystem>
        static std::shared_ptr<TSingletonSystem> getSingletonSystem();

        /**
         * @brief Get the SystemType id for a particular system.
         * 
         * The SystemType of a system corresponds to a position in the bitset of an entity's system mask, or other system related signatures.  Its meaning varies depending on the usage of the bitset.
         * 
         * For the system mask, as seen in a call to SystemManager::enableEntity(), it indicates which systems the manager is expected to activate the entity for.
         * 
         * @tparam TSystem The system whose SystemType id is being retrieved.
         * @return SystemType The system type id retrieved.
         */
        template <typename TSystem>
        SystemType getSystemType();

        /**
         * @brief Tests whether a particular entity is enabled for a particular system.
         * 
         * @tparam TSystem The system being queried.
         * @param entityID The entity whose activation is being tested.
         * @retval true Indicates that the entity is enabled for a system;
         * @retval false Indicates that the entity is disabled for a system;
         */
        template <typename TSystem>
        bool isEnabled(EntityID entityID);

        /**
         * @brief Tests whether an entity is eligible for membership with a system, per its component signature.
         * 
         * @tparam TSystem The system being queried.
         * @param entityID The entity whose eligibility is being tested.
         * @retval true Indicates the entity is eligible;
         * @retval false Indicates the entity is ineligible.
         */
        template <typename TSystem>
        bool isRegistered(EntityID entityID);

        /**
         * @brief Creates a new entity that will exist in this ECSWorld.
         * 
         * This mainly means that a new EntityID is created and stored from a list of previously retired IDs or a completely new EntityID.  In the same step, and if desired, new components belonging to the entity can be initialized and stored as entries in their respective component arrays.
         * 
         * @tparam TComponents Components to initialize and store right after the entity is created.
         * @param components The initial values of the components being stored.
         * @return Entity A handle to the entity, through which its system membership and its components can be accessed.
         */
        template<typename ...TComponents>
        Entity createEntity(TComponents...components);

        /**
         * @brief Create a prototype entity object.
         * 
         * This is an entity whose components and system membership are stored in the prototype ECSWorld, until it is moved to its actual ECSWorld.
         * 
         * @tparam TComponents Components to initialize and store right after the entity is created.
         * @param components The initial values of the components being stored.
         * @return Entity A handle to the entity, through which its system membership and its components can be accessed.
         * 
         * @see createEntity()
         */
        template <typename ...TComponents>
        static Entity createEntityPrototype(TComponents...components);

        // Simulation lifecycle events

        /**
         * @brief Runs the initialization step for this world and the systems that belong to it.
         * 
         */
        void initialize();

        /**
         * @brief Marks this world as an active one, calling the activation callbacks of systems that belong to it.
         * 
         */
        void activateSimulation();

        /**
         * @brief Marks ths world as an inactive one, and suspends operation for its member systems.
         * 
         */
        void deactivateSimulation();

        // Simulation loop events

        /**
         * @brief Runs callbacks associated with the start of a simulation update.
         * 
         * 
         * @param simStepMillis The time in milliseconds that the simulation will be advanced by.
         * 
         * @see SystemManager::handleSimulationPreStep()
         * @see ComponentManager::handlePreSimulationStep()
         */
        void simulationPreStep(uint32_t simStepMillis);

        /**
         * @brief Runs callbacks associated with the simulation step.
         * 
         * @param simStepMillis The time in milliseconds that the simulation will be advanced by.
         * 
         * @see SystemManager::handleSimulationStep()
         */
        void simulationStep(uint32_t simStepMillis);

        /**
         * @brief Runs calbacks associated with the end of the simulation step.
         * 
         * @param simStepMillis The time in milliseconds that the simulation will be advanced by.
         * 
         * @see SystemManager::handleSimulationPostStep()
         */
        void simulationPostStep(uint32_t simStepMillis);

        /**
         * @brief Runs callbacks associated with the end of transform updates.
         * 
         * @param timeStepMillis The time, in milliseconds, since the end of the last transform update.
         * 
         * @see SystemManager::handlePostTransformUpdate()
         */
        void postTransformUpdate(uint32_t timeStepMillis);

        /**
         * @brief Runs callbacks associated with the variable step of the game loop.
         * 
         * The variable step is run once each frame.  It may be run multiple times between simulation steps, or once every handful of simulation steps, according to the capabilities of its platform, and be affected by any other ongoing operations taking place on that platform.
         * 
         * @param simulationProgress Progress, from 0.0 to 1.0, since the end of the last simulation step and the beginning of the next one.
         * @param variableStepMillis The time, in milliseconds, since the last frame.
         * 
         * @see SystemManager::handleVariableStep()
         */
        void variableStep(float simulationProgress, uint32_t variableStepMillis);

        /**
         * @brief Runs callbacks associated with the start of the rendering step.
         * 
         * @param simulationProgress The progress towards the next simulation step since the previous one, represented as a number between 0 and 1.
         * 
         * @see SystemManager::handlePreRenderStep()
         */
        void preRenderStep(float simulationProgress);

        /**
         * @brief Runs callbacks associated with the end of the rendering step.
         * 
         * @param simulationProgress The progress towards the next simulation step since the previous one, represented as a number between 0 and 1.
         * 
         * @see SystemManager::handlePostRenderStep()
         */
        void postRenderStep(float simulationProgress);

        /**
         * @brief Loses references to member systems, component arrays, and entities, resulting in their destruction.
         * 
         */
        void cleanup();

        /**
         * @brief Gets the ID associated with this world.
         * 
         * @note a prototype world has an ID of 0.
         * 
         * @return WorldID The ID of this world.
         */
        inline WorldID getID() const { return mID; }

    private:

        /**
         * @brief Creates an ECSWorld
         * 
         * @return std::shared_ptr<ECSWorld> The new ECS World.
         */
        static std::shared_ptr<ECSWorld> createWorld();

        /**
         * @brief Creates an instance of the prototype ECSWorld, and returns a reference to it.
         * 
         * @return std::weak_ptr<ECSWorld> A reference to the prototype ECSWorld.
         */
        static std::weak_ptr<ECSWorld> getInstance();

        /**
         * @brief Construct a new ECSWorld object
         * 
         */
        ECSWorld() = default;

        /**
         * @brief Copies components from one entity to another within a single ECSWorld.
         * 
         * @param to The entity which will receive the copied components.
         * @param from The entity from whose components will be copied.
         */
        void copyComponents(EntityID to, EntityID from);

        /**
         * @brief Copies components from one entity to another, where the source entity may be from another ECSWorld.
         * 
         * @param to The destination entity, belonging to this world.
         * @param from The source entity whose components will be copied.
         * @param other The world that owns the source entity.
         */
        void copyComponents(EntityID to, EntityID from, ECSWorld& other);

        /**
         * @brief Moves an entity from one ECSWorld to this one.
         * 
         * @param entity The handle for an entity, presumably from a different ECSWorld.
         */
        void relocateEntity(Entity& entity);

        /**
         * @brief Creates a new entity and assigns it the components specified as arguments.
         * 
         * @tparam TComponents The types of components to initialize with the entity.
         * @param components The initial values of the components of the entity.
         * @return Entity A handle to the new entity.
         */
        template<typename ...TComponents>
        Entity privateCreateEntity(TComponents...components);

        /**
         * @brief Destroys an Entity with a specific ID.
         * 
         * The ID is moved into a list of available IDs which can be reused by a new entity.  All entries relating to the entity in owning Systems and ComponentArrays are removed.
         * 
         * @param entityID The Entity being destroyed.
         */
        void destroyEntity(EntityID entityID);

        /**
         * @brief Enables an entity for a specific system.
         * 
         * @tparam TSystem The system the entity will be influenced by post-activation.
         * @param entityID The entity being enabled.
         * 
         * @see SystemManager::enableEntity()
         */
        template<typename TSystem>
        void enableEntity(EntityID entityID);

        /**
         * @brief Enables an entity for multiple systems, restricted by its component signature and its system mask.
         * 
         * @param entityID The entity being activated.
         * @param systemMask Bitset representing which systems the entity is allowed to be activated for.
         * 
         * @see SystemManager::enableEntity()
         */
        void enableEntity(EntityID entityID, Signature systemMask = Signature{}.set());

        /**
         * @brief Disables an entity on a particular system.
         * 
         * @tparam TSystem The system for which the entity is being deactivated.
         * @param entityID The entity being deactivated.
         * 
         * @see SystemManager::disableEntity()
         */
        template<typename TSystem>
        void disableEntity(EntityID entityID);

        /**
         * @brief Disables an entity for all systems.
         * 
         * @param entityID The entity being deactivated.
         * @see SystemManager::disableEntity()
         */
        void disableEntity(EntityID entityID);

        /**
         * @brief Adds a new component to the entity.
         * 
         * @tparam TComponent The type of component being added.
         * @param entityID The entity to which the component is being added.
         * @param component The initial value of the component.
         * 
         * @see ComponentManager::addComponent()
         */
        template<typename TComponent>
        void addComponent(EntityID entityID, const TComponent& component);

        /**
         * @brief Adds a new component to an entity based on its description in JSON.
         * 
         * @param entityID The entity to which the component is being added.
         * @param jsonComponent The initial value of the component, described in JSON.
         * 
         * @see ComponentManager::addComponent()
         */
        void addComponent(EntityID entityID, const nlohmann::json& jsonComponent);


        /**
         * @brief Get a component associated with an entity.
         * 
         * @tparam TComponent The type of component being retrieved.
         * @param entityID The entity whose component is being retrieved.
         * @param simulationProgress The progress since the last simulation step towards the next simulation step, represented as a number between 0 and 1.
         * @return TComponent The value of the component, interpolated according to simulationProgress
         * 
         * @see ComponentManager::getComponent()
         */
        template<typename TComponent>
        TComponent getComponent(EntityID entityID, float simulationProgress=1.f) const;

        /**
         * @brief Get a component associated with an entity, specialized for Systems.
         * 
         * @tparam TComponent The type of component being retrieved.
         * @tparam TSystem The system requesting the component.
         * @param entityID The entity whose component is being retrieved.
         * @param simulationProgress The progress since the last simulation step towards the next simulation step, represented as a number between 0 and 1.
         * @return TComponent The value of the component, interpolated according to simulationProgress.
         * @see ComponentManager::getComponent()
         */
        template<typename TComponent, typename TSystem>
        TComponent getComponent(EntityID entityID, float simulationProgress=1.f) const;

        /**
         * @brief Tests whether an entity has a component of a specific type.
         * 
         * @tparam TComponent The type of component being tested.
         * @param entityID The entity being queried.
         * @retval true Indicates the entity has such a component;
         * @retval false Indicates the entity doesn't have such a component;
         * 
         * @see ComponentManager::hasComponent()
         */
        template <typename TComponent>
        bool hasComponent(EntityID entityID) const;

        /**
         * @brief Tests whether an entity has a component of a specific type.
         * 
         * @param entityID The entity being queried.
         * @param typeName the component type string of the component being tested.
         * 
         * @retval true Indicates the entity has such a component;
         * @retval false Indicates the entity doesn't have such a component;
         * 
         * @see ComponentManager::updateComponent()
         */
        bool hasComponent(EntityID entityID, const std::string& typeName) const;

        /**
         * @brief Updates a component belonging to an entity with a new value.
         * 
         * @tparam TComponent The type of component receiving an update.
         * @param entityID The entity whose component is being updated.
         * @param newValue The new value the component will have post update.
         * 
         * @see ComponentManager::updateComponent()
         */
        template<typename TComponent>
        void updateComponent(EntityID entityID, const TComponent& newValue);

        /**
         * @brief Updates a component belonging to an entity with a new value, based on its JSON description.
         * 
         * @param entityID The entity being updated.
         * @param newValue The type and new value of the component, described in JSON.
         * 
         * @see ComponentManager::updateComponent()
         */
        void updateComponent(EntityID entityID, const nlohmann::json& newValue);

        /**
         * @brief Updates a component belonging to an entity with a new value, noting the System causing the update.
         * 
         * @tparam TComponent The component being updated.
         * @tparam TSystem The system causing the update.
         * @param entityID The entity being updated.
         * @param newValue The new value for the component.
         * 
         * @see ComponentManager::updateComponent()
         */
        template<typename TComponent, typename TSystem>
        void updateComponent(EntityID entityID, const TComponent& newValue);

        /**
         * @brief Updates a component belonging to an entity with a new value, noting the system causing the update.
         * 
         * @tparam TSystem The system causing the update.
         * @param entityID The entity being updated.
         * @param newValue The new value for the component.
         * 
         * @see ComponentManager::updateComponent()
         */
        template<typename TSystem>
        void updateComponent(EntityID entityID, const nlohmann::json& newValue);

        /**
         * @brief Removes a component from an entity.
         * 
         * @tparam TComponent The type of component being removed.
         * @param entityID The entity being updated.
         * 
         * @see ComponentManager::removeComponent()
         */
        template<typename TComponent>
        void removeComponent(EntityID entityID);

        /**
         * @brief Removes a component from an entity based on its component type string.
         * 
         * @param entityID The entity being updated.
         * @param typeName The component type string of the component being removed.
         */
        void removeComponent(EntityID entityID, const std::string& typeName);

        /**
         * @brief Removes all components associated with an entity.
         * 
         * @param entityID The entity whose components are being removed.
         */
        void removeComponentsAll(EntityID entityID);

        /**
         * @brief A reference to this world's ComponentManager
         * 
         */
        std::unique_ptr<ComponentManager> mComponentManager {nullptr};

        /**
         * @brief A reference to this world SystemManager
         * 
         */
        std::unique_ptr<SystemManager> mSystemManager {nullptr};

        /**
         * @brief A list of previously existing entity IDs that were since retired and are available to be used again.
         * 
         */
        std::vector<EntityID> mDeletedIDs {};

        /**
         * @brief A new EntityID that has never been created before in this world.
         * 
         * @see mDeletedIDs
         * 
         */
        EntityID mNextEntity {};

        /**
         * @brief The unique ID associated with this ECSWorld.
         * 
         */
        WorldID mID {0};

        /**
         * @brief The ID the next ECSWorld to be instantiated will receive.
         * 
         */
        static WorldID s_nextWorld;

    friend class Entity;
    friend class BaseSystem;
    };

    /**
     * @ingroup ToyMakerECS
     * 
     * @brief The Entity is a wrapper on an entity ID, used as the primary interface between an application and the engine's ECS system.
     * 
     * Holds an entity ID, and has several methods for querying and changing components, participation in different systems belonging to its ECSWorld.
     * 
     */
    class Entity {
    public:
        /**
         * @brief Construct a new Entity object.
         * 
         * @param other The entity whose components will be copied to create this one.
         */
        Entity(const Entity& other);
        /**
         * @brief Construct a new Entity object
         * 
         * @param other Soon to be destroyed entity whose resources this object will now own.
         */
        Entity(Entity&& other) noexcept;

        /**
         * @brief Copies an Entity object, replacing any component values currently present on this one.
         * 
         * @param other The entity whose components will be copied.
         * @return Entity& A reference to this entity after its components have been updated.
         */
        Entity& operator=(const Entity& other);

        /**
         * @brief Moves other's resources into this object, while destroying previously held resources associated with this.
         * 
         * @param other Soon to be destroyed Entity.
         * @return Entity& A reference to this object, post-move.
         */
        Entity& operator=(Entity&& other) noexcept;

        /**
         * @brief Destroy the Entity object
         * 
         */
        ~Entity();

        /**
         * @brief Gets the ID associated with this Entity.
         * 
         * @return EntityID The ID of this Entity.
         */
        inline EntityID getID() { return mID; };

        /**
         * @brief Copies component values from another entity into this one.
         * 
         * @param other The Entity being copied from
         * 
         * @see ECSWorld::copyComponents()
         */
        void copy(const Entity& other);

        /**
         * @brief Adds a new component to this Entity.
         * 
         * @tparam TComponent The type of component being added.
         * @param component The initial value of the component when added.
         * 
         * @see ECSWorld::addComponent()
         */
        template<typename TComponent> 
        void addComponent(const TComponent& component);

        /**
         * @brief Adds a new component to this Entity based on its JSON description.
         * 
         * @param jsonComponent A description of the new component, including its type and initial value.
         * 
         * @see ECSWorld::addComponent()
         */
        void addComponent(const nlohmann::json& jsonComponent);

        /**
         * @brief Removes a component from an entity.
         * 
         * @tparam TComponent The type of component being retrieved.
         * 
         * @see ECSWorld::removeComponent()
         */
        template<typename TComponent> 
        void removeComponent();

        /**
         * @brief Removes a component from an entity.
         * 
         * @param typeName The component type string of the component being removed.
         * 
         * @see ECSWorld::removeComponent()
         */
        void removeComponent(const std::string& typeName);


        /**
         * @brief Tests whether this entity has a particular component.
         * 
         * @tparam TComponent The type of component whose existence is being tested.
         * @retval true The component is present;
         * @retval false The component is absent;
         * 
         * @see ECSWorld::hasComponent()
         */
        template<typename TComponent>
        bool hasComponent() const;

        /**
         * @brief Tests whether this entity has a particular component.
         * 
         * @param typeName The component type string of the component being tested.
         * @retval true The component is present;
         * @retval false The component is absent;
         * 
         * @see ECSWorld::hasComponent()
         */
        bool hasComponent(const std::string& typeName) const;

        /**
         * @brief Get the value of the component at a specific time this frame.
         * 
         * @tparam TComponent The type of component being retrieved.
         * @param simulationProgress Progress towards the next simulation step from the last one, a value between 0 and 1.
         * @return TComponent The component value retrieved.
         * 
         * @see ECSWorld::getComponent()
         */
        template<typename TComponent> 
        TComponent getComponent(float simulationProgress=1.f) const;

        /**
         * @brief Updates the value of a component belonging to this Entity.
         * 
         * @tparam TComponent The type of component being updated.
         * @param newValue The new value of the component.
         * 
         * @see ECSWorld::updateComponent()
         */
        template<typename TComponent>
        void updateComponent(const TComponent& newValue);

        /**
         * @brief Updates the value of a component belonging to this Entity.
         * 
         * @param jsonValue A json description of the value, which also includes its type.
         * 
         * @see ECSWorld::updateComponent()
         */
        void updateComponent(const nlohmann::json& jsonValue);

        /**
         * @brief Tests whether this entity is enabled for a particular system.
         * 
         * @tparam TSystem The System being queried.
         * @retval true This entity is active on the system;
         * @retval false This entity is not active on the system;
         * 
         * @see ECSWorld::isEnabled()
         */
        template<typename TSystem>
        bool isEnabled() const;

        /**
         * @brief Tests whether this entity is eligible for participation with a given system.
         * 
         * @tparam TSystem The System being queried.
         * @retval true This entity fulfills participation prerequisites;
         * @retval false This entity fails participation prerequisites;
         * 
         * @see ECSWorld::isRegistered()
         */
        template <typename TSystem>
        bool isRegistered() const;

        /**
         * @brief Enables this Entity for a particular System
         * 
         * @tparam TSystem The System this entity is being enabled for.
         * 
         * @see ECSWorld::enableEntity()
         */
        template<typename TSystem>
        void enableSystem();

        /**
         * @brief Disables this entity for a particular system.
         * 
         * @tparam TSystem The System this entity is being disabled on.
         * 
         * @see ECSWorld::disableEntity()
         */
        template<typename TSystem>
        void disableSystem();

        /**
         * @brief Disables this entity's participation in all systems.
         * 
         * @see ECSWorld::disableEntity()
         */
        void disableSystems();

        /**
         * @brief Enables this entity's participation in a set of systems.
         * 
         * @param systemMask A bitset, each position of which corresponds to a SystemType for which activation is to be attempted.
         */
        void enableSystems(Signature systemMask);

        /**
         * @brief Get the ECSWorld object this Entity belongs to.
         * 
         * @return std::weak_ptr<ECSWorld> A reference to the ECSWorld this entity belongs to.
         */
        inline std::weak_ptr<ECSWorld> getWorld() { return mWorld; }

        /**
         * @brief Removes this Entity from its current ECS world, and moves it to the new one.
         * 
         * @param world The ECSWorld this Entity will belong to.
         */
        void joinWorld(ECSWorld& world);

    private:
        /**
         * @brief Construct a new Entity object, with a new ID as a member of a new ECSWorld.
         * 
         * @param entityID The ID of the new Entity.
         * @param world The ECSWorld the Entity belongs to.
         */
        Entity(EntityID entityID, std::shared_ptr<ECSWorld> world): mID{ entityID }, mWorld{ world } {};

        /**
         * @brief The ID of this entity within its owning ECSWorld.
         * 
         */
        EntityID mID;

        /**
         * @brief The world this Entity belongs to.
         * 
         */
        std::weak_ptr<ECSWorld> mWorld;
    friend class ECSWorld;
    };


    /**
     * @ingroup ToyMakerECSComponent 
     * 
     * @brief Step interpolation implementation, compatible with most types.
     * 
     */
    template<typename T>
    T Interpolator<T>::operator() (const T& previousState, const T& nextState, float simulationProgress) const {
        if(simulationProgress < .5f) return previousState;
        return nextState;
    }

    template<typename TComponent>
    void ComponentArray<TComponent>::addComponent(EntityID entityID, const TComponent& component) {
        assert(mEntityToComponentIndex.find(entityID) == mEntityToComponentIndex.end() && "Component already added for this entity");

        std::size_t newComponentID { mComponentsNext.size() };

        mComponentsNext.push_back(component);
        mComponentsPrevious.push_back(component);
        mEntityToComponentIndex[entityID] = newComponentID;
        mComponentToEntity[newComponentID] = entityID;
    }

    template <typename TComponent>
    void ComponentArray<TComponent>::addComponent(EntityID entityID, const nlohmann::json& jsonComponent) {
        addComponent(entityID, ComponentFromJSON<TComponent>::get(jsonComponent));
    }

    template <typename TComponent>
    void ComponentArray<TComponent>::updateComponent(EntityID entityID, const nlohmann::json& jsonComponent) {
        updateComponent(entityID, ComponentFromJSON<TComponent>::get(jsonComponent));
    }

    template<typename TComponent>
    bool ComponentArray<TComponent>::hasComponent(EntityID entityID) const {
        return mEntityToComponentIndex.find(entityID) != mEntityToComponentIndex.end();
    }

    template<typename TComponent>
    void ComponentArray<TComponent>::removeComponent(EntityID entityID) {
        assert(mEntityToComponentIndex.find(entityID) != mEntityToComponentIndex.end());

        const std::size_t removedComponentIndex { mEntityToComponentIndex[entityID] };
        const std::size_t lastComponentIndex { mComponentsNext.size() - 1 };
        const std::size_t lastComponentEntity { mComponentToEntity[lastComponentIndex] };

        // store last component in the removed components place
        mComponentsNext[removedComponentIndex] = mComponentsNext[lastComponentIndex];
        mComponentsPrevious[removedComponentIndex] = mComponentsPrevious[lastComponentIndex];
        // map the last component's entity to its new index
        mEntityToComponentIndex[lastComponentEntity] = removedComponentIndex;
        // map the removed component's index to the last entity
        mComponentToEntity[removedComponentIndex] = lastComponentEntity;

        // erase all traces of the removed entity's component and other
        // invalid references
        mComponentsNext.pop_back();
        mComponentsPrevious.pop_back();
        mEntityToComponentIndex.erase(entityID);
        mComponentToEntity.erase(lastComponentIndex);
    }

    template <typename TComponent>
    TComponent ComponentArray<TComponent>::getComponent(EntityID entityID, float simulationProgress) const {
        static Interpolator<TComponent> interpolator{};
        assert(mEntityToComponentIndex.find(entityID) != mEntityToComponentIndex.end());
        std::size_t componentID { mEntityToComponentIndex.at(entityID) };
        return interpolator(mComponentsPrevious[componentID], mComponentsNext[componentID], simulationProgress);
    }

    template <typename TComponent>
    void ComponentArray<TComponent>::updateComponent(EntityID entityID, const TComponent& newComponent) {
        assert(mEntityToComponentIndex.find(entityID) != mEntityToComponentIndex.end());
        std::size_t componentID { mEntityToComponentIndex.at(entityID) };
        mComponentsNext[componentID] = newComponent;
    }

    template <typename TComponent>
    void ComponentArray<TComponent>::handleEntityDestroyed(EntityID entityID) {
        if(mEntityToComponentIndex.find(entityID) != mEntityToComponentIndex.end()) {
            removeComponent(entityID);
        }
    }

    template<typename TComponent>
    void ComponentArray<TComponent>::handlePreSimulationStep() {
        std::copy<typename std::vector<TComponent>::iterator, typename std::vector<TComponent>::iterator>(
            mComponentsNext.begin(), mComponentsNext.end(), 
            mComponentsPrevious.begin()
        );
    }

    template <typename TComponent>
    void ComponentArray<TComponent>::copyComponent(EntityID to, EntityID from) {
        copyComponent(to, from, *this);
    }

    template <typename TComponent>
    void ComponentArray<TComponent>::copyComponent(EntityID to, EntityID from, BaseComponentArray& other) {
        assert(to < kMaxEntities && "Cannot copy to an entity with an invalid entity ID");
        ComponentArray<TComponent>& downcastOther { static_cast<ComponentArray<TComponent>&>(other) };
        if(downcastOther.mEntityToComponentIndex.find(from) == downcastOther.mEntityToComponentIndex.end()) return;

        const TComponent& componentValueNext { downcastOther.mComponentsNext[downcastOther.mEntityToComponentIndex[from]] };
        const TComponent& componentValuePrevious { downcastOther.mComponentsPrevious[downcastOther.mEntityToComponentIndex[from]] };

        if(mEntityToComponentIndex.find(to) == mEntityToComponentIndex.end()) {
            addComponent(to, componentValueNext);
        } else {
            mComponentsNext[mEntityToComponentIndex[to]] = componentValueNext;
        }
        mComponentsPrevious[mEntityToComponentIndex[to]] = componentValuePrevious;
    }

    template<typename TComponent> 
    void ComponentManager::registerComponentArray() {
        const std::size_t componentHash { typeid(TComponent).hash_code() };
        // nop when a component array for this type already exists
        if(mHashToComponentType.find(componentHash) != mHashToComponentType.end()) {
            return;
        }

        std::string componentTypeName { getComponentTypeName<TComponent>{}() };
        assert(mHashToComponentType.size() + 1 < kMaxComponents && "Component type limit reached");
        assert(mNameToComponentHash.find(componentTypeName) == mNameToComponentHash.end() && "Another component with this name\
            has already been registered");

        mNameToComponentHash.insert_or_assign(componentTypeName, componentHash);
        mHashToComponentArray.insert_or_assign(
            componentHash, std::static_pointer_cast<BaseComponentArray>(std::make_shared<ComponentArray<TComponent>>(mWorld))
        );
        mHashToComponentType[componentHash] = mHashToComponentType.size();
    }

    template<typename TComponent>
    ComponentType ComponentManager::getComponentType() const {
        const std::size_t componentHash { typeid(TComponent).hash_code() };
        assert(mHashToComponentType.find(componentHash) != mHashToComponentType.end() && "Component type has not been registered");
        return mHashToComponentType.at(componentHash);
    }

    template<typename TComponent>
    void ComponentManager::addComponent(EntityID entityID, const TComponent& component) {
        getComponentArray<TComponent>()->addComponent(entityID, component);
        mEntityToSignature[entityID].set(getComponentType<TComponent>(), true);
    }

    template <typename TComponent>
    bool ComponentManager::hasComponent(EntityID entityID) const {
        return getComponentArray<TComponent>()->hasComponent(entityID);
    }

    template<typename TComponent>
    void ComponentManager::removeComponent(EntityID entityID) {
        getComponentArray<TComponent>()->removeComponent(entityID);
        mEntityToSignature[entityID].set(getComponentType<TComponent>(), false);
    }

    template<typename TComponent>
    TComponent ComponentManager::getComponent(EntityID entityID, float simulationProgress) const {
        return getComponentArray<TComponent>()->getComponent(entityID, simulationProgress);
    }

    template<typename TComponent>
    void ComponentManager::updateComponent(EntityID entityID, const TComponent& newValue) {
        getComponentArray<TComponent>()->updateComponent(entityID, newValue);
    }

    template <typename TComponent>
    void ComponentManager::copyComponent(EntityID to, EntityID from) {
        assert(mEntityToSignature[from].test(getComponentType<TComponent>()) && "The entity being copied from does not have this component");
        getComponentArray<TComponent>()->copyComponent(to, from);
        mEntityToSignature[to].set(getComponentType<TComponent>(), true);
    }

    template<typename TSystem>
    SystemType SystemManager::getSystemType() const {
        const std::string systemTypeName{ TSystem::getSystemTypeName() };
        assert(mNameToSystemType.find(systemTypeName) != mNameToSystemType.end() && "Component type has not been registered");
        return mNameToSystemType.at(systemTypeName);
    }

    template <typename TSystem>
    bool ECSWorld::isEnabled(EntityID entityID) {
        return mSystemManager->isEnabled<TSystem>(entityID);
    }
    template <typename TSystem>
    bool ECSWorld::isRegistered(EntityID entityID) {
        return mSystemManager->isRegistered<TSystem>(entityID);
    }

    template <typename TComponent>
    bool ECSWorld::hasComponent(EntityID entityID) const {
        return mComponentManager->hasComponent<TComponent>(entityID);
    }


    template <typename TSystemDerived, typename ...TListenedForComponents, typename ...TRequiredComponents>
    void System<TSystemDerived, std::tuple<TListenedForComponents...>, std::tuple<TRequiredComponents...>>::registerSelf() {
        ECSWorld::registerComponentTypes<TRequiredComponents...>();
        ECSWorld::registerComponentTypes<TListenedForComponents...>();
        ECSWorld::registerSystem(ECSWorld::SystemRegistrationArgs<TSystemDerived, std::tuple<TListenedForComponents...>, std::tuple<TRequiredComponents...>>{});
    }


    template<typename TSystem>
    void SystemManager::registerSystem(const Signature& signature, const Signature& listenedForComponents) {
        const std::string systemTypeName { TSystem::getSystemTypeName() };
        assert(mNameToSignature.find(systemTypeName) == mNameToSignature.end() && "System has already been registered");
        assert(mNameToSystemType.size() + 1 < kMaxSystems && "System type limit reached");

        mNameToSignature[systemTypeName] = signature;
        mNameToListenedForComponents[systemTypeName] = listenedForComponents;
        mNameToSystem.insert_or_assign(systemTypeName, std::make_shared<TSystem>(mWorld));
        mNameToSystemType[systemTypeName] = mNameToSystemType.size();
    }

    template<typename TSystem>
    std::shared_ptr<TSystem> SystemManager::getSystem() {
        std::string systemTypeName { TSystem::getSystemTypeName() };
        assert(mNameToSignature.find(systemTypeName) != mNameToSignature.end() && "System has not yet been registered");
        return std::dynamic_pointer_cast<TSystem>(mNameToSystem[systemTypeName]);
    }

    template<typename TSystem>
    void SystemManager::enableEntity(EntityID entityID) {
        std::string systemTypeName { TSystem::getSystemTypeName() };
        mNameToSystem[systemTypeName]->enableEntity(entityID);
    }

    template<typename TSystem>
    void SystemManager::disableEntity(EntityID entityID) {
        std::string systemTypeName { TSystem::getSystemTypeName() };
        mNameToSystem[systemTypeName]->disableEntity(entityID);
    }

    template <typename TComponent>
    void Entity::addComponent(const TComponent& component) {
        mWorld.lock()->addComponent<TComponent>(mID, component);
    }

    template <typename TComponent>
    bool Entity::hasComponent() const {
        return mWorld.lock()->hasComponent<TComponent>(mID);
    }

    template<typename TComponent>
    TComponent Entity::getComponent(float simulationProgress) const {
        return mWorld.lock()->getComponent<TComponent>(mID, simulationProgress);
    }

    template<typename TComponent>
    void Entity::updateComponent(const TComponent& newValue) {
        mWorld.lock()->updateComponent<TComponent>(mID, newValue);
    }

    template <typename TComponent>
    void Entity::removeComponent() {
        mWorld.lock()->removeComponent<TComponent>(mID);
    }

    template <typename TSystem>
    void Entity::enableSystem() {
        mWorld.lock()->enableEntity<TSystem>(mID);
    }

    template <typename TSystem>
    bool Entity::isEnabled() const {
        return mWorld.lock()->isEnabled<TSystem>(mID);
    }

    template <typename TSystem>
    bool Entity::isRegistered() const {
        return mWorld.lock()->isRegistered<TSystem>(mID);
    }

    template <typename TSystem>
    void Entity::disableSystem() {
        mWorld.lock()->disableEntity<TSystem>(mID);
    }

    template <typename TSystem>
    bool SystemManager::isEnabled(EntityID entityID) {
        const std::string systemTypeName { TSystem::getSystemTypeName() };
        return mNameToSystem[systemTypeName]->isEnabled(entityID);
    }

    template <typename TSystem>
    bool SystemManager::isRegistered(EntityID entityID) {
        const std::string systemTypeName { TSystem::getSystemTypeName() };
        return mNameToSystem[systemTypeName]->isRegistered(entityID);
    }

    template<typename ...TComponents>
    Entity ECSWorld::privateCreateEntity(TComponents...components) {
        assert((mNextEntity < kMaxEntities || !mDeletedIDs.empty()) && "Max number of entities reached");

        EntityID nextID;
        if(!mDeletedIDs.empty()){
            nextID = mDeletedIDs.back();
            mDeletedIDs.pop_back();
        } else {
            nextID = mNextEntity++;
        }

        Entity entity { nextID, shared_from_this()};

        (addComponent<TComponents>(nextID, components), ...);
        return entity;
    }

    template<typename TSystem>
    SystemType ECSWorld::getSystemType() {
        return mSystemManager->getSystemType<TSystem>();
    }

    template<typename TComponent>
    void ECSWorld::addComponent(EntityID entityID, const TComponent& component) {
        assert(entityID < kMaxEntities && "Cannot add a component to an entity that does not exist");
        mComponentManager->addComponent<TComponent>(entityID, component);
        Signature signature { mComponentManager->getSignature(entityID) };
        mSystemManager->handleEntitySignatureChanged(entityID, signature);
    }

    template<typename TComponent>
    void ECSWorld::removeComponent(EntityID entityID) {
        mComponentManager->removeComponent<TComponent>(entityID);
        Signature signature { mComponentManager->getSignature(entityID) };
        mSystemManager->handleEntitySignatureChanged(entityID, signature);
    }

    template<typename TSystem>
    void ECSWorld::enableEntity(EntityID entityID) {
        mSystemManager->enableEntity<TSystem>(entityID);
    }

    template<typename TSystem>
    void ECSWorld::disableEntity(EntityID entityID) {
        mSystemManager->disableEntity<TSystem>(entityID);
    }

    template<typename TComponent>
    TComponent ECSWorld::getComponent(EntityID entityID, float progress) const {
        return mComponentManager->getComponent<TComponent>(entityID, progress);
    }

    template<typename TComponent, typename TSystem>
    TComponent ECSWorld::getComponent(EntityID entityID, float progress) const {
        assert(
            (
                mSystemManager->mNameToSignature.at(TSystem::getSystemTypeName())
                    .test(mComponentManager->getComponentType<TComponent>())
            )
            && "This system cannot access this kind of component"
        );
        return getComponent<TComponent>(entityID, progress);
    }


    template<typename TComponent>
    void ECSWorld::updateComponent(EntityID entityID, const TComponent& newValue) {
        mComponentManager->updateComponent<TComponent>(entityID, newValue);
        mSystemManager->handleEntityUpdated(
            entityID,
            mComponentManager->getSignature(entityID),
            mComponentManager->getComponentType<TComponent>()
        );
    }

    template<typename TComponent, typename TSystem>
    void ECSWorld::updateComponent(EntityID entityID, const TComponent& newValue) {
        assert(
            (
                mSystemManager->mNameToSignature.at(TSystem::getSystemTypeName())
                    .test(mComponentManager->getComponentType<TComponent>())
            )
            && "This system cannot access this kind of component"
        );
        mComponentManager->updateComponent<TComponent>(entityID, newValue);
        mSystemManager->handleEntityUpdatedBySystem<TSystem>(
            entityID,
            mComponentManager->getSignature(entityID),
            mComponentManager->getComponentType<TComponent>()
        );
    }

    template <typename TSystem>
    void ECSWorld::updateComponent(EntityID entityID, const nlohmann::json& newValue) {
        assert(
            (
                mSystemManager->mNameToSignature.at(TSystem::getSystemTypeName())
                    .test(mComponentManager->getComponentType(newValue.at("type")))
            )
            && "This system cannot access this kind of component"
        );
        mComponentManager->updateComponent(entityID, newValue);
        mSystemManager->handleEntityUpdatedBySystem<TSystem>(
            entityID,
            mComponentManager->getSignature(entityID),
            mComponentManager->getComponentType(newValue.at("type"))
        );
    }

    template<typename ...TComponents>
    void ECSWorld::registerComponentTypes() {
        ((getInstance().lock()->mComponentManager->registerComponentArray<TComponents>()),...);
    }

    template<typename TSystem, typename ...TListenedForComponents, typename ...TRequiredComponents>
    void ECSWorld::registerSystem(
        ECSWorld::SystemRegistrationArgs<TSystem, std::tuple<TListenedForComponents...>, std::tuple<TRequiredComponents...>>
    ) {
        Signature listensFor {};
        Signature required {};

        (listensFor.set(getInstance().lock()->mComponentManager->getComponentType<TListenedForComponents>()), ...);
        (required.set(getInstance().lock()->mComponentManager->getComponentType<TRequiredComponents>()), ...);

        getInstance().lock()->mSystemManager->registerSystem<TSystem>(required|listensFor, listensFor);
    }

    template<typename ...TComponents>
    Entity ECSWorld::createEntity(TComponents...components) {
        return privateCreateEntity<TComponents...>(components...);
    }

    template <typename ...TComponents>
    Entity ECSWorld::createEntityPrototype(TComponents...components) {
        return ECSWorld::getInstance().lock()->privateCreateEntity<TComponents...>(components...);
    }

    template<typename TSystem>
    std::shared_ptr<TSystem> ECSWorld::getSystem() {
        return mSystemManager->getSystem<TSystem>();
    }

    template<typename TSystem>
    std::shared_ptr<TSystem> ECSWorld::getSystemPrototype() {
        return getInstance().lock()->mSystemManager->getSystem<TSystem>();
    }

    template <typename TSingletonSystem>
    std::shared_ptr<TSingletonSystem> ECSWorld::getSingletonSystem() {
        std::shared_ptr<TSingletonSystem> system { getInstance().lock()->getSystem<TSingletonSystem>() };
        assert(system->isSingleton() && "System specified is not an ECSWorld-aware singleton system");
        return system;
    }

    template<typename TComponent, typename TSystem>
    TComponent BaseSystem::getComponent_(EntityID entityID, float progress) const {
        assert(!isSingleton() && "Singletons cannot retrieve entity components through entity ID alone");
        return mWorld.lock()->getComponent<TComponent, TSystem>(entityID, progress);
    }
    template<typename TSystem, typename ...TListenedForComponents, typename ...TRequiredComponents>
    std::shared_ptr<BaseSystem> System<TSystem, std::tuple<TListenedForComponents...>, std::tuple<TRequiredComponents...>>::instantiate(std::weak_ptr<ECSWorld> world) {
        if(isSingleton()) return shared_from_this();
        return std::make_shared<TSystem>(world);
    }

    template <typename TComponent>
    std::shared_ptr<BaseComponentArray> ComponentArray<TComponent>::instantiate(std::weak_ptr<ECSWorld> world) const {
        return std::make_shared<ComponentArray<TComponent>>(world);
    }

    template<typename TComponent, typename TSystem>
    void BaseSystem::updateComponent_(EntityID entityID, const TComponent& component) {
        assert(!isSingleton() && "Singletons cannot retrieve entity components through entity ID alone");
        mWorld.lock()->updateComponent<TComponent, TSystem>(entityID, component);
    }

    template<typename TSystem>
    void SystemManager::handleEntityUpdatedBySystem(EntityID entityID, Signature signature, ComponentType updatedComponent) {
        std::string originatingSystemTypeName { TSystem::getSystemTypeName() };
        for(const auto& pair: mNameToSignature) {
            // see if the updated entity's signature matches that of the system
            if((pair.second&signature) != pair.second) continue;

            // suppress update callback from the system that caused this update
            if(pair.first == originatingSystemTypeName) continue;

            // see if the system is listening for updates to this system
            if(!mNameToListenedForComponents[pair.first].test(updatedComponent)) continue;

            // ignore disabled and singleton systems
            BaseSystem& system { *(mNameToSystem[pair.first]).get() };
            if(system.isSingleton() || !system.isEnabled(entityID)) continue;

            // apply update
            system.onEntityUpdated(entityID);
        }
    }

}
#endif

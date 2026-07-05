/**
 * @ingroup ToyMakerPhysics
 * @file physics/system.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief File containing ToyMaker's implementation of XPBD, extended position based dynamics
 * @version 0.2.3
 * @date 2026-05-23
 *
 */

/**
 * @defgroup ToyMakerPhysics Physics properties, maths, contraints, etc.
 * @ingroup ToyMakerEngine
 *
 */

#ifndef TOYMAKERENGINE_PHYSICSSYSTEM_H
#define TOYMAKERENGINE_PHYSICSSYSTEM_H

#include <unordered_set>

#include <nlohmann/json.hpp>

#include "../core/ecs_world.hpp"
#include "../signals.hpp"
#include "../spatial_query/types.hpp"

#include "types.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToyMakerPhysics
     *
     * @brief Names two distinct entities participating in a collision, with their entity
     * IDs sorted in ascending order.
     *
     * Built to be used in an std::set
     *
     */
    class CollisionPair {
    private:
        /**
         * @brief The first entity in a constraint link.
         *
         */
        EntityID mFirst;

        /**
         * @brief The second entity in a constraint link.
         *
         */
        EntityID mSecond;

    public:
        /**
         * @brief Creates a constraint link out of two distinct entities.
         *
         */
        CollisionPair(EntityID first, EntityID second): mFirst { first }, mSecond { second } {
            assert(first != second && "Entities in constraint must be distinct");
            if (second < first) {
                std::swap(mFirst, mSecond);
            }
        }

        /**
         * @brief Returns the first entity in the link
         *
         */
        inline EntityID first() const { return mFirst; }

        /**
         * @brief Returns the second entity in the link
         *
         */
        inline EntityID second() const { return mSecond; }

        inline bool operator < (const CollisionPair& other) const {
            return (
                mFirst < other.mFirst
                || (
                    mFirst == other.mFirst
                    && mSecond < other.mSecond
               )
            );
        }
    };

    /**
     * @ingroup ToyMakerPhysics ToyMakerECSSystem
     * @brief The physics system, an ECS system that tracks and updates the state of each physics
     * object in the scene according to its properties.
     *
     * Relies strongly on the spatial query system to provide information about object overlaps
     * and collisions.  Collision events are generated when an object with a certain interaction mask
     * overlaps with objects belonging to one of the layers indicated by that mask.
     *
     * The physics system implemented here is based on XPBD (Extended Position Based Dynamics).  See paper
     * [Detailed Rigid Body Simulation using Extended Position Based Dynamics](https://matthias-research.github.io/pages/publications/PBDBodies.pdf).
     *
     *
     * @see ToyMakerSpatialQuerySystem
     *
     */
    class PhysicsSystem: public System<PhysicsSystem, std::tuple<PhysicsState, ObjectBounds>, std::tuple<>>, public SignalTracker {
    public:
        /**
         * @brief ID naming a unique constraint registered with the physics system.
         *
         * @see registerConstraint()
         * @see unregisterConstraint()
         */
        using ConstraintID = std::size_t;

        /**
         * @brief Struct containing data about a collision event.
         *
         */
        using SignalCollidedData = std::pair<CollisionPair, Collision>;

        /**
         * @brief Struct containing data about a separation event.
         *
         */
        using SignalSeparatedData = CollisionPair;

        /**
         * @brief Signal for reporting collision start events.
         *
         */
        using SignalCollided = std::shared_ptr<Signal<SignalCollidedData>>;

        /**
         * @brief Signal for reporting the separation of two colliding bodies.
         *
         */
        using SignalSeparated = std::shared_ptr<Signal<SignalSeparatedData>>;


        /**
         * @brief Prefix for the name under which collided Signals are advertised.
         *
         */
        static const std::string SignalCollidedPrefix;

        /**
         * @brief Prefix for the name under which separated Signals are advertised.
         *
         */
        static const std::string SignalSeparatedPrefix;

        explicit PhysicsSystem(std::weak_ptr<ECSWorld> world):
        System<PhysicsSystem, std::tuple<PhysicsState, ObjectBounds>, std::tuple<>>{ world }
        {}

        /**
         * @brief The system type string for this class
         *
         * @return std::string This class' system type string
         *
         */
        inline static std::string getSystemTypeName() { return "PhysicsSystem"; }

        /**
         * @brief Sets the number of substeps used for physics integration using XPBD.
         *
         * See [Detailed Rigid Body Simulation with Extended Position Based Dynamics - Matthias
         * Muller et al](https://matthias-research.github.io/pages/publications/PBDBodies.pdf)
         */
        inline void setSubsteps(uint8_t newSubsteps) {
            assert(newSubsteps > 0 && "Substeps must be 1 or greater");
            mSubsteps = newSubsteps;
        }

        /**
         * @brief Gets the number of substeps used in the physics solver's XPBD implementation.
         *
         */
        inline uint8_t getSubsteps() const {
            return mSubsteps;
        }

        /**
         * @brief Registers a constraint for the physics system to evaluate every substep.
         *
         */
        template<typename TConstraint, typename TConstraintData>
        ConstraintID registerConstraint(const std::vector<std::pair<EntityID, TConstraintData>>& constraintData, float compliance);

        /**
         * @brief Removes a constraint.
         *
         */
        void unregisterConstraint(ConstraintID constraint);

    private:

        // storage for intermediate physics state
        struct PhysicsStateFull {
            ObjectBounds mBounds;
            PhysicsState mPhysics;
        };

        // storage for intermediate collision events
        union CollisionEvent {
            SignalCollidedData mCollided;
            SignalSeparatedData mSeparated;
        };

        struct CollisionReport {
            /**
             * @brief Whether this report corresponds to a collided or separated event.
             *
             */
            bool mCollided { false };
            CollisionEvent mEvent;
        };

        /**
         * @brief Marks this system as requiring initialization on the nearest update.
         *
         */
        void onSimulationActivated() override;

        /**
         * @brief Updates physics simulation
         *
         * @param timestepMillis Number of milliseconds by which to advance the simulation
         *
         */
        void onSimulationStep(uint32_t timestepMillis) override;

        /**
         * @brief Updates this entity's physics properties
         *
         * @param entityID The updated entity
         *
         */
        void onEntityEnabled(EntityID entityID) override;

        /**
         * @brief Updates this entity's physics properties
         *
         * @param entityID The updated entity
         * @param updatedComponent The updated component belonging to the entity
         *
         */
        void onEntityUpdated(EntityID entityID, ComponentType updatedComponent) override;

        /**
         * @brief Removes entity from to-initialize list
         *
         */
        void onEntityDisabled(EntityID entityID) override;

        /**
         * @brief Whether or not a particular constraint is active.
         *
         */
        bool isConstraintActive(ConstraintID constraint) const;

        /**
         * @brief Computes rotational inertia from mass and object bounds
         *
         * @param entityID The entity in need of an update
         */
        void updateProperties(EntityID entityID);

        /**
         * @brief Determines which constraints are inactive (by virtue of all of their entities
         * being enabled) and which ones aren't.
         *
         */
        void refreshActiveConstraints();

        /**
         * @brief Derives position and rotation updates for physics objects based on
         * their current state and forces acting on them.
         *
         * See [Impulse Based Dynamics](https://people.eecs.berkeley.edu/~jfc/mirtich/impulse.html) for derivations
         * of integration steps.
         *
         */
        void integrateForces(float substepSeconds, std::unordered_map<EntityID, PhysicsStateFull>& previousState, std::unordered_map<EntityID, PhysicsStateFull>& currentState);

        /**
         * @brief Derives actual object velocities after integration and constraint solve.
         *
         */
        void deriveVelocities(float substepSeconds, const std::unordered_map<EntityID, PhysicsStateFull>& previousState, std::unordered_map<EntityID, PhysicsStateFull>& currentState);

        /**
         * @brief Correctly applies collision position constraint for each potential collision detected.
         *
         */
        void applyPositionCollisionConstraints(std::map<CollisionPair, CollisionConstraint>& potentialCollisions, float substepSeconds, std::unordered_map<EntityID, PhysicsStateFull>& currentState);

        /**
         * @brief Correctly applies collision velocity constraint for each potential collision detected.
         *
         */
        void applyVelocityCollisionConstraints(std::map<CollisionPair, CollisionConstraint>& constraints, float substepSeconds, std::unordered_map<EntityID, PhysicsStateFull>& currentState);

        /**
         * @brief Applies all active position constraints
         *
         */
        void applyPositionConstraints(std::map<CollisionPair, CollisionConstraint>& constraints, float substepSeconds, std::unordered_map<EntityID, PhysicsStateFull>& currentState);

        /**
         * @brief Applies all active velocity constraints
         *
         */
        void applyVelocityConstraints(std::map<CollisionPair, CollisionConstraint>& constraints, float substepSeconds, std::unordered_map<EntityID, PhysicsStateFull>& currentState);

        /**
         * @brief Tests each pair of potential colliders for intersection, adds collision report to the queue if
         * configured.
         *
         * @param potentialCollisions Map of all collisions that _might_ take place this frame
         * @param queuedReports All reports due to be signalled this frame.
         */
        void updateCollisionEventQueue(
            std::map<CollisionPair, CollisionConstraint>& potentialCollisions,
            std::queue<CollisionReport>& queuedReports,
            std::unordered_map<EntityID, PhysicsStateFull>& currentStates
        );

        /**
         * @brief Registers a pair of entities as intersecting, signalling to collision signal observers if required.
         *
         */
        void onCollided(const CollisionPair& pair, const Collision& collision, std::queue<CollisionReport>& queuedReports);

        /**
         * @brief Removes pair of entities from signal list, signalling to observers if required.
         *
         */
        void onSeparated(const CollisionPair& pair, std::queue<CollisionReport>& queuedReports);

        /**
         * @brief Registers collision signals for this entity.
         *
         */
        void registerCollisionSignal(EntityID entity);

        /**
         * @brief Unregisters collision signals for this entity.
         *
         */
        void unregisterCollisionSignal(EntityID entity);

        /**
         * @brief Signals all collisions and separations collected over the course of the simulation
         * step, in the order those events occurred.
         *
         */
        void reportCollisions(
            std::queue<CollisionReport>& reports,
            std::unordered_map<EntityID, std::pair<SignalCollided, SignalSeparated>> signallers
        ) const;

        /**
         * @brief Whether or not a particular pair of entities have their collision tracked
         * by the physics system
         *
         */
        bool wasColliding(const CollisionPair& pair) const;

        /**
         * @brief Collects potential collisions and builds constraints from them
         *
         */
        std::map<CollisionPair, CollisionConstraint> collectPotentialCollisions(float substepSeconds);

        /**
         * @brief Holds entities that haven't undergone proper initialization, and therefore
         * aren't eligible for proper physics updates
         *
         */
        std::set<EntityID> mEntitiesUninitialized {};

        /**
         * @brief Associates each entity with its set of constraints, used to determine whether the constraint
         * can be evaluated at a given substep.
         *
         */
        std::unordered_map<EntityID, std::set<ConstraintID>> mEntityConstraintMap {};

        /**
         * @brief Storage for pairs of entities that are likely to collide this simulation frame.
         *
         */
        std::map<CollisionPair, CollisionConstraint> mPotentialCollisions {};

        /**
         * @brief Storage for collision reports that should be signalled this simulation frame.
         *
         */
        std::queue<CollisionReport> mCollisionReports {};

        /**
         * @brief A list of all constraints known to the physics system, evaluated every frame.
         *
         * Constraints whose indices are found in mDeletedConstraints are inactive, and will not be evaluated.
         *
         */
        std::vector<std::pair<std::unique_ptr<BaseConstraint>, std::vector<EntityID>>> mConstraints {};

        /**
         * @brief IDs of constraints that were deleted and which may be reused to define a new constraint
         *
         */
        std::unordered_set<ConstraintID> mConstraintsDeleted {};

        /**
         * @brief Constraints whose entities are not active, causing the constraint itself to become inactive.
         *
         */
        std::unordered_set<ConstraintID> mConstraintsInactive {};

        /**
         * @brief All currently intersecting entities.
         *
         */
        std::unordered_map<EntityID, std::set<CollisionPair>> mColliding {};

        /**
         * @brief Entities whose collision events are signalled by this system.
         *
         */
        std::unordered_map<EntityID, std::pair<SignalCollided, SignalSeparated>> mCollisionSignallers {};

        /**
         * @brief The number of substeps used in the physics system's XPBD implementation.
         *
         * @see getSubsteps
         * @see setSubsteps
         *
         * See [Detailed Rigid Body Simulation with Extended Position Based Dynamics - Matthias
         * Muller et al](https://matthias-research.github.io/pages/publications/PBDBodies.pdf)
         */
        uint8_t mSubsteps { 8 };

        /**
         * @brief Whether physics properties for all eligible entities should be recomputed
         *
         */
        bool mRequiresInitialization { true };

        /**
         * @brief Whether we have yet to determine which constraints are active and which are not
         *
         */
        bool mConstraintsInitialized { true };
    };

    inline const std::string PhysicsSystem::SignalCollidedPrefix { "collided_" };
    inline const std::string PhysicsSystem::SignalSeparatedPrefix { "separated_" };

    template<typename TConstraint, typename TConstraintData>
    PhysicsSystem::ConstraintID PhysicsSystem::registerConstraint(const std::vector<std::pair<EntityID, TConstraintData>>& participants, float compliance) {
        // reclaim a deleted constraint id if possible
        ConstraintID newConstraint { mConstraints.size() };
        if(!mConstraintsDeleted.empty()) {
            newConstraint = *mConstraintsDeleted.begin();
            mConstraintsDeleted.erase(newConstraint);
        }

        // get separate constraint parameter and participating entity lists
        std::vector<EntityID> entities {};
        std::vector<TConstraintData> constraintData {};
        for(const std::pair<EntityID, TConstraintData>& participant: participants) {
            constraintData.push_back(participant.second);
            entities.push_back(participant.first);
        }

        // create constraint
        if(newConstraint == mConstraints.size()) {
            mConstraints.push_back({
                std::make_unique<TConstraint>(constraintData, compliance),
                entities
            });
        } else {
            mConstraints[newConstraint] = {
                std::make_unique<TConstraint>(constraintData, compliance),
                entities
            };
        }

        for(const auto& entity: entities) {
            mEntityConstraintMap[entity].insert(newConstraint);
        }

        return newConstraint;
    }
}

#endif


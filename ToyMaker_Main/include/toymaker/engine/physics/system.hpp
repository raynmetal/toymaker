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


#include <nlohmann/json.hpp>

#include "../core/ecs_world.hpp"

#include "../spatial_query/types.hpp"
#include "types.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToyMakerPhysics ToyMakerECSSystem
     * @brief The physics system, an ECS system that tracks and updates the state of each physics
     * object in the scene according to its properties.
     *
     * Relies strongly on the spatial query system to provide information about object overlaps
     * and collisions.  Collision events are generated when an object with a certain interaction mask
     * overlaps with objects belonging to one of the layers indicated by that mask.
     *
     * The physics system implemented here is based on XPBD (Extended Position Based Dynamics).
     *
     * @see ToyMakerSpatialQuerySystem
     *
     */
    class PhysicsSystem: public System<PhysicsSystem, std::tuple<PhysicsLocal, ObjectBounds>, std::tuple<>> {

    public:
        explicit PhysicsSystem(std::weak_ptr<ECSWorld> world):
        System<PhysicsSystem, std::tuple<PhysicsLocal, ObjectBounds>, std::tuple<>>{ world }
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
         * @brief Gets the number of substeps used in the physics solver's XPBD implementation
         *
         */
        inline uint8_t getSubsteps() const {
            return mSubsteps;
        }

    private:

        /**
         * @brief Computes rotational inertia from mass and object bounds
         *
         * @param entityID The entity in need of an update
         */
        void updatePhysicsProperties(EntityID entityID);

        /**
         * @brief Marks this system as requiring initialization on the nearest update.
         *
         */
        void onSimulationActivated() override;

        /**
         * @brief Updates physics simulation
         *
         * @param timestepMillis number of milliseconds by which to advance the
         * simulation
         */
        void onSimulationStep(uint32_t timestepMillis) override;

        /**
         * @brief Updates this entity's physics properties
         *
         * @param entityID The updated entity
         */
        void onEntityEnabled(EntityID entityID) override;

        /**
         * @brief Updates this entity's physics properties
         *
         * @param entityID The updated entity
         * @param updatedComponent The updated component belonging to the entity
         */
        void onEntityUpdated(EntityID entityID, ComponentType updatedComponent) override;

        /**
         * @brief Removes entity from to-initialize list
         *
         */
        void onEntityDisabled(EntityID entityID) override;


        /**
         * @brief Whether physics properties for all eligible entities should be recomputed
         *
         */
        bool mRequiresInitialization { true };

        /**
         * @brief Holds entities that haven't undergone proper initialization, and therefore
         * aren't eligible for proper physics updates
         *
         */
        std::set<EntityID> mEntitiesUninitialized {};

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
    };
}

#endif


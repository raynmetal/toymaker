/**
 * @ingroup ToyMakerSpatialQuerySystem
 * @file spatial_query/sweep_prune.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Acceleration structure for finding pairs of objects whose AABBs are intersecting.
 * @version 0.3.2
 * @date 2025-09-10
 *
 */

#ifndef TOYMAKERENGINE_SPATIALQUERYSWEEPPRUNE_H
#define TOYMAKERENGINE_SPATIALQUERYSWEEPPRUNE_H

#include "toymaker/engine/input_system/input_data.hpp"
#include "types.hpp"
#include "../core/ecs_world.hpp"

namespace ToyMaker {
    class SweepPrune;

    /**
     * @ingroup ToyMakerSpatialQuerySystem
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
         * @brief Creates a collision pair out of two distinct entities, where the first entity's
         * ID is lesser than the second's.
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
     * @ingroup ToyMakerSpatialQuerySystem
     *
     * @brief A class designed around finding which pairs of AABBs may be intersecting (so as to limit
     * the number of full collision checks that need be performed).
     */
    class SweepPrune {
    public:
        /**
         * @brief Returns all collision pairs that have been detected thus far.
         *
         */
        std::set<CollisionPair> getCollisionPairs();

        /**
         * @brief Adds an object for sweep and prune to track.
         *
         */
        void addObject(EntityID entity, const AxisAlignedBounds& bounds);

        /**
         * @brief Removes an object from Sweep and Prune's tracking lists.
         *
         */
        void removeObject(EntityID entity);

        /**
         * Updates the values of the boundaries of an object tracked by sweep and prune.
         *
         */
        void updateObject(EntityID entity, const AxisAlignedBounds& bounds);

    private:
        using EdgeIndex = std::size_t;

        enum Axis: uint8_t {
            X,
            Y,
            Z,
        };
        /**
         * @brief Contains the start and end indices of a sweep and prune object in
         * each dimension's edge list.
         *
         */
        struct Object {
            std::array<std::pair<EdgeIndex, EdgeIndex>, 3> mIndices {};
        };

        /**
         * @brief Represents the projection of a bounding box along the axis represented by the edge list that
         * this item appears in.
         *
         * Relates an edge to its owning object, and provides a comparison method for sorting edge lists.
         *
         */
        struct Edge {
            EntityID mEntity;
            float mValue;
            bool mBegin;

            inline bool operator<(const Edge& other) {
                return (
                    mValue < other.mValue || (
                        mValue == other.mValue
                        && mEntity < other.mEntity
                        || (
                            mValue == other.mValue
                            && mEntity == other.mEntity
                            && mBegin > other.mBegin
                        )
                    )
                );
            }
        };

        /**
         * @brief Sorts edges along every axis.
         *
         */
        void sortEdges();

        /**
         * @brief Sorts edges along a particular axis.
         *
         */
        void sortEdges(Axis axis);

        /**
         * @brief Scans edge lists to find likely collision pairs.
         *
         */
        void findCollisionPairs();

        /**
         * @brief Indices into edge lists for each axis per entity.
         *
         */
        std::unordered_map<EntityID, Object> mObjects {};

        /**
         * @brief Storage for all detected collision pairs.
         *
         */
        std::set<CollisionPair> mCollisionPairs {};

        /**
         * @brief Sorted list of edges along each axis.
         *
         */
        std::array<std::vector<Edge>, 3> mEdges {};

        /**
         * @brief Pairs of entities which are overlapping on each axis.
         *
         */
        std::array<std::set<CollisionPair>, 3> mOverlaps {};

        /**
         * @brief Whether collision pairs need to be recomputed, as they would after an update
         *
         */
        bool mRecomputeCollisions { true };
    };
}

#endif

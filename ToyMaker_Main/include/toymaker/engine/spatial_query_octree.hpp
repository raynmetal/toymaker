/**
 * @ingroup ToyMakerSpatialQuerySystem
 * @file spatial_query_octree.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Data structures, functions, and methods relating to the hierarchical organization and representation of spatial data.
 * @version 0.3.2
 * @date 2025-09-10
 * 
 */

#ifndef FOOLSENGINE_SPATIALQUERYOCTREE_H
#define FOOLSENGINE_SPATIALQUERYOCTREE_H

#include <memory>
#include <array>
#include <algorithm>
#include <cmath>

#include "spatial_query_math.hpp"
#include "spatial_query_basic_types.hpp"
#include "core/ecs_world.hpp"

namespace ToyMaker {
    class Octree;

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief A single node of an octree, representing a single octant of the 8 that make up its parent region.
     * 
     */
    class OctreeNode: public std::enable_shared_from_this<OctreeNode> {
    public:
        /**
         * @brief A number representing the portion of the parent region represented by this node.
         * 
         * @see OctantSpecifier
         * 
         */
        using Octant = uint8_t;

        /**
         * @brief The depth of this node relative to the overall octree;  the number of hops to go from this node up to the root node of the tree.
         * 
         */
        using Depth = uint8_t;

        /**
         * @brief The full address of this node, where every three bits right-to-left represent the octant in the hierarchy to select in order to reach it, and the leftmost bits represent the depth at which it is found.
         * 
         */
        using Address = uint64_t;

        /**
         * @brief Mask values which, when applied to the octant value, tell you which sub-region of the parent region this node represents.
         * 
         */
        enum OctantSpecifier: Octant {
            RIGHT=0x1,
            TOP=0x2,
            FRONT=0x4,
        };

        /**
         * @brief The number of bits on the left hand side of a node address giving the depth of this node.
         * 
         */
        static constexpr uint8_t knDepthBits { 5 };

        /**
         * @brief (Computed) The number of bits by which to shift the address right in order to retrieve the depth value of the node.
         * 
         * @see knDepthBits
         */
        static constexpr uint8_t kDepthBitOffset { sizeof(Address)*8 - knDepthBits };

        /**
         * @brief The number of bits of the address, starting from the right, representing the route to follow in the Octree hierarchy in order to reach this node.
         * 
         */
        static constexpr uint8_t knRouteBits { (kDepthBitOffset/3) * 3};

        /**
         * @brief The total depth representable given the value range of the depth-section of the address, and the space available in the route-section.
         * 
         */
        static constexpr Depth kMaxDepthInclusive {std::min(
            1 + knRouteBits/3, 
            1 + (1<<knDepthBits) 
        )}; // +1 as depth 0 needs no bits

        /**
         * @brief A special value reserved for the absence of an address, as in the root node of the Octree.
         * 
         */
        static constexpr Address kNoAddress { 0x0 };

        /**
         * @brief Prepends the address of a child octant to the address of its parent in order to make the child's address.
         * 
         * @param childOctant The octant of the parent that the child will represent.
         * @param parentAddress The (presumably valid) address of the parent.
         * @return Address The new address belonging to the child.
         */
        static Address MakeAddress(Octant childOctant, Address parentAddress);

        /**
         * @brief Gets the depth value of a node based on its address.
         * 
         * @param address The address of the node.
         * @return Depth The depth of the node, or the number of hops from it to the root node.
         */
        static Depth GetDepth(Address address);

        /**
         * @brief Maps an octant to its corresponding growth direction.
         * 
         * The growth-directions are the directions in which each dimension of this octant would need to be doubled in order to convert it into its own parent region, per its octant value.
         * 
         * 
         * This method isn't really used, but is present for symmetry with ToOctant()
         * 
         * @param octant The octant being converted into a "growth direction".
         * @return Octant The growth direction for this octant.
         */
        static Octant ToGrowthDirection(Octant octant);

        /**
         * @brief Converts a growth direction to its corresponding octant.
         * 
         * The growth direction tells you in which direction an octant's dimensions should be doubled in order to become its parent.  Conversely, the direction of the subdivision of the enlarged region gives you the octant value for the original sub-region.
         * 
         * This is mainly used when the area to be encapsulated by the octree has expanded, say when an object is added which is outside of the Octree's currently enclosed region.
         * 
         * Once all the growth steps (a series of growth directions from the old root of the octree) have been computed, this operation produces the octant value for the node corresponding to each step.
         * 
         * @param growthDirection The direction in which a smaller node was doubled in order to produce the larget one.
         * @return Octant The address of the smaller node, relative to the newly made larger one.
         */
        static Octant ToOctant(Octant growthDirection);

        /**
         * @brief Returns the 3 bits representing just this node's octant value, relative to its own parent.
         * 
         * @param address The address of the node whose octant value we want.
         * @return Octant The node's octant value.
         */
        static Octant GetOctant(Address address);

        /**
         * @brief Gets a bit mask covering the first 3*depth bits based on a given depth value.
         * 
         * @param baseDepth The depth up to which we want the route bits for.
         * @return Address The mask covering all the route bits up to the specified depth.
         */
        static Address GetBaseRouteMask(Depth baseDepth);

        /**
         * @brief Gets the value of the route section of an address up to some specified depth.
         * 
         * @param address The address whose route (up to baseDepth) we want to obtain.
         * @param baseDepth The depth up to which we want the route for.
         * @return Address The value of the route up to base depth in the provided address.
         */
        static Address GetBaseRoute(Address address, Depth baseDepth);

        /**
         * @brief Gets the octant corresponding to a specific depth within an address.
         * 
         * @param address The address in which the octant is present (as part of its route)
         * @param depth The depth of the octant being retrieved.
         * @return Octant The 3-bit value of the octant itself.
         */
        static Octant GetOctantAt(Address address, Depth depth);

        /**
         * @brief Adds root-address' route to address' route.
         * 
         * The depth of the root address is added to the old address (the first argument) in order to produce that node or object's new address.
         * 
         * This is used when the octree has grown to enclose a larger region, and its previous nodes must be recomputed relative to the new root of the octree.
         * 
         * @param address The (previous) address of an octant or object.
         * @param rootAddress The address from the new root of the octree to the old root (or its leaf-most ancestor).
         * @return Address The new address for the octant or object.
         */
        static Address GrowAddress(Address address, Address rootAddress);

        /**
         * @brief Shrinks an address according to the depth removed.
         * 
         * This is used when, after the removal of an object, octree nodes higher up in the tree are no longer required.  A node that was previously a descendant node is promoted to the root node of the octree, and all addresses are changed accordingly.
         * 
         * @param address The (previous) address of a node or object, now being shrunk.
         * @param depthRemoved The depth being removed from the address.
         * @return Address The new address after the shrinkage.
         */
        static Address ShrinkAddress(Address address, Depth depthRemoved);

        /**
         * @brief Tests whether two nodes are present on the same branch of an octree (or in other words, whether one is the descendant or ancestor of the other).
         * 
         * @param one The address of the first node.
         * @param two The address of the second node.
         * @retval true Both addresses correspond to nodes or objects on the same branch of the octree;
         * @retval false The addresses diverge, and are not on the same branch of the octree;
         */
        static bool SharesBranch(Address one, Address two);

        /**
         * @brief Mask values for retrieving the different sections of an Address.
         * 
         */
        enum AddressMasks: Address {
            DEPTH_MASK = -(1ull << kDepthBitOffset), //< Mask for the most significant knDepthBits of the address; mask corresponding to the depth section of the address.
            ROUTE_MASK = ~(-(1ull << knRouteBits)), //< Mask for the least significant knRouteBits of the address; mask corresponding to the route section of the address.
        };
        static_assert(DEPTH_MASK != 0 && "Depth mask cannot be zero");
        static_assert(ROUTE_MASK != 0 && "Route mask cannot be zero");
        static_assert(knDepthBits + kDepthBitOffset == sizeof(Address)*8 && "sum of depth bits and depth offset must add up to the size of the address in bits");
        static_assert(kDepthBitOffset >= knRouteBits && "There must be at least as many bits in the depth bit offset as those used to make the route");
        static_assert(kNoAddress == 0 && "NoAddress must correspond with 0");

        /**
         * @brief Produces a node for the root of an octree that encloses the region to be divided.
         * 
         * @param subdivisionThreshold The number of objects on a single node beyond which a subdivision of the node should be attempted.
         * @param boundRegion The region containing all of the axis aligned bounds to be enclosed by this octree.
         * @return std::shared_ptr<OctreeNode> The newly created node representing the root of a new octree.
         */
        static std::shared_ptr<OctreeNode> CreateRootNode(
            uint8_t subdivisionThreshold, AxisAlignedBounds boundRegion
        );

        /**
         * @brief Expands an octree such that it encloses a previously unmapped region, and creates a node to be used as the new root node for the octree.
         * 
         * @param oldRoot The root node prior to the addition of an object falling outside its bounds.
         * @param regionToCover The region enclosing both the old octree as well as any new objects added falling outside of it.
         * @return std::shared_ptr<OctreeNode> The new root node for the octree.
         */
        static std::shared_ptr<OctreeNode> GrowTreeAndCreateRoot(
            std::shared_ptr<OctreeNode> oldRoot,
            const AxisAlignedBounds& regionToCover
        );

        /**
         * Available query methods
         */

        /**
         * @brief Retrieves all entities in this octant and its descendants.
         * 
         * @return std::vector<std::pair<EntityID, AxisAlignedBounds>> All entities mapped to this node and its descendant nodes.
         */
        std::vector<std::pair<EntityID, AxisAlignedBounds>> findAllMemberEntities() const;

        /**
         * @brief Retrieves all entities that intersect with the region described by searchBounds.
         * 
         * @param searchBounds The region whose entities we would like to fetch.
         * 
         * @return std::vector<std::pair<EntityID, AxisAlignedBounds>> The entities overlapping the search region.
         */
        std::vector<std::pair<EntityID, AxisAlignedBounds>> findEntitiesOverlapping(const AxisAlignedBounds& searchBounds) const;
        /**
         * @brief Retrieves all entities that intersect with the ray described by searchRay.
         * 
         * @param searchRay The ray whose intersecting objects we would like to fetch.
         * 
         * @return std::vector<std::pair<EntityID, AxisAlignedBounds>> The entities intersecting with the search ray.
         */
        std::vector<std::pair<EntityID, AxisAlignedBounds>> findEntitiesOverlapping(const Ray& searchRay) const;

        /**
         * @brief Gets the number of active child octants this octant has.
         * 
         * @return uint8_t The number of active children of this node.
         */
        uint8_t getChildCount() const;

        /**
         * @brief Gets the address value for this node.
         * 
         * @return Address This node's address.
         */
        Address getAddress() const { return mAddress; }

        /**
         * @brief Retrieves the AABB representing the region this node covers.
         * 
         * @return AxisAlignedBounds The AABB representing the region this node covers.
         */
        inline AxisAlignedBounds getWorldBounds() const { return mWorldBounds; }

        /**
         * @brief Adds an entity to this node or its subtree, per its configuration and the bounds of the object.
         * 
         * @param entityID The ID of the object being added.
         * @param entityWorldBounds The AABB of the entity.
         * @return Address The new address of the node, post-addition.
         */
        Address insertEntity(EntityID entityID, const AxisAlignedBounds& entityWorldBounds);

        /**
         * @brief Removes an entity situated at a node at some address (or on a descendant node).
         * 
         * @param entityID The ID of the entity being removevd.
         * @param entityAddressHint The address of the node (or that node's ancestor) at which the entity is present.
         * @return std::shared_ptr<OctreeNode> (If the octree must be shrunk) the node of the new to-be root node.
         */
        std::shared_ptr<OctreeNode> removeEntity(EntityID entityID, Address entityAddressHint=kNoAddress);

        /**
         * @brief Gets a descendant of this node (or this node itself) by its address.
         * 
         * @param octantAddress The octree address of the node being fetched.
         * 
         * @return std::shared_ptr<OctreeNode> The node at the address specified.
         */
        std::shared_ptr<OctreeNode> getNode(Address octantAddress);

        /**
         * @brief Gets the child node corresponding to the next node in the argument's route section relative to the this node.
         * 
         * @param octantAddress The address of the node being looked for.
         * 
         * @return std::shared_ptr<OctreeNode> The child node corresponding to the next 3 bits of the route section of the address.
         */
        std::shared_ptr<OctreeNode> nextNodeInAddress(Address octantAddress);

        /**
         * @brief Gets the node whose region just encompasses the bounds provided as input.
         * 
         * The region of the node retrieved is such that any sub-region would at most overlap, but not enclose, the argument bounds.
         * 
         * @param entityWorldBounds The bounds which the node retrieved should just envelop.
         * @return std::shared_ptr<OctreeNode> The node enclosing the the argument bounds.
         */
        std::shared_ptr<OctreeNode> getSmallestNodeContaining(const AxisAlignedBounds& entityWorldBounds);

        /**
         * @brief Gets the smallest node, this node or a descendant, whose region encompasses all entities remaining in the Octree.
         * 
         * Usually called after the removal of a node, when an Octree shrinkage may be in order.
         * 
         * @return std::shared_ptr<OctreeNode> The smallest node containing all the objects in the scene.
         */
        std::shared_ptr<OctreeNode> findCandidateRoot();

        /**
         * @brief Gets the route section of the address up to the current node's depth.
         * 
         * @param address The address whose base route is being retrieved.
         * @return Address The base route of the argument address, up to this node's depth.
         */
        Address getBaseRoute(Address address) const;

        /**
         * @brief The depth of this node relative to the root of the Octree it is a part of.
         * 
         * @return Depth This node's Octree depth.
         */
        Depth getDepth() const;

        /**
         * @brief Gets the octant value of this node relative to its parent.
         * 
         * @return Octant This node's octant value relative to its parent.
         */
        Octant getOctant() const;

        /**
         * @brief Fetches the octant of the next node in the route section of the argument address.
         * 
         * @param address The address to a node or an entity contained by it.
         * @return Octant The octant of this node's child, which is the next node in the route beyond this one.
         */
        Octant nextOctant(Address address) const;

        /**
         * @brief Trims the addresses of this node and its children (and consequently their entities) such that this node is the root.
         * 
         */
        void shrinkTreeAndBecomeRoot();

    private:
        /**
         * @brief Constructs a new OctreeNode.
         * 
         * @param octantAddress The address this node is initialized with.
         * @param subdivisionThreshold The number of member entities a node may have beyond which its subdivision should be attempted.
         * @param worldBounds The total region (as an AABB) to be enclosed by this node.
         * @param parent A reference to the parent of this node.
         */
        OctreeNode(
            Address octantAddress,
            uint8_t subdivisionThreshold,
            AxisAlignedBounds worldBounds,
            std::shared_ptr<OctreeNode> parent
        ):
        mAddress { octantAddress },
        mSubdivisionThreshold { subdivisionThreshold },
        mWorldBounds { worldBounds },
        mParent { parent }
        {}

        /**
         * @brief The address of this node, where kNoAddress is the address of the root node of an octree.
         * 
         */
        Address mAddress { 0x0 };

        /**
         * @brief The number of member entities a node (or its descendant) may have, beyond which the node's subdivision should be attempted.
         * 
         */
        uint8_t mSubdivisionThreshold { 40 };

        /**
         * @brief The region, as an AABB, encompassed by this node.
         * 
         */
        AxisAlignedBounds mWorldBounds {};

        /**
         * @brief The parent of this node, which this node is an octant of.
         * 
         */
        std::weak_ptr<OctreeNode> mParent {};

        /**
         * @brief An array of up to 8 child nodes maintained by this node, where each index corresponds to one possible value of an Octant.
         * 
         * @see OctantSpecifier
         * 
         */
        std::array<std::shared_ptr<OctreeNode>, 8> mChildren {};

        /**
         * @brief The member entities of this node.
         * 
         */
        std::map<EntityID, AxisAlignedBounds> mEntities{};
    };

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * 
     * @brief A data structure used for speeding up spatial queries about 3-dimensional objects in the scene.
     * 
     * The octree is essentially a node representing a cuboidal region that can be subdivided into equally-proportioned smaller cuboidal regions recursively.  Each node of the octree maintains a list entities contained by its region.
     * 
     * When given a query in the form of some geometry (currently only AABBs and Rays), where entities overlapping or contained by that geometry are desired, the octree speeds up the query by limiting its search to only those entities whose nodes overlap the query geometry.
     * 
     * Octrees presume the finiteness of geometry contained by them.  An octree can grow in size or shrink so long as the region it encloses does not become infinite, nor fall below some threshold granularity.
     * 
     * @see OctreeNode
     * 
     */
    class Octree {
    public:
        /**
         * @brief The maximum possible ratio between two dimensions of the region enclosed by the octree.
         * 
         * Specified in order to keep nodes more-or-less cuboidal, and preventing them from being too flat or line-like (and thereby causing the octree to have to expand or shrink too often).
         * 
         */
        static constexpr float kMaxDimensionRatio { 20.f };

        /**
         * @brief An entity-id node-address pair indicating the address at which some entity known by the entity is present.
         * 
         */
        using EntityAddressPair = std::pair<EntityID, OctreeNode::Address>;

        /**
         * @brief Constructs a new octree which encapsulates the bounds specified in the argument.
         * 
         * @param subdivisionThreshold The number of member entities for a single node beyond which that node's subdivision should be attempted.
         * @param totalWorldBounds The region containing all of the objects in the world, which this octree will be defined to encompass.
         */
        Octree(uint8_t subdivisionThreshold, const AxisAlignedBounds& totalWorldBounds):
        mRootNode { OctreeNode::CreateRootNode(subdivisionThreshold, totalWorldBounds) }
        {}

        /**
         * Available query methods
         */

        /**
         * @brief Retrieves all entities in this octant and its descendants.
         * 
         * @return std::vector<std::pair<EntityID, AxisAlignedBounds>> A list of entities (their IDs and AABBs) known by the octree.
         * 
         */
        inline std::vector<std::pair<EntityID, AxisAlignedBounds>> findAllMemberEntities() const {
            return mRootNode->findAllMemberEntities();
        }

        /**
         * @brief Retrieve all entities that intersect with the region described by searchBounds.
         * 
         * @param searchBounds The AABB intersecting or within which entities should be retrieved.
         * @return std::vector<std::pair<EntityID, AxisAlignedBounds>> The entities found to match the query.
         */
        inline std::vector<std::pair<EntityID, AxisAlignedBounds>> findEntitiesOverlapping(const AxisAlignedBounds& searchBounds) const {
            return mRootNode->findEntitiesOverlapping(searchBounds);
        }

        /**
         * @brief Retrieves all entities that intersect with the ray described by searchRay.
         * 
         * @param searchRay The ray intersecting which entities need to be retrieved.
         * @return std::vector<std::pair<EntityID, AxisAlignedBounds>> The entities found to match the query.
         */
        std::vector<std::pair<EntityID, AxisAlignedBounds>> findEntitiesOverlapping(const Ray& searchRay) const;

        /**
         * @brief Inserts an entity into the octree.
         * 
         * @param entityID The ID of the entity being inserted.
         * @param entityWorldBounds The bounds of the entity's AABB.
         */
        void insertEntity(EntityID entityID, const AxisAlignedBounds& entityWorldBounds);

        /**
         * @brief Removes an entity from the octree, based on its cached node address.
         * 
         * @param entityID The ID of the entity being removed.
         */
        void removeEntity(EntityID entityID);

    private:
        /**
         * @brief The root node of the octree.
         * 
         */
        std::shared_ptr<OctreeNode> mRootNode;

        /**
         * @brief A mapping of entity IDs to their addresses computed at entity insertion (and recomputed when the octree grew or shrank).
         * 
         */
        std::map<EntityID, OctreeNode::Address> mEntityAddresses {};
    };
}

#endif

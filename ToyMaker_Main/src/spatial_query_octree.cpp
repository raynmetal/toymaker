#include <iostream>
#include <limits>
#include <queue>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "toymaker/engine/spatial_query_octree.hpp"

using namespace ToyMaker;

void printAABBExtents(const AxisAlignedBounds& bounds) {
    std::cout << "Extents: (right top front " << glm::to_string(bounds.getAxisAlignedBoxExtents().first) 
        << ") (left bottom back " << glm::to_string(bounds.getAxisAlignedBoxExtents().second)
        << "\n";
}

void printOctreeNodeAddress(const OctreeNode::Address& address) {
    std::cout << "depth-" << static_cast<uint32_t>(OctreeNode::GetDepth(address))
        << "\nroute-" << std::bitset<OctreeNode::knRouteBits>{OctreeNode::GetBaseRoute(address, OctreeNode::GetDepth(address))}
        << "\n";
}

void printOctreeNodeMemberDetails(const AxisAlignedBounds& bounds, const OctreeNode::Address& address, const EntityID& entityID) {
    std::cout << "Inserted entity: ID-" << entityID << "\n";
    printOctreeNodeAddress(address);
    printAABBExtents(bounds);
    std::cout << "\n";
}

std::shared_ptr<OctreeNode> OctreeNode::CreateRootNode(
    uint8_t subdivisionThreshold,
    AxisAlignedBounds boundRegion
) {
    assert(boundRegion.isSensible() && "Invalid world bounds provided");
    assert(isPositive(boundRegion.getDimensions()) && "Octree node must have bounds that encapsulate a volume");

    const glm::vec3 boundsDimensions { boundRegion.getDimensions() };
    const float maxDimensionLength { std::max({boundsDimensions.x, boundsDimensions.y, boundsDimensions.z}) };
    const float boundsAspect { 
        maxDimensionLength
        / std::min({boundsDimensions.x, boundsDimensions.y, boundsDimensions.z})
    };

    if(boundsAspect > Octree::kMaxDimensionRatio) {
        const float minDimensionClampValue { maxDimensionLength / Octree::kMaxDimensionRatio };
        boundRegion.setDimensions(glm::max(boundsDimensions, glm::vec3{minDimensionClampValue}));;
    }
    return std::shared_ptr<OctreeNode>(new OctreeNode{kNoAddress, subdivisionThreshold, boundRegion, nullptr});
}

OctreeNode::Depth OctreeNode::GetDepth(Address address) {
    return (address & DEPTH_MASK) >> kDepthBitOffset;
}

OctreeNode::Octant OctreeNode::GetOctantAt(Address address, Depth depth) {
    return ((0x7ull << (3 * (depth - 1))) & address) >> (3 * (depth - 1));
}

OctreeNode::Octant OctreeNode::GetOctant(Address address) {
    return GetOctantAt(address, GetDepth(address));
}

OctreeNode::Address OctreeNode::MakeAddress(Octant childOctant, Address parentAddress) {
    const Depth parentDepth { GetDepth(parentAddress) };
    return (
        ((1ull + parentDepth) << kDepthBitOffset)
        | (
            GetBaseRoute(parentAddress, parentDepth)
            | (static_cast<Address>(childOctant) << (3 * parentDepth))
        )
    );
}

OctreeNode::Octant OctreeNode::ToGrowthDirection(Octant octant) {
    return ToOctant(octant);
}

OctreeNode::Octant OctreeNode::ToOctant(Octant growthDirection) {
    return (~growthDirection) & 0x7;
}

OctreeNode::Address OctreeNode::GetBaseRouteMask(Depth baseDepth) {
    return ~(-(1ull << (3 * baseDepth)));
}

OctreeNode::Address OctreeNode::GetBaseRoute(Address address, Depth baseDepth) {
    return GetBaseRouteMask(baseDepth) & address;
}

OctreeNode::Address OctreeNode::GrowAddress(Address address, Address baseAddress) {
    const Depth baseDepth { GetDepth(baseAddress) };
    const Depth newDepth { static_cast<Depth>(GetDepth(address) + baseDepth) };
    return (
        (
            static_cast<Address>(newDepth) << kDepthBitOffset
        ) | ((
            (
                (address << (3 * baseDepth))
                & ROUTE_MASK
            ) | (
                GetBaseRoute(baseAddress, baseDepth)
            )
        ) & ROUTE_MASK)
    );
}

OctreeNode::Address OctreeNode::ShrinkAddress(Address address, Depth depthRemoved) {
    const Address oldDepth { GetDepth(address) };
    if(oldDepth <= depthRemoved) { return kNoAddress; }
    return (
        (static_cast<Address>(GetDepth(address) - depthRemoved) << kDepthBitOffset)
        | ((
            (address & ROUTE_MASK) >> (3 * depthRemoved)
        ) & ROUTE_MASK)
    );
}

bool OctreeNode::SharesBranch(Address one, Address two) {
    const Depth minDepth { std::min(GetDepth(one), GetDepth(two)) };
    return GetBaseRoute(one, minDepth) == GetBaseRoute(two, minDepth);
}

OctreeNode::Address OctreeNode::insertEntity(EntityID entityID, const AxisAlignedBounds& entityWorldBounds) {
    // whoever is in charge of managing this node must ensure that
    // the submitted entity can be contained within it
    assert(contains(entityWorldBounds, mWorldBounds) && "Invalid insertion into octree attempted");

    // see if one of this node's existing children can accept the object
    std::shared_ptr<OctreeNode> smallestNodeContainingEntity {
        getSmallestNodeContaining(entityWorldBounds)
    };
    if(smallestNodeContainingEntity != shared_from_this()) {
        return smallestNodeContainingEntity->insertEntity(entityID, entityWorldBounds);
    }

    // we know placement in existing octants obviously not possible once we reach
    // here. see if a subdivision would reduce membership in this node (provided our
    // subdivision threshold would be/already has been exceeded, and maximum depth
    // hasn't yet been reached)
    if (
        getDepth() < kMaxDepthInclusive 
        && mEntities.size() + 1 >= mSubdivisionThreshold
        // account for floating point errors in oddly placed or extremely sized bounds
        && isPositive(.5f * mWorldBounds.getDimensions())
    ) {
        bool anyOctantsCreated { false };
        for(Octant octant {0x0}; octant < 0x8; ++octant) {
            // see earlier loop
            if(mChildren[octant]) continue;
            AxisAlignedBounds::Extents newExtents { mWorldBounds.getAxisAlignedBoxExtents() };
            glm::vec3 center { mWorldBounds.getPosition() };
            octant&RIGHT? newExtents.second.x = center.x: newExtents.first.x = center.x;
            octant&TOP? newExtents.second.y = center.y: newExtents.first.y = center.y;
            octant&FRONT? newExtents.second.z = center.z: newExtents.first.z = center.z;
            AxisAlignedBounds octantBounds { newExtents };

            // see if creating an octant will lower membership
            bool shouldCreateOctant {
                contains(entityWorldBounds, octantBounds)
            };
            if(!shouldCreateOctant) {
                for(const std::pair<const EntityID, AxisAlignedBounds>& objectBounds: mEntities) {
                    if(contains(objectBounds.second, octantBounds)) {
                        shouldCreateOctant = true;
                        break;
                    }
                }
            }
            if(!shouldCreateOctant) continue;

            // There is at least one entity that may be moved into this octant, so create it
            mChildren[octant] = std::shared_ptr<OctreeNode>(new OctreeNode{
                MakeAddress(octant, getAddress()),
                mSubdivisionThreshold,
                octantBounds,
                shared_from_this()
            });
            anyOctantsCreated = true;
        }

        // remove and reinsert all member objects into this node, having done
        // as many subdivisions as possible. objects will trickle down to the
        // smallest nodes that can contain them
        if(anyOctantsCreated) {
            std::map<EntityID, AxisAlignedBounds> ourObjects {};
            std::swap(ourObjects, mEntities);
            for(const std::pair<const EntityID, AxisAlignedBounds>& entity: ourObjects) {
                insertEntity(entity.first, entity.second);
            }
            return insertEntity(entityID, entityWorldBounds);
        }
    }

    // At this point, there's simply no getting around adding this entity 
    // to our list of member objects, so just get it over with
    mEntities[entityID] = entityWorldBounds;
    return getAddress();
}

std::shared_ptr<OctreeNode> OctreeNode::removeEntity(EntityID entityID, Address entityAddressHint) {
    assert(SharesBranch(entityAddressHint, mAddress) && "This address belongs to a different branch of the octree");

    // Address hint indicates that the node is further down, so keep going
    if(GetDepth(entityAddressHint) > GetDepth(mAddress)) {
        const Octant next { nextOctant(entityAddressHint) };
        assert(mChildren[next] && "The next octant specified in the address does not exist");
        mChildren[next] = mChildren[next]->removeEntity(entityID, entityAddressHint);
        return shared_from_this();
    }

    // this entity is not a member of this node. Search for it in our children,
    // have it removed, and exit
    if(mEntities.find(entityID) == mEntities.end()) {
        if(getDepth() < kMaxDepthInclusive) {
            for(uint8_t octant {0}; octant < 8; ++octant) {
                if(mChildren[octant]) {
                    mChildren[octant] = mChildren[octant]->removeEntity(entityID);
                }
            }
        }

        return shared_from_this();
    }

    // Base case: this entity is definitely a member of ours.  Live the consequences of removing
    // it
    mEntities.erase(entityID);

    // There is nothing present in this sub-branch, so it can be safely removed (provided
    // it isn't the root node)
    if(mEntities.empty() && getChildCount() == 0 && mAddress != kNoAddress) {
        return nullptr;
    }

    return shared_from_this();
}

std::shared_ptr<OctreeNode> OctreeNode::nextNodeInAddress(Address octantAddress) {
    assert(getBaseRoute(octantAddress) == getBaseRoute(getAddress()) && "The address being searched for does not belong to this node or any of its descendants");
    assert(GetDepth(octantAddress) > getDepth() && "Address belonging to current node or its ancestor has been specified, when one belonging to its descendants was expected");

    const Octant nxtOctant { nextOctant(octantAddress) };
    assert(mChildren[nxtOctant] && "The next octant specified by this address does not exist");
    return mChildren[nxtOctant];
}

std::shared_ptr<OctreeNode> OctreeNode::getNode(Address octantAddress) {
    assert(getBaseRoute(octantAddress) == getBaseRoute(getAddress()) && "The address being searched for is not present in this node or any of its descendants");
    assert(GetDepth(octantAddress) >= getDepth() && "Address belonging to current node's ancestor has been specified, when one belonging to it or its descendants was expected");

    const Octant nextOctant { GetOctantAt(octantAddress, 1 + getDepth()) };
    assert(mChildren[nextOctant] && "The next octant specified by this address does not exist");

    std::shared_ptr<OctreeNode> nextNode { nextNodeInAddress(octantAddress) };

    // if the right node has been found, return it, ...
    if(nextNode->mAddress == octantAddress) { return mChildren[nextOctant]; }

    // ... otherwise continue the search
    return nextNode->getNode(octantAddress);
}

std::shared_ptr<OctreeNode> OctreeNode::getSmallestNodeContaining(const AxisAlignedBounds& worldBounds) {
    assert(contains(worldBounds, mWorldBounds) && "This node cannot contain the bounds specified");

    for(std::shared_ptr<OctreeNode> child: mChildren) {
        if(child && contains(worldBounds, child->mWorldBounds)) {
            return child->getSmallestNodeContaining(worldBounds);
        }
    }

    return shared_from_this();
}

std::shared_ptr<OctreeNode> OctreeNode::findCandidateRoot() {
    // If we aren't qualified to be a root node, look further down
    if(getChildCount() == 1 && mEntities.empty()) {
        for(std::shared_ptr<OctreeNode> child: mChildren) {
            if(child) {
                return child->findCandidateRoot();
            }
        }
    }

    // We deserve to be the root node (by virtue of having member
    // entities, or having multiple children, or being the last
    // node alive)
    return shared_from_this();
}

OctreeNode::Address OctreeNode::getBaseRoute(Address address) const {
    return GetBaseRoute(address, getDepth());
}

OctreeNode::Depth OctreeNode::getDepth() const {
    return GetDepth(mAddress);
}

OctreeNode::Octant OctreeNode::getOctant() const {
    return GetOctant(mAddress);
}

OctreeNode::Octant OctreeNode::nextOctant(Address address) const {
    return GetOctantAt(address, 1 + getDepth());
}

std::vector<std::pair<EntityID, AxisAlignedBounds>> OctreeNode::findAllMemberEntities() const {
    // initialize vector with own immediate entities
    std::vector<std::pair<EntityID, AxisAlignedBounds>> memberEntities { mEntities.begin(), mEntities.end() };

    // collect entities present in children
    for(std::shared_ptr<OctreeNode> child: mChildren) {
        if(!child) continue;

        std::vector<std::pair<EntityID, AxisAlignedBounds>> childEntities { child->findAllMemberEntities() };
        memberEntities.insert(
            memberEntities.cend(),
            childEntities.begin(),
            childEntities.end()
        );
    }

    return memberEntities;
}

std::vector<std::pair<EntityID, AxisAlignedBounds>> OctreeNode::findEntitiesOverlapping(const AxisAlignedBounds& searchBounds) const {
    // If our bounds don't even overlap, return nothing
    if(!overlaps(getWorldBounds(), searchBounds)){
        return {};
    }

    // if the bounds being searched for encompass us, return all our members
    if(contains(getWorldBounds(), searchBounds)) {
        findAllMemberEntities();
    }

    std::vector<std::pair<EntityID, AxisAlignedBounds>> resultEntities {};

    // there's definitely some overlap, so test our entities against
    // the search region
    for(const auto& memberEntity: mEntities) {
        if(overlaps(searchBounds, memberEntity.second)) {
            resultEntities.push_back(memberEntity);
        }
    }

    // let the children find their own member entities that intersect with the search
    // bounds
    for(std::shared_ptr<OctreeNode> child: mChildren) {
        if(child && overlaps(searchBounds, child->getWorldBounds())) {
            std::vector<std::pair<EntityID, AxisAlignedBounds>> childEntities { child->findEntitiesOverlapping(searchBounds) };
            resultEntities.insert(resultEntities.cend(), childEntities.begin(), childEntities.end());
        }
    }

    return resultEntities;
}

std::vector<std::pair<EntityID, AxisAlignedBounds>> OctreeNode::findEntitiesOverlapping(const Ray& searchRay) const {
    if(!overlaps(searchRay, mWorldBounds)) return {};

    std::vector<std::pair<EntityID, AxisAlignedBounds>> resultEntities {};

    // there's definitely some overlap, so test our entities against
    // the search region
    for(const auto& memberEntity: mEntities) {
        if(overlaps(searchRay, memberEntity.second)) {
            resultEntities.push_back(memberEntity);
        }
    }

    // let the children find their own member entities that intersect with the search
    // bounds
    for(std::shared_ptr<OctreeNode> child: mChildren) {
        if(child && overlaps(searchRay, child->getWorldBounds())) {
            std::vector<std::pair<EntityID, AxisAlignedBounds>> childEntities { child->findEntitiesOverlapping(searchRay) };
            resultEntities.insert(resultEntities.cend(), childEntities.begin(), childEntities.end());
        }
    }

    return resultEntities;
}


void OctreeNode::shrinkTreeAndBecomeRoot() {
    mAddress = kNoAddress;
    mParent.reset();

    // recompute all descendant addresses
    std::queue<std::shared_ptr<OctreeNode>> toVisit { {shared_from_this()} };
    while(!toVisit.empty()) {
        std::shared_ptr<OctreeNode> octreeNode { toVisit.front() };
        toVisit.pop();
        for(Octant octant { 0 }; octant < mChildren.size(); ++octant) {
            if(!octreeNode->mChildren[octant]) continue;

            toVisit.push(octreeNode->mChildren[octant]);
            octreeNode->mChildren[octant]->mAddress = MakeAddress(octant, octreeNode->mAddress);
        }
    }
}

std::shared_ptr<OctreeNode> OctreeNode::GrowTreeAndCreateRoot (
    std::shared_ptr<OctreeNode> oldRoot,
    const AxisAlignedBounds& regionToCover
) {
    assert(
        contains(oldRoot->getWorldBounds(), regionToCover)
        && "Region to cover must be larger than the world bounds of the current Octree"
    );
    std::vector<AxisAlignedBounds> expandedWorldBounds {};
    std::vector<Octant> growthSteps {};
    AxisAlignedBounds newWorldBounds { oldRoot->getWorldBounds() };

    // greedily expand new world bounds in doubles of old world bounds until
    // the new region specified is completely contained, tracking growth steps 
    // and intermediate world bounds along the way
    while(!contains(regionToCover, newWorldBounds)) {
        const AxisAlignedBounds::Extents oldExtents { newWorldBounds.getAxisAlignedBoxExtents() };
        const glm::vec3 oldDimensions { newWorldBounds.getDimensions() };

        // Determine growth direction by seeing which dimensions is most in need of being
        // expanded towards.
        const glm::vec3 diffPositive { 
            glm::clamp(
                regionToCover.getAxisAlignedBoxExtents().first - oldExtents.first,
                glm::vec3{0.f},
                glm::vec3{std::numeric_limits<float>::infinity()}
            )
        };
        const glm::vec3 diffNegative {
            glm::clamp(
                oldExtents.second - regionToCover.getAxisAlignedBoxExtents().second,
                glm::vec3{0.f},
                glm::vec3{std::numeric_limits<float>::infinity()}
            )
        };
        const Octant growthDirection { static_cast<Octant> (
            (diffPositive.x >= diffNegative.x? OctantSpecifier::RIGHT: 0)
            | (diffPositive.y >= diffNegative.y? OctantSpecifier::TOP: 0)
            | (diffPositive.z >= diffNegative.z? OctantSpecifier::FRONT: 0)
        )};

        // Compute and store the AABB associated with this growth
        AxisAlignedBounds::Extents newExtents {newWorldBounds.getAxisAlignedBoxExtents()};
        growthDirection&OctantSpecifier::RIGHT? newExtents.first.x = oldExtents.second.x + 2.f * oldDimensions.x: newExtents.second.x = oldExtents.first.x - 2.f * oldDimensions.x;
        growthDirection&OctantSpecifier::TOP? newExtents.first.y = oldExtents.second.y + 2.f * oldDimensions.y: newExtents.second.y = oldExtents.first.y - 2.f * oldDimensions.y;
        growthDirection&OctantSpecifier::FRONT? newExtents.first.z = oldExtents.second.z + 2.f * oldDimensions.z: newExtents.second.z = oldExtents.first.z - 2.f * oldDimensions.z;
        expandedWorldBounds.push_back(AxisAlignedBounds{ newExtents });
        growthSteps.push_back(growthDirection);

        // update new world bounds for the next iteration
        newWorldBounds = expandedWorldBounds.back();
        assert(newWorldBounds.isSensible() && "Bounds expanded beyond maximum supported value for this type");
    }

    assert(contains(regionToCover, newWorldBounds) && "Expansion step has failed, and the newly coputed node's world bounds does not contain all entities");
    const std::size_t depthAdded { expandedWorldBounds.size() };
    if(!depthAdded) return oldRoot;

    // Create a new Octree root
    std::shared_ptr<OctreeNode> newRootNode { CreateRootNode(oldRoot->mSubdivisionThreshold, expandedWorldBounds.back()) };
    expandedWorldBounds.pop_back();

    // Add children to the octree until we reach the descendant with the same dimensions
    // as the old Octree root (or we run out of available depth)
    Depth currentDepth { 1 };
    std::shared_ptr<OctreeNode> parentNode { newRootNode };
    while(currentDepth <= kMaxDepthInclusive && !expandedWorldBounds.empty()) {
        newWorldBounds = expandedWorldBounds.back();
        expandedWorldBounds.pop_back();
        Octant currentOctant { ToOctant(growthSteps.back()) };
        growthSteps.pop_back();

        parentNode->mChildren[currentOctant] = std::shared_ptr<OctreeNode>(new OctreeNode {
            MakeAddress(currentOctant, parentNode->mAddress),
            parentNode->mSubdivisionThreshold,
            newWorldBounds,
            parentNode
        });

        parentNode = parentNode->mChildren[currentOctant];
        ++currentDepth;
    }

    // edge case: maximum depth reached before integration of the old tree 
    // became possible. Hand over all old octree's member entities to the leafmost 
    // node of the new Octree
    if(currentDepth == kMaxDepthInclusive) {
        for(std::pair<EntityID, AxisAlignedBounds> memberEntity: oldRoot->findAllMemberEntities()) {
            parentNode->mEntities.insert(memberEntity);
        }
        return newRootNode;
    }
    assert(currentDepth < kMaxDepthInclusive && "Current depth cannot have reached or exceeded the maximum allowable depth beyond this point");
    assert(growthSteps.size() == 1 && "Growth steps vector should have exactly one element remaining corresponding to the growth direction \
        from the old Octree root node");

    // Integrate old root node as child of deepest node of the new octree
    Octant oldRootsNewOctant { ToOctant(growthSteps.back()) };
    growthSteps.pop_back();
    parentNode->mChildren[oldRootsNewOctant] = oldRoot;
    oldRoot->mAddress = MakeAddress(oldRootsNewOctant, parentNode->mAddress);
    oldRoot->mParent = parentNode;

    // edge case where old root is situated exactly at kMaxDepthInclusive. In this
    // case, move all member entities from descendants into the old root node
    if(oldRoot->getDepth() == kMaxDepthInclusive) {
        for(std::pair<EntityID, AxisAlignedBounds> memberEntity: oldRoot->findAllMemberEntities()) {
            oldRoot->mEntities.insert(memberEntity);
        }
        return newRootNode;
    }

    // integrate the old root node's descendants
    std::queue<std::shared_ptr<OctreeNode>> toVisit {};
    for(std::shared_ptr<OctreeNode> child: oldRoot->mChildren) {
        if(child) { toVisit.push(child); }
    }
    while(!toVisit.empty() && toVisit.front()->getDepth() < kMaxDepthInclusive) {
        std::shared_ptr<OctreeNode> currentNode { toVisit.front() };
        toVisit.pop();
        for(std::shared_ptr<OctreeNode> child: currentNode->mChildren) {
            if(child) { toVisit.push(child); }
        }
        parentNode = currentNode->mParent.lock();
        currentNode->mAddress = MakeAddress(currentNode->getOctant(), currentNode->mParent.lock()->getAddress());
    }

    // max depth reached . Put all descendant owned entities into leaf 
    // nodes, i.e., the nodes remaining in `toVisit`
    while(!toVisit.empty()) {
        std::shared_ptr<OctreeNode> currentNode { toVisit.front() };
        toVisit.pop();
        assert(currentNode->getDepth() == kMaxDepthInclusive && "If execution entered this block, it should mean that the maximum depth was reached");

        parentNode = currentNode->mParent.lock();
        currentNode->mAddress = MakeAddress(currentNode->getOctant(), currentNode->mParent.lock()->getAddress());

        for(std::pair<EntityID, AxisAlignedBounds> memberEntity: currentNode->findAllMemberEntities()) {
            currentNode->mEntities.insert(memberEntity);
        }
    }

    return newRootNode;
}

uint8_t OctreeNode::getChildCount() const {
    uint8_t childCount { 0 };
    for(std::shared_ptr<OctreeNode> child: mChildren) {
        if(child) ++childCount;
    }
    return childCount;
}

void Octree::insertEntity(EntityID entityID, const AxisAlignedBounds& entityWorldBounds) {
    assert(entityID < kMaxEntities && "Entity with an invalid id cannot be inserted into the octree.");
    assert(mEntityAddresses.find(entityID) == mEntityAddresses.end() && "This entity already exists in the octree. \
        Please remove it before attempting to reinsert it into the octree.");

    // if this entity is covered by the span already present in our
    // world, we merely pass it along to the root node
    if(contains(entityWorldBounds, mRootNode->getWorldBounds())) {
        mEntityAddresses[entityID] = mRootNode->insertEntity(entityID, entityWorldBounds);
        // printOctreeNodeMemberDetails(entityWorldBounds, mEntityAddresses[entityID], entityID);
        return;
    }


    // otherwise, our octree must grow
    std::shared_ptr<OctreeNode> newRootNode {
        OctreeNode::GrowTreeAndCreateRoot(mRootNode, entityWorldBounds + mRootNode->getWorldBounds())
    };
    assert(newRootNode != mRootNode && "An octree expansion should have taken place by now. That it hasn't means something has gone horribly wrong.");
    std::shared_ptr<OctreeNode> oldOctreeContainerNode { newRootNode->getSmallestNodeContaining(mRootNode->getWorldBounds()) };

    // edge case: nothing of the old octree remains in the new expanded
    // octree. Simply update all cached addresses with that of the leafmost 
    // container node
    if(oldOctreeContainerNode != mRootNode) {
        const OctreeNode::Address newAddress { oldOctreeContainerNode->getAddress() };
        // update all cached entity addresses
        for(
            auto addressIter { mEntityAddresses.begin() }, endIter { mEntityAddresses.end() };
            addressIter != endIter;
            ++addressIter
        ) {
            addressIter->second = newAddress;
        }

    // some or all portions of the old octree have made it into the new one
    } else {
        // update all cached entity addresses
        const OctreeNode::Address oldRootNewAddress { mRootNode->getAddress() };
        for(
            auto addressIter { mEntityAddresses.begin() }, endIter { mEntityAddresses.end() };
            addressIter != endIter;
            ++addressIter
        ) {
            addressIter->second = OctreeNode::GrowAddress(addressIter->second, oldRootNewAddress);
        }
    }

    mRootNode = newRootNode;
    mEntityAddresses[entityID] = mRootNode->insertEntity(entityID, entityWorldBounds);

    // printOctreeNodeMemberDetails(entityWorldBounds, mEntityAddresses[entityID], entityID);
};

void Octree::removeEntity(EntityID entityID) {
    if(mEntityAddresses.find(entityID) == mEntityAddresses.end()) return;

    // attempt to remove the entity, starting our search for it at its last known
    // address
    mRootNode->removeEntity(entityID, mEntityAddresses[entityID]);
    mEntityAddresses.erase(entityID);

    std::shared_ptr<OctreeNode> candidateRoot {
        mRootNode->findCandidateRoot()
    };

    // Do nothing if no shrinkage is required
    if(candidateRoot == mRootNode) return;

    // otherwise, apply shrinkage and make the candidate our new root
    const OctreeNode::Depth depthRemoved { candidateRoot->getDepth() };
    candidateRoot->shrinkTreeAndBecomeRoot();
    for(
        auto addressIter { mEntityAddresses.begin() }, endIter { mEntityAddresses.end() };
        addressIter != endIter;
        ++addressIter
    ) {
        addressIter->second = OctreeNode::ShrinkAddress(addressIter->second, depthRemoved);
    }

    mRootNode = candidateRoot;
}

std::vector<std::pair<EntityID, AxisAlignedBounds>> Octree::findEntitiesOverlapping(const Ray& searchRay) const {
    std::vector<std::pair<EntityID, AxisAlignedBounds>> results{ mRootNode->findEntitiesOverlapping(searchRay) };
    std::sort(results.begin(), results.end(), [&searchRay](std::pair<EntityID, AxisAlignedBounds>& volumeOne, std::pair<EntityID, AxisAlignedBounds>& volumeTwo) {
        // the first intersection point for both objects
        const glm::vec3 intersectionOne { computeIntersections(searchRay, volumeOne.second).second.first - searchRay.mStart };
        const glm::vec3 intersectionTwo { computeIntersections(searchRay, volumeTwo.second).second.first - searchRay.mStart };
        const bool volumeOneContainsRayOrigin { contains(searchRay.mStart, volumeOne.second) };

        return (
            // if the ray starts either inside of or outside of both volumes, then the first ray 
            // to intersect the volume comes first
            (
                volumeOneContainsRayOrigin == contains(searchRay.mStart, volumeTwo.second)
                && glm::dot(intersectionOne, intersectionOne) < glm::dot(intersectionTwo, intersectionTwo)

            // otherwise, the volume containing the ray's origin takes priority 
            ) || volumeOneContainsRayOrigin
        );
    });
    return results;
}


#include "toymaker/engine/spatial_query/sweep_prune.hpp"

using namespace ToyMaker;

std::set<CollisionPair> SweepPrune::getCollisionPairs() const {
    return findCollisionPairs();
}

void SweepPrune::addObject(EntityID entity, const AxisAlignedBounds& bounds) {
    assert(mObjects.find(entity) == mObjects.end() && "Entity already registered with this table");
    assert(bounds.isSensible() && "These bounds aren't sensible.");

    mObjects[entity] = { .mIndices {{
        { mEdges[X].size(), mEdges[X].size() + 1 },
        { mEdges[Y].size(), mEdges[Y].size() + 1 },
        { mEdges[Z].size(), mEdges[Z].size() + 1 },
    }} };

    const AxisAlignedBounds::Extents extents {
        bounds.getAxisAlignedBoxExtents()
    };
    mEdges[X].push_back({
        .mEntity { entity },
        .mValue { extents.second.x },
        .mBegin { true },
    });
    mEdges[X].push_back({
        .mEntity { entity },
        .mValue { extents.first.x },
        .mBegin { false },
    });
    mEdges[Y].push_back({
        .mEntity { entity },
        .mValue { extents.second.y },
        .mBegin { true },
    });
    mEdges[Y].push_back({
        .mEntity { entity },
        .mValue { extents.first.y },
        .mBegin { false },
    });
    mEdges[Z].push_back({
        .mEntity { entity },
        .mValue { extents.second.z },
        .mBegin { true },
    });
    mEdges[Z].push_back({
        .mEntity { entity },
        .mValue { extents.first.z },
        .mBegin { false },
    });

    sortEdges();
}

void SweepPrune::removeObject(EntityID entity) {
    assert(mObjects.find(entity) != mObjects.cend() && "This entity is untracked by this table");

    // move the edges belonging to this entity to the end of their edge lists
    for(auto axis { 0 }; axis < 3; ++axis) {
        mEdges[axis][mObjects[entity].mIndices[axis].first].mValue = std::numeric_limits<float>::max();
        mEdges[axis][mObjects[entity].mIndices[axis].second].mValue = std::numeric_limits<float>::max();
    }

    sortEdges();

    // remove this entity and its edges
    mObjects.erase(entity);
    for(auto axis { 0 }; axis < 3; ++axis) {
        mEdges[axis].pop_back();
        mEdges[axis].pop_back();
    }
}

void SweepPrune::updateObject(EntityID entity, const AxisAlignedBounds& bounds) {
    assert(mObjects.find(entity) != mObjects.cend() && "This entity is untracked by this table");

    const AxisAlignedBounds::Extents extents {
        bounds.getAxisAlignedBoxExtents()
    };

    mEdges[X][mObjects[entity].mIndices[X].first].mValue = extents.second.x;
    mEdges[X][mObjects[entity].mIndices[X].second].mValue = extents.first.x;
    mEdges[Y][mObjects[entity].mIndices[Y].first].mValue = extents.second.y;
    mEdges[Y][mObjects[entity].mIndices[Y].second].mValue = extents.first.y;
    mEdges[Z][mObjects[entity].mIndices[Z].first].mValue = extents.second.z;
    mEdges[Z][mObjects[entity].mIndices[Z].second].mValue = extents.first.z;

    sortEdges();
}

void SweepPrune::sortEdges() {
    for(auto axis{0}; axis < 3; ++axis) {
        sortEdges(static_cast<Axis>(axis));
    }
}

void SweepPrune::sortEdges(Axis axis) {
    std::vector<Edge>& edges { mEdges[axis] };
    const std::size_t edgeCount { edges.size() };
    for(auto i { 0 }; i < edgeCount; ++i) {
        for(auto j { i }; j > 0; --j) {
            if(edges[j-1] < edges[j]) {
                break;
            }
            std::swap(edges[j-1], edges[j]);

            // update object table entries as we go
            if(edges[j-1].mBegin) {
                mObjects[edges[j-1].mEntity].mIndices[axis].first = j - 1;
            } else {
                mObjects[edges[j-1].mEntity].mIndices[axis].second = j - 1;
            }

            if(edges[j].mBegin) {
                mObjects[edges[j].mEntity].mIndices[axis].first = j;
            } else {
                mObjects[edges[j].mEntity].mIndices[axis].second = j;
            }
        }
    }
}

std::set<CollisionPair> SweepPrune::findCollisionPairs() const {
    std::set<CollisionPair> collisionPairs {};
    const std::array<std::set<CollisionPair>, 3> overlapping {{
        findOverlaps(X),
        findOverlaps(Y),
        findOverlaps(Z),
    }};
    for(const auto& pair: overlapping[X]) {
        if(
            overlapping[Z].find(pair) != overlapping[Z].cend()
            && overlapping[Y].find(pair) != overlapping[Z].cend()
        ) {
            collisionPairs.insert(pair);
        }
    }
    return collisionPairs;
}

std::set<CollisionPair> SweepPrune::findOverlaps(Axis axis) const {
    // list of entity pairs that overlap on this axis
    std::set<CollisionPair> overlapping {};

    // list of entities that aren't closed in the current iteration
    std::set<EntityID> connected {};

    for(const auto& edge: mEdges[axis]) {
        if(!edge.mBegin) {
            connected.erase(edge.mEntity);
            continue;
        }

        for(const auto other: connected) {
            overlapping.insert({edge.mEntity, other});
        }
        connected.insert(edge.mEntity);
    }

    assert(connected.empty() && "There are projections that haven't been closed");

    return overlapping;
}


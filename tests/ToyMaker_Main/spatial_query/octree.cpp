#include <doctest/doctest.h>

#include "toymaker/engine/spatial_query/octree.hpp"

TEST_SUITE("Octree Interaction Layers") {

TEST_CASE("2 objects, one octree node") {
    std::vector<ToyMaker::ObjectBounds> spheres { {
        ToyMaker::ObjectBounds::create(
            ToyMaker::VolumeSphere { .mRadius = 5.f },
            glm::vec3 { -1.f, 0.f, 0.f },
            glm::vec3 { 0.f },
            (1 << 0) // 0th interaction layer
        ),
        ToyMaker::ObjectBounds::create(
            ToyMaker::VolumeSphere { .mRadius = 5.f },
            glm::vec3 { 1.f, 0.f, 0.f },
            glm::vec3 { 0.f },
            (1 << 1) // 1st interaction layer
        )
    } };

    ToyMaker::Octree octree(3, ToyMaker::AxisAlignedBounds { spheres [0] } + ToyMaker::AxisAlignedBounds { spheres [1] });

    octree.insertEntity(0, spheres[0]);
    octree.insertEntity(1, spheres[1]);

    // Intersects both sphere 0 and 1
    const ToyMaker::Ray rightRay {
        .mStart { -100.f, 0.f, 0.f },
        .mDirection { 1.f, 0.f, 0.f },
        .mLength { 1e3 },
    };

    const std::vector<ToyMaker::Ray> forwardRays {{
        // only intersects sphere 0
        {
            .mStart { -5.f, 0.f, 10.f },
            .mDirection { 0.f, 0.f, -1.f },
            .mLength { 1e3 },
        },
        // only intersects sphere 1
        {
            .mStart { 5.f, 0.f, 10.f },
            .mDirection { 0.f, 0.f, -1.f },
            .mLength { 1e3 },
        },
    }};

    const ToyMaker::AxisAlignedBounds fullSearchRegion {
        ToyMaker::AxisAlignedBounds::Extents{ glm::vec3 { 5.f }, glm::vec3 { -5.f } }
    };
    const std::vector<ToyMaker::AxisAlignedBounds> searchRegions {{
        // only intersects sphere 0
        ToyMaker::AxisAlignedBounds::Extents {
            glm::vec3 { -5.f, 5.f, 5.f },
            glm::vec3 { -6.f, -5.f, -5.f }
        },
        // only intersects sphere 1
        ToyMaker::AxisAlignedBounds::Extents {
            glm::vec3 { 6.f, 5.f, 5.f },
            glm::vec3 { 5.f, -5.f, -5.f }
        },
    }};

    const std::vector<ToyMaker::AxisAlignedBounds> irrelevantEntities {
        GENERATE(
            // no other entities in octree
            std::vector<ToyMaker::AxisAlignedBounds>{},

            // one other entity in same region of octree
            std::vector<ToyMaker::AxisAlignedBounds> {
                {
                    fullSearchRegion.getAxisAlignedBoxExtents(),
                    0 // Unsearchable
                }
            },

            // entity outside of current octree bounds
            std::vector<ToyMaker::AxisAlignedBounds> {
                ToyMaker::AxisAlignedBounds {
                    ToyMaker::AxisAlignedBounds::Extents {
                        glm::vec3 { 1000.f }, glm::vec3 { 999.f }
                    },
                    0 // Unsearchable
                },
            },

            // entities that cause breaking out of suboctant
            std::vector<ToyMaker::AxisAlignedBounds> {
                ToyMaker::AxisAlignedBounds {
                    ToyMaker::AxisAlignedBounds::Extents {
                        glm::vec3 { 5.f }, glm::vec3 { 4.f }
                    },
                    0 // Unsearchable
                },
                ToyMaker::AxisAlignedBounds {
                    ToyMaker::AxisAlignedBounds::Extents {
                        glm::vec3 { 5.f }, glm::vec3 { 4.f }
                    },
                    0 // Unsearchable
                },
                ToyMaker::AxisAlignedBounds {
                    ToyMaker::AxisAlignedBounds::Extents {
                        glm::vec3 { 5.f }, glm::vec3 { 4.f }
                    },
                    0 // Unsearchable
                },
                ToyMaker::AxisAlignedBounds {
                    ToyMaker::AxisAlignedBounds::Extents {
                        glm::vec3 { 5.f }, glm::vec3 { 4.f }
                    },
                    0 // Unsearchable
                },
                ToyMaker::AxisAlignedBounds {
                    ToyMaker::AxisAlignedBounds::Extents {
                        glm::vec3 { 5.f }, glm::vec3 { 4.f }
                    },
                    0 // Unsearchable
                },
            }
        )
    };

    const std::size_t nEntitiesTotal { spheres.size() + irrelevantEntities.size() };

    // populate octree with entities irrelevant to our search, which
    // nonetheless modify the octree in different ways
    for(std::size_t i { 0 }; i < irrelevantEntities.size(); ++i) {
        octree.insertEntity(spheres.size() + i, irrelevantEntities[i]);
    }

    // There should be one entity each on layer 0 and 1
    SUBCASE("Different Layers") {
        // by default we find all entities
        CHECK(octree.findAllMemberEntities().size() == 2);
        // only select member entities belonging to a particular layer
        CHECK(octree.findAllMemberEntities(1<<0).size() == 1);
        CHECK(octree.findAllMemberEntities(1<<0)[0].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findAllMemberEntities(1<<0)[0].first == 0);
        CHECK(octree.findAllMemberEntities(1<<1).size() == 1);
        CHECK(octree.findAllMemberEntities(1<<1)[0].second.getInteractionLayers() == (1<<1));
        CHECK(octree.findAllMemberEntities(1<<1)[0].first == 1);

        // find all entities overlapping a ray
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay).size() == 2);
        // ... belonging to layer 0,
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay, 1<<0).size() == 1);
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay, 1<<0)[0].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay, 1<<0)[0].first == 0);
        // ... belonging to layer 1
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay, 1<<1).size() == 1);
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay, 1<<1)[0].second.getInteractionLayers() == (1<<1));
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay, 1<<1)[0].first == 1);
        // ... touching only object 0
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[0]).size() == 1);
        //     ... searching in layer 0,
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[0], 1<<0).size() == 1);
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[0], 1<<0)[0].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[0], 1<<0)[0].first == 0);
        //     ... searching in layer 1
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[0], 1<<1).size() == 0);
        // ... touching only object 1
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[1]).size() == 1);
        //     ... searching in layer 0,
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[1], 1<<0).size() == 0);
        //     ... searching in layer 1
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[1], 1<<1).size() == 1);
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[1], 1<<1)[0].second.getInteractionLayers() == (1<<1));
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[1], 1<<1)[0].first == 1);

        // find all entities overlapping a region
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion).size() == 2);
        // ... belonging to layer 0
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion, 1<<0).size() == 1);
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion, 1<<0)[0].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion, 1<<0)[0].first == 0);
        // ... belonging to layer 1
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion, 1<<1).size() == 1);
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion, 1<<1)[0].second.getInteractionLayers() == (1<<1));
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion, 1<<1)[0].first == 1);
        // ... touching only object 0
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[0]).size() == 1);
        //     ... searching in layer 0
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[0], (1<<0)).size() == 1);
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[0], (1<<0))[0].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[0], (1<<0))[0].first == 0);
        //     ... searching in layer 1
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[0], (1<<1)).size() == 0);
        // ... touching only object 1
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[1]).size() == 1);
        //     ... searching in layer 0
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[1], (1<<0)).size() == 0);
        //     ... searching in layer 1
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[1], (1<<1)).size() == 1);
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[1], (1<<1))[0].second.getInteractionLayers() == (1<<1));
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[1], (1<<1))[0].first == 1);
    }


    SUBCASE("Same Layer") {
        // Put both spheres on interaction layer 0
        octree.removeEntity(1);
        spheres[1].setInteractionLayers(1<<0);
        octree.insertEntity(1, spheres[1]);

        CHECK(octree.findAllMemberEntities(1<<0).size() == 2);
        CHECK(octree.findAllMemberEntities(1<<0)[0].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findAllMemberEntities(1<<0)[1].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findAllMemberEntities(1<<0)[0].first == 0);
        CHECK(octree.findAllMemberEntities(1<<0)[1].first == 1);
        CHECK(octree.findAllMemberEntities(1<<1).size() == 0);

        // find all entities overlapping a ray
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay).size() == 2);
        // ... belonging to layer 0,
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay, 1<<0).size() == 2);
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay, 1<<0)[0].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay, 1<<0)[0].first == 0);
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay, 1<<0)[1].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay, 1<<0)[1].first == 1);
        // ... belonging to layer 1
        CHECK(octree.findEntitiesOverlappingCoarse(rightRay, 1<<1).size() == 0);
        // ... touching only object 0
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[0]).size() == 1);
        //     ... searching in layer 0,
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[0], 1<<0).size() == 1);
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[0], 1<<0)[0].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[0], 1<<0)[0].first == 0);
        //     ... searching in layer 1
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[0], 1<<1).size() == 0);
        // ... touching only object 1
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[1]).size() == 1);
        //     ... searching in layer 0,
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[1], 1<<0).size() == 1);
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[1], 1<<0)[0].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[1], 1<<0)[0].first == 1);
        //     ... searching in layer 1
        CHECK(octree.findEntitiesOverlappingCoarse(forwardRays[1], 1<<1).size() == 0);

        // find all entities overlapping a region
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion).size() == 2);
        // ... belonging to layer 0
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion, 1<<0).size() == 2);
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion, 1<<0)[0].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion, 1<<0)[0].first == 0);
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion, 1<<0)[1].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion, 1<<0)[1].first == 1);
        // ... belonging to layer 1
        CHECK(octree.findEntitiesOverlappingCoarse(fullSearchRegion, 1<<1).size() == 0);
        // ... touching only object 0
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[0]).size() == 1);
        //     ... searching in layer 0
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[0], (1<<0)).size() == 1);
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[0], (1<<0))[0].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[0], (1<<0))[0].first == 0);
        //     ... searching in layer 1
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[0], (1<<1)).size() == 0);
        // ... touching only object 1
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[1]).size() == 1);
        //     ... searching in layer 0
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[1], (1<<0)).size() == 1);
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[1], (1<<0))[0].second.getInteractionLayers() == (1<<0));
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[1], (1<<0))[0].first == 1);
        //     ... searching in layer 1
        CHECK(octree.findEntitiesOverlappingCoarse(searchRegions[1], (1<<1)).size() == 0);
    }
}

}


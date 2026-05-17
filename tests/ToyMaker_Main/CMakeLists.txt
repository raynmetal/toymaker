target_sources(TestDriver
    PRIVATE
        spatial_query/aabb.cpp
        spatial_query/ray_intersections.cpp
        spatial_query/object_overlaps.cpp
        spatial_query/octree.cpp
)

target_link_libraries(TestDriver
    PRIVATE
        ToyMaker
)

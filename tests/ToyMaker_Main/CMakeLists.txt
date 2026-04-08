target_sources(TestDriver
    PRIVATE
        spatial_query/aabb.cpp
        spatial_query/ray_intersections.cpp
)

target_link_libraries(TestDriver
    PRIVATE
        ToyMaker
)
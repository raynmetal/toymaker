target_sources(TestDriver
    PRIVATE
        spatial_query_math.cpp
)

target_link_libraries(TestDriver
    PRIVATE
        ToyMaker
)
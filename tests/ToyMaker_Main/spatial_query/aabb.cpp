#include "toymaker/engine/spatial_query_math.hpp"
#include "string_conversions.hpp"
#include <doctest/doctest.h>
#include <cmath>

namespace ToyMakerTests {

TEST_SUITE("Spatial Queries") {

TEST_CASE("AABB Object Bounds Encapsulation") {
    SUBCASE("Sphere at Zero") {
        ToyMaker::ObjectBounds sphere {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeSphere {
                    .mRadius { 5.f },
                },
                glm::vec3 { 0.f },
                glm::vec3 { 0.f }
            )
        };
        ToyMaker::AxisAlignedBounds enclosingAABB { sphere };
        const bool isSphereContained { ToyMaker::contains(sphere, enclosingAABB) };
        CHECK(isSphereContained);
    }
}

}

}
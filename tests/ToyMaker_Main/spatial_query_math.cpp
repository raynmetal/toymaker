#include "toymaker/engine/spatial_query_math.hpp"
#include "string_conversions.hpp"
#include <doctest/doctest.h>
#include <cmath>

namespace ToyMakerTests {

TEST_SUITE("Spatial Queries") {
    TEST_CASE("Finite Ray and Object Oriented Box Intersection") {
        ToyMaker::Ray ray {
            .mStart { 0.f, 0.f, 0.f },
            .mDirection { 0.f, 0.f, -1.f },
            .mLength { 100.f },
        };
        REQUIRE(ray.mLength == 100.f);
        REQUIRE(ray.mDirection.z == -1.f);
        REQUIRE(glm::length(ray.mStart) == 0);

        SUBCASE("Completely Inside") {
            const ToyMaker::ObjectBounds box {
                ToyMaker::ObjectBounds::create(
                    ToyMaker::VolumeBox {
                        .mDimensions { 200.f, 200.f, 200.f }
                    },
                    glm::vec3 { 0.f, 0.f, -50.f },
                    glm::vec3 { 0.f, 0.f, 0.f }
                )
            };
            REQUIRE(box.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -50.f});
            REQUIRE(box.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

            const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
                ToyMaker::computeIntersections(ray, box)
            };
            CHECK(intersectionResults.first == 0);
            CHECK(intersectionResults.second.first == glm::vec3 { std::numeric_limits<float>::infinity() });
            CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

            const bool startOverlaps { ToyMaker::overlaps(ray.mStart, box) };
            CHECK(startOverlaps == true);

            const bool endOverlaps {
                ToyMaker::overlaps(
                    ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                    box
                )
            };
            CHECK(endOverlaps == true);

            const bool overlaps { ToyMaker::overlaps(ray, box) };
            CHECK(overlaps == true);

            const bool contains { ToyMaker::contains(ray, box) };
            CHECK(contains == true);
        }

        SUBCASE("Completely Outside") {
            const ToyMaker::ObjectBounds box {
                ToyMaker::ObjectBounds::create(
                    ToyMaker::VolumeBox {
                        .mDimensions { 10.f, 10.f, 10.f }
                    },
                    glm::vec3 { 0.f, 50.f, -50.f },
                    glm::vec3 { 0.f, 0.f, 0.f }
                )
            };
            REQUIRE(box.getComputedWorldPosition() == glm::vec3{ 0.f, 50.f, -50.f});
            REQUIRE(box.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

            const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
                ToyMaker::computeIntersections(ray, box)
            };
            CHECK(intersectionResults.first == 0);
            CHECK(intersectionResults.second.first == glm::vec3 { std::numeric_limits<float>::infinity() });
            CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

            const bool startOverlaps { ToyMaker::overlaps(ray.mStart, box) };
            CHECK(startOverlaps == false);

            const bool endOverlaps {
                ToyMaker::overlaps(
                    ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                    box
                )
            };
            CHECK(endOverlaps == false);

            const bool overlaps { ToyMaker::overlaps(ray, box) };
            CHECK(overlaps == false);

            const bool contains { ToyMaker::contains(ray, box) };
            CHECK(contains == false);
        }

        SUBCASE("Starts Inside") {
            const ToyMaker::ObjectBounds box {
                ToyMaker::ObjectBounds::create(
                    ToyMaker::VolumeBox {
                        .mDimensions { 10.f, 10.f, 10.f }
                    },
                    glm::vec3 { 0.f, 0.f, 0.f },
                    glm::vec3 { 0.f, 0.f, 0.f }
                )
            };
            REQUIRE(box.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, 0.f});
            REQUIRE(box.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

            const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
                ToyMaker::computeIntersections(ray, box)
            };
            CHECK(intersectionResults.first == 1);
            CHECK(intersectionResults.second.first == glm::vec3 { 0.f, 0.f, -5.f });
            CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

            const bool startOverlaps { ToyMaker::overlaps(ray.mStart, box) };
            CHECK(startOverlaps == true);

            const bool endOverlaps {
                ToyMaker::overlaps(
                    ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                    box
                )
            };
            CHECK(endOverlaps == false);

            const bool overlaps { ToyMaker::overlaps(ray, box) };
            CHECK(overlaps == true);

            const bool contains { ToyMaker::contains(ray, box) };
            CHECK(contains == false);
        }

        SUBCASE("Ends Inside") {
            const ToyMaker::ObjectBounds box {
                ToyMaker::ObjectBounds::create(
                    ToyMaker::VolumeBox {
                        .mDimensions { 10.f, 10.f, 10.f }
                    },
                    glm::vec3 { 0.f, 0.f, -100.f },
                    glm::vec3 { 0.f, 0.f, 0.f }
                )
            };
            REQUIRE(box.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -100.f});
            REQUIRE(box.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

            const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
                ToyMaker::computeIntersections(ray, box)
            };
            CHECK(intersectionResults.first == 1);
            CHECK(intersectionResults.second.first == glm::vec3 { 0.f, 0.f, -95.f });
            CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

            const bool startOverlaps { ToyMaker::overlaps(ray.mStart, box) };
            CHECK(startOverlaps == false);

            const bool endOverlaps {
                ToyMaker::overlaps(
                    ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                    box
                )
            };
            CHECK(endOverlaps == true);

            const bool overlaps { ToyMaker::overlaps(ray, box) };
            CHECK(overlaps == true);

            const bool contains { ToyMaker::contains(ray, box) };
            CHECK(contains == false);
        }

        SUBCASE("Passes Through") {
            const ToyMaker::ObjectBounds box {
                ToyMaker::ObjectBounds::create(
                    ToyMaker::VolumeBox {
                        .mDimensions { 10.f, 10.f, 10.f }
                    },
                    glm::vec3 { 0.f, 0.f, -50.f },
                    glm::vec3 { 0.f, 0.f, 0.f }
                )
            };
            REQUIRE(box.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -50.f});
            REQUIRE(box.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

            const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
                ToyMaker::computeIntersections(ray, box)
            };
            CHECK(intersectionResults.first == 2);
            CHECK(intersectionResults.second.first == glm::vec3 { 0.f, 0.f, -45.f });
            CHECK(intersectionResults.second.second == glm::vec3 { 0.f, 0.f, -55.f });

            const bool startOverlaps { ToyMaker::overlaps(ray.mStart, box) };
            CHECK(startOverlaps == false);

            const bool endOverlaps {
                ToyMaker::overlaps(
                    ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                    box
                )
            };
            CHECK(endOverlaps == false);

            const bool overlaps { ToyMaker::overlaps(ray, box) };
            CHECK(overlaps == true);

            const bool contains { ToyMaker::contains(ray, box) };
            CHECK(contains == false);
        }
    }
}

}
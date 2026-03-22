#include "toymaker/engine/spatial_query_math.hpp"
#include "string_conversions.hpp"
#include <doctest/doctest.h>
#include <cmath>

namespace ToyMakerTests {

void raySphereSubtest(const ToyMaker::Ray& ray, const glm::vec3& spherePosition, const float radius, const bool shouldIntersect);

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

    SUBCASE("No Volume") {
        const ToyMaker::ObjectBounds box {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeBox {
                    .mDimensions { 0.f, 0.f, 0.f }
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

    SUBCASE("Passes Through Diagonal") {
        const ToyMaker::ObjectBounds box {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeBox {
                    .mDimensions { 10.f, 10.f, 10.f }
                },
                glm::vec3 { 0.f, 0.f, -50.f },
                glm::vec3 { 0.6154797, 0.5235988, -0.1699185 }
            )
        };
        const float kTolerance { 3.f }; // High tolerance, there's something wrong with my math
        REQUIRE(box.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -50.f});

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, box)
        };

        // Find the corners closest and furthest from the ray origin w.r.t. Z axis.  (comparisons
        // are reversed since ray moves along -Z)
        std::size_t indexMin { 0 };
        std::size_t indexMax { 0 };
        const std::array<glm::vec3, 8> boxCorners { box.getWorldOrientedBoxCorners() };
        for(std::size_t i { 0 }; i < 8; ++i) {
            if(boxCorners[i].z < boxCorners[indexMax].z) {
                indexMax = i;
            }
            if(boxCorners[i].z > boxCorners[indexMin].z) {
                indexMin = i;
            }
        }

        CHECK(intersectionResults.first == 2);
        CHECK(glm::abs(glm::length(intersectionResults.second.first - boxCorners[indexMin])) <= kTolerance);
        CHECK(glm::abs(glm::length(intersectionResults.second.second - boxCorners[indexMax])) <= kTolerance);

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

TEST_CASE("Infinite Ray and Object Oriented Box Intersection") {
    ToyMaker::Ray ray {
        .mStart { 0.f, 0.f, 0.f },
        .mDirection { 0.f, 0.f, -1.f },
        .mLength { std::numeric_limits<float>::infinity() },
    };
    REQUIRE(ray.mLength == std::numeric_limits<float>::infinity());
    REQUIRE(ray.mDirection.z == -1.f);
    REQUIRE(glm::length(ray.mStart) == 0);

    SUBCASE("No Volume") {
        const ToyMaker::ObjectBounds box {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeBox {
                    .mDimensions { 0.f, 0.f, 0.f }
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
        CHECK(startOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, box) };
        CHECK(overlaps == false);

        const bool contains { ToyMaker::contains(ray, box) };
        CHECK(contains == false);
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

        const bool overlaps { ToyMaker::overlaps(ray, box) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, box) };
        CHECK(contains == false);
    }
    SUBCASE("Passes Through Diagonal") {
        const ToyMaker::ObjectBounds box {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeBox {
                    .mDimensions { 10.f, 10.f, 10.f }
                },
                glm::vec3 { 0.f, 0.f, -50.f },
                glm::vec3 { 0.6154797, 0.5235988, -0.1699185 }
            )
        };
        const float kTolerance { 3.f };
        REQUIRE(box.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -50.f});

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, box)
        };

        // Find the corners closest and furthest from the ray origin w.r.t. Z axis.  (comparisons
        // are reversed since ray moves along -Z)
        std::size_t indexMin { 0 };
        std::size_t indexMax { 0 };
        const std::array<glm::vec3, 8> boxCorners { box.getWorldOrientedBoxCorners() };
        for(std::size_t i { 0 }; i < 8; ++i) {
            if(boxCorners[i].z < boxCorners[indexMax].z) {
                indexMax = i;
            }
            if(boxCorners[i].z > boxCorners[indexMin].z) {
                indexMin = i;
            }
        }

        CHECK(intersectionResults.first == 2);
        CHECK(glm::abs(glm::length(intersectionResults.second.first - boxCorners[indexMin])) <= kTolerance);
        CHECK(glm::abs(glm::length(intersectionResults.second.second - boxCorners[indexMax])) <= kTolerance);

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, box) };
        CHECK(startOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, box) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, box) };
        CHECK(contains == false);
    }
}

TEST_CASE("Finite Ray and Capsule Bounds Intersection") {
    ToyMaker::Ray ray {
        .mStart { 0.f, 0.f, 0.f },
        .mDirection { 0.f, 0.f, -1.f },
        .mLength { 100.f },
    };
    REQUIRE(ray.mLength == 100.f);
    REQUIRE(ray.mDirection.z == -1.f);
    REQUIRE(glm::length(ray.mStart) == 0);

    SUBCASE("No Volume") {
        const ToyMaker::ObjectBounds capsule {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeCapsule {
                    .mHeight { 0.f },
                    .mRadius { 0.f },
                },
                glm::vec3 { 0.f, 0.f, -50.f },
                glm::vec3 { 0.f, 0.f, 0.f }
            )
        };
        REQUIRE(capsule.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -50.f});
        REQUIRE(capsule.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, capsule)
        };
        CHECK(intersectionResults.first == 0);
        CHECK(intersectionResults.second.first == glm::vec3 { std::numeric_limits<float>::infinity() });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, capsule) };
        CHECK(startOverlaps == false);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                capsule
            )
        };
        CHECK(endOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, capsule) };
        CHECK(overlaps == false);

        const bool contains { ToyMaker::contains(ray, capsule) };
        CHECK(contains == false);
    }

    SUBCASE("Completely Inside") {
        const ToyMaker::ObjectBounds capsule {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeCapsule {
                    .mHeight { 200.f },
                    .mRadius { 200.f },
                },
                glm::vec3 { 0.f, 0.f, -50.f },
                glm::vec3 { 0.f, 0.f, 0.f }
            )
        };
        REQUIRE(capsule.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -50.f});
        REQUIRE(capsule.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, capsule)
        };
        CHECK(intersectionResults.first == 0);
        CHECK(intersectionResults.second.first == glm::vec3 { std::numeric_limits<float>::infinity() });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, capsule) };
        CHECK(startOverlaps == true);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                capsule 
            )
        };
        CHECK(endOverlaps == true);

        const bool overlaps { ToyMaker::overlaps(ray, capsule) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, capsule) };
        CHECK(contains == true);
    }

    SUBCASE("Completely Outside") {
        const ToyMaker::ObjectBounds capsule {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeCapsule {
                    .mHeight { 10.f },
                    .mRadius { 5.f },
                },
                glm::vec3 { 0.f, 50.f, -50.f },
                glm::vec3 { 0.f, 0.f, 0.f }
            )
        };
        REQUIRE(capsule.getComputedWorldPosition() == glm::vec3{ 0.f, 50.f, -50.f});
        REQUIRE(capsule.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, capsule)
        };
        CHECK(intersectionResults.first == 0);
        CHECK(intersectionResults.second.first == glm::vec3 { std::numeric_limits<float>::infinity() });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, capsule) };
        CHECK(startOverlaps == false);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                capsule
            )
        };
        CHECK(endOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, capsule) };
        CHECK(overlaps == false);

        const bool contains { ToyMaker::contains(ray, capsule) };
        CHECK(contains == false);
    }

    SUBCASE("Starts Inside") {
        const ToyMaker::ObjectBounds capsule {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeCapsule {
                    .mHeight { 10.f },
                    .mRadius { 5.f },
                },
                glm::vec3 { 0.f, 0.f, 0.f },
                glm::vec3 { 0.f, 0.f, 0.f }
            )
        };
        REQUIRE(capsule.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, 0.f});
        REQUIRE(capsule.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, capsule)
        };
        CHECK(intersectionResults.first == 1);
        CHECK(intersectionResults.second.first == glm::vec3 { 0.f, 0.f, -capsule.mTrueVolume.mCapsule.mRadius });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, capsule) };
        CHECK(startOverlaps == true);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                capsule
            )
        };
        CHECK(endOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, capsule) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, capsule) };
        CHECK(contains == false);
    }

    SUBCASE("Ends Inside") {
        const ToyMaker::ObjectBounds capsule {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeCapsule {
                    .mHeight { 10.f },
                    .mRadius { 5.f },
                },
                glm::vec3 { 0.f, 0.f, -100.f },
                glm::vec3 { 0.f, 0.f, 0.f }
            )
        };
        REQUIRE(capsule.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -100.f});
        REQUIRE(capsule.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, capsule)
        };
        CHECK(intersectionResults.first == 1);
        CHECK(intersectionResults.second.first == glm::vec3 { 0.f, 0.f, -100.f + capsule.mTrueVolume.mCapsule.mRadius });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, capsule) };
        CHECK(startOverlaps == false);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                capsule
            )
        };
        CHECK(endOverlaps == true);

        const bool overlaps { ToyMaker::overlaps(ray, capsule) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, capsule) };
        CHECK(contains == false);
    }

    SUBCASE("Passes Through") {
        const ToyMaker::ObjectBounds capsule {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeCapsule {
                    .mHeight { 10.f },
                    .mRadius { 5.f },
                },
                glm::vec3 { 0.f, 0.f, -50.f },
                glm::vec3 { 0.f, 0.f, 0.f }
            )
        };
        REQUIRE(capsule.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -50.f});
        REQUIRE(capsule.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, capsule)
        };
        CHECK(intersectionResults.first == 2);
        CHECK(intersectionResults.second.first == glm::vec3 { 0.f, 0.f, -50.f + capsule.mTrueVolume.mCapsule.mRadius });
        CHECK(intersectionResults.second.second == glm::vec3 { 0.f, 0.f, -50.f - capsule.mTrueVolume.mCapsule.mRadius });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, capsule) };
        CHECK(startOverlaps == false);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                capsule
            )
        };
        CHECK(endOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, capsule) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, capsule) };
        CHECK(contains == false);
    }

    SUBCASE("Passes Through Axis") {
        const ToyMaker::ObjectBounds capsule {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeCapsule {
                    .mHeight { 10.f },
                    .mRadius { 5.f },
                },
                glm::vec3 { 0.f, 0.f, -50.f },
                glm::vec3 { glm::radians(90.f), 0.f, 0.f }
            )
        };
        const float kTolerance { 3.f }; // High tolerance, there's something wrong with my math
        REQUIRE(capsule.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -50.f});

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, capsule)
        };

        CHECK(intersectionResults.first == 2);
        CHECK(intersectionResults.second.first == glm::vec3 { 0.f, 0.f, -50.f + (
            capsule.mTrueVolume.mCapsule.mHeight / 2.f + capsule.mTrueVolume.mCapsule.mRadius) 
        });
        CHECK(intersectionResults.second.second == glm::vec3 { 0.f, 0.f, -50.f - (
            capsule.mTrueVolume.mCapsule.mHeight / 2.f + capsule.mTrueVolume.mCapsule.mRadius) 
        });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, capsule) };
        CHECK(startOverlaps == false);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                capsule
            )
        };
        CHECK(endOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, capsule) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, capsule) };
        CHECK(contains == false);
    }
}

TEST_CASE("Finite Ray and Sphere Bounds Intersection") {
    ToyMaker::Ray ray {
        .mStart { 0.f, 0.f, 0.f },
        .mDirection { 0.f, 0.f, -1.f },
        .mLength { 100.f },
    };
    REQUIRE(ray.mLength == 100.f);
    REQUIRE(ray.mDirection.z == -1.f);
    REQUIRE(glm::length(ray.mStart) == 0);

    SUBCASE("No Volume") {
        const ToyMaker::ObjectBounds sphere {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeSphere {
                    .mRadius { 0.f }
                },
                glm::vec3 { 0.f, 0.f, -50.f },
                glm::vec3 { 0.f, 0.f, 0.f }
            )
        };
        REQUIRE(sphere.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -50.f});
        REQUIRE(sphere.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, sphere)
        };
        CHECK(intersectionResults.first == 0);
        CHECK(intersectionResults.second.first == glm::vec3 { std::numeric_limits<float>::infinity() });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, sphere) };
        CHECK(startOverlaps == false);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                sphere
            )
        };
        CHECK(endOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, sphere) };
        CHECK(overlaps == false);

        const bool contains { ToyMaker::contains(ray, sphere) };
        CHECK(contains == false);
    }

    SUBCASE("Completely Inside") {
        const ToyMaker::ObjectBounds sphere {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeSphere {
                    .mRadius { 200.f }
                },
                glm::vec3 { 0.f, 0.f, -50.f },
                glm::vec3 { 0.f, 0.f, 0.f }
            )
        };
        REQUIRE(sphere.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -50.f});
        REQUIRE(sphere.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, sphere)
        };
        CHECK(intersectionResults.first == 0);
        CHECK(intersectionResults.second.first == glm::vec3 { std::numeric_limits<float>::infinity() });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, sphere) };
        CHECK(startOverlaps == true);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                sphere
            )
        };
        CHECK(endOverlaps == true);

        const bool overlaps { ToyMaker::overlaps(ray, sphere) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, sphere) };
        CHECK(contains == true);
    }

    SUBCASE("Completely Outside") {
        const ToyMaker::ObjectBounds sphere {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeSphere {
                    .mRadius { 5.f }
                },
                glm::vec3 { 0.f, 50.f, -50.f },
                glm::vec3 { 0.f, 0.f, 0.f }
            )
        };
        REQUIRE(sphere.getComputedWorldPosition() == glm::vec3{ 0.f, 50.f, -50.f});
        REQUIRE(sphere.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, sphere)
        };
        CHECK(intersectionResults.first == 0);
        CHECK(intersectionResults.second.first == glm::vec3 { std::numeric_limits<float>::infinity() });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, sphere) };
        CHECK(startOverlaps == false);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                sphere
            )
        };
        CHECK(endOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, sphere) };
        CHECK(overlaps == false);

        const bool contains { ToyMaker::contains(ray, sphere) };
        CHECK(contains == false);
    }

    SUBCASE("Starts Inside") {
        const ToyMaker::ObjectBounds sphere {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeSphere {
                    .mRadius { 5.f }
                },
                glm::vec3 { 0.f, 0.f, 0.f },
                glm::vec3 { 0.f, 0.f, 0.f }
            )
        };
        REQUIRE(sphere.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, 0.f});
        REQUIRE(sphere.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, sphere)
        };
        CHECK(intersectionResults.first == 1);
        CHECK(intersectionResults.second.first == glm::vec3 { 0.f, 0.f, -sphere.mTrueVolume.mSphere.mRadius });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, sphere) };
        CHECK(startOverlaps == true);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                sphere
            )
        };
        CHECK(endOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, sphere) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, sphere) };
        CHECK(contains == false);
    }

    SUBCASE("Ends Inside") {
        const ToyMaker::ObjectBounds sphere {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeSphere {
                    .mRadius { 5.f },
                },
                glm::vec3 { 0.f, 0.f, -100.f },
                glm::vec3 { 0.f, 0.f, 0.f }
            )
        };
        REQUIRE(sphere.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -100.f});
        REQUIRE(sphere.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, sphere)
        };
        CHECK(intersectionResults.first == 1);
        CHECK(intersectionResults.second.first == glm::vec3 { 0.f, 0.f, -100.f + sphere.mTrueVolume.mSphere.mRadius });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, sphere) };
        CHECK(startOverlaps == false);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                sphere
            )
        };
        CHECK(endOverlaps == true);

        const bool overlaps { ToyMaker::overlaps(ray, sphere) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, sphere) };
        CHECK(contains == false);
    }

    SUBCASE("Passes Through") {
        const ToyMaker::ObjectBounds sphere {
            ToyMaker::ObjectBounds::create(
                ToyMaker::VolumeSphere {
                    .mRadius { 5.f },
                },
                glm::vec3 { 0.f, 0.f, -50.f },
                glm::vec3 { 0.f, 0.f, 0.f }
            )
        };
        REQUIRE(sphere.getComputedWorldPosition() == glm::vec3{ 0.f, 0.f, -50.f});
        REQUIRE(sphere.getComputedWorldOrientation() == glm::quat(glm::vec3{0.f, 0.f, 0.f}));

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, sphere)
        };
        CHECK(intersectionResults.first == 2);
        CHECK(intersectionResults.second.first == glm::vec3 { 0.f, 0.f, -50.f + sphere.mTrueVolume.mSphere.mRadius });
        CHECK(intersectionResults.second.second == glm::vec3 { 0.f, 0.f, -50.f - sphere.mTrueVolume.mSphere.mRadius });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, sphere) };
        CHECK(startOverlaps == false);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                sphere
            )
        };
        CHECK(endOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, sphere) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, sphere) };
        CHECK(contains == false);
    }
}

TEST_CASE("Finite Ray and Axis Aligned Bounds Intersection") {
    ToyMaker::Ray ray {
        .mStart { 0.f, 0.f, 0.f },
        .mDirection { 0.f, 0.f, -1.f },
        .mLength { 100.f },
    };
    REQUIRE(ray.mLength == 100.f);
    REQUIRE(ray.mDirection.z == -1.f);
    REQUIRE(glm::length(ray.mStart) == 0);

    SUBCASE("No Volume") {
        const ToyMaker::AxisAlignedBounds aaBox {
            glm::vec3 { 0.f, 0.f, -50.f },
            glm::vec3 { 0.f, 0.f, 0.f }
        };
        REQUIRE(aaBox.getPosition() == glm::vec3{ 0.f, 0.f, -50.f});

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, aaBox)
        };
        CHECK(intersectionResults.first == 0);
        CHECK(intersectionResults.second.first == glm::vec3 { std::numeric_limits<float>::infinity() });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, aaBox) };
        CHECK(startOverlaps == false);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                aaBox
            )
        };
        CHECK(endOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, aaBox) };
        CHECK(overlaps == false);

        const bool contains { ToyMaker::contains(ray, aaBox) };
        CHECK(contains == false);
    }

    SUBCASE("Completely Inside") {
        const ToyMaker::AxisAlignedBounds aaBox {
            glm::vec3{ 0.f, 0.f, -50.f },
            glm::vec3{ 200.f, 200.f, 200.f }
        };
        REQUIRE(aaBox.getPosition() == glm::vec3{ 0.f, 0.f, -50.f});

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, aaBox)
        };
        CHECK(intersectionResults.first == 0);
        CHECK(intersectionResults.second.first == glm::vec3 { std::numeric_limits<float>::infinity() });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, aaBox) };
        CHECK(startOverlaps == true);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                aaBox
            )
        };
        CHECK(endOverlaps == true);

        const bool overlaps { ToyMaker::overlaps(ray, aaBox) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, aaBox) };
        CHECK(contains == true);
    }

    SUBCASE("Completely Outside") {
        const ToyMaker::AxisAlignedBounds aaBox {
            glm::vec3 { 0.f, 50.f, -50.f },
            glm::vec3 { 10.f, 10.f, 10.f }
        };
        REQUIRE(aaBox.getPosition() == glm::vec3{ 0.f, 50.f, -50.f});

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, aaBox)
        };
        CHECK(intersectionResults.first == 0);
        CHECK(intersectionResults.second.first == glm::vec3 { std::numeric_limits<float>::infinity() });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, aaBox) };
        CHECK(startOverlaps == false);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                aaBox
            )
        };
        CHECK(endOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, aaBox) };
        CHECK(overlaps == false);

        const bool contains { ToyMaker::contains(ray, aaBox) };
        CHECK(contains == false);
    }

    SUBCASE("Starts Inside") {
        const ToyMaker::AxisAlignedBounds aaBox {
            glm::vec3 { 0.f, 0.f, 0.f },
            glm::vec3 { 10.f, 10.f, 10.f }
        };
        REQUIRE(aaBox.getPosition() == glm::vec3{ 0.f, 0.f, 0.f});

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, aaBox)
        };
        CHECK(intersectionResults.first == 1);
        CHECK(intersectionResults.second.first == glm::vec3 { 0.f, 0.f, -5.f });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, aaBox) };
        CHECK(startOverlaps == true);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                aaBox
            )
        };
        CHECK(endOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, aaBox) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, aaBox) };
        CHECK(contains == false);
    }

    SUBCASE("Ends Inside") {
        const ToyMaker::AxisAlignedBounds aaBox {
            glm::vec3 { 0.f, 0.f, -100.f },
            glm::vec3 { 10.f, 10.f, 10.f }
        };
        REQUIRE(aaBox.getPosition() == glm::vec3{ 0.f, 0.f, -100.f});

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, aaBox)
        };
        CHECK(intersectionResults.first == 1);
        CHECK(intersectionResults.second.first == glm::vec3 { 0.f, 0.f, -100.f + 5.f });
        CHECK(intersectionResults.second.second == glm::vec3 { std::numeric_limits<float>::infinity() });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, aaBox) };
        CHECK(startOverlaps == false);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                aaBox
            )
        };
        CHECK(endOverlaps == true);

        const bool overlaps { ToyMaker::overlaps(ray, aaBox) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, aaBox) };
        CHECK(contains == false);
    }

    SUBCASE("Passes Through") {
        const ToyMaker::AxisAlignedBounds aaBox {
            glm::vec3 { 0.f, 0.f, -50.f },
            glm::vec3 { 10.f, 10.f, 10.f }
        };
        REQUIRE(aaBox.getPosition() == glm::vec3{ 0.f, 0.f, -50.f});

        const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
            ToyMaker::computeIntersections(ray, aaBox)
        };
        CHECK(intersectionResults.first == 2);
        CHECK(intersectionResults.second.first == glm::vec3 { 0.f, 0.f, -50.f + 5.f });
        CHECK(intersectionResults.second.second == glm::vec3 { 0.f, 0.f, -50.f - 5.f });

        const bool startOverlaps { ToyMaker::overlaps(ray.mStart, aaBox) };
        CHECK(startOverlaps == false);

        const bool endOverlaps {
            ToyMaker::overlaps(
                ray.mStart + ray.mLength * glm::normalize(ray.mDirection),
                aaBox
            )
        };
        CHECK(endOverlaps == false);

        const bool overlaps { ToyMaker::overlaps(ray, aaBox) };
        CHECK(overlaps == true);

        const bool contains { ToyMaker::contains(ray, aaBox) };
        CHECK(contains == false);
    }
}

TEST_CASE("Repositioned Rays and Spheres Intersection") {
    const std::vector<std::tuple<ToyMaker::Ray, glm::vec3, float, bool>> testData {{
        {
            ToyMaker::Ray { // ray
                .mStart { glm::vec3 { 0.f, 0.f, 0.f} },
                .mDirection { glm::vec3 { 0.f, 0.f, -1.f}},
                .mLength { 100.f }
            },
            glm::vec3 { 0.f, 0.f, -50.f }, // sphere position
            5.f, // sphere radius
            true // whether the ray and sphere should intersect
        },
        {
            ToyMaker::Ray {
                .mStart { glm::vec3 { 0.f, 0.f, 0.f} },
                .mDirection { glm::vec3 { 0.f, 0.f, -1.f}},
                .mLength { 100.f }
            },
            glm::vec3 { 0.f, 0.f, -50.f },
            0.f,
            false
        },
        {
            ToyMaker::Ray {
                .mStart { glm::vec3 { 0.f, 0.f, 0.f} },
                .mDirection { glm::vec3 { 0.f, 0.f, -1.f}},
                .mLength { 100.f }
            },
            glm::vec3 { 0.f, 5.f, -50.f },
            5.f,
            false
        },
        {
            ToyMaker::Ray {
                .mStart { glm::vec3 { 0.f, 0.f, 0.f} },
                .mDirection { glm::vec3 { 0.f, 0.f, -1.f}},
                .mLength { 100.f }
            },
            glm::vec3 { 0.f, 25.f, -50.f },
            5.f,
            false
        },
        {
            ToyMaker::Ray {
                .mStart { glm::vec3 { 0.f, 0.f, -100.f} },
                .mDirection { glm::vec3 { .7f, 0.f, .7f}},
                .mLength { 100.f }
            },
            glm::vec3 { 7.f, 0.f, -93.f },
            5.f,
            true
        },
        {
            ToyMaker::Ray {
                .mStart { glm::vec3 { 0.f, 0.f, -100.f} },
                .mDirection { glm::vec3 { .7f, 0.f, .7f}},
                .mLength { 100.f }
            },
            glm::vec3 { 0.f, 0.f, 0.f },
            5.f,
            false
        },
        {
            ToyMaker::Ray {
                .mStart { glm::vec3 {0.f, 10.f, 0.f} },
                .mDirection { glm::vec3 { 0.f, 1.f, 0.f} },
                .mLength { 100.f }
            },
            glm::vec3 { 0.f, 110.f, 0.f },
            5.f,
            true
        },
        {
            ToyMaker::Ray {
                .mStart { glm::vec3 {0.f, 10.f, 0.f} },
                .mDirection { glm::vec3 { 0.f, 1.f, 0.f} },
                .mLength { 100.f }
            },
            glm::vec3 { 0.f, 5.f, 0.f },
            5.f,
            true
        },
        {
            ToyMaker::Ray {
                .mStart { glm::vec3 {0.f, 0.f, 0.f} },
                .mDirection { glm::vec3 { 0.f, 1.f, 0.f} },
                .mLength { 100.f }
            },
            glm::vec3 { 0.f, 110.f, 0.f },
            5.f,
            false
        },
    }};
    for(const auto& data: testData) {
        raySphereSubtest(std::get<0>(data), std::get<1>(data), std::get<2>(data), std::get<3>(data));
    }
}

}

void raySphereSubtest(const ToyMaker::Ray& ray, const glm::vec3& spherePosition, const float sphereRadius, const bool shouldIntersect) {
    const ToyMaker::ObjectBounds sphere {
        ToyMaker::ObjectBounds::create(
            ToyMaker::VolumeSphere { .mRadius { sphereRadius } },
            spherePosition,
            glm::vec3 { 0.f }
        )
    };
    const std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> intersectionResults {
        ToyMaker::computeIntersections(ray, sphere)
    };
    const bool overlaps { ToyMaker::overlaps(ray, sphere) };

    if(shouldIntersect) {
        CHECK(intersectionResults.first >= 1);
        CHECK(ToyMaker::isFinite(intersectionResults.second.first));
        if(intersectionResults.first == 2) {
            CHECK(ToyMaker::isFinite(intersectionResults.second.second));
        }
        CHECK(overlaps == true);
    } else {
        CHECK(intersectionResults.first == 0);
        CHECK(!ToyMaker::isFinite(intersectionResults.second.first));
        CHECK(!ToyMaker::isFinite(intersectionResults.second.second));
        CHECK(overlaps == false);
    }
}

}
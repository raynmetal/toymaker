/**
 * @ingroup ToyMakerSpatialQuerySystem
 * @file spatial_query/math.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Geometrical, mathematical functions and related structs used to answer some simple questions about shapes situated somewhere in the world.
 * 
 * @version 0.3.2
 * @date 2025-09-08
 * 
 * 
 */

#ifndef TOYMAKERENGINE_SPATIALQUERYMATH_H
#define TOYMAKERENGINE_SPATIALQUERYMATH_H

#include <glm/glm.hpp>

#include "types.hpp"

namespace ToyMaker {

    static constexpr uint8_t kMaxIterations { 64 };

    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns a bool-vector pair, with bool indicating whether a point of intersection was found, and the vector containing the point of intersection.
     * 
     * @warning Error if invalid ray or plane specified (plane with no normal, or ray with no direction)
     */
    std::pair<bool, glm::vec3> computeIntersection(const Ray& ray, const Plane& plane);
    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns a bool-vector pair, with bool indicating whether a point of intersection was found, and the vector containing the point of intersection
     * 
     * @warning Error if invalid ray or triangle specified (triangle with no area, or ray with no direction)
     */
    std::pair<bool, glm::vec3> computeIntersection(const Ray& ray, const AreaTriangle& triangle);
    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns an unsigned int and vector-pair pair, with unsigned indicating whether any and how many points of intersection were found with the AABB, and the vector containing the points of intersection.
     * 
     * (if the ray "glances off" the volume, it does not count as an intersection per my implementation)
     * 
     * @warning Error if invalid ray or AABB specified, which includes an AABB with negative or zero volume.
     * 
     */
    std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> computeIntersections(const Ray& ray, const AxisAlignedBounds& axisAlignedBounds);
    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns an unsigned int and vector pair pair, with int indicating whether and how many points of intersection were found, and the vector containing the points of intersection
     * 
     * Object bounds only supports convex shapes, and so one can expect that if a point of intersection exists, then there will be a second to go with it. (provided the ray is long enough) (and also unless the ray "glances off" the volume, which counts as no intersection per my implementation)
     * 
     */
    std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> computeIntersections(const Ray& ray, const ObjectBounds& objectbounds);

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns a bool value indicating whether `point` is contained by `bounds`
     */
    bool overlaps(const glm::vec3& point, const ObjectBounds& bounds);
    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns a bool value indicating whether `point` is contained by `bounds`
     */
    inline bool overlaps(const ObjectBounds& bounds, const glm::vec3& point) {
        return overlaps(point, bounds);
    }
    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns a bool value indicating whether the ray passes through the object volume
     */
    bool overlaps(const Ray& ray, const ObjectBounds& bounds);
    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns a bool value indicating whether the ray passes through the object volume
     */
    inline bool overlaps(const ObjectBounds& bounds, const Ray& ray) {
        return overlaps(ray, bounds);
    }
    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `point` is contained by `bounds`.
     * 
     */
    bool overlaps(const glm::vec3& point, const AxisAlignedBounds& bounds);
    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `point` is contained by `bounds`.
     * 
     */
    inline bool overlaps(const AxisAlignedBounds& bounds, const glm::vec3& point) {
        return overlaps(point, bounds);
    }
    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `ray` overlaps with `bounds`.
     * 
     */
    bool overlaps(const Ray& ray, const AxisAlignedBounds& bounds);
    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `ray` overlaps with `bounds`.
     * 
     */
    inline bool overlaps(const AxisAlignedBounds& bounds, const Ray& ray) {
        return overlaps(ray, bounds);
    }
    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `one` overlaps `two`. 
     * 
     */
    bool overlaps(const AxisAlignedBounds& one, const AxisAlignedBounds& two);
    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `one` overlaps `two`. 
     * 
     */
    bool overlaps(const ObjectBounds& one, const ObjectBounds& two);
    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `one` overlaps `two`. 
     * 
     */
    bool overlaps(const AxisAlignedBounds& one, const ObjectBounds& two);
    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `one` overlaps `two`. 
     * 
     */
    inline bool overlaps(const ObjectBounds& one, const AxisAlignedBounds& two) {
        return overlaps(two, one);
    }

    /** 
     * @brief GJK implementation based on Casey Muratori's video lecture, which
     * is available [here](https://www.youtube.com/watch?v=Qupqu1xe7Io)
     * 
     */
    template <typename T, typename U>
    std::pair<bool, Simplex> gjkOverlaps(const T& one, const U& two);

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `point` is contained by `bounds`
     * 
     */
    bool contains(const glm::vec3& point, const ObjectBounds& bounds);
    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `point` is contained by `bounds`. 
     * 
     */
    bool contains(const glm::vec3& point, const AxisAlignedBounds& bounds);
    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `ray` is contained by `bounds`. 
     * 
     */
    bool contains(const Ray& ray, const AxisAlignedBounds& bounds);
    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `ray` is contained by `bounds`. 
     * 
     */
    bool contains(const Ray& ray, const ObjectBounds& bounds);
    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `one` is contained by `two`. 
     * 
     */
    bool contains(const AxisAlignedBounds& one, const AxisAlignedBounds& two);
    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `one` is contained by `two`
     * 
     */
    bool contains(const ObjectBounds& one, const AxisAlignedBounds& two);

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * 
     * @brief Closely associated with GJK; finds the Minkowski difference between two shapes
     * situated somewhere in the world.
     * 
     * @see gjkOverlaps
     * 
     */
    template<typename T, typename U>
    inline glm::vec3 minkowskiDifference(const T& one, const U& two, const glm::vec3& along) {
        const glm::vec3 supportOne { one.getSupportAlong(along) };
        const glm::vec3 supportTwo { two.getSupportAlong(-along) };
        return supportOne - supportTwo;
    }

    template <typename T, typename U>
    inline std::pair<bool, Simplex> gjkOverlaps(const T& one, const U& two) {
        assert(one.isSensible() && two.isSensible() && "Invalid object bounds provided");

        // Both need to be non-degenerate bounds to actually overlap
        if(!one.isPositiveStrict() || !two.isPositiveStrict()) {
            return { false, Simplex {} };
        }


        glm::vec3 searchDirection { two.getComputedWorldPosition() - one.getComputedWorldPosition() };

        // Two non-degenerate objects share the same origin, obviously they overlap
        if(squareDistance(searchDirection) == 0.f) {
            return { true, Simplex {} };
        }

        Simplex simplex {};
        simplex.append(minkowskiDifference(one, two, searchDirection));

        // we go towards the origin from the first point we found
        searchDirection = squareDistance(simplex.mPoints[0]) ? -simplex.mPoints[0]: glm::vec3 { 1.f, 0.f, 0.f };
        bool found { false };
        for(uint8_t iteration { 0 }; iteration < kMaxIterations; ++iteration) {

            // We force a tetrahedron to form by picking arbitrary search directions
            // in the event our degenerate simplex contains (0, 0, 0)
            if(!squareDistance(searchDirection)) {
                const float signMult { (iteration & 1)? -1.f: 1.f };

                // Pick a new search direction to expand 1-simplex (line) to 2-simplex (triangle)
                if(simplex.mNPoints == 2) {
                    const float signMult2 { (iteration & 2)? -1.f: 1.f };
                    const glm::vec3 dirAB { glm::normalize(simplex.mPoints[1] - simplex.mPoints[0]) };
                    assert(squareDistance(dirAB) > 0 && "A and B should never be the same point");

                    const glm::vec3 dirOffset { (dirAB.x != 0.f || dirAB.z != 0.f)? 
                        glm::vec3{ 0.f, signMult * 1.f, 0.f } : glm::vec3{ signMult2 * 1.f, 0.f, signMult * 1.f }
                    };

                    searchDirection = -dirAB + dirOffset;

                // Pick a new search direction to expand 2-simplex (triangle) to 3-simplex (tetrahedron)
                } else if(simplex.mNPoints == 3) {
                    const glm::vec3 normABC { glm::normalize(glm::cross(simplex.mPoints[1] - simplex.mPoints[0], simplex.mPoints[2] - simplex.mPoints[0])) };
                    searchDirection = signMult * normABC;

                } else {
                    assert(false && "A simplex with 4 points is not degenerate, and there's no solving necessary here");
                }
            }

            const glm::vec3 candidatePoint { minkowskiDifference(one, two, searchDirection) };

            // We couldn't find a point past the origin in this direction, so obviously there is no intersection
            if(glm::dot(candidatePoint, searchDirection) <= 0) {
                return { false, simplex };
            }

            // This will return false generally when we're trying to fabricate
            // a search direction, in which case we just fabricate another one at
            // the start of the loop.
            if(!simplex.append(candidatePoint)) {
                searchDirection = glm::vec3 { 0.f, 0.f, 0.f };
                continue;
            }

            const auto result { simplex.evaluate() };
            if(result.first) {
                return { true, simplex };
            }

            searchDirection = result.second;
        }
        assert(false && "We should not have exited the loop without finding a simplex");
        return { false, simplex };
    }
}

#endif

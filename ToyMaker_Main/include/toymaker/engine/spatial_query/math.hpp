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
}

#endif

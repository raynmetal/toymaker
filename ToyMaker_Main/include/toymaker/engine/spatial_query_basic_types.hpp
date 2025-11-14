/**
 * @ingroup ToyMakerSpatialQuerySystem
 * @file spatial_query_basic_types.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Classes and structs representing data related to the engine's spatial query system (the precursor to a full-fledged physics system).
 * @version 0.3.2
 * @date 2025-09-08
 * 
 * 
 */

#ifndef FOOLSENGINE_SPATIALQUERYGEOMETRY_H
#define FOOLSENGINE_SPATIALQUERYGEOMETRY_H

#include <cmath>
#include <array>
#include <glm/glm.hpp>

namespace ToyMaker {
    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Type used to represent the name of the corner of a box.
     * 
     */
    using BoxCorner = uint8_t;

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Enum values correspond to bits on a BoxCorner which help specify which side of the box on each axis is being indicated.
     * 
     */
    enum BoxCornerSpecifier: BoxCorner {
        RIGHT=0x1,
        TOP=0x2,
        FRONT=0x4
    };


    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Tests whether a given number is finite.
     * 
     * @param number The number being tested.
     * @retval true The number is finite;
     * @retval false The number is not finite;
     */
    inline bool isFinite(float number) {
        return std::isfinite(number);
    }
    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Tests whether a set of 3 numbers is finite.
     * 
     * @param vector The numbers being tested.
     * @retval true The numbers are (all) finite;
     * @retval false One or more numbers are not finite;
     */
    inline bool isFinite(const glm::vec3& vector) {
        return isFinite(vector.x) && isFinite(vector.y) && isFinite(vector.z);
    }

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Tests whether a number is positive.
     * 
     * @param number The number being tested.
     * @retval true The number is positive;
     * @retval false The number is not positive;
     */
    inline bool isPositive(float number) {
        return number > 0.f;
    }

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Tests whether a set of 3 numbers is positive.
     * 
     * @param vector The numbers being tested.
     * @retval true The numbers are (all) positive;
     * @retval false One or more numbers are not positive;
     */
    inline bool isPositive(const glm::vec3& vector) {
        return isPositive(vector.x) && isPositive(vector.y) && isPositive(vector.z);
    }

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Tests whether a number is non-negative.
     * 
     * @param number The number being tested.
     * @retval true The number is non-negative;
     * @retval false The number is negative;
     */
    inline bool isNonNegative(float number) {
        return number >= 0.f;
    }

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Tests whether a set of numbers is non-negative.
     * 
     * @param vector The numbers being tested.
     * @retval true The numbers are (all) non-negative.
     * @retval false One or more of the numbers are negative.
     */
    inline bool isNonNegative(const glm::vec3& vector) {
        return isNonNegative(vector.x) && isNonNegative(vector.y) && isNonNegative(vector.z);
    }

    template <typename TDerived>
    struct Volume;


    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief The base class of all spatial query volumes.
     * 
     * Provides functions and methods for retrieving and computing volume relative and axis aligned box corner coordinates.
     * 
     */
    struct VolumeBase_ {
        /**
         * @brief Returns an array populated with axis-wise sign multipliers, where the positions on the array correspond to the corner of a box.
         * 
         * @return constexpr std::array<glm::vec3, 8> Array of axis-wise sign multipliers for box corners.
         * 
         * @see BoxCornerSpecifier
         * @see BoxCorner
         */
        inline static constexpr std::array<glm::vec3, 8> GetCornerSignsArray() {
            std::array<glm::vec3, 8> cornerSigns {};
            for(BoxCorner corner {0}; corner < 8; ++corner) {
                cornerSigns[corner].x = corner&BoxCornerSpecifier::RIGHT? 1.f: -1.f;
                cornerSigns[corner].y = corner&BoxCornerSpecifier::TOP? 1.f: -1.f;
                cornerSigns[corner].z = corner&BoxCornerSpecifier::FRONT? 1.f: -1.f;
            }
            return cornerSigns;
        }

        /**
         * @brief Computes the model relative corners of a box, given the dimensions of the box.
         * 
         * @param boxDimensions A box's width, height, and depth.
         * @return std::array<glm::vec3, 8> An array of box corner coordinates.
         */
        inline static std::array<glm::vec3, 8> ComputeBoxCorners(const glm::vec3& boxDimensions) {
            const std::array<glm::vec3, 8> cornerSignsArray { GetCornerSignsArray() };
            glm::vec3 absoluteCornerOffset { .5f * boxDimensions };
            std::array<glm::vec3, 8> cornerArray { .5f * boxDimensions };
            for(uint8_t corner{0}; corner < 8; ++corner) {
                cornerArray[corner] = cornerSignsArray[corner] * absoluteCornerOffset;
            }
            return cornerArray;
        }

        /** 
         * @brief Poor man's vtable which doesn't break Volume's ability to be aggregate initialized.
         * 
         * @see Volume::getVolumeRelativeBoxCorners()
         * 
         * @todo I forget why I was doing this in the first place.  Do we have a problem here?
         * 
         */
        template <typename TDerived>
        inline std::array<glm::vec3, 8> getVolumeRelativeBoxCorners() const {
            return Volume<TDerived>::getVolumeRelativeBoxCorners();
        }
    };

    /** @ingroup ToyMakerSpatialQuerySystem */
    template <typename TDerived>
    struct Volume: public VolumeBase_ {
        /**
         * @brief Poor man's vtable cont'd.
         * 
         * @see VolumeBase_::getVolumeRelativeBoxCorners()
         * 
         */
        inline std::array<glm::vec3, 8> getVolumeRelativeBoxCorners() const {
            return TDerived::getVolumeRelativeBoxCorners();
        }
    };

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Holds the parameters describing the spatial query volume of a simple three-dimensionsal box.
     * 
     */
    struct VolumeBox: public Volume<VolumeBox> {
        /**
         * @brief The dimensions of the box, its width, height, and depth.
         * 
         */
        glm::vec3 mDimensions {0.f, 0.f, 0.f};

        /**
         * @brief Returns an array of coordinates corresponding to the corners of the box.
         * 
         * @return std::array<glm::vec3, 8> An array of box corner coordinates.
         */
        inline std::array<glm::vec3, 8> getVolumeRelativeBoxCorners () const {
            return ComputeBoxCorners(mDimensions);
        }

        /**
         * @brief Tests whether the values representing the box are valid (as opposed to invalid, infinite, or degenerate).
         * 
         * @retval true The box is sensible;
         * @retval false The box is not sensible;
         */
        inline bool isSensible() const {
            return isPositive(mDimensions) && isFinite(mDimensions);
        }
    };

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Holds the parameters describing the spatial query volume of a simple three-dimensionsal capsule (or pill, or whatever you like).
     * 
     */
    struct VolumeCapsule: public Volume<VolumeCapsule> {
        /**
         * @brief The height of the cylindrical section of the capsule.
         * 
         */
        float mHeight {0.f};

        /**
         * @brief The radius of the hemispheres on either end of the capsule.
         * 
         */
        float mRadius {0.f};

        /**
         * @brief Gets an array containing the coordinates of the corners of the volume aligned box just containing the capsule.
         * 
         * @return std::array<glm::vec3, 8> An array of box corner coordinates.
         */
        inline std::array<glm::vec3, 8> getVolumeRelativeBoxCorners () const {
            const glm::vec3 boxDimensions { 2.f*mRadius, mHeight + 2.f*mRadius, 2.f*mRadius};
            return ComputeBoxCorners(boxDimensions);
        }

        /**
         * @brief Tests whether the values representing the capsule make sense (as opposed to being invalid, infinite, or degenerate.)
         * 
         * @retval true The capsule parameters are sensible;
         * 
         * @retval false The capsule parameters are not sensible;
         * 
         */
        inline bool isSensible() const {
            return (
                isPositive(mHeight) && isFinite(mHeight)
                && isPositive(mRadius) && isFinite(mRadius)
            );
        }
    };

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Holds parameters describing a spherical spatial query volume.
     * 
     */
    struct VolumeSphere: public Volume<VolumeSphere> {
        /**
         * @brief The radius of the sphere.
         * 
         */
        float mRadius {0.f};

        /**
         * @brief Gets an array of coordinates of corners of a box just encapsulating the sphere.
         * 
         * @return std::array<glm::vec3, 8> An array of coordinates of a box containing the sphere.
         */
        inline std::array<glm::vec3, 8> getVolumeRelativeBoxCorners () const {
            return ComputeBoxCorners(glm::vec3{2*mRadius});
        }

        /**
         * @brief Tests whether this volumes parameters are sensible (as opposed to invalid, infinite, or degenerate).
         * 
         * @retval true The sphere is sensible;
         * @retval false The sphere is not sensible;
         */
        inline bool isSensible() const {
            return isPositive(mRadius) && isFinite(mRadius);
        }
    };

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief A set of 3 points located in the world forming a (hopefully sensible) triangle.
     * 
     */
    struct AreaTriangle {
        /**
         * @brief The points of the triangle, where each point has 3 components.
         * 
         */
        std::array<glm::vec3, 3> mPoints {};

        /**
         * @brief Tests whether the points describing the triangle are sensible (as opposed to invalid, infinite, or degenerate).
         * 
         * @retval true The triangle is sensible;
         * @retval false The triangle is not sensible;
         */
        inline bool isSensible() const {
            return (
                (
                    isFinite(mPoints[0]) && isFinite(mPoints[1]) && isFinite(mPoints[2])
                ) && (
                    isPositive(
                        glm::cross(
                            mPoints[2] - mPoints[0], mPoints[1] - mPoints[0]
                        ).length()
                    )
                )
            );
        }
    };

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief A set of numbers representing a single circle situated somewhere in the world.
     * 
     */
    struct AreaCircle {
        /**
         * @brief The radius of the circle.
         * 
         */
        float mRadius { 0.f };

        /**
         * @brief The real-world coordinates of the center of the circle.
         * 
         */
        glm::vec3 mCenter { 0.f };

        /**
         * @brief A vector normal to the surface of the circle, in whose direction it may be assumed the circle is facing.
         * 
         */
        glm::vec3 mNormal { 0.f, -1.f, 0.f };

        /**
         * @brief Tests whether the circle described by these parameters is valid (as opposed to invalid, infinite, or degenerate)
         * 
         * @retval true The circle is sensible;
         * @retval false The circle is not sensible;
         */
        inline bool isSensible() const {
            return (
                (
                    isFinite(mRadius) && isPositive(mRadius)
                ) && (
                    isFinite(mNormal) && isPositive(mNormal.length())
                ) && (
                    isFinite(mCenter)
                )
            );
        }
    };


    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief A set of numbers describing a ray with its source at some finite point in the world, projected in a direction for some positive length.
     * 
     */
    struct Ray {
        /**
         * @brief A point representing the starting point of the ray.
         * 
         */
        glm::vec3 mStart { 0.f };

        /**
         * @brief The direction the ray is pointing in.
         * 
         */
        glm::vec3 mDirection { 0.f, 0.f, -1.f };

        /**
         * @brief The length of the ray, infinite by default.
         * 
         */
        float mLength { std::numeric_limits<float>::infinity() };

        /**
         * @brief Tests whether the ray is sensible (as opposed to invalid or degenerate).
         * 
         * @retval true The ray is sensible;
         * @retval false The ray isn't sensible;
         */
        inline bool isSensible() const {
            return (
                (
                    isFinite(mDirection) && isPositive(mDirection.length())
                ) && (
                    isFinite(mStart)
                ) && (
                    isPositive(mLength)
                )
            );
        }
    };

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief A set of numbers describing a plane situated somewhere in the world.
     * 
     */
    struct Plane {
        /**
         * @brief A known point on the plane.
         * 
         */
        glm::vec3 mPointOnPlane { 0.f };

        /**
         * @brief A vector normal to the plane.
         * 
         */
        glm::vec3 mNormal { 0.f, 0.f, -1.f };

        /**
         * @brief Tests whether the plane described is sensible (as opposed to invalid, infinite, or degenerate).
         * 
         * @retval true The plane is sensible;
         * @retval false The plane is not sensible;
         */
        inline bool isSensible() const {
            return (
                (
                    isFinite(mNormal) && isPositive(mNormal.length())
                ) && (
                    isFinite(mPointOnPlane)
                )
            );
        }
    };
}

#endif

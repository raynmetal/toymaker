/** @ingroup ToyMakerSpatialQuerySystem
 * @file spatial_query/types.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Classes and structs representing data related to the engine's spatial query system (the precursor to a full-fledged physics system).
 * @version 0.3.2
 * @date 2025-09-08
 *
 *
 */

#ifndef TOYMAKERENGINE_SPATIALQUERYTYPES_H
#define TOYMAKERENGINE_SPATIALQUERYTYPES_H

#include <cmath>
#include <array>
#include <queue>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>
#include <string>

namespace ToyMaker {
    struct AreaTriangle;

    inline float squareDistance(const glm::vec3& vector) {
        return glm::dot(vector, vector);
    }

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
     * @brief Tests whether a number is strictly positive.
     *
     * @param number The number being tested.
     * @retval true The number is strictly positive;
     * @retval false The number is not positive;
     */
    inline bool isPositiveStrict(float number) {
        return number > 0.f;
    }
    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Tests whether a set of 3 numbers is strictly positive.
     *
     * @param vector The numbers being tested.
     * @retval true The numbers are (all) strictly positive;
     * @retval false One or more numbers are not positive;
     */
    inline bool isPositiveStrict(const glm::vec3& vector) {
        return isPositiveStrict(vector.x) && isPositiveStrict(vector.y) && isPositiveStrict(vector.z);
    }

    inline bool isNumber(float number) {
        return !std::isnan(number);
    };

    inline bool isNumber(const glm::vec3& vector) {
        return isNumber(vector.x) && isNumber(vector.y) && isNumber(vector.z);
    };

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
     * @brief Bit field where each bit represents an interaction layer -- a physics/spatial layer
     * that can trigger or receive events from objects in other related layers
     *
     */
    using InteractionLayerMask = uint16_t;

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
         * @brief Gets corners of the object-relative bounding box that encapsulates the derived volume.
         *
         * Forces subclasses to implement their own version of this function without breaking
         * their ability to be aggregate initialized.
         *
         * @see Volume::getVolumeRelativeBoxCorners()
         *
         */
        template <typename TDerived>
        inline std::array<glm::vec3, 8> getVolumeRelativeBoxCorners() const {
            return Volume<TDerived>::getVolumeRelativeBoxCorners();
        }

        /**
         * @brief Returns whether or not the derived volume has sensible parameters (i.e., isn't infinite or
         * invalid).
         *
         * Forces subclasses to implement their own version of this function without breaking
         * their ability to be aggregate initialized.
         *
         * @see Volume::isSensible()
         *
         */
        template <typename TDerived>
        inline bool isSensible() const {
            return Volume<TDerived>::isSensible();
        }
        /**
         * @brief Returns whether or not the derived volume has strictly positive parameters.
         *
         * Forces subclasses to implement their own version of this function without breaking
         * their ability to be aggregate initialized
         *
         * @see Volume::isPositiveStrict()
         *
         */
        template <typename TDerived>
        inline bool isPositiveStrict() const {
            return Volume<TDerived>::isPositiveStrict();
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

        /**
         * @brief Poor man's vtable cont'd
         *
         * @see VolumeBase_::isSensible()
         *
         */
        inline bool isSensible() const {
            return TDerived::isSensible();
        }

        /**
         * @brief Poor man's vtable cont'd
         *
         * @see VolumeBase_::isPositiveStrict()
         *
         */
        inline bool isPositiveStrict() const {
            return TDerived::isPositiveStrict();
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
         * @brief Tests whether the values representing the box are valid (as opposed to invalid or infinite).
         *
         * @retval true The box is sensible;
         * @retval false The box is not sensible;
         */
        inline bool isSensible() const {
            return isNonNegative(mDimensions) && isFinite(mDimensions);
        }

        /**
         * @brief Tests whether the values representing the box are strictly positive
         *
         * @retval true The box's parameters are all strictly positive;
         * @retval false The box's parameters are not all strictly positive;
         */
        inline bool isPositiveStrict() const {
            return ToyMaker::isPositiveStrict(mDimensions);
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
         * @brief Tests whether the values representing the capsule make sense (as opposed to being invalid or infinite.)
         *
         * @retval true The capsule parameters are sensible;
         *
         * @retval false The capsule parameters are not sensible;
         *
         */
        inline bool isSensible() const {
            return (
                isNonNegative(mHeight) && isFinite(mHeight)
                && isNonNegative(mRadius) && isFinite(mRadius)
            );
        }

        /**
         * @brief Tests whether the values representing the capsule are all strictly positive
         *
         * @retval true The capsule parameters are strictly positive;
         * @retval false The capsule's parameters are not strictly positive
         *
         */
        inline bool isPositiveStrict() const {
            return (
                ToyMaker::isPositiveStrict(mRadius)
                // You can still have a capsule which occupies space even if its
                // spine is length 0, so no test for mHeight
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
         * @brief Tests whether this volume's parameters are sensible (as opposed to invalid or infinite).
         *
         * @retval true The sphere is sensible;
         * @retval false The sphere is not sensible;
         */
        inline bool isSensible() const {
            return isNonNegative(mRadius) && isFinite(mRadius);
        }

        /**
         * @brief Tests whether this volume's parameters are strictly positive
         *
         * @retval true The sphere's radius is strictly positive;
         * @retval false The sphere's radius is not strictly positive;
         *
         */
        inline bool isPositiveStrict() const {
            return ToyMaker::isPositiveStrict(mRadius);
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
         * @brief Tests whether the points describing the triangle are sensible (as opposed to invalid or infinite).
         *
         * @retval true The triangle is sensible;
         * @retval false The triangle is not sensible;
         *
         */
        inline bool isSensible() const {
            return (
                (
                    isFinite(mPoints[0]) && isFinite(mPoints[1]) && isFinite(mPoints[2])
                ) && (
                    isNonNegative(
                        glm::length(glm::cross(
                            mPoints[2] - mPoints[0], mPoints[1] - mPoints[0]
                        ))
                    )
                )
            );
        }

        /**
         * @brief Tests whether the points of this triangle encapsulate some area in space
         *
         * @retval true This triangle is non-trivial, and covers some definite space
         * @retval false This triangle is trivial, degenerate, and covers no real area
         *
         */
        inline bool isPositiveStrict() const {
            return (
                ToyMaker::isPositiveStrict(
                    glm::length(
                        glm::cross(
                            mPoints[2] - mPoints[0], mPoints[1] - mPoints[0]
                        )
                    )
                )
            );
        }
    };

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Generates a list of triangles making up the surface of a box situated somewhere in the world, given the coordinates of its corners.
     *
     * @param boxCorners An array containing coordinates for the corners of a box located somewhere in the world.
     *
     * @return std::array<AreaTriangle, 12> The triangles representing the faces of the box.
     *
     */
    std::array<AreaTriangle, 12> computeBoxFaceTriangles(const std::array<glm::vec3, 8>& boxCorners);

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
         * @brief Tests whether the circle described by these parameters is valid (as opposed to invalid or infinite)
         *
         * @retval true The circle is sensible;
         * @retval false The circle is not sensible;
         */
        inline bool isSensible() const {
            return (
                (
                    isFinite(mRadius) && isNonNegative(mRadius)
                ) && (
                    isFinite(mNormal) && ToyMaker::isPositiveStrict(glm::length(mNormal))
                ) && (
                    isFinite(mCenter)
                )
            );
        }

        /**
         * @brief Tests whether the circle's parameters are strictly positive, and hence
         * whether the circle encloses some real region in space
         *
         * @retval true The circle is strictly positive;
         * @retval false The circle is not strictly positive;
         */
        inline bool isPositiveStrict() const {
            return (
                ToyMaker::isPositiveStrict(mRadius)
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
         * @brief Tests whether the ray is sensible (as opposed to invalid).
         *
         * @retval true The ray is sensible;
         * @retval false The ray isn't sensible;
         */
        inline bool isSensible() const {
            return (
                (
                    isFinite(mDirection) && ToyMaker::isPositiveStrict(glm::length(mDirection))
                ) && (
                    isFinite(mStart)
                ) && (
                    isNonNegative(mLength)
                )
            );
        }

        /**
         * @brief Tests whether the ray actually travels any distance from its origin
         *
         * @retval true The ray travels some length away from its origin;
         * @retval false The ray does not travel any length away from its origin
         */
        inline bool isPositiveStrict() const {
            return ToyMaker::isPositiveStrict(mLength);
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
                    isFinite(mNormal) && ToyMaker::isPositiveStrict(glm::length(mNormal))
                ) && (
                    isFinite(mPointOnPlane)
                )
            );
        }

        /**
         * @brief Same as Plane::isSensible()
         *
         * @see Plane::isSensible()
         *
         * @retval true Plane is sensible.
         * @retval false Plane isn't sensible.
         */
        inline bool isPositiveStrict() const {
            return isSensible();
        }
    };

    /**
     * @brief Primitive for GJK algorithm.
     *
     * Stores up to 4 points which should enclose the origin in the
     * Minkowski different of two shapes (if those shapes intersect).
     *
     */
    struct Simplex {
        /**
         * @brief Points representing the simplex, derived by finding the Minksowski difference of
         * support points on two convex shapes.
         *
         */
        std::array<glm::vec3, 4> mPoints;

        /**
         * @brief The point on the LHS of the Minkowski difference, where each index
         * corresponds to a point on `mPoints`
         */
        std::array<glm::vec3, 4> mPointsSupportA;

        /**
         * @brief The point on the RHS of the Minkowski difference, where each index
         * corresponds to a point on `mPoints`
         */
        std::array<glm::vec3, 4> mPointsSupportB;

        /**
         * @brief The number of points in this simplex
         *
         */
        uint8_t mNPoints { 0 };

        /**
         * @brief Adds a new point to the simplex
         *
         * @retval false A duplicate was found, and this point could not be added
         * @retval true Appended new point successfully
         */
        inline bool append(const glm::vec3& candidatePoint, const glm::vec3& supportA, const glm::vec3& supportB) {
            assert(mNPoints < 4 && "We already have 4 (or more) points, there's no more needed");
            assert(supportA - supportB == candidatePoint && "The difference in supports should yield the candidate point");

            // Hackery to ensure that we don't add the same point twice (which is a possibility
            // since the hackery to force 3-simplex might cause us to look in the same direction
            // twice)
            for(uint8_t index { 0 }; index < mNPoints; ++index) {
                if(mPoints[index] == candidatePoint) {
                    return false;
                }
            }
            mPoints[mNPoints] = candidatePoint;
            mPointsSupportA[mNPoints] = supportA;
            mPointsSupportB[mNPoints] = supportB;
            ++mNPoints;

            return true;
        }

        /**
         * @brief Replaces the current simplex with points from a new list (which will usually have as many or fewer points)
         *
         */
        inline void reorder(const std::vector<uint8_t> reorderedIndices) {
            assert(reorderedIndices.size() <= 4 && "Invalid point list provided");
            const std::array<glm::vec3, 4> oldPoints { mPoints };
            const std::array<glm::vec3, 4> oldPointsSupportA { mPointsSupportA };
            const std::array<glm::vec3, 4> oldPointsSupportB { mPointsSupportB };
            mNPoints = 0;
            for(const auto& index: reorderedIndices) {
                const bool pointAddedSuccessfully{ append(oldPoints[index], oldPointsSupportA[index], oldPointsSupportB[index]) };
                assert(pointAddedSuccessfully && "Invalid point list provided");
            }
        }

        /**
         * @brief Tries to find a 3-simplex that encloses the origin
         *
         * Implementation of the simplex evaluation/simplification test in GJK, an algorithm
         * for determining whether 2 convex shapes intersect.  When they do intersect, this
         * simplex forms a tetrahedron containing point (0, 0, 0).
         *
         * @returns Whether this simplex forms a tetrahedron enclosing (0, 0, 0), and a search
         * direction to use in the next support function call
         *
         */
        std::pair<bool, glm::vec3> evaluate();

    private:
        std::pair<bool, glm::vec3> doSimplex4();
        glm::vec3 doSimplex3();
        glm::vec3 doSimplex2();
    };

    /**
     * @brief Primitive for EPA algorithm
     *
     * Represents a polygonal shape contained within the Minkowski difference of
     * two intersecting shapes.  Stores polytope faces in order of their proximity to
     * the origin.
     *
     */
    class Polytope {
    private:
        /**
         * @brief Stores the indices of a single face of _this_ polytope
         */
        struct Face {
            std::array<uint16_t, 3>  mIndices {};
        };

        /**
         * Initializes an empty polytope
         */
        Polytope();


        /**
         * @brief The list of triangle faces representing this polytope
         *
         */
        std::priority_queue<Face, std::vector<Face>, std::function<bool(const Face&, const Face&)>> mFaces;

        /**
         * @brief List of points representing this polytope
         */
        std::vector<glm::vec3> mPoints {};
        /**
         * @brief List of points representing support point A, where each point corresponds
         * to the polytope point it generated in `mPoints` at the same index
         */
        std::vector<glm::vec3> mPointsSupportA {};

        /**
         * @brief List of points representing support point B, where each point corresponds
         * to the polytope point it generated in `mPoints` at the same index
         */
        std::vector<glm::vec3> mPointsSupportB {};

        inline glm::vec3 getTriangleCross(const Face& face) const {
            return glm::cross(
                mPoints[face.mIndices[1]] - mPoints[face.mIndices[0]],
                mPoints[face.mIndices[2]] - mPoints[face.mIndices[0]]
            );
        }

        inline glm::vec3 getTriangleNorm(const Face& face) const {
            return glm::normalize(getTriangleCross(face));
        }

    public:
        /**
         * @brief Creates a polytope object out of a simplex.
         *
         * Simplex _must_ be a tetrahedron enclosing the origin -- creation will fail otherwise.
         */
        Polytope(const Simplex& simplex);

        /**
         * @brief Returns the next search direction for a point to add
         * to the polytope
         */
        glm::vec3 getNextSearch() const;

        /**
         * Returns the number of faces that this polytope is comprised of
         */
        std::size_t getNumFaces() const {
            return mFaces.size();
        }

        std::size_t getNumPoints() const {
            return mPoints.size();
        }

        /**
         * @brief Returns the closest point to the origin on the triangle face
         * closest to the origin
         *
         */
        glm::vec3 getClosestPoint() const;

        /**
         * @brief Returns the direction to the closest point on the polytope's surface from
         * the origin
         *
         * Directly represents the contact normal between two objects.
         *
         */
        glm::vec3 getClosestTriangleNormal() const;

        /**
         * @brief Returns the closest polytope triangle.
         *
         * This is useful when we want to find the barycentric coordinates of this triangle
         * to derive contact points for a pair of intersecting shapes.
         *
         */
        AreaTriangle getClosestTriangle() const;

        /**
         * @brief Gets the points of shape A that were responsible for generating the closest triangle
         * of the polytope.
         *
         */
        AreaTriangle getClosestTriangleSupportA() const;

        /**
         * @brief Gets the points of shape B that were responsible for generating the closest triangle
         * of the polytope.
         *
         */
        AreaTriangle getClosestTriangleSupportB() const;

        /**
         * @brief Appends a new point, replacing the topmost triangle in the polygon
         * with 3 more triangles including the new point
         *
         * @param newPoint The new point to add to the polytope
         * @param supportA The term on the LHS of the Minkowski difference that created `newPoint`
         * @param supportB The term on the RHS of the Minkowski difference that created `newPoint`
         *
         * @retval false New point is not beyond current closest point in latest
         * search direction.
         * @retval true Successfully appended triangles including new point to the list.
         */
        bool append(const glm::vec3& newPoint, const glm::vec3& supportA, const glm::vec3& supportB);
    };

    /**
     * @brief Object representing contact information between a pair of convex shapes from the
     * perspective of _one_ of those shapes.
     *
     * All 3D points and vectors are given relative to the world.  Additional processing of
     * this data is necessary to convert them to local-space data.
     *
     */
    struct Contact {
        /**
         * @brief The worldspace point on the surface of this shape that made contact with the other shape
         *
         */
        glm::vec3 mPoint;

        /**
         * @brief Worldspace normal pointing inward from the surface of this
         * shape such that moving in this direction by the penetration depth
         * will remove the overlap.
         */
        glm::vec3 mNormal;

        /**
         * @brief Worldspace tangent orthogonal to the contact normal
         * and the other contact tangent.
         */
        glm::vec3 mTangent1;

        /**
         * @brief Worldspace tangent orthogonal to the contact normal
         * and the other contact tangent.
         */
        glm::vec3 mTangent2;

        /**
         * @brief The length by which to move this object in the direction of
         * the contact normal to separate the colliding objects
         *
         */
        float mPenetrationDepth;
    };

    /**
     * @brief Data representing everything about a collision.
     *
     * This data includes:
     *
     * - Whether a collision occurred
     *
     * - Contact information relative to each object, referred to as A and
     * B, depending on the order of parameters passed to the collision evaluator
     *
     */
    struct Collision {
        /**
         * @brief Whether a collision occurred
         *
         */
        bool mCollided;

        /**
         * @brief Contact information relative to the _first_ collision shape
         *
         */
        Contact mContactA;

        /**
         * @brief Contact information relative to the _second_ collision shape
         *
         */
        Contact mContactB;
    };

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief A component defining the true bounds of a spatially queryable object situated somewhere in the world.
     *
     * Also provides methods for retrieving related axis aligned and volume aligned box properties.
     *
     * @see ECSWorld::registerComponentTypes()
     *
     */
    struct ObjectBounds {
        /**
         * @brief Fetches the component type string associated with this class.
         *
         * @return std::string The component type string of this class.
         */
        inline static std::string getComponentTypeName() { return "ObjectBounds"; }

        /**
         * @brief The types of volumes supported by the engine.
         *
         */
        enum class TrueVolumeType: uint8_t {
            BOX=0,
            SPHERE,
            CAPSULE,
        };

        /**
         * @brief A union of supported volume structs.
         *
         */
        union TrueVolume {
            VolumeBox mBox { .mDimensions{ glm::vec3{0.f} } };
            VolumeCapsule mCapsule;
            VolumeSphere mSphere;
        };

        /**
         * @brief Creates bounds for an object in the shape of a box.
         *
         * @param box The data defining the volume itself, independent of its placement.
         * @param positionOffset The offset of the center of the box, relative to the owning entity's position per its Transform.
         * @param orientationOffset The quaternion specifying the angle offset for these bounds
         * @param interactionLayers Which set of interaction layers this object belongs to
         * @param interactionMask Which set of interaction layers this object triggers interactions with
         * @return ObjectBounds The constructed object bounds.
         *
         * @todo Oriention offset should just be a quaternion.  Multiple quaternions may produce the same forward vector while still altering the up and right vectors.
         *
         */
        static ObjectBounds create(
            const VolumeBox& box,
            const glm::vec3& positionOffset,
            const glm::quat& orientationOffset,
            InteractionLayerMask interactionLayers=0x1,
            InteractionLayerMask interactionMask=std::numeric_limits<InteractionLayerMask>::max()
        );

        /**
         * @brief Creates bounds for an object in the shape of a capsule.
         *
         * @param capsule The data defining the volume itself.
         * @param positionOffset The offset of the center of the capsule, relative to the owning entity's position per its Transform.
         * @param orientationOffset The quaternion specifying the angle offset for these bounds
         * @param interactionLayers Which set of interaction layers this object belongs to
         * @param interactionMask Which set of interaction layers this object triggers interactions with
         * @return ObjectBounds The constructed object bounds.
         *
         * @todo Oriention offset should just be a quaternion.  Multiple quaternions may produce the same forward vector while still altering the up and right vectors.
         *
         */
        static ObjectBounds create(
            const VolumeCapsule& capsule,
            const glm::vec3& positionOffset,
            const glm::quat& orientationOffset,
            InteractionLayerMask interactionLayers=0x1,
            InteractionLayerMask interactionMask=std::numeric_limits<InteractionLayerMask>::max()
        );

        /**
         * @brief Creates bounds for an object in the shape of a sphere.
         *
         * @param sphere The data defining the volume itself.
         * @param positionOffset The offset of the center of the sphere, relative to the owning entity's position per its Transform.
         * @param orientationOffset The quaternion specifying the angle offset for these bounds
         * @param interactionLayers Which set of interaction layers this object belongs to
         * @param interactionMask Which set of interaction layers this object triggers interactions with
         * @return ObjectBounds The constructed object bounds.
         *
         * @todo Oriention offset should just be a quaternion.  Multiple quaternions may produce the same forward vector while still altering the up and right vectors.
         *
         */
        static ObjectBounds create(
            const VolumeSphere& sphere,
            const glm::vec3& positionOffset,
            const glm::quat& orientationOffset,
            InteractionLayerMask interactionLayers=0x1,
            InteractionLayerMask interactionMask=std::numeric_limits<InteractionLayerMask>::max()
        );

        /**
         * @brief Value indicating the type of the volume represented by this object.
         *
         */
        TrueVolumeType mType { TrueVolumeType::BOX };

        /**
         * @brief The data defining the volume itself, independent of its position.
         *
         */
        TrueVolume mTrueVolume {};

        /**
         * @brief The position, in the real world, of origin of the scene node this data is attached to.
         *
         */
        glm::vec3 mPositionOrigin { 0.f };

        /**
         * @brief The position of the origin of the spatial query volume relative to the origin of the node it is attached to.
         *
         */
        glm::vec3 mPositionOffset { 0.f };

        /**
         * @brief The orientation in the real world of the origin of the scene node this bounds component is attached to.
         *
         */
        glm::quat mOrientationOrigin { glm::vec3{ 0.f } };

        /**
         * @brief The transformation mapping forward as known by the underlying scene node, to forward as known by the spatial query volume.
         *
         */
        glm::quat mOrientationOffset { glm::vec3{ 0.f } };

        /**
         * @brief Determines which spatial queries and collisions this object participates in.
         *
         */
        InteractionLayerMask mInteractionLayers { 0x1 };

        /**
         * @brief Determines which spatial queries and collisions this object will generate.
         *
         */
        InteractionLayerMask mInteractionMask { std::numeric_limits<InteractionLayerMask>::max() };

        /**
         * @brief Whether an update on the object's transform is allowed to modify object bounds properties.
         *
         */
        bool mUpdatedFromTransform { true };

        /**
         * @brief Flag set when object position or orientation has been set through this
         * component, indicating that the user facing Transform should be updated accordingly
         *
         */
        bool mTransformUpdateRequired { false };

        /**
         * @brief Flag set when these object bounds were set by the engine, through light
         * bounds compute or model bounds compute.
         *
         * This is set to false when bounds are set by the scene description or by a
         * user-side aspect.
         *
         * @see SpatialQuerySystem::StaticModelBoundsComputeSystem
         * @see SpatialQuerySystem::LightBoundsComputeSystem
         *
         */
        bool mVolumeSystemComputed { true };

        /**
         * @brief Flag set when this object's bounds must be recomputed by a related
         * system
         *
         * @see SpatialQuerySystem::StaticModelBoundsComputeSystem
         * @see SpatialQuerySystem::LightBoundsComputeSystem
         */
        bool mVolumeUpdateRequired { true };

        /**
         * @brief Computes new mPosition and mOrientation offsets based on (presumably) the model transform of the underlying scene object.
         *
         * @param modelMatrix The transform specifying the new origin of the underlying scene object.
         */
        void applyModelMatrix(const glm::mat4& modelMatrix);

        /**
         * @brief Gets the rotation matrix associated with this object's orientation offset.
         *
         * @return glm::mat3 This object's orientation offset's rotation matrix representation.
         */
        inline glm::mat3 getRotationTransformLocal() const {
            return glm::mat3_cast(glm::normalize(mOrientationOffset));
        }

        /**
         * @brief Gets the rotation matrix associated with the underlying scene object's orientation
         *
         * @return glm::mat3 The orientation of the underlying scene object's origin.
         */
        inline glm::mat4 getRotationTransformOrigin() const {
            return glm::mat4_cast(glm::normalize(mOrientationOrigin));
        }

        /**
         * @brief Gets the translation matrix associated with the underlying scene object's translation
         *
         * @return glm::mat4 The translation matrix of the underlying scene object's origin
         */
        inline glm::mat4 getTranslationTransformOrigin() const {
            return glm::mat4 {
                {   1.f, 0.f, 0.f, 0.f },
                {   0.f, 1.f, 0.f, 0.f },
                {   0.f, 0.f, 1.f, 0.f },
                { mPositionOrigin, 1.f },
            };
        }

        /**
         * @brief The final position of the origin of the object bounds in the world.
         *
         * @return glm::vec3 The origin of this node's object bounds.
         */
        glm::vec3 getPositionWorld() const;

        /**
         * @brief Sets the new world position for the object while keeping its offset relative
         * to its entity fixed.
         *
         * @param newPosition The new position this object should appear in
         */
        void setPositionWorld(const glm::vec3& newPosition);

        /**
         * @brief The final orientation of the object bounds in the world.
         *
         * Orientation here refers to global orientation, where w=1,xyz=0 corresponds to the object facing
         * down the -Z axis
         *
         * @return glm::quat The orientation of this node's object bounds.
         */
        glm::quat getOrientationWorld() const;

        /**
         * @brief Sets the new world orientation for the object while keeping its offset relative to
         * its entity fixed.
         *
         * Orientation here refers to global orientation, where w=1,xyz=0 corresponds to the object facing
         * down the -Z axis
         *
         */
        void setOrientationWorld(const glm::quat& newOrientation);

        /**
         * @brief Gets the corners of the box just encapsulating this object's true volume, relative to the origin of the spatial query volume alone.
         *
         * @return std::array<glm::vec3, 8> An array of coordinates representing the corners of the box just containing the spatial query volume.
         */
        std::array<glm::vec3, 8> getVolumeRelativeBoxCorners() const;

        /**
         * @brief Gets the corners of the box just encapsulating this object's true volume and sharing its position and orientation, relative to
         * the origin of the underlying scene node at 0,0,0 (in short, in model space).
         *
         * @return std::array<glm::vec3, 8> An array of coordinates of bounds-aligned box corners in model space.
         */
        std::array<glm::vec3, 8> getLocalOrientedBoxCorners() const;

        /**
         * @brief Gets the corners of the box just encapsulating this object's true volume relative to the origin of the underlying scene node in world space.
         *
         * @return std::array<glm::vec3, 8> An array of coordinates of bounds-aligned box corners in world space.
         */
        std::array<glm::vec3, 8> getWorldOrientedBoxCorners() const;

        /**
         * @brief Gets an array of triangles that make up the faces of the bounds-aligned box corners in world space.
         *
         * @return std::array<AreaTriangle, 12> An array of triangles for the faces of the bounds-aligned box.
         *
         * @see getWorldOrientedBoxCorners()
         *
         */
        std::array<AreaTriangle, 12> getWorldOrientedBoxFaceTriangles() const { return computeBoxFaceTriangles(getWorldOrientedBoxCorners()); };

        /**
         * @brief Returns the point on this object's surface furthest along a given axis from the origin of
         * this object.
         *
         * Strongly related to the Gilbert-Johnson-Keerthi (GJK) algorithm for detecting intersections
         * of convex shapes.
         *
         * @see Simplex
         *
         */
        glm::vec3 getSupportAlong(const glm::vec3& axis) const;

        /**
         * @brief Returns this shape's projection along some unit vector.
         *
         * @return std::pair<float, float> The minimum and maximum points of this shape on the vector
         *
         */
        std::pair<float, float> getProjectionAlong(const glm::vec3& axis) const;

        /**
         * @brief Returns whether the underlying volume has sensible parameters (i.e., finite, non-degenerate, non-negative
         * parameters)
         *
         */
        bool isSensible() const;

        /**
         * @ingroup ToyMakerSpatialQuerySystem
         * @brief Checks whether underlying bounds is non-trivial, as in each important
         * parameter that represents the volume is greater than 0
         */
        bool isPositiveStrict() const;

        /**
         * @ingroup ToyMakerSpatialQuerySystem
         * @brief Returns an integer representing which spatial queries and collisions this object will participate in
         *
         */
        inline InteractionLayerMask getInteractionLayers() const { return mInteractionLayers; }

        /**
         * @ingroup ToyMakerSpatialQuerySystem
         * @brief Moves this object into a new group of interaction layers
         *
         */
        inline void setInteractionLayers(InteractionLayerMask newLayers) { mInteractionLayers = newLayers; }

        /**
         * @ingroup ToyMakerSpatialQuerySystem
         * @brief Returns an integer representing which layers this object will generate spatial queries and collisions for
         *
         */
        inline InteractionLayerMask getInteractionMask() const { return mInteractionMask; }

        /**
         * @ingroup ToyMakerSpatialQuerySystem
         * @brief Changes which set of interaction layers this object can trigger events with
         *
         */
        inline void setInteractionMask(InteractionLayerMask newMask) { mInteractionMask = newMask; }
    protected:
    };

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief An object containing a coarse simplified representation, AABB, of spatially queryable objects.
     *
     * AABBs, Axis-aligned bounding boxes, are defined by two 3D coordinates, each corresponding to opposite corners of an axis-aligned box in the world.  The axes here are the unit vectors of the world space (in which exists the node owning this component).
     *
     * An AABB will just encapsulate all the corners of the ObjectBounds it is derived from.
     *
     * @see ECSWorld::registerComponentTypes()
     *
     */
    class AxisAlignedBounds {
    public:
        /**
         * @brief Gets the component type string for this object.
         *
         * @return std::string The component type string of this class.
         */
        inline static std::string getComponentTypeName() { return "AxisAlignedBounds"; }

        /**
         * @brief Pair where `first`: right top front corner;  `second`: left back bottom corner of an AABB
         *
         */
        using Extents = std::pair<glm::vec3, glm::vec3>;

        /**
         * @brief Constructs a new empty Axis Aligned Bounds object.
         *
         */
        AxisAlignedBounds();

        /**
         * @brief Construct a new Axis Aligned Bounds object based on ObjectBounds.
         *
         * @param objectBounds The basis for the new AABB.
         */
        AxisAlignedBounds(const ObjectBounds& objectBounds);

        /**
         * @brief Constructs a new Axis Aligned Bounds object based on a pair of coordinates representing the top-right-front and bottom-left-back corners of the axis aligned box.
         *
         * @param axisAlignedExtents A pair of coordinates representing the top-right-front and bottom-left-back corners of an axis aligned box.
         * @param interactionLayers An integer indicating what spatial queries and collisions this object will participate in
         */
        AxisAlignedBounds(const Extents& axisAlignedExtents, InteractionLayerMask interactionLayers=0x0);

        /**
         * @brief Constructs a new Axis Aligned Bounds object based on the position of the origin and the dimensions of the box.
         *
         * @param position Box origin.
         * @param dimensions Box breadth, height, and depth.
         */
        AxisAlignedBounds(const glm::vec3& position, const glm::vec3& dimensions, InteractionLayerMask interactionLayers=0x0):
        AxisAlignedBounds { Extents{{position + .5f*dimensions}, {position - .5f*dimensions}}, interactionLayers }
        {}

        /**
         * @brief Creates a new axis-aligned box which just contains both this object and the box being added to it.
         *
         * @param other The box being added to this box.
         * @return AxisAlignedBounds Bounds just containing the boxes being added.
         */
        AxisAlignedBounds operator+(const AxisAlignedBounds& other) const;

        /**
         * @brief Gets an array of coordinates of the corners of this box.
         *
         * @return std::array<glm::vec3, 8> This box's corner coordinates.
         */
        std::array<glm::vec3, 8> getAxisAlignedBoxCorners() const;

        /**
         * @brief Gets an array of triangles in the world which make up the surface of this box.
         *
         * @return std::array<AreaTriangle, 12> This box's surface triangles.
         */
        inline std::array<AreaTriangle, 12> getAxisAlignedBoxFaceTriangles() const { return computeBoxFaceTriangles(getAxisAlignedBoxCorners()); }

        /**
         * @brief Gets the pair of coordinates representing the extreme corners of this box.
         *
         * @return Extents This box's top-front-right and bottom-back-left corners.
         */
        Extents getAxisAlignedBoxExtents() const;

        /**
         * @brief Gets the dimensions of this box.
         *
         * @return glm::vec3 In order: breadth, height, depth.
         */
        inline glm::vec3 getDimensions() const { return mExtents.first - mExtents.second; }

        /**
         * @brief Gets the coordinates of the center of this box
         *
         * @return glm::vec3 The coordinates of the center of this box
         */
        inline glm::vec3 getPositionWorld() const { return mExtents.second + .5f * getDimensions(); }

        /**
         * @brief Tests whether this box is sensible (it has a finite position, and finite non-negative dimensions).
         *
         * @retval true This box is sensible;
         * @retval false This box is not sensible;
         */
        inline bool isSensible() const {
            return (
                isFinite(mExtents.first) && isFinite(mExtents.second)
                && isNonNegative(mExtents.first - mExtents.second)
            );
        }

        /**
         * @brief Returns the point on this object's surface furthest along a given axis from the origin of
         * this object.
         *
         * Strongly related to the Gilbert-Johnson-Keerthi (GJK) algorithm for detecting intersections
         * of convex shapes.
         *
         */
        glm::vec3 getSupportAlong(const glm::vec3& axis) const;

        /**
         * @brief Tests whether this box has strictly positive parameters and hence encloses some region in space.
         *
         * @retval true This box encloses some real region;
         * @retval false This box does not enclose any real region;
         */
        inline bool isPositiveStrict() const {
            return (
                ToyMaker::isPositiveStrict(mExtents.first - mExtents.second)
            );
        }

        /**
         * @ingroup ToyMakerSpatialQuerySystem
         * @brief Returns an integer representing which spatial queries and collisions this object will participate in
         *
         */
        inline InteractionLayerMask getInteractionLayers() const { return mInteractionLayers; }

        /**
         * @ingroup ToyMakerSpatialQuerySystem
         * @brief Sets interaction layer mask for this object
         *
         */
        inline void setInteractionLayers(InteractionLayerMask newLayers) { mInteractionLayers = newLayers; }

        /**
         * @brief Sets the position of this box.
         *
         * @param position The new position of this box.
         */
        inline void setPosition(const glm::vec3& position) {
            assert(isFinite(position) && "Invalid position specified. Position must be finite");
            const glm::vec3 deltaPosition { position - getPositionWorld() };
            mExtents.first += deltaPosition;
            mExtents.second += deltaPosition;
        }

        /**
         * @brief Sets the dimensions of this box.
         *
         * @param dimensions The new dimensions of this box.
         */
        inline void setDimensions(const glm::vec3& dimensions) {
            assert(isNonNegative(dimensions) && isFinite(dimensions) && "Invalid dimensions provided.  Dimensions must be non negative and finite");
            const glm::vec3 position { getPositionWorld() };
            const glm::vec3 deltaDimensions { dimensions - getDimensions() };
            mExtents.first += .5f * deltaDimensions;
            mExtents.second -= .5f * deltaDimensions;
        }
    private:
        /**
         * @brief Sets the extents of this box.
         *
         * @param axisAlignedExtents A pair of coordinates representing the top-right-front and bottom-left-back corners of this box.
         */
        void setByExtents(const Extents& axisAlignedExtents);

        /**
         * @brief The pair of coordinates at the extreme corners of this box (i.e., the top-right-front and bottom-left-back corners)
         *
         */
        Extents mExtents {glm::vec3{0.f}, glm::vec3{0.f}};

        /**
         * @brief Determines which spatial queries and collisions this object participates in.
         *
         */
        InteractionLayerMask mInteractionLayers { 0x0 };
    };

    /**
     * @ingroup ToyMakerSerialization ToyMakerSpatialQuerySystem
     *
     */
    NLOHMANN_JSON_SERIALIZE_ENUM( ObjectBounds::TrueVolumeType, {
        {ObjectBounds::TrueVolumeType::BOX, "box"},
        {ObjectBounds::TrueVolumeType::SPHERE, "sphere"},
        {ObjectBounds::TrueVolumeType::CAPSULE, "capsule"},
    })

    /**
     * @ingroup ToyMakerSerialization ToyMakerSpatialQuerySystem
     *
     */
    inline void to_json(nlohmann::json& json, const ObjectBounds& objectBounds) {
        json = {
            {"type", ObjectBounds::getComponentTypeName()},
            {"volume_type", objectBounds.mType},
            {"position_offset", { objectBounds.mPositionOffset.x, objectBounds.mPositionOffset.y, objectBounds.mPositionOffset.z}},
            {"orientation_offset", { objectBounds.mOrientationOffset.w, objectBounds.mOrientationOffset.x, objectBounds.mOrientationOffset.y, objectBounds.mOrientationOffset.z }},
        };
        switch(objectBounds.mType) {
            case ObjectBounds::TrueVolumeType::BOX:
                json["volume_properties"] = {
                    {"width", objectBounds.mTrueVolume.mBox.mDimensions.x},
                    {"height", objectBounds.mTrueVolume.mBox.mDimensions.y},
                    {"depth", objectBounds.mTrueVolume.mBox.mDimensions.z},
                };
                break;
            case ObjectBounds::TrueVolumeType::SPHERE:
                json["volume_properties"] = {
                    {"radius", objectBounds.mTrueVolume.mSphere.mRadius},
                };
                break;
            case ObjectBounds::TrueVolumeType::CAPSULE:
                json["volume_properties"] = {
                    {"radius", objectBounds.mTrueVolume.mCapsule.mRadius},
                    {"height", objectBounds.mTrueVolume.mCapsule.mHeight},
                };
                break;
        };
    }

    /** @ingroup ToyMakerSerialization ToyMakerSpatialQuerySystem */
    inline void from_json(const nlohmann::json& json, ObjectBounds& objectBounds) {
        assert(json.at("type") == ObjectBounds::getComponentTypeName() && "Incorrect type property for an objectBounds component");
        const glm::vec3 positionOffset {
            json.at("position_offset")[0],
            json.at("position_offset")[1],
            json.at("position_offset")[2],
        };
        const glm::vec3 orientationOffset {
            glm::eulerAngles(glm::normalize(glm::quat {
                json.at("orientation_offset")[0],
                json.at("orientation_offset")[1],
                json.at("orientation_offset")[2],
                json.at("orientation_offset")[3]
            }))
        };
        const InteractionLayerMask interactionLayers { json.at("interaction_layers").get<InteractionLayerMask>() };
        const InteractionLayerMask interactionMask { json.at("interaction_mask").get<InteractionLayerMask>() };

        switch (static_cast<ObjectBounds::TrueVolumeType>(json.at("volume_type"))) {
            case ObjectBounds::TrueVolumeType::BOX:
                objectBounds = ObjectBounds::create(
                    VolumeBox{ .mDimensions {
                        json.at("volume_properties").at("width").get<float>(),
                        json.at("volume_properties").at("height").get<float>(),
                        json.at("volume_properties").at("depth").get<float>(),
                    } },
                    positionOffset,
                    orientationOffset,
                    interactionLayers,
                    interactionMask
                );
                break;

            case ObjectBounds::TrueVolumeType::SPHERE:
                objectBounds = ObjectBounds::create(
                    VolumeSphere { .mRadius { json.at("volume_properties").at("radius").get<float>() } },
                    positionOffset,
                    orientationOffset,
                    interactionLayers,
                    interactionMask
                );
                break;

            case ObjectBounds::TrueVolumeType::CAPSULE:
                objectBounds = ObjectBounds::create(
                    VolumeCapsule {
                        .mHeight { json.at("volume_properties").at("height").get<float>() },
                        .mRadius { json.at("volume_properties").at("radius").get<float>() },
                    },
                    positionOffset,
                    orientationOffset,
                    interactionLayers,
                    interactionMask
                );
                break;
        }
        objectBounds.mVolumeSystemComputed = false;
    }

    /** @ingroup ToyMakerSerialization ToyMakerSpatialQuerySystem */
    inline void to_json(nlohmann::json& json, const AxisAlignedBounds& axisAlignedBounds) { /* never used, so pass */
        (void)json; // prevent unused parameter warnings
        (void)axisAlignedBounds; // prevent unused parameter warnings
    }

    /** @ingroup ToyMakerSerialization ToyMakerSpatialQuerySystem */
    inline void from_json(const nlohmann::json& json, AxisAlignedBounds& objectBounds) { /* never used, so pass */
        (void)json; // prevent unused parameter warnings
        (void)objectBounds; // prevent unused parameter warnings
    }
}

#endif


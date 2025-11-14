/**
 * @ingroup ToyMakerSpatialQuerySystem
 * @file spatial_query_math.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Geometrical, mathematical functions and related structs used to answer some simple questions about shapes situated somewhere in the world.
 * 
 * @version 0.3.2
 * @date 2025-09-08
 * 
 * 
 */

#ifndef FOOLSENGINE_SPATIALQUERYMATH_H
#define FOOLSENGINE_SPATIALQUERYMATH_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "spatial_query_basic_types.hpp"
#include "core/ecs_world.hpp"

namespace ToyMaker {
    struct ObjectBounds;
    class AxisAlignedBounds;

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
    // std::pair<uint8_t, std::pair<glm::vec3, glm::vec3>> computeIntersections(const Ray& ray, const ObjectBounds& objectbounds);

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns a bool value indicating whether the ray passes through the object volume
     */
    // bool overlaps(const Ray& ray, const ObjectBounds& objectBounds);

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `point` is contained by `bounds`.
     * 
     */
    bool overlaps(const glm::vec3& point, const AxisAlignedBounds& bounds);

    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `ray` overlaps with `bounds`.
     * 
     */
    bool overlaps(const Ray& ray, const AxisAlignedBounds& bounds);

    /** 
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns whether `one` overlaps `two`. 
     * 
     */
    bool overlaps(const AxisAlignedBounds& one, const AxisAlignedBounds& two);

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
     * @brief Returns whether `one` is contained by `two`. 
     * 
     */
    bool contains(const AxisAlignedBounds& one, const AxisAlignedBounds& two);

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
            BOX,
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
         * @param orientationOffset The direction that is considered "forward" when the object is in model space.
         * @return ObjectBounds The constructed object bounds.
         * 
         * @todo Oriention offset should just be a quaternion.  Multiple quaternions may produce the same forward vector while still altering the up and right vectors.
         * 
         */
        static ObjectBounds create(const VolumeBox& box, const glm::vec3& positionOffset, const glm::vec3& orientationOffset);

        /**
         * @brief Creates bounds for an object in the shape of a capsule.
         * 
         * @param capsule The data defining the volume itself.
         * @param positionOffset The offset of the center of the capsule, relative to the owning entity's position per its Transform.
         * @param orientationOffset The direction that is considered "forward" when the object is in model space.
         * @return ObjectBounds The constructed object bounds.
         * 
         * @todo Oriention offset should just be a quaternion.  Multiple quaternions may produce the same forward vector while still altering the up and right vectors.
         * 
         */
        static ObjectBounds create(const VolumeCapsule& capsule, const glm::vec3& positionOffset, const glm::vec3& orientationOffset);

        /**
         * @brief Creates bounds for an object in the shape of a sphere.
         * 
         * @param sphere The data defining the volume itself.
         * @param positionOffset The offset of the center of the sphere, relative to the owning entity's position per its Transform.
         * @param orientationOffset The direction that is considered "forward" when the object is in model space.
         * @return ObjectBounds The constructed object bounds.
         * 
         * @todo Oriention offset should just be a quaternion.  Multiple quaternions may produce the same forward vector while still altering the up and right vectors.
         * 
         */
        static ObjectBounds create(const VolumeSphere& sphere, const glm::vec3& positionOffset, const glm::vec3& orientationOffset);

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
         * @brief The position, in the real world, of the scene node this data is attached to.
         * 
         */
        glm::vec3 mPosition { 0.f };

        /**
         * @brief The position of the origin of the spatial query volume relative to the origin of the node it is attached to.
         * 
         */
        glm::vec3 mPositionOffset { 0.f };

        /**
         * @brief The orientation in the real world of the scene node this bounds component is attached to.
         * 
         */
        glm::quat mOrientation { glm::vec3{ 0.f } };

        /**
         * @brief The transformation mapping forward as known by the underlying scene node, to forward as known by the spatial query volume.
         * 
         * 
         */
        glm::quat mOrientationOffset { glm::vec3{ 0.f } };

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
        inline glm::mat3 getLocalRotationTransform() const { 
            return glm::mat3_cast(glm::normalize(mOrientationOffset));
        }

        /**
         * @brief Gets the rotation matrix associated with the underlying scene object's orientation, derived from its cached transform.
         * 
         * @return glm::mat3 The orientation of the underlying scene object.
         */
        inline glm::mat3 getWorldRotationTransform() const {
            return glm::mat3_cast(glm::normalize(mOrientation));
        }

        /**
         * @brief The final position of the origin of the object bounds in the world.
         * 
         * @return glm::vec3 The origin of this node's object bounds.
         */
        glm::vec3 getComputedWorldPosition() const;

        /**
         * @brief The final orientation of the object bounds in the world.
         * 
         * @return glm::quat The orientation of this node's object bounds.
         */
        glm::quat getComputedWorldOrientation() const;

        /**
         * @brief Gets the corners of the box just encapsulating this object's true volume, relative to the origin of the spatial query volume alone.
         * 
         * @return std::array<glm::vec3, 8> An array of coordinates representing the corners of the box just containing the spatial query volume.
         */
        std::array<glm::vec3, 8> getVolumeRelativeBoxCorners() const;

        /**
         * @brief Gets the corners of the box just encapsulating this object's true volume and sharing its position and orientation, relative to the origin of the underlying scene node at 0,0,0 (in short, in model space).
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
         */
        AxisAlignedBounds(const Extents& axisAlignedExtents);

        /**
         * @brief Constructs a new Axis Aligned Bounds object based on the position of the origin and the dimensions of the box.
         * 
         * @param position Box origin.
         * @param dimensions Box breadth, height, and depth.
         */
        AxisAlignedBounds(const glm::vec3& position, const glm::vec3& dimensions):
        AxisAlignedBounds { Extents{{position + .5f*dimensions}, {position - .5f*dimensions}} }
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
         * @brief Gets the position of the origin of this box.
         * 
         * @return glm::vec3 
         */
        inline glm::vec3 getPosition() const { return mExtents.second + .5f * getDimensions(); }; 

        /**
         * @brief Tests whether this box is sensible (it has a finite position, and finite positive dimensions).
         * 
         * @retval true This box is sensible;
         * @retval false This box is not sensible;
         */
        inline bool isSensible() const {
            return (
                isFinite(mExtents.first) && isFinite(mExtents.second)
            );
        }

        /**
         * @brief Sets the position of this box.
         * 
         * @param position The new position of this box.
         */
        inline void setPosition(const glm::vec3& position) {
            assert(isFinite(position) && "Invalid position specified. Position must be finite");
            const glm::vec3 deltaPosition { position - getPosition() };
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
            const glm::vec3 position { getPosition() };
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

        switch (static_cast<ObjectBounds::TrueVolumeType>(json.at("volume_type"))) {
            case ObjectBounds::TrueVolumeType::BOX:
                objectBounds = ObjectBounds::create(
                    VolumeBox{ .mDimensions {
                        json.at("volume_properties").at("width").get<float>(),
                        json.at("volume_properties").at("height").get<float>(),
                        json.at("volume_properties").at("depth").get<float>(),
                    } },
                    positionOffset,
                    orientationOffset
                );
                break;

            case ObjectBounds::TrueVolumeType::SPHERE:
                objectBounds = ObjectBounds::create(
                    VolumeSphere { .mRadius { json.at("volume_properties").at("radius").get<float>() }},
                    positionOffset,
                    orientationOffset
                );
                break;

            case ObjectBounds::TrueVolumeType::CAPSULE:
                objectBounds = ObjectBounds::create(
                    VolumeCapsule {
                        .mHeight { json.at("volume_properties").at("height").get<float>() },
                        .mRadius { json.at("volume_properties").at("radius").get<float>() },
                    },
                    positionOffset,
                    orientationOffset
                );
                break;
        }
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

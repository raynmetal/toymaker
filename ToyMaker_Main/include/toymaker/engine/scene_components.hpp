/**
 * @ingroup ToyMakerSceneSystem
 * @file scene_components.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Stores structs and classes for common components used by the SceneSystem and other related Systems.
 * @version 0.3.2
 * @date 2025-09-05
 * 
 * 
 */

#ifndef FOOLSENGINE_SCENECOMPONENTS_H
#define FOOLSENGINE_SCENECOMPONENTS_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <nlohmann/json.hpp>

#include "core/ecs_world.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToyMakerSceneSystem ToyMakerECSComponent
     * @brief A component representing the position, rotation, and scale of an entity.
     * 
     * @see ECSWorld::registerComponentTypes()
     * 
     */
    struct Placement {
        /**
         * @brief This entity's position.
         * 
         */
        glm::vec4 mPosition {glm::vec3{ 0.f }, 1.f};

        /**
         * @brief This entity's orientation, as a glm::quat value.
         * 
         * The forward vector for an object is found by applying this quaternion to a vector facing the -Z axis (i.e {0.0, 0.0, -1.0, 0.0}) after having applied rotation transforms of objects higher up in the scene hierarchy.
         * 
         */
        glm::quat mOrientation { glm::vec3{ 0.f } };

        /**
         * @brief Factors along each axis by which geometry should be multiplied and scaled.
         * 
         */
        glm::vec3 mScale { 1.f, 1.f, 1.f };

        /**
         * @brief Gets the component type string for this component.
         * 
         * @return std::string The component type string for this object.
         */
        inline static std::string getComponentTypeName() { return "Placement"; }

        /**
         * @brief Compares this placement object to another for equality.
         * 
         * @param other The placement this one is being compared to.
         * @retval true These two placement objects are equal;
         * @retval false These two placement objects are not equal;
         */
        inline bool operator==(const Placement& other) {
            return (
                mPosition == other.mPosition 
                && mOrientation == other.mOrientation
                && mScale == other.mScale
            );
        }

        /**
         * @brief An explicit specification for the inequality operator.
         * 
         * @param other The placement this one is being compared to for inequality.
         * @retval true These two placements are unequal.
         * @retval false These two placements are equal.
         */
        inline bool operator!=(const Placement& other) { 
            return !(*this == other);
        }
    };

    /**
     * @ingroup ToyMakerSceneSystem ToyMakerECSComponent
     * @brief The transform component, which moves the vertices of a model to their world space coordinates during rendering.
     * 
     * Computed based on the parameters specified in the placement component along with transforms of objects higher up in the scene hierarchy.
     * 
     * @see ECSWorld::registerComponentTypes()
     * 
     */
    struct Transform {
        /**
         * @brief The actual currently cached model matrix for this entity.
         * 
         */
        glm::mat4 mModelMatrix {1.f};

        /**
         * @brief Gets the component type string for this object.
         * 
         * @return std::string This object's component type string.
         */
        inline static std::string getComponentTypeName() { return "Transform"; }
    };

    /**
     * @ingroup ToyMakerSceneSystem ToyMakerECSComponent
     * @brief Component representing hierarchical information related to this entity.
     * 
     * Necessary because quite often systems will not/should not have direct access to the SceneSystem.
     * 
     * @see ECSWorld::registerComponentTypes()
     * 
     */
    struct SceneHierarchyData {
        /**
         * @brief The entityID of this entity's parent.
         * 
         * Set to kMaxEntities when this entity has no parent (eg., an entity not in the scene, or the root entity of the scene tree).
         * 
         */
        EntityID mParent { kMaxEntities };

        /**
         * @brief The entityID of this entity's next sibling.
         * 
         * Set to kMaxEntities when this entity has no next sibling.
         * 
         */
        EntityID mSibling { kMaxEntities };

        /**
         * @brief The first child of this entity.
         * 
         * Set to kMaxEntities if this entity is a leaf node and has no children.
         * 
         */
        EntityID mChild { kMaxEntities };

        /**
         * @brief Gets the component type string associated with this object.
         * 
         * @return std::string This object's component type string.
         */
        inline static std::string getComponentTypeName() { return "SceneHierarchyData"; }
    };

    /** @ingroup ToyMakerSerialization ToyMakerSceneSystem */
    inline void from_json(const nlohmann::json& json, Placement& placement) {
        assert(json.at("type").get<std::string>() == Placement::getComponentTypeName() && "Type mismatch. Component json must have type Placement");
        json.at("position")[0].get_to(placement.mPosition.x);
        json.at("position")[1].get_to(placement.mPosition.y);
        json.at("position")[2].get_to(placement.mPosition.z);
        json.at("position")[3].get_to(placement.mPosition.w);

        json.at("orientation")[0].get_to(placement.mOrientation.w);
        json.at("orientation")[1].get_to(placement.mOrientation.x);
        json.at("orientation")[2].get_to(placement.mOrientation.y);
        json.at("orientation")[3].get_to(placement.mOrientation.z);
        placement.mOrientation = glm::normalize(placement.mOrientation);

        json.at("scale")[0].get_to(placement.mScale.x);
        json.at("scale")[1].get_to(placement.mScale.y);
        json.at("scale")[2].get_to(placement.mScale.z);
    }

    /** @ingroup ToyMakerSerialization  ToyMakerSceneSystem */
    inline void to_json(nlohmann::json& json, const Placement& placement) {
        json = {
            {"type", Placement::getComponentTypeName()},
            {"position", {
                placement.mPosition.x,
                placement.mPosition.y,
                placement.mPosition.z,
                placement.mPosition.w,
            }},
            {"orientation", {
                placement.mOrientation.w,
                placement.mOrientation.x,
                placement.mOrientation.y,
                placement.mOrientation.z,
            }},
            {"scale", {
                placement.mScale.x,
                placement.mScale.y,
                placement.mScale.z,
            }}
        };
    }

    /** @ingroup ToyMakerSerialization ToyMakerSceneSystem */
    inline void to_json(nlohmann::json& json, const SceneHierarchyData& sceneHierarchyData) {
        (void)json; (void)sceneHierarchyData; // prevent unused parameter warnings
    }

    /** @ingroup ToyMakerSerialization ToyMakerSceneSystem */
    inline void from_json(const nlohmann::json& json, SceneHierarchyData& sceneHierarchyData) {
        (void)json; (void)sceneHierarchyData; // prevent unused parameter warnings
    }

    /** @ingroup ToyMakerSerialization ToyMakerSceneSystem */
    inline void to_json(nlohmann::json& json, const Transform& transform) {
        (void)transform; // prevent unused parameter warnings
        json = {
            {"type", Transform::getComponentTypeName()},
        };
    }

    /** @ingroup ToyMakerSerialization ToyMakerSceneSystem */
    inline void from_json(const nlohmann::json& json, Transform& transform) {
        assert(json.at("type") == Transform::getComponentTypeName() && "Type mismatch. Component json must have type Transform");
        transform.mModelMatrix = glm::mat4{1.f};
    }

    /**
     * @ingroup ToyMakerECSComponent ToyMakerSceneSystem
     * @brief Override of the Placement component's Interpolator.
     * 
     * Uses linear interpolation for position and scale, and spherical interpolation for quaternions.
     * 
     * @tparam Specialization of the Interpolator<T> object
     * @param previousState The Placement value at the end of the last simulation step.
     * @param nextState The Placement value which will be at the start of the next simulation step.
     * @param simulationProgress Progress, as a number from 0 to 1, from the previousState to the nextState as of this frame.
     * @return Placement The interpolated Placement value.
     */
    template<>
    inline Placement Interpolator<Placement>::operator() (
        const Placement& previousState, const Placement& nextState,
        float simulationProgress
    ) const {
        simulationProgress = mProgressLimits(simulationProgress);
        return {
            .mPosition{ (1.f - simulationProgress) * previousState.mPosition + simulationProgress * nextState.mPosition },
            .mOrientation{ glm::slerp(previousState.mOrientation, nextState.mOrientation, simulationProgress) },
            .mScale{ (1.f - simulationProgress) * previousState.mScale + simulationProgress * nextState.mScale }
        };
    }

    /**
     * @ingroup ToyMakerECSComponent ToyMakerSceneSystem
     * @brief Override of the Transform component's Interpolator.
     * 
     * Simple linear interpolation for this Transform's model matrix.
     * 
     * @tparam Specialization of the Interpolator<T> object.
     * @param previousState The state of Transform at the end of the last simulation step.
     * @param nextState The state of the Transform which will be at the beginning of the next simulation step.
     * @param simulationProgress The progress towards the next state from the previous state, as a value betweeen 0 and 1.
     * @return Transform The interpolated value of Transform.
     */
    template<>
    inline Transform Interpolator<Transform>::operator() (
        const Transform& previousState, const Transform& nextState,
        float simulationProgress
    ) const {
        simulationProgress = mProgressLimits(simulationProgress);
        return {
            previousState.mModelMatrix * (1.f - simulationProgress)
            + nextState.mModelMatrix * (simulationProgress)
        };
    }

    /**
     * @ingroup ToyMakerECSComponent ToyMakerSceneSystem
     * @brief Override of the SceneHierarchyData Interpolator.
     * 
     * Simply returns SceneHierarchyData in its most current state, its state in the next simulation step.
     * 
     * @tparam Specialization of Interpolator for SceneHierarchyData.
     * @param previousState The state of the component at the end of the previous simulation step.
     * @param nextState The state of the component as it will be at the beginning of the next simulation step.
     * @param simulationProgress The progress towards the next state from the previous state as a number between 0 and 1.
     * @return SceneHierarchyData The latest value of this component.
     */
    template <>
    inline SceneHierarchyData Interpolator<SceneHierarchyData>::operator() (
        const SceneHierarchyData& previousState, const SceneHierarchyData& nextState,
        float simulationProgress
    ) const {
        (void)previousState; // prevent unused parameter warnings
        (void)simulationProgress; // prevent unused parameter warnings
        return nextState;
    }

}

#endif

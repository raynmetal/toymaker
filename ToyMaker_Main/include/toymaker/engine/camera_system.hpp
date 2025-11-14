/**
 * @ingroup ToyMakerRenderSystem
 * @file camera_system.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains headers for the system and structs used by the engine's camera system.
 * @version 0.3.2
 * @date 2025-09-03
 * 
 */

#ifndef FOOLSENGINE_CAMERASYSTEM_H
#define FOOLSENGINE_CAMERASYSTEM_H

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "core/ecs_world.hpp"
#include "scene_components.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToyMakerRenderSystem ToyMakerECSComponent
     * @brief Struct that encapsulates properties which define the (geometric) aspects of a scene camera.
     * 
     * Its appearance in json is as follows:
     * 
     * ```jsonc
     * 
     * {
     *     "fov": 45.0,
     *     "aspect": 0.0,
     *     "orthographic_dimensions": {
     *         "horizontal": 1366,
     *         "vertical": 768
     *     },
     *     "near_far_planes": {
     *         "near": -1000,
     *         "far": 1000
     *     },
     *     "projection_mode": "orthographic",
     *     "type": "CameraProperties"
     * }
     * 
     * ```
     * 
     * 
     * @see ECSWorld::registerComponentTypes()
     * 
     */
    struct CameraProperties {
        /**
         * @brief The type of projection used by this camera.
         * 
         */
        enum class ProjectionType: uint8_t {

            FRUSTUM, //< A frustum camera, or a camera whose view looks like a pyramid.  Objects further off appear smaller than objects close by.

            ORTHOGRAPHIC, //< A camera where measurements along a dimension are the same regardless of how close or far an object is, where the view space looks like a cuboid.
        };

        /**
         * @brief The type of projection used by the camera.
         * 
         */
        ProjectionType mProjectionType { ProjectionType::FRUSTUM };

        /**
         * @brief (If ProjectionType::FRUSTUM) The vertical Field of View described by the camera, used to calculate mProjectMatrix
         * 
         */
        float mFov {45.f};

        /**
         * @brief The ratio of the x dimension to the y dimension of the screen or image associated with the camera.
         * 
         */
        float mAspect { 16.f/9.f };

        /**
         * @brief (If ProjectionType::ORTHOGRAPHIC) The dimensions, in scene units, of the screen face of the viewing volume of an orthographic camera.
         * 
         */
        glm::vec2 mOrthographicDimensions { 19.f, 12.f };

        /**
         * @brief The distance, in scene units, at which the near and far planes of the viewing volume of are located relative to the camera's origin.
         * 
         */
        glm::vec2 mNearFarPlanes { 100.f, -100.f };

        /**
         * @brief The projection matrix of the camera, computed based on its other properties.
         * 
         */
        glm::mat4 mProjectionMatrix {};

        /**
         * @brief The view matrix of the camera, which transforms all vertices from their world coordinates to their coordinates relative to the origin of the camera, where the camera faces down the -Z axis.
         * 
         */
        glm::mat4 mViewMatrix {};

        /**
         * @brief The component type string of the camera properties component.
         * 
         * @return std::string This object's component type string.
         * 
         * 
         */
        inline static std::string getComponentTypeName() { return "CameraProperties"; }
    };

    /** 
     * @ingroup ToyMakerRenderSystem ToyMakerSerialization
     * 
     */
    NLOHMANN_JSON_SERIALIZE_ENUM(CameraProperties::ProjectionType, {
        {CameraProperties::ProjectionType::FRUSTUM, "frustum"},
        {CameraProperties::ProjectionType::ORTHOGRAPHIC, "orthographic"},
    });

    /**
     * @ingroup ToyMakerRenderSystem ToyMakerECSSystem
     * @brief System responsible for managing all active cameras belonging to this world, tracking and updating associated projection and view matrices.
     * 
     */
    class CameraSystem: public System<CameraSystem, std::tuple<Transform, CameraProperties>, std::tuple<>> {
    public:
        /**
         * @brief Construct a new CameraSystem object
         * 
         * @param world The world to which this system belongs.
         */
        explicit CameraSystem(std::weak_ptr<ECSWorld> world):
        System<CameraSystem, std::tuple<Transform, CameraProperties>, std::tuple<>>{world}
        {}

        /**
         * @brief Updates all matrices associated with active cameras in this world per their properties and positions.
         *
         */
        void updateActiveCameraMatrices();

        /**
         * @brief Returns the ECS system type string for this object.
         * 
         * @return std::string This system's ECS system type string.
         * 
         * @see ECSWorld::registerSystem()
         */
        static std::string getSystemTypeName() { return "CameraSystem"; }

    private:
        /**
         * @brief Adds enabled entity to the projection update and view update queues.
         * 
         * @param entityID The enabled entity.
         * 
         * @see mProjectionUpdateQueue
         * @see mViewUpdateQueue
         */
        void onEntityEnabled(EntityID entityID) override;

        /**
         * @brief Removes extra entity related structures from system bookkeeping, if necessary.
         * 
         * @param entityID Entity being removed.
         */
        void onEntityDisabled(EntityID entityID) override;

        /**
         * @brief Adds entity to projection and view update queues.
         * 
         * @param entityID The entity that was updated
         */
        void onEntityUpdated(EntityID entityID) override;

        /**
         * @brief Initializes the CameraSystem, querying and adding all eligible entities to update queues.
         * 
         */
        void onSimulationActivated() override;

        /**
         * @brief The step in which new projection and view matrices are actually computed for all active cameras.
         * 
         * @param simulationProgress The progress since the end of the last simulation update to the start of the next one, as a number between 0 and 1.
         */
        void onPreRenderStep(float simulationProgress) override;

        /**
         * @brief Entities whose camera properties were updated this frame, whose projection matrix should be recomputed as soon as possible.
         * 
         */
        std::set<EntityID> mProjectionUpdateQueue {};

        /**
         * @brief Entities whose position or rotation were updated this frame, whose view matrix should be recomputed as soon as possible.
         * 
         */
        std::set<EntityID> mViewUpdateQueue {};
    };

    /**
     * @ingroup ToyMakerRenderSystem ToyMakerECSComponent
     * @brief Interpolation override for the camera properties struct, mainly using linear interpolation for each member
     * 
     * @tparam CameraProperties Specialization for the CameraProperties component.
     * @param previousState The state of the component after the last simulation update.
     * @param nextState The state of the component at the end of this simulation update.
     * @param simulationProgress The progress towards the next simulation update state.
     * @return CameraProperties The interpolated CameraProperties value.
     */
    template<>
    inline CameraProperties Interpolator<CameraProperties>::operator() (
        const CameraProperties& previousState, const CameraProperties& nextState,
        float simulationProgress
    ) const {
        simulationProgress = mProgressLimits(simulationProgress);
        return {
            .mProjectionType {previousState.mProjectionType
            },
            .mFov { simulationProgress * nextState.mFov + (1.f-simulationProgress) * previousState.mFov },
            .mAspect { simulationProgress * nextState.mAspect + (1.f-simulationProgress) * previousState.mAspect},
            .mOrthographicDimensions { 
                (simulationProgress * nextState.mOrthographicDimensions)
                + (1.f-simulationProgress) * previousState.mOrthographicDimensions
            },
            .mNearFarPlanes {
                (simulationProgress * nextState.mNearFarPlanes)
                + (1.f-simulationProgress) * previousState.mNearFarPlanes
            },
            .mProjectionMatrix {
                (simulationProgress * nextState.mProjectionMatrix) 
                + ((1.f-simulationProgress) * previousState.mProjectionMatrix)
            },
            .mViewMatrix {
                (simulationProgress * nextState.mViewMatrix)
                + ((1.f-simulationProgress) * previousState.mViewMatrix)
            },
        };
    }

    /**
     * @ingroup ToyMakerSerialization ToyMakerRenderSystem
     */
    inline void from_json(const nlohmann::json& json, CameraProperties& cameraProperties) {
        assert(json.at("type").get<std::string>() == CameraProperties::getComponentTypeName() && "Type mismatch, json must be of camera properties type");
        json.at("projection_mode").get_to(cameraProperties.mProjectionType);
        json.at("fov").get_to(cameraProperties.mFov);
        json.at("aspect").get_to(cameraProperties.mAspect);
        json.at("orthographic_dimensions")
            .at("horizontal")
            .get_to(cameraProperties.mOrthographicDimensions.x);
        json.at("orthographic_dimensions")
            .at("vertical")
            .get_to(cameraProperties.mOrthographicDimensions.y);
        json.at("near_far_planes").at("near").get_to(cameraProperties.mNearFarPlanes.x);
        json.at("near_far_planes").at("far").get_to(cameraProperties.mNearFarPlanes.y);
    }

    /** @ingroup ToyMakerSerialization */
    inline void to_json(nlohmann::json& json, const CameraProperties& cameraProperties) {
        json = {
            {"type", CameraProperties::getComponentTypeName()},
            {"projection_mode", cameraProperties.mProjectionType},
            {"fov", cameraProperties.mFov},
            {"aspect", cameraProperties.mAspect},
            {"orthographic_dimensions", {
                {"horizontal", cameraProperties.mOrthographicDimensions.x},
                {"vertical", cameraProperties.mOrthographicDimensions.y},
            }},
            {"near_far_planes", {
                {"near", cameraProperties.mNearFarPlanes.x},
                {"far", cameraProperties.mNearFarPlanes.y},
            }}
        };
    }

}
#endif

/**
 * @ingroup ToyMakerRenderSystem
 * @file render_system.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains definitions relating to the render system defined for this object.
 * @version 0.3.2
 * @date 2025-09-05
 * 
 * 
 */

/**
 * @defgroup ToyMakerRenderSystem Render System
 * @ingroup ToyMakerEngine
 * 
 */

#ifndef FOOLSENGINE_RENDERSYSTEM_H
#define FOOLSENGINE_RENDERSYSTEM_H

#include <vector>
#include <map>
#include <string>
#include <array>

#include "input_system/input_system.hpp"
#include "core/ecs_world.hpp"
#include "shapegen.hpp"
#include "material.hpp"
#include "render_stage.hpp"
#include "camera_system.hpp"

namespace ToyMaker {
    struct RenderSet;

    /** @ingroup ToyMakerRenderSystem */
    using RenderSetID = uint32_t;

    /** 
     * @ingroup ToyMakerRenderSystem
     * @brief The total number of RenderSets, per ECSWorld, that can be created.
     * 
     */
    const RenderSetID kMaxRenderSetIDs { 10000 };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief A collection of shaders, render configurations, cameras, and related framebuffers used by a viewport within an ECSWorld.
     * 
     * These render sets have in common with each other opaque geometry, light sources, a skybox, and a common uniform buffer for storing camera matrices.
     * 
     */
    struct RenderSet {
        /**
         * @brief Enum listing the different rendering pipelines available.
         * 
         * @todo Split this out into different configuration files for the render system instead of their current "if-else" structure.
         */
        enum class RenderType: uint8_t {
            BASIC_3D, //< The 3D pipeline, with geometry, lighting, and post-processing shaders.
            ADDITION, //< The addition pipeline, which combines multiple textures into a single composite texture.
        };

        /**
         * @brief Renders the next debug texture to the render system's final render target.
         * 
         * Mainly a debug option.
         * 
         */
        void renderNextTexture ();

        /**
         * @brief Sets the render properties for this render set.
         * 
         * Creates and reconfigures all the shader stages in the render pipeline with the properties specified.
         * 
         * @param renderDimensions The texture dimensions used for all non resize-stage and screen-stage framebuffers.
         * @param targetDimensions The texture dimensions used by the resize-stage and screen-stage framebuffers.
         * @param viewportDimensions The sub-region of the pipeline target dimensions rendered to.
         * @param renderType The type of pipeline desired for this RenderSet.
         */
        void setRenderProperties(glm::u16vec2 renderDimensions, glm::u16vec2 targetDimensions, const SDL_Rect& viewportDimensions, RenderType renderType);

        /**
         * @brief Returns the currently active debug render texture.
         * 
         * @return std::shared_ptr<Texture> A reference to the currently active debug render texture.
         */
        std::shared_ptr<Texture> getCurrentScreenTexture ();

        /**
         * @brief Uses the resize-stage to scale the rendered image up or down.
         * 
         */
        void copyAndResize();

        /**
         * @brief Adds a named render source, presumably intended for use by a render stage in this pipeline.
         * 
         * @param name The name of the render source.
         * @param renderSource A handle to the texture used as render source.
         */
        void addOrAssignRenderSource(const std::string& name, std::shared_ptr<Texture> renderSource);

        /**
         * @brief Removes a named render source.
         * 
         * @param name The name of the render source removed.
         */
        void removeRenderSource(const std::string& name);

        /**
         * @brief Sets the texture of the skybox painted to the background of the current scene in the 3D pipeline.
         * 
         * @param skyboxTexture An appropriately formatted cubemap texture.
         */
        void setSkybox(std::shared_ptr<Texture> skyboxTexture);

        /**
         * @brief Sets the entity to be treated as this render set's active camera.
         * 
         * @param cameraEntity An entity which fulfils CameraSystem prerequisites.
         */
        void setCamera(EntityID cameraEntity);

        /**
         * @brief Sets the gamma value used for gamma correction in the tonemapping stage of the pipeline.
         * 
         * @param gamma The new gamma value.
         */
        void setGamma(float gamma);
        /**
         * @brief Gets the gamma value used in this set's tonemapping-stage.
         * 
         * @return float This set's gamma value.
         */
        float getGamma();

        /**
         * @brief Sets the exposure value responsible for determining the behaviour of tonemapping in the tonemapping-stage.
         * 
         * @param exposure The new exposure value, similar to real-world camera exposure.
         * 
         */
        void setExposure(float exposure);

        /**
         * @brief Returns the value of exposure set on this render set.
         * 
         * @return float This set's exposure.
         */
        float getExposure();

        /**
         * @brief The index of the current texture being treated as this object's texture target.
         * 
         */
        std::size_t mCurrentScreenTexture {7};

        /**
         * @brief All the debug/screen textures produced by this target.
         * 
         */
        std::vector<std::shared_ptr<Texture>> mScreenTextures {};

        /**
         * @brief The ID of the entity treated as this render set's active camera.
         * 
         */
        EntityID mActiveCamera {};

        /**
         * @brief Storage for light-related settings.
         * 
         */
        std::shared_ptr<Material> mLightMaterialHandle {nullptr};

        /**
         * @brief Handle to this set's geometry render stage.
         * 
         */
        std::shared_ptr<GeometryRenderStage> mGeometryRenderStage { nullptr };

        /**
         * @brief Handle to this set's lighting render stage.
         * 
         */
        std::shared_ptr<LightingRenderStage> mLightingRenderStage { nullptr };

        /**
         * @brief Handle to this set's blur render stage (used for bloom).
         * 
         */
        std::shared_ptr<BlurRenderStage> mBlurRenderStage { nullptr };

        /**
         * @brief Handle to this set's tonemapping and gamma render stage.
         * 
         */
        std::shared_ptr<TonemappingRenderStage> mTonemappingRenderStage { nullptr };

        /**
         * @brief Handle to this set's skybox render stage.
         * 
         */
        std::shared_ptr<SkyboxRenderStage> mSkyboxRenderStage { nullptr };

        /**
         * @brief Handle to this set's resize render stage.
         * 
         */
        std::shared_ptr<ResizeRenderStage> mResizeRenderStage { nullptr };

        /**
         * @brief Handle to this set's screen render stage.
         * 
         */
        std::shared_ptr<ScreenRenderStage> mScreenRenderStage { nullptr };

        /**
         * @brief Handle to this set's addition render stage.
         * 
         */
        std::shared_ptr<AdditionRenderStage> mAdditionRenderStage { nullptr };

        /**
         * @brief Textures designated as sources for this render set.
         * 
         */
        std::map<std::string, std::shared_ptr<Texture>> mRenderSources {};

        /**
         * @brief An ID representing the type of pipeline used in this set.
         * 
         */
        RenderType mRenderType { RenderType::BASIC_3D };

        /**
         * @brief This sets presently configured gamma correction value.
         * 
         */
        float mGamma { 2.f };

        /**
         * @brief This sets presently configured exposure value.
         * 
         */
        float mExposure { 1.f };

        /**
         * @brief A marker for when the 3D pipeline or the addition pipeline has been rerendered, and a corresponding screen or resize step is required.
         * 
         */
        bool mRerendered { true };
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief The render system for a single ECSWorld, which joins together various RenderStages into a render pipeline for objects present in that world.
     * 
     */
    class RenderSystem: public System<RenderSystem, std::tuple<>, std::tuple<CameraProperties>> {
    public:
        /**
         * @brief Constructs a new RenderSystem belonging to a single ECSWorld.
         * 
         * @param world The world this System belongs to.
         */
        explicit RenderSystem(std::weak_ptr<ECSWorld> world):
        System<RenderSystem, std::tuple<>, std::tuple<CameraProperties>>{world}
        {}

        /**
         * @brief Gets the system type string associated with the RenderSystem.
         * 
         * @return std::string The render system's system type string.
         */
        static std::string getSystemTypeName() { return "RenderSystem"; }

        /**
         * @brief A subsystem of the RenderSystem; tracks light objects in this ECSWorld scheduled for rendering at the next render step.
         * 
         */
        class LightQueue: public System<LightQueue, std::tuple<>, std::tuple<Transform, LightEmissionData>>{
        public:
            /**
             * @brief Constructs a new LightQueue System.
             * 
             * @param world 
             */
            explicit LightQueue(std::weak_ptr<ECSWorld> world):
            System<RenderSystem::LightQueue, std::tuple<>, std::tuple<Transform, LightEmissionData>>{world}
            {}

            /**
             * @brief Adds a light unit to be rendered by this render stage at the next render step.
             * 
             * @param renderStage The stage the light is being added to.
             * @param simulationProgress The progress from the end of the previous simulation step to the next simulation step, as a number between 1 and 0.
             */
            void enqueueTo(BaseRenderStage& renderStage, float simulationProgress);

            /**
             * @brief Gets the system type string associated with this system.
             * 
             * @return std::string The system type string associated with this system.
             */
            static std::string getSystemTypeName() { return "RenderSystem::LightQueue"; }
        private:
            /**
             * @brief Initializes objects related to this queue.
             * 
             */
            void onInitialize() override;

            /**
             * @brief The light volume associated with each light object, scaled up or down according to its computed radius.
             * 
             */
            std::shared_ptr<StaticMesh> mSphereMesh { nullptr };
        };

        /**
         * @brief A subsystem of the RenderSystem; tracks opaque and alpha-tested models present in this ECSWorld to be rendered at the next render step.
         * 
         */
        class OpaqueQueue: public System<OpaqueQueue, std::tuple<>, std::tuple<Transform, std::shared_ptr<StaticModel>>> {
        public:
            /**
             * @brief Constructs a new OpaqueQueue System.
             * 
             * @param world The world this queue will belong to.
             */
            explicit OpaqueQueue(std::weak_ptr<ECSWorld> world):
            System<OpaqueQueue, std::tuple<>, std::tuple<Transform, std::shared_ptr<StaticModel>>>{world}
            {}

            /**
             * @brief Adds opaque render units to a render stage to be rendered this frame.
             * 
             * @param renderStage The render stage being added to.
             * @param simulationProgress The progress towards the next simulation step after the previous one, represented as a number between 0 and 1.
             */
            void enqueueTo(BaseRenderStage& renderStage, float simulationProgress);

            /**
             * @brief Gets the system type string associated with this System.
             * 
             * @return std::string This queue's system type string.
             */
            static std::string getSystemTypeName() { return "RenderSystem::OpaqueQueue"; }
        };

        /**
         * @brief Runs through all the render stages in the render pipeline for this frame.
         * 
         * @param simulationProgress The progress towards the next simulation step since the last simulation step, as a number between 0 and 1.
         */
        void execute(float simulationProgress);

        /**
         * @brief Uploads camera matrices to the GPU per the camera's current stage.
         * 
         * @param simulationProgress The progress towards the next simulation step since the last simulation step, as a number between 0 and 1.
         */
        void updateCameraMatrices(float simulationProgress);

        /**
         * @brief Renders the currently active screen texture (RenderSet::mCurrentScreenTexture) to the (global) screen or window texture.
         * 
         */
        void renderToScreen();

        /**
         * @brief Sets (or nulls) the skybox texture currently being used as the background to this scene's geometry.
         * 
         * @param skyboxTexture A texture in a supported cubemap format, or nullptr.
         */
        void setSkybox(std::shared_ptr<Texture> skyboxTexture);

        /**
         * @brief Gets a handle to the skybox texture used in this RenderSystem's ECSWorld.
         * 
         * @return std::shared_ptr<Texture> A handle to this ECSWorld's skybox texture.
         */
        inline std::shared_ptr<Texture> getSkybox() const { return mSkyboxTexture; }

        /**
         * @brief Creates a RenderSet based on parameters provided by its caller.
         * 
         * @param renderDimensions The dimensions, in pixels, of the world rendered.
         * @param targetDimensions The dimensions, in pixels, for the target texture, which may be different from the render dimensions.
         * @param viewportDimensions The sub-region of the target texture that the render texture will be mapped to.
         * @param renderType The type of pipeline used for this render set.
         * @return RenderSetID The ID representing the RenderSet created by this method.
         */
        RenderSetID createRenderSet(
            glm::u16vec2 renderDimensions,
            glm::u16vec2 targetDimensions,
            const SDL_Rect& viewportDimensions,
            RenderSet::RenderType renderType=RenderSet::RenderType::BASIC_3D
        );

        /**
         * @brief Adds a "render source" to the currently bound RenderSet.
         * 
         * A render source can be any miscellaneous texture used as input to one of the render pipeline's render stages.
         * 
         * @param name The name of the render source (presumably indicating its purpose).
         * @param renderSource The handle to the render source.
         */
        void addOrAssignRenderSource(const std::string& name, std::shared_ptr<Texture> renderSource);

        /**
         * @brief Removes a render source from this render set.
         * 
         * @param name The name of the render source being removed.
         */
        void removeRenderSource(const std::string& name);

        /**
         * @brief Marks a particular RenderSet as active, i.e., its resources (cameras, textures) are used to render something.
         * 
         * @param renderSet The ID of the render set being made active.
         */
        void useRenderSet(RenderSetID renderSet);

        /**
         * @brief Sets the render properties of the currently active RenderSet.
         * 
         * @param renderDimensions The dimensions of the render texture.
         * @param targetDimensions The dimensions of the final texture, may be different from render dimensions.
         * @param viewportDimensions The sub-region of the target texture the render texture will be mapped to.
         * @param renderType The type of render pipeline to use in this render set.
         */
        void setRenderProperties(glm::u16vec2 renderDimensions, glm::u16vec2 targetDimensions, const SDL_Rect& viewportDimensions, RenderSet::RenderType renderType);

        /**
         * @brief Deletes, removes resources of a render set created in this ECSWorld.
         * 
         * @param renderSet The render set being deleted.
         */
        void deleteRenderSet(RenderSetID renderSet);


        /**
         * @brief (Used in debug) Sets the next texture in the currently active RenderSet's screen texture list as that set's target texture.
         * 
         */
        void renderNextTexture ();

        /**
         * @brief Sets a camera with a particular ID as the active camera for the currently active RenderSet.
         * 
         * @param cameraEntity The ID of the active camera for the currently active render set.
         */
        void setCamera(EntityID cameraEntity);

        /**
         * @brief Sets the gamma value for the active RenderSet.
         * 
         * @param gamma The new gamma value to use.
         */
        void setGamma(float gamma);

        /**
         * @brief Gets the gamma value associated with the current render set.
         * 
         * @return float This RenderSet's current gamma value.
         */
        float getGamma();

        /**
         * @brief Sets the exposure value for the currently active render set.
         * 
         * @param exposure The active RenderSet's new exposure value.
         */
        void setExposure(float exposure);

        /**
         * @brief Gets the exposure value for the currently active render set.
         * 
         * @return float The active RenderSet's current exposure value.
         */
        float getExposure();

        /**
         * @brief Gets a handle to the currently active RenderSet's active screen texture.
         * 
         * @return std::shared_ptr<Texture> A handle to this RenderSet's active screen texture.
         * 
         * @see RenderSet::mCurrentScreenTexture
         */
        std::shared_ptr<Texture> getCurrentScreenTexture ();

    private:
        /**
         * @brief Initializes this render system.
         * 
         */
        void onInitialize() override;

        /**
         * @brief Command which invokes the ResizeRenderStage of the currently active RenderSet's current screen texture if necessary.
         * 
         */
        void copyAndResize();

        /**
         * @brief Returns a handle to the background texture for the geometry in this scene.
         * 
         */
        std::shared_ptr<Texture> mSkyboxTexture { nullptr };

        /**
         * @brief A list of render sets that were created for this render system.
         * 
         */
        std::map<RenderSetID, RenderSet> mRenderSets {};

        /**
         * @brief The ID of the presently active RenderSet.
         * 
         */
        RenderSetID mActiveRenderSetID;
        /**
         * @brief IDs of render sets which existed before, which may be used again to name new RenderSets.
         * 
         */
        std::set<RenderSetID> mDeletedRenderSetIDs {};

        /**
         * @brief The highest number of RenderSets which were active at once during the running of this program.
         * 
         * The next useable ID for a render set if that number is exceeded.
         * 
         */
        RenderSetID mNextRenderSetID { 0 };

        /**
         * @brief The ID for this system's uniform buffer object storing camera related matrices.
         * 
         */
        GLuint mMatrixUniformBufferIndex { 0 };

        /**
         * @brief The ID for this system's uniform buffer binding point, where shader programs expect to find camera matrices.
         * 
         */
        GLuint mMatrixUniformBufferBinding { 0 };
    };
}

#endif

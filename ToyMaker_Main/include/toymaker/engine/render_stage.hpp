/**
 * @ingroup ToyMakerRenderSystem
 * @file render_stage.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief A file containing render stage related classes, this engine's representation of a single "step" in a graphics pipeline.
 * @version 0.3.2
 * @date 2025-09-04
 * 
 */

#ifndef FOOLSENGINE_RENDERSTAGE_H
#define FOOLSENGINE_RENDERSTAGE_H

#include <string>
#include <map>
#include <queue>

#include <SDL2/SDL.h>

#include "texture.hpp"
#include "shader_program.hpp"
#include "framebuffer.hpp"
#include "mesh.hpp"
#include "material.hpp"
#include "instance.hpp"
#include "model.hpp"
#include "light.hpp"
#include "util.hpp"

namespace ToyMaker {
    /**
     * @ingroup ToyMakerRenderSystem
     * @brief An object representing a single opaque mesh-material pair, to be rendered this frame.
     * 
     * Its sort key is computed such that render priority looks like this:
     * 
     * Mesh > Material Texture > Material Everything Else
     * 
     * @todo Maybe there's a better order for opaque queues?  Investigate.
     * 
     */
    struct OpaqueRenderUnit {

        /**
         * @brief Constructs a new Opaque Render Unit object
         * 
         * @param meshHandle Handle to the mesh associated with this unit.
         * @param materialHandle Handle to the material associated with this unit.
         * @param modelMatrix This object's model matrix.
         */
        OpaqueRenderUnit(std::shared_ptr<StaticMesh> meshHandle,std::shared_ptr<Material> materialHandle, glm::mat4 modelMatrix):
            mMeshHandle{meshHandle}, mMaterialHandle{materialHandle}, mModelMatrix{modelMatrix}
        {
            setSortKey();
        }

        /**
         * @brief Compares this unit to another based on priority.
         * 
         * @param other The unit this one is being compared to
         * @retval true This unit has a lower sort key than the other.
         * @retval false This unit does not have a lower sort key than the other.
         */
        bool operator<(const OpaqueRenderUnit& other) const {
            return mSortKey < other.mSortKey;
        }

        /**
         * @brief The computed sort key for this object.
         * 
         */
        std::uint32_t mSortKey {};

        /**
         * @brief The mesh handle for this render unit.
         * 
         */
        std::shared_ptr<StaticMesh> mMeshHandle;

        /**
         * @brief The material handle for this render unit.
         * 
         */
        std::shared_ptr<Material> mMaterialHandle;

        /**
         * @brief The model matrix to apply to this unit.
         * 
         */
        glm::mat4 mModelMatrix;

        /**
         * @brief Method responsible for actually computing this unit's sort key.
         * 
         */
        void setSortKey() {
            std::uint32_t meshHash { static_cast<uint32_t>(std::hash<StaticMesh*>{}(mMeshHandle.get())) };
            std::uint32_t materialHash { static_cast<uint32_t>(std::hash<Material*>{}(mMaterialHandle.get()))};
            mSortKey |= (meshHash << ((sizeof(uint32_t)/2)*8)) & 0xFFFF0000;
            mSortKey |= (materialHash << 0) & 0x0000FFFF;
        }
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Contains the model matrix, mesh, and light emission for a single light object being rendered this frame.
     * 
     */
    struct LightRenderUnit {
        /**
         * @brief Constructs a new Light Render Unit object
         * 
         * @param meshHandle The mesh representing this light's volume.
         * @param lightEmissionData The emissive properties of this light.
         * @param modelMatrix The matrix which correctly places the light in the scene.
         */
        LightRenderUnit(std::shared_ptr<StaticMesh> meshHandle, const LightEmissionData& lightEmissionData, const glm::mat4& modelMatrix):
        mMeshHandle{ meshHandle },
        mModelMatrix{ modelMatrix },
        mLightAttributes { lightEmissionData }
        { setSortKey(); }

        /**
         * @brief Compares one light unit with another based on its sort key.
         * 
         * @param other The light unit this one is being compared to.
         * @retval true This unit has a lower sort key than the other;
         * @retval false This unit does not have a lower sort key than the other;
         */
        bool operator<(const LightRenderUnit& other) const {
            return mSortKey < other.mSortKey;
        }

        /**
         * @brief The sort key for this light unit, based on its mesh.
         * 
         */
        std::uint32_t mSortKey {};

        /**
         * @brief The mesh representing the lighting volume for this light.
         * 
         */
        std::shared_ptr<StaticMesh> mMeshHandle;

        /**
         * @brief The matrix which places this renderable light into the scene.
         * 
         */
        glm::mat4 mModelMatrix;

        /**
         * @brief The emissive properties for this light.
         * 
         */
        LightEmissionData mLightAttributes;

        /**
         * @brief Sets the sort key for this object based on its mesh.
         * 
         */
        void setSortKey() {
            std::uint32_t meshHash { static_cast<uint32_t>(std::hash<StaticMesh*>{}(mMeshHandle.get())) };
            mSortKey = meshHash;
        }
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Represents a single render stage or a render step that applies to the default (window) framebuffer.
     * 
     * Also contains handles to the various resources required by it, the first of which is the OpenGL shader program it uses.
     * 
     */
    class BaseRenderStage {
    public:
        /**
         * @brief Constructs a new BaseRenderStage object.
         * 
         * @param shaderFilepath The path to the JSON header for the shader program it uses.
         */
        BaseRenderStage(const std::string& shaderFilepath);

        BaseRenderStage(const BaseRenderStage& other) = delete;
        BaseRenderStage(BaseRenderStage&& other) = delete;
        BaseRenderStage& operator=(const BaseRenderStage& other) = delete;
        BaseRenderStage& operator=(BaseRenderStage&& other) = delete;

        /**
         * @brief Destroys the Base Render Stage object
         * 
         */
        virtual ~BaseRenderStage();

        /**
         * @brief Sets up the program per its configuration.
         * 
         * A part of this process includes creating and storing needed resources, and registering material properties used by this render stage.
         * 
         * @param targetDimensions The dimensions of the texture to which this stage will render.
         * 
         * @todo Much of this is finicky and poorly defined.  Stricter requirements needed.
         * 
         */
        virtual void setup(const glm::u16vec2& targetDimensions) = 0;

        /**
         * @brief Validates this stage by checking for availability of required resources, connections with adjacent render stages.
         * 
         * @todo Much of this is finicky and poorly defined.  Stricter requirements needed.
         * 
         */
        virtual void validate() = 0;

        /**
         * @brief Executes this render stage, presumably after preceding render stages have been executed.
         * 
         * Mostly anything can happen here, where this stage's behaviour is assumed to be defined in the material properties of the things it renders.
         * 
         */
        virtual void execute() = 0;

        /**
         * @brief Attaches a named texture to this rendering stage.
         * 
         * The purpose of this attachment is defined by the pipeline it's being used in.  It could be the texture holding the output for a previous stage, or could be where the results of this stage should be written.
         * 
         * @param name The name of the texture being attached, from this stage's point-of-view.
         * @param textureHandle The actual handle to the attached texture.
         */
        void attachTexture(const std::string& name, std::shared_ptr<Texture> textureHandle);

        /**
         * @brief Attaches a named texture to this rendering stage.
         * 
         * The meaning of this attachment is defined according to this stage's implementation.
         * 
         * @param name The name of the mesh being attached, from this stage's point-of-view.
         * @param meshHandle The actual handle to the attached mesh.
         */
        void attachMesh(const std::string& name, std::shared_ptr<StaticMesh> meshHandle);

        /**
         * @brief Attaches a named material to this rendering stage.
         * 
         * The usage of this attachment depends on the implementation of the stage, but it can be used to specify, for example, render-stage-level configurations.
         * 
         * @param name The name of this material attachment.
         * @param materialHandle The actual handle to the attached material.
         */
        void attachMaterial(const std::string& name, std::shared_ptr<Material> materialHandle);

        /**
         * @brief Gets a reference to an attached texture by its name.
         * 
         * @param name The name of an attached texture.
         * @return std::shared_ptr<Texture> A handle to the attached texture.
         */
        std::shared_ptr<Texture> getTexture(const std::string& name);

        /**
         * @brief Gets a reference to an attached mesh by its name.
         * 
         * @param name The name of an attached mesh.
         * @return std::shared_ptr<StaticMesh> A handle to the attached mesh.
         */
        std::shared_ptr<StaticMesh> getMesh(const std::string& name);

        /**
         * @brief Gets a reference to an attached material by its name.
         * 
         * @param name The name of an attached material.
         * @return std::shared_ptr<Material> A handle to the attached material.
         */
        std::shared_ptr<Material> getMaterial(const std::string& name);

        /**
         * @brief Should be called once this stage's shader is made active; applies the viewport config associated with this render stage.
         * 
         * @see setTargetViewport()
         */
        void useViewport();

        /**
         * @brief Sets the rendering area for this stage, a rectangular sub-region of the target texture.
         * 
         * @param targetViewport The definition of the sub-region.
         */
        void setTargetViewport(const SDL_Rect& targetViewport);

        /**
         * @brief Adds an opaque render unit expected by this stage to its associated render queue.
         * 
         * @param renderUnit The opaque render unit being added.
         */
        void submitToRenderQueue(OpaqueRenderUnit renderUnit);

        /**
         * @brief Adds a light render unit expected by this stage to its associated render queue.
         * 
         * @param lightRenderUnit 
         */
        void submitToRenderQueue(LightRenderUnit lightRenderUnit);

    protected:
        
        /**
         * @brief The OpenGL vertex array object associated with this object.
         * 
         * In theory it saves this stage's shader from having to respecify where its associated buffers are on the GPU.
         * 
         * @todo In practice, we end up doing that anyway because I'm an idiot.
         * 
         */
        GLuint mVertexArrayObject {};

        /**
         * @brief A handle to the compiled and uploaded shader program associated with this render stage.
         * 
         */
        std::shared_ptr<ShaderProgram> mShaderHandle;

        /**
         * @brief This stage's named texture attachments.
         * 
         */
        std::map<std::string, std::shared_ptr<Texture>> mTextureAttachments {};

        /**
         * @brief This stage's named mesh attachments.
         * 
         */
        std::map<std::string, std::shared_ptr<StaticMesh>> mMeshAttachments {};

        /**
         * @brief This stage's named material attachments.
         * 
         */
        std::map<std::string, std::shared_ptr<Material>> mMaterialAttachments {};

        /**
         * @brief A queue containing all the opaque meshes to be rendered this frame by this stage.
         * 
         */
        std::priority_queue<OpaqueRenderUnit> mOpaqueMeshQueue {};

        /**
         * @brief A queue containing all the light units to be rendered this frame by this stage.
         * 
         */
        std::priority_queue<LightRenderUnit> mLightQueue {};

        /**
         * @brief The rectangle defining the sub-region of its target texture to which this render stage renders.
         * 
         */
        SDL_Rect mTargetViewport {0, 0, 800, 600};
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Base class for render stages which render to an offscreen buffer.
     * 
     */
    class BaseOffscreenRenderStage: public BaseRenderStage {
    public:
        /**
         * @brief Constructs a new Base Offscreen Render Stage object
         * 
         * @param shaderFilepath The path to the JSON shader program header used by this stage.
         * @param templateFramebufferDescription A JSON description of this stage's Framebuffer's initial state.
         */
        BaseOffscreenRenderStage(const std::string& shaderFilepath, const nlohmann::json& templateFramebufferDescription);

        /**
         * @brief I don't really know what my intention for this was.
         * 
         * @param texture The texture being made a target, presumably.
         * @param targetID The color attachment ID the texture should be bound to in this stage's framebuffer, presumably.
         * 
         * @todo Figure out whatever it was I might have wanted this for.
         */
        void setTargetTexture(std::shared_ptr<Texture> texture, std::size_t targetID);

        /**
         * @brief Adds a texture to this stage's list of target textures.
         * 
         * @param textureHandle The texture being added to this stage's target texture list.
         * @return std::size_t The ID of the newly added target texture.
         */
        std::size_t attachTextureAsTarget(std::shared_ptr<Texture> textureHandle);
        /**
         * @brief Adds a texture to this stage's list of named target textures.
         * 
         * @param targetName The name of the texture as target, once attached.
         * @param textureHandle The actual texture being attached as a target.
         * @return std::size_t The index associated with the newly added target texture.
         * 
         * @see declareRenderTarget()
         */
        std::size_t attachTextureAsTarget(const std::string& targetName, std::shared_ptr<Texture> textureHandle);

        /**
         * @brief Assigns a name to an attached target texture, intended to be used by the system connecting render stages together by input and output.
         * 
         * @param name The name of the render target.
         * @param index The target texture attachment ID.
         */
        void declareRenderTarget(const std::string& name, unsigned int index);

        /**
         * @brief Gets a named render target texture from this stage.
         * 
         * @param name The name of the render target.
         * @return std::shared_ptr<Texture> The handle to the target.
         * 
         * @see declareRenderTarget()
         * @see attachTextureAsTarget()
         */
        std::shared_ptr<Texture> getRenderTarget(const std::string& name);

        /**
         * @brief (in theory) Removes a target texture from this stage's list of target textures.
         * 
         * @param targetTextureID The index of the target texture to remove.
         * 
         * @todo Figure out what my intentions were re: this method.
         */
        void detachTargetTexture(std::size_t targetTextureID);


        /**
         * @brief (in theory) Removes a target texture that was given a name from a list of named target textures.
         * 
         * @param name The name of the target texture to be made nameless.
         * 
         * @todo Figure out what my intentions were re: this method.
         */
        void removeRenderTarget(const std::string& name);

        /**
         * @brief Tests whether an RBO was attached to this render stage.
         * 
         * @retval true This stage has an RBO attached.
         * @retval false This stage has no RBO attached.
         */
        bool hasAttachedRBO() const;

        /**
         * @brief Tests whether this stage has created and owns an RBO.
         * 
         * @retval true This stage has an RBO of its own.
         * @retval false This stage has no RBO of its own.
         */
        bool hasOwnRBO() const;

        /**
         * @brief Returns the RBO owned by this stage's Framebuffer, if it has one.
         * 
         * @return RBO& A reference to the RBO owned by this stage.
         */
        RBO& getOwnRBO();

        /**
         * @brief Attaches an RBO to this stage's Framebuffer which may or may not be owned by it.
         * 
         * @param rbo A reference to the RBO being attached to this stage.
         */
        void attachRBO(RBO& rbo);

        /**
         * @brief Detaches the RBO currently attached to this stage's Framebuffer.
         * 
         */
        void detachRBO();

    protected:
        /**
         * @brief The framebuffer owned by this stage, to which this stage renders its results.
         * 
         */
        std::shared_ptr<Framebuffer> mFramebufferHandle;

        /**
         * @brief A description of this stage's framebuffer, used as a template to create a framebuffer matching requested render dimensions.
         * 
         */
        nlohmann::json mTemplateFramebufferDescription;

        /**
         * @brief A list of named render target textures along with their indices in this stage's framebuffer's target color buffer list.
         * 
         */
        std::map<std::string, unsigned int> mRenderTargets {};
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Render step responsible for translating mesh-material data into geometry buffers.
     * 
     * This render stage produces a position buffer, normal buffer, an albedo-specular buffer, and a depth buffer.
     * 
     * The first three are available as this stage's "geometryPosition", "geometryNormal", and "geometryAlbedoSpecular" render targets, while the third is available on the RBO owned by this stage.
     * 
     * To queue an opaque or alpha-tested object to this buffer, call submitToRenderQueue() with an OpaqueRenderUnit object.
     * 
     */
    class GeometryRenderStage : public BaseOffscreenRenderStage {
    public:
        /**
         * @brief Constructs a new geometry render stage object with templates for its position, normal, and albedo-spec color buffers.
         * 
         * @param shaderFilepath The path to the header JSON of this object's geometry shader object.
         */
        GeometryRenderStage(const std::string& shaderFilepath) 
        : BaseOffscreenRenderStage{shaderFilepath, nlohmann::json::object({
            {"type", Framebuffer::getResourceTypeName()},
            {"method", FramebufferFromDescription::getResourceConstructorName()},
            {"parameters", {
                {"nColorAttachments", 3},
                {"dimensions", nlohmann::json::array({
                    800, 600
                })},
                {"ownsRBO", true },
                {"colorBufferDefinitions",{
                    ColorBufferDefinition{
                        .mDataType=GL_FLOAT,
                        .mComponentCount=4
                    },
                    ColorBufferDefinition{
                        .mDataType=GL_FLOAT,
                        .mComponentCount=4
                    },
                    ColorBufferDefinition{
                        .mDataType=GL_UNSIGNED_BYTE,
                        .mComponentCount=4
                    }
                }},
            }}
        })}
        {}

        virtual void setup(const glm::u16vec2& textureDimensions) override;
        virtual void validate() override;
        virtual void execute() override;
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Render stage which takes geometry buffers and scene lights as inputs, and produces a lit scene as output.
     * 
     * Requires "positionMap", "normalMap", and "albedoSpecularMap" texture attachments.
     * 
     * Produces "litScene" and "brightCutoff" as its render targets.  "brightCutoff"  stores color values from "litScene" that exceed a particular intensity threshold, per the following formula:
     * 
     *   `intensity = dot(outColor.xyz, vec3(.2f, .7f, .1f))`
     * 
     * The cutoff is not adjustable presently.
     * 
     */
    class LightingRenderStage : public BaseOffscreenRenderStage {
    public:
        LightingRenderStage(const std::string& shaderFilepath)
        : BaseOffscreenRenderStage{shaderFilepath, nlohmann::json::object({
            {"type", Framebuffer::getResourceTypeName()},
            {"method", FramebufferFromDescription::getResourceConstructorName()},
            {"parameters", {
                {"nColorAttachments", 2},
                {"dimensions", nlohmann::json::array({
                    800, 600
                })},
                {"ownsRBO", true },
                {"colorBufferDefinitions",{
                    ColorBufferDefinition{
                        .mDataType=GL_FLOAT,
                        .mComponentCount=4
                    },
                    ColorBufferDefinition{
                        .mDataType=GL_FLOAT,
                        .mComponentCount=4
                    },
                }},
            }}
        })}
        {}
        virtual void setup(const glm::u16vec2& textureDimensions) override;
        virtual void validate() override;
        virtual void execute() override;
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Multipurpose render stage which at present is used to compute a simple bloom effect from a scene's "brightCutoff" texture.
     * 
     * As input expects an "unblurredImage" texture attachment.  Produces "pingBuffer" and "pongBuffer" and render targets.
     * 
     * The number of blur passes performed, and hence the intensity of the blur, may be adjusted by changing the "nBlurPasses" float property of this stage's "screenMaterial" Material attachment.
     * 
     */
    class BlurRenderStage : public BaseOffscreenRenderStage {
    public:
        BlurRenderStage(const std::string& shaderFilepath)
        : BaseOffscreenRenderStage{shaderFilepath, nlohmann::json::object({
            {"type", Framebuffer::getResourceTypeName()},
            {"method", FramebufferFromDescription::getResourceConstructorName()},
            {"parameters", {
                {"nColorAttachments", 2},
                {"dimensions", nlohmann::json::array({
                    800, 600 
                })},
                {"ownsRBO", false},
                {"colorBufferDefinitions",{
                    ColorBufferDefinition{
                        .mDataType=GL_FLOAT,
                        .mComponentCount=4
                    },
                    ColorBufferDefinition{
                        .mDataType=GL_FLOAT,
                        .mComponentCount=4
                    },
                }},
            }}
        })}
        {}
        virtual void setup(const glm::u16vec2& textureDimensions) override;
        virtual void validate() override;
        virtual void execute() override;
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Uses a skybox texture (i.e a texture with a supported cubemap format) to render a skybox behind geometry in the scene.
     * 
     * As input expects "skybox" texture attachment,  and a "unitCube" mesh attachment, and also should have the geometry buffer's RBO attached (for its depth buffer).
     * 
     * As output, is expected to modify the "litScene" target produced by the LightingRenderStage, available as its own "litSceneWithSkybox" target.
     * 
     */
    class SkyboxRenderStage : public BaseOffscreenRenderStage {
    public:
        SkyboxRenderStage(const std::string& filepath)
        : BaseOffscreenRenderStage(filepath, nlohmann::json::object({
            {"type", Framebuffer::getResourceTypeName()},
            {"method", FramebufferFromDescription::getResourceConstructorName()},
            {"parameters", {
                {"nColorAttachments", 1},
                {"dimensions", nlohmann::json::array({
                    800, 600
                })},
                {"ownsRBO", false},
                {"colorBufferDefinitions", nlohmann::json::array()}
            }}
        }))
        {}

        virtual void setup(const glm::u16vec2& textureDimensions) override;
        virtual void validate() override;
        virtual void execute() override;
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Takes lit scene (with bloom if it's used), and produces a tonemapped and gamma corrected version of the scene.
     * 
     * As input expects a "screenMesh" mesh attachment, and "litScene" and "bloomEffect" texture attachments.
     * 
     * As output, produces a "tonemappedScene" render target.
     * 
     * The gamma correction value, and the exposure value used for tonemapping, are available as this stage's "screenMaterial" Material attachment's "gamma" and "exposure" float properties respectively.
     * 
     */
    class TonemappingRenderStage : public BaseOffscreenRenderStage {
    public:
        TonemappingRenderStage(const std::string& shaderFilepath)
        : BaseOffscreenRenderStage{shaderFilepath, nlohmann::json::object({
            {"type", Framebuffer::getResourceTypeName()},
            {"method", FramebufferFromDescription::getResourceConstructorName()},
            {"parameters", {
                {"nColorAttachments", 1},
                {"dimensions", nlohmann::json::array({
                    800, 600
                })},
                {"ownsRBO", false},
                {"colorBufferDefinitions",{
                    ColorBufferDefinition{
                        .mDataType=GL_UNSIGNED_BYTE,
                        .mComponentCount=4
                    },
                }},
            }}
        })}
        {}

        virtual void setup(const glm::u16vec2& textureDimensions) override;
        virtual void validate() override;
        virtual void execute() override;
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Stage responsible for combining multiple textures together.
     * 
     * Expects as input "textureAddend_0", "textureAddend_1", ..., "textureAddend_n" texture attachments.
     * 
     * As output, produces a composite texture available on its "textureSum" render target.
     * 
     */
    class AdditionRenderStage: public BaseOffscreenRenderStage {
    public:
        AdditionRenderStage(const std::string& shaderFilepath):
        BaseOffscreenRenderStage(shaderFilepath, nlohmann::json::object({
            {"type", Framebuffer::getResourceTypeName()},
            {"method", FramebufferFromDescription::getResourceConstructorName()},
            {"parameters", {
                {"nColorAttachments", 1},
                {"dimensions", nlohmann::json::array({
                    800, 600
                })},
                {"ownsRBO", false},
                {"colorBufferDefinitions", {
                    ColorBufferDefinition {
                        .mDataType=GL_UNSIGNED_BYTE,
                        .mComponentCount=4,
                    },
                }},
            }},
        }))
        {}

        virtual void setup(const glm::u16vec2& textureDimensions) override;
        virtual void validate() override;
        virtual void execute() override;
    private:
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Render stage responsible for rendering any texture attached as source to the screen.
     * 
     * Expects as input a "renderSource" texture attachment.
     * 
     * Produces no output, but has the side effect of rendering to the window/screen per its configuration.
     * 
     */
    class ScreenRenderStage: public BaseRenderStage {
    public:
        ScreenRenderStage(const std::string& shaderFilepath):
        BaseRenderStage(shaderFilepath)
        {}

        virtual void setup(const glm::u16vec2& targetDimensions) override;
        virtual void validate() override;
        virtual void execute() override;
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Render stage responsible for resizing a texture to its final intended resolution.
     * 
     * Useful when the rendering resolution for a pipeline is different from the resolution of its target.
     * 
     * Expects as input a "renderSource" texture attachment.
     * 
     * As output, produces a "resizedTexture" render target.
     * 
     */
    class ResizeRenderStage : public BaseOffscreenRenderStage {
    public:
        ResizeRenderStage(const std::string& shaderFilepath)
        : BaseOffscreenRenderStage{shaderFilepath, nlohmann::json::object({
            {"type", Framebuffer::getResourceTypeName()},
            {"method", FramebufferFromDescription::getResourceConstructorName()},
            {"parameters", {
                {"nColorAttachments", 1},
                {"dimensions", nlohmann::json::array({
                    800, 600
                })},
                {"ownsRBO", false},
                {"colorBufferDefinitions",{
                    ColorBufferDefinition{
                        .mDataType=GL_UNSIGNED_BYTE,
                        .mComponentCount=4,
                    },
                }},
            }}
        })}
        {}

        virtual void setup(const glm::u16vec2& textureDimensions) override;
        virtual void validate() override;
        virtual void execute() override;

    private:
    };

}

#endif

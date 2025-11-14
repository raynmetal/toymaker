/**
 * @ingroup ToyMakerRenderSystem
 * @file framebuffer.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief File containing wrapper over OpenGL Framebuffers and related objects.
 * @version 0.3.2
 * @date 2025-09-02
 * 
 */

#ifndef FOOLSENGINE_FRAMEBUFFER_H
#define FOOLSENGINE_FRAMEBUFFER_H

#include <vector>
#include <memory>

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "texture.hpp"
#include "core/resource_database.hpp"

namespace ToyMaker {

    class Framebuffer;

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief Wrapper class over OpenGL RBOs.
     * 
     * An RBO, or a render buffer object, is a texture representing the depth map or stencil buffer for the framebuffer it's attached to.
     * 
     */
    class RBO {
    public:
        /**
         * @brief Creates a new RBO.
         * 
         * @param dimensions The dimensions specified for the RBO.
         * @return std::unique_ptr<RBO> Pointer to the created RBO.
         */
        inline static std::unique_ptr<RBO> create(const glm::vec2& dimensions) {
            return std::unique_ptr<RBO>{ new RBO{dimensions} };
        }
        /**
         * @brief Destroys the RBO object
         * 
         */
        ~RBO();

        /**
         * @brief Gets the ID associated with this RBO.
         * 
         * @return GLuint The RBO's ID.
         */
        inline GLuint getID() const { return mID; }

        /**
         * @brief Deletes previously allocated RBO buffer and allocates a new one.
         * 
         * @param dimensions The new dimensions for the RBO.
         */
        void resize(const glm::vec2& dimensions);
    private:
        /**
         * @brief Constructs a new RBO object.
         * 
         * @param dimensions The dimensions the RBO should have.
         */
        RBO(const glm::vec2& dimensions);

        /**
         * @brief The ID of the RBO.
         * 
         */
        GLuint mID;
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief A wrapper class over OpenGL framebuffers.
     * 
     * Framebuffers are, in essence, a collection of buffers that relate to each other in some way.  Each framebuffer must have either an RBO or color buffer or both, and may have multiple color buffers.  Their actual usage depends on the requirements of the program or module using them.
     * 
     * The window to which an application renders is also a texture belonging to a special framebuffer, managed by default by OpenGL itself, with an ID of 0.
     * 
     */
    class Framebuffer : public Resource<Framebuffer> {
    public:
        /**
         * @brief Assuming an allocated OpenGL framebuffer already exists, constructs a Framebuffer object and hands over resources passed as arguments.
         * 
         * @param framebuffer The ID of the framebuffer being given to this object.
         * @param dimensions The dimensions of the framebuffer, in pixels.
         * @param nColorAttachments The number of color attachments in use by this framebuffer.
         * @param colorBuffers Handles to the actual color buffers in use by this framebuffer.
         * @param rbo A reference to the RBO being used by this framebuffer.
         */
        Framebuffer(
            GLuint framebuffer,
            glm::vec2 dimensions,
            GLuint nColorAttachments,
            const std::vector<std::shared_ptr<Texture>>& colorBuffers,
            std::unique_ptr<RBO> rbo
        );

        /**
         * @brief Destroys the framebuffer object.
         * 
         */
        ~Framebuffer() override;

        /* copy constructor */
        Framebuffer(const Framebuffer& other);
        /* move constructor */
        Framebuffer(Framebuffer&& other);
        /* copy assignment operator */
        Framebuffer& operator=(const Framebuffer& other);
        /* move assignment operator */
        Framebuffer& operator=(Framebuffer&& other);

        /**
         * @brief Attaches a new color buffer to this framebuffer.
         * 
         * @param colorBufferHandle A handle to the color buffer being attached.
         * @return std::size_t The index corresponding to the attached color buffer.
         */
        std::size_t addTargetColorBufferHandle(std::shared_ptr<Texture> colorBufferHandle);

        /**
         * @brief Returns a vector of handles to this framebuffer's color buffers.
         * 
         * @return std::vector<std::shared_ptr<const Texture>> Vector of color buffer handles.
         * 
         */
        std::vector<std::shared_ptr<const Texture>> getTargetColorBufferHandles() const;

        /**
         * @brief Returns a constant vector of handles to this framebuffer's color buffers.
         * 
         * @return const std::vector<std::shared_ptr<Texture>>& Const reference to the vector of color buffer handles.
         */
        const std::vector<std::shared_ptr<Texture>>& getTargetColorBufferHandles();

        /**
         * @brief Answers whether this framebuffer has an RBO attached.
         * 
         * @retval true An RBO corresponding to this framebuffer exists.
         * @retval false An RBO corresponding to this framebuffer does not exist.
         */
        bool hasAttachedRBO() const;

        /**
         * @brief Answers whether an RBO description was specified when creating this framebuffer.
         * 
         * This would mean that the RBO with this framebuffer is owned **by** this framebuffer.
         * 
         * @retval true 
         * @retval false 
         */
        bool hasOwnRBO() const;

        /**
         * @brief Gets the RBO owned by this framebuffer.
         * 
         * @return RBO& Reference to the RBO owned by this framebuffer.
         */
        RBO& getOwnRBO();

        /**
         * @brief Attaches the RBO (of possibly another framebuffer) to this framebuffer object.
         * 
         * @param rbo Reference to the RBO being attached to this framebuffer.
         */
        void attachRBO(RBO& rbo);

        /**
         * @brief Detaches any RBOs currently attached to this framebuffer.
         * 
         */
        void detachRBO();

        /**
         * @brief Makes this framebuffer the currently active framebuffer in this OpenGL context.
         * 
         */
        void bind();

        /**
         * @brief Gets the dimensions specified for this framebuffer.
         * 
         * @return glm::u16vec2 The dimensions, in pixels, for this framebuffer object.
         */
        glm::u16vec2 getDimensions() const { return mDimensions; } 

        /**
         * @brief Unbind this framebuffer (or in other words, bind the default framebuffer)
         * 
         */
        void unbind();

        /**
         * @brief Gets the resource type string associated with the Framebuffer resource.
         * 
         * @return std::string The resource type string for Framebuffers.
         */
        inline static std::string getResourceTypeName() { return "Framebuffer"; }

    private:
        /**
         * @brief The ID corresponding to this framebuffer.
         * 
         */
        GLuint mID {};

        /**
         * @brief The RBO owned by this framebuffer, if such a one exists.
         * 
         */
        std::unique_ptr<RBO> mOwnRBO {};

        /**
         * @brief The number of color attachments active on this framebuffer when render is called.
         * 
         */
        GLuint mNColorAttachments {};

        /**
         * @brief The dimensions, in pixels, for textures attached to this framebuffer.
         * 
         */
        glm::vec2 mDimensions {};

        /**
         * @brief All color buffers associated with this framebuffer, owned by this framebuffer.
         * 
         */
        std::vector<std::shared_ptr<Texture>> mTextureHandles {};

        /**
         * @brief Tracks whether an RBO was attached to this framebuffer (including ones that aren't owned by it).
         * 
         */
        bool mHasAttachedRBO { false };

        /**
         * @brief Attaches an RBO associated with another Framebuffer to this object.
         * 
         * @param rbo The RBO being attached.
         */
        void attachRBO_(RBO& rbo);

        /**
         * @brief Unbinds any RBO currently attached to this framebuffer.
         * 
         */
        void detachRBO_();

        /**
         * @brief Destroys resources associated with this framebuffer.
         * 
         */
        void destroyResource();

        /**
         * @brief Releases resources associated with this framebuffer, allowing other object(s) to manage them instead.
         * 
         */
        void releaseResource();

        /**
         * @brief Copies resources associated with another framebuffer.
         * 
         * @param other The framebuffer being copied from.
         */
        void copyResource(const Framebuffer& other);

        /**
         * @brief Steals resources associated with another framebuffer.
         * 
         * @param other The framebuffer being stolen from.
         */
        void stealResource(Framebuffer& other);
    };

    /**
     * @ingroup ToyMakerRenderSystem ToyMakerResourceDB
     * @brief Constructs a Framebuffer from its description in JSON.
     * 
     */
    class FramebufferFromDescription: public ResourceConstructor<Framebuffer, FramebufferFromDescription> {
    public:
        /**
         * @brief Constructs a new FramebufferFromDescription object
         * 
         */
        FramebufferFromDescription(): 
            ResourceConstructor<Framebuffer,FramebufferFromDescription>{0} 
        {}

        /**
         * @brief Get the resource constructor type string associated with this constructor.
         * 
         * @return std::string The resource constructor type string associated with this constructor.
         * 
         * @see ResourceConstructor<TResource, TConstructor>
         * 
         */
        static std::string getResourceConstructorName() { return "fromDescription"; }

    private:
        /**
         * @brief The method actually responsible for the creation of a framebuffer with this constructor.
         * 
         * @param methodParams The framebuffer parameters, in JSON.
         * @return std::shared_ptr<IResource> A reference to the framebuffer resource.
         */
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParams) override;
    };
}

#endif

#include <memory>
#include <vector>


#include <GL/glew.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "toymaker/engine/core/resource_database.hpp"
#include "toymaker/engine/texture.hpp"

#include "toymaker/engine/framebuffer.hpp"

using namespace ToyMaker;

RBO::RBO(const glm::vec2& dimensions) {
    glGenRenderbuffers(1, &mID);
    resize(dimensions);
}

void RBO::resize(const glm::vec2& dimensions) {
    glBindRenderbuffer(GL_RENDERBUFFER, mID);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, dimensions.x, dimensions.y);
    glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

RBO::~RBO() {
    glDeleteRenderbuffers(1, &mID);
}

Framebuffer::Framebuffer(
    GLuint framebuffer,
    glm::vec2 dimensions,
    GLuint nColorAttachments,
    const std::vector<std::shared_ptr<Texture>>& colorBuffers,
    std::unique_ptr<RBO> rbo
) :
Resource<Framebuffer>{0},
mID { framebuffer },
mOwnRBO { std::move(rbo) },
mNColorAttachments { nColorAttachments },
mDimensions { dimensions },
mTextureHandles { colorBuffers }
{
    if(mOwnRBO) {
        attachRBO(*mOwnRBO);
    }
}

Framebuffer::Framebuffer(const Framebuffer& other): Resource<Framebuffer>{0}{
    copyResource(other);
}

Framebuffer::Framebuffer(Framebuffer&& other):Resource<Framebuffer>{0} {
    stealResource(other);
}

Framebuffer& Framebuffer::operator=(const Framebuffer& other) {
    if(&other == this) return *this;
    destroyResource();
    copyResource(other);
    return *this;
}
Framebuffer& Framebuffer::operator=(Framebuffer&& other) {
    if(&other == this) return *this;
    destroyResource();
    stealResource(other);
    return *this;
}

Framebuffer::~Framebuffer() {
    destroyResource();
}

std::size_t Framebuffer::addTargetColorBufferHandle(std::shared_ptr<Texture> targetColorBuffer) {
    const std::size_t newIndex { mTextureHandles.size() };
    assert(glm::u16vec2(targetColorBuffer->getWidth(), targetColorBuffer->getHeight()) == glm::u16vec2(mDimensions)
        && "Dimensions of a target texture must be the same as those of the color buffer");
    mTextureHandles.push_back(targetColorBuffer);
    return newIndex;
}

bool Framebuffer::hasAttachedRBO() const { 
    return mHasAttachedRBO;
}

bool Framebuffer::hasOwnRBO() const {
    return mOwnRBO? true: false;
}

RBO& Framebuffer::getOwnRBO() {
    return *mOwnRBO;
}

void Framebuffer::attachRBO(RBO& rbo) {
    bind();
        attachRBO_(rbo);
    unbind();
}
void Framebuffer::attachRBO_(RBO& rbo) {
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo.getID());
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        throw std::runtime_error("Could not complete creation of framebuffer with ID " + std::to_string(mID));
    }
    mHasAttachedRBO = true;
}

void Framebuffer::detachRBO() {
    bind();
        detachRBO_();
    unbind();
}
void Framebuffer::detachRBO_() {
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
    mHasAttachedRBO = false;
}

std::vector<std::shared_ptr<const Texture>>  Framebuffer::getTargetColorBufferHandles() const {
    return { mTextureHandles.cbegin(), mTextureHandles.cend() };
}

const std::vector<std::shared_ptr<Texture>>& Framebuffer::getTargetColorBufferHandles() {
    return mTextureHandles;
}

void Framebuffer::bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, mID);
}
void Framebuffer::unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Framebuffer::destroyResource() {
    glDeleteFramebuffers(1, &mID);
    mOwnRBO.reset();
    mTextureHandles.clear();
    mDimensions = {0.f, 0.f};
    mNColorAttachments = 0;
    mID = 0;
}

void Framebuffer::releaseResource() {
    mTextureHandles.clear();
    mDimensions = {0.f, 0.f};
    mNColorAttachments = 0;
    mID = 0;
}

void Framebuffer::copyResource(const Framebuffer& other) {
    if(!mID)
        glGenFramebuffers(1, &mID);

    mNColorAttachments = other.mNColorAttachments;
    mDimensions = other.mDimensions;
    mTextureHandles.clear();

    bind();
        GLuint count {0};
        for(const std::shared_ptr<Texture>& textureHandle: other.mTextureHandles) {
            mTextureHandles.push_back(
                ResourceDatabase::ConstructAnonymousResource<Texture>(nlohmann::json{
                    {"type", Texture::getResourceTypeName()},
                    {"method", TextureFromColorBufferDefinition::getResourceConstructorName()},
                    {"parameters", textureHandle->getColorBufferDefinition()}
                })
            );
            if(count < mNColorAttachments){
                mTextureHandles.back()->attachToFramebuffer(count);
            }
            ++count;
        }

        if(mNColorAttachments) {
            std::vector<GLenum> colorAttachments(mNColorAttachments);
            for(GLuint i{0}; i < mNColorAttachments; ++i) {
                colorAttachments[i] = GL_COLOR_ATTACHMENT0 + i;
            }
            glDrawBuffers(mNColorAttachments, colorAttachments.data());
        }

        //generate a render buffer object, if necessary
        if(other.mOwnRBO){
            if(!mOwnRBO) {
                mOwnRBO = RBO::create(mDimensions);
            } else {
                mOwnRBO->resize(mDimensions);
            }
            attachRBO_(*mOwnRBO);
        }

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            throw std::runtime_error("Could not complete creation of framebuffer with ID " + std::to_string(mID));
        }
    unbind();
}

void Framebuffer::stealResource(Framebuffer& other) {
    mID = other.mID;
    mOwnRBO = std::move(other.mOwnRBO);
    mNColorAttachments = other.mNColorAttachments;
    mDimensions = other.mDimensions;
    mTextureHandles = other.mTextureHandles;

    other.releaseResource();
}

std::shared_ptr<IResource> FramebufferFromDescription::createResource(const nlohmann::json& methodParams) {
    uint32_t nColorAttachments { methodParams.at("nColorAttachments").get<uint32_t>() };
    bool ownsRBO { methodParams.at("ownsRBO").get<bool>() };
    glm::vec2 dimensions { 
        methodParams.at("dimensions").at(0).get<float>(),
        methodParams.at("dimensions").at(1).get<float>()
    };
    std::vector<nlohmann::json> colorBufferParams { 
        methodParams.at("colorBufferDefinitions").begin(), methodParams.at("colorBufferDefinitions").end()
    };

    GLuint framebuffer;
    std::unique_ptr<RBO> rbo {};
    glGenFramebuffers(1, &framebuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        std::vector<std::shared_ptr<Texture>> textureHandles {};

        // If there are color buffers defined, we'll create and store them
        for(uint16_t i {0}; i < colorBufferParams.size(); ++i){
            nlohmann::json colorBufferDescription {
                {"type", Texture::getResourceTypeName()},
                {"method", TextureFromColorBufferDefinition::getResourceConstructorName()},
                {"parameters", colorBufferParams[i]},
            };
            textureHandles.push_back(
                ResourceDatabase::ConstructAnonymousResource<Texture>(colorBufferDescription)
            );
            //Assume the first n color buffers become the first n
            //framebuffer attachments
            if(i < nColorAttachments) {
                textureHandles.back()->attachToFramebuffer(i);
            }
        }

        // Declare which color attachments this framebuffer will use when rendering
        if(nColorAttachments){
            std::vector<GLenum> colorAttachments(nColorAttachments);
            for(GLuint i{0}; i < nColorAttachments; ++i) {
                colorAttachments[i] = GL_COLOR_ATTACHMENT0+i;
            }
            glDrawBuffers(nColorAttachments, colorAttachments.data());
        }

        if(ownsRBO) {
            rbo = RBO::create(dimensions);
        }

        // assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE && "Could not complete creation of framebuffer");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    return std::make_shared<Framebuffer>(framebuffer, dimensions, nColorAttachments, textureHandles, std::move(rbo));
}

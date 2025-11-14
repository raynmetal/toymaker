#include <string>
#include <vector>
#include <iostream>
#include <map>

#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <SDL2/SDL_image.h>

#include "toymaker/engine/texture.hpp"

using namespace ToyMaker;

/** @todo Rewrite these through the JSON_SERIALIZE_ENUM macro provided by nlohmann json */
const std::map<std::string, GLenum> kStringToFilter {
    {"linear", GL_LINEAR},
    {"nearest", GL_NEAREST},
};
/** @todo Rewrite these through the JSON_SERIALIZE_ENUM macro provided by nlohmann json */
const std::map<GLenum, std::string> kFilterToString {
    {GL_LINEAR, "linear"},
    {GL_NEAREST, "nearest"},
};
/** @todo Rewrite these through the JSON_SERIALIZE_ENUM macro provided by nlohmann json */
const std::map<std::string, GLenum> kStringToWrap {
    { "clamp-border", GL_CLAMP_TO_BORDER },
    { "clamp-edge", GL_CLAMP_TO_EDGE },
    { "repeat", GL_REPEAT },
    { "repeat-mirrored", GL_MIRRORED_REPEAT },
};
/** @todo Rewrite these through the JSON_SERIALIZE_ENUM macro provided by nlohmann json */
const std::map<GLenum, std::string> kWrapToString {
    {GL_CLAMP_TO_BORDER, "clamp-border"},
    {GL_CLAMP_TO_EDGE, "clamp-edge"},
    {GL_REPEAT, "repeat"},
    {GL_MIRRORED_REPEAT, "repeat-mirrored"},
};

Texture::Texture(
    GLuint textureID,
    const ColorBufferDefinition& colorBufferDefinition,
    const std::string& filepath
) : 
Resource<Texture>{0},
mID { textureID }, 
mFilepath { filepath },
mColorBufferDefinition{colorBufferDefinition}
{}

Texture::Texture(Texture&& other) noexcept: Resource<Texture>(0), mID{other.mID}, mFilepath{other.mFilepath}, mColorBufferDefinition{other.mColorBufferDefinition} {
    //Prevent other from destroying this texture when its
    //deconstructor is called
    other.releaseResource();
}

Texture::Texture(const Texture& other)
: Resource<Texture>{0}, mFilepath {other.mFilepath}, mColorBufferDefinition {other.mColorBufferDefinition}
{
    copyImage(other);
}

Texture& Texture::operator=(Texture&& other) noexcept {
    if(&other == this) return *this;

    free();

    mID = other.mID;
    mFilepath = other.mFilepath;
    mColorBufferDefinition = other.mColorBufferDefinition;

    // Prevent the destruction of the texture we now own
    // when other's deconstructor is called
    other.releaseResource();

    return *this;
}

Texture& Texture::operator=(const Texture& other) {
    if(&other == this) return *this;

    free();

    mFilepath = other.mFilepath;
    mColorBufferDefinition = other.mColorBufferDefinition;
    copyImage(other);

    return *this;
}

Texture::~Texture() {
    free();
}

void Texture::free() {
    if(!mID) return;
    glDeleteTextures(1, &mID);
    releaseResource();
}

GLuint Texture::getTextureID() const { return mID; }

GLint Texture::getWidth() const{
    if(!mID) return 0;

    return mColorBufferDefinition.mDimensions.x;
}
GLint Texture::getHeight() const {
    if(!mID) return 0;
    return mColorBufferDefinition.mDimensions.y;
}

void Texture::copyImage(const Texture& other)  {
    // Allocate memory to our texture
    glActiveTexture(GL_TEXTURE0);
    generateTexture();

    // Create 2 temporary framebuffers which we'll use to copy other's texture data
    GLuint tempReadFBO;
    GLuint tempWriteFBO;
    glGenFramebuffers(1, &tempReadFBO);
    glGenFramebuffers(1, &tempWriteFBO);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, tempReadFBO);
        glFramebufferTexture2D(
            GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, other.mID, 0
        );
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        assert(glCheckFramebufferStatus(GL_READ_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE 
            && "Something went wrong while creating read FBO for texture copy! "
        );
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempWriteFBO);
        glFramebufferTexture2D(
            GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mID, 0
        );
        glDrawBuffer(GL_COLOR_ATTACHMENT0);
        assert(glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE 
            && "Something went wrong while creating draw FBO for texture copy! "
        );
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

    // Blit other's data into our colour buffer
    glBindFramebuffer(GL_READ_FRAMEBUFFER, tempReadFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, tempWriteFBO);
        glBlitFramebuffer(0, 0, 
            mColorBufferDefinition.mDimensions.x, mColorBufferDefinition.mDimensions.y, 
            0, 0,
            mColorBufferDefinition.mDimensions.x, mColorBufferDefinition.mDimensions.y, 
            GL_COLOR_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

    // Delete temporary buffers created for this operation
    glDeleteFramebuffers(1, &tempReadFBO);
    glDeleteFramebuffers(1, &tempWriteFBO);

    GLuint error {glGetError()};
    assert(error == GL_FALSE && "Error while copying texture!");
}

void Texture::destroyResource() {
    free();
}

void Texture::releaseResource() {
    mID = 0;
    mFilepath = "";
}

void Texture::bind(GLuint textureUnit) const {
    glActiveTexture(GL_TEXTURE0 + textureUnit);
        glBindTexture(GL_TEXTURE_2D, mID);
    glActiveTexture(GL_TEXTURE0);
}

void Texture::attachToFramebuffer(GLuint attachmentUnit) const {
    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0 + attachmentUnit,
        GL_TEXTURE_2D,
        mID,
        0
    );
}

void Texture::generateTexture() {
    assert(mColorBufferDefinition.mDataType == GL_FLOAT || mColorBufferDefinition.mDataType == GL_UNSIGNED_BYTE);
    assert(mColorBufferDefinition.mComponentCount == 1 || mColorBufferDefinition.mComponentCount == 4);

    if(!mID) glGenTextures(1, &mID);

    glBindTexture(GL_TEXTURE_2D, mID);
        glTexImage2D(
            GL_TEXTURE_2D, 0, internalFormat(), 
            mColorBufferDefinition.mDimensions.x, mColorBufferDefinition.mDimensions.y, 0, externalFormat(), mColorBufferDefinition.mDataType, NULL
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mColorBufferDefinition.mMagFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mColorBufferDefinition.mMinFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, mColorBufferDefinition.mWrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, mColorBufferDefinition.mWrapT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

GLenum Texture::internalFormat() {
    return deduceInternalFormat(mColorBufferDefinition);
}

GLenum Texture::externalFormat() {
    return deduceExternalFormat(mColorBufferDefinition);
}

std::shared_ptr<IResource> TextureFromFile::createResource(const nlohmann::json& methodParameters) {
    std::string filepath { methodParameters["path"].get<std::string>() };
    ColorBufferDefinition colorBufferDefinition {
        .mDataType { GL_UNSIGNED_BYTE },
        .mUsesWebColors { true }
    };

    if(methodParameters.find("cubemap_layout") != methodParameters.end()) {
        methodParameters.get_to(colorBufferDefinition.mCubemapLayout);

    } else {
        colorBufferDefinition.mCubemapLayout = ColorBufferDefinition::CubemapLayout::NA;

    }

    // Load image from file into a convenient SDL surface, per the image
    SDL_Surface* textureImage { IMG_Load(filepath.c_str()) };
    assert(textureImage && "SDL image loading failed");

    // Convert the image from its present format to RGBA
    SDL_Surface* pretexture { SDL_ConvertSurfaceFormat(textureImage, SDL_PIXELFORMAT_RGBA32, 0) };
    SDL_FreeSurface(textureImage);
    textureImage = nullptr;
    assert(pretexture && "Could not convert SDL image to known image format");

    // Move surface pixels to the graphics card
    GLuint texture;
    
    colorBufferDefinition.mDimensions = {pretexture->w, pretexture->h};
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0,
        //assume linear space if not an albedo texture
        deduceInternalFormat(colorBufferDefinition),
        colorBufferDefinition.mDimensions.x, colorBufferDefinition.mDimensions.y,
        0, GL_RGBA, GL_UNSIGNED_BYTE,
        reinterpret_cast<void*>(pretexture->pixels)
    );
    SDL_FreeSurface(pretexture);
    pretexture=nullptr;

    // Check for OpenGL errors
    GLuint error { glGetError() };
    assert(error == GL_NO_ERROR && "An error occurred during allocation of openGL texture");

    // Configure a few texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, colorBufferDefinition.mWrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, colorBufferDefinition.mWrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, colorBufferDefinition.mMinFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, colorBufferDefinition.mMagFilter);

    // Construct and return a shared pointer to this texture to the caller
    return std::make_shared<Texture>(texture, colorBufferDefinition, filepath);
}

std::shared_ptr<IResource> TextureFromColorBufferDefinition::createResource(const nlohmann::json& methodParameters) {
    ColorBufferDefinition colorBufferDefinition = methodParameters;
    assert(colorBufferDefinition.mComponentCount == 1 || colorBufferDefinition.mComponentCount == 4);
    GLuint texture;
    glGenTextures(1, &texture);

    glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            deduceInternalFormat(colorBufferDefinition),
            colorBufferDefinition.mDimensions.x,
            colorBufferDefinition.mDimensions.y,
            0, 
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            NULL
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, colorBufferDefinition.mMagFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, colorBufferDefinition.mMinFilter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, colorBufferDefinition.mWrapS);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, colorBufferDefinition.mWrapT);
    glBindTexture(GL_TEXTURE_2D, 0);

    return std::make_shared<Texture>(texture, colorBufferDefinition);
};

void ToyMaker::from_json(const nlohmann::json& json, ColorBufferDefinition& colorBufferDefinition) {
    json.at("dimensions").at(0).get_to(colorBufferDefinition.mDimensions.x);
    json.at("dimensions").at(1).get_to(colorBufferDefinition.mDimensions.y);
    if(json.find("cubemap_layout") != json.end()) {
        json.get_to(colorBufferDefinition.mCubemapLayout);
    } else {
        colorBufferDefinition.mCubemapLayout = ColorBufferDefinition::CubemapLayout::NA;
    }
    colorBufferDefinition.mMagFilter = kStringToFilter.at(json.at("mag_filter").get<std::string>());
    colorBufferDefinition.mMinFilter = kStringToFilter.at(json.at("min_filter").get<std::string>());
    colorBufferDefinition.mWrapS = kStringToWrap.at(json.at("wrap_s").get<std::string>());
    colorBufferDefinition.mWrapT = kStringToWrap.at(json.at("wrap_t").get<std::string>());
    colorBufferDefinition.mDataType = json.at("data_type") == "float"? GL_FLOAT: GL_UNSIGNED_BYTE;
    json.at("component_count").get_to(colorBufferDefinition.mComponentCount);
    json.at("uses_web_colors").get_to(colorBufferDefinition.mUsesWebColors);
    assert(colorBufferDefinition.mComponentCount == 1 || colorBufferDefinition.mComponentCount == 4);
}

void ToyMaker::to_json(nlohmann::json& json, const ColorBufferDefinition& colorBufferDefinition) {
    json = {
        {"dimensions", { colorBufferDefinition.mDimensions.x, colorBufferDefinition.mDimensions.y }},
        {"mag_filter", kFilterToString.at(colorBufferDefinition.mMagFilter)},
        {"min_filter", kFilterToString.at(colorBufferDefinition.mMinFilter)},
        {"wrap_s", kWrapToString.at(colorBufferDefinition.mWrapS)},
        {"wrap_t", kWrapToString.at(colorBufferDefinition.mWrapT)},
        {"data_type", colorBufferDefinition.mDataType == GL_FLOAT? "float": "unsigned-byte"},
        {"cubemap_layout", colorBufferDefinition.mCubemapLayout},
        {"component_count", colorBufferDefinition.mComponentCount},
        {"uses_web_colors", colorBufferDefinition.mUsesWebColors},
    };
}

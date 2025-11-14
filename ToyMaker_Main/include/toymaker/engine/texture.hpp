/**
 * @ingroup ToyMakerRenderSystem
 * @file texture.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Header containing definitions of classes and functions related to loading and using Texture resources.
 * @version 0.3.2
 * @date 2025-09-10
 * 
 * 
 */

#ifndef FOOLSENGINE_TEXTURE_H
#define FOOLSENGINE_TEXTURE_H

#include <string>

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "core/resource_database.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToymakerRenderSystem ToyMakerECSComponent
     * @brief A struct containing the definition of a color buffer, using which similar color buffers can be created.
     * 
     */
    struct ColorBufferDefinition {
        /**
         * @brief The type of texture defined.
         * 
         */
        enum class Type {
            TEXTURE_2D, //< A simple 2D texture.
            CUBEMAP, //< The texture of a cubemap (unused I think?)
        };

        /**
         * @brief For a 2D texture - determines the manner in which the Texture should be sampled in order for it to be used as a cubemap.
         * 
         */
        enum class CubemapLayout: uint8_t {
            NA, //< This color buffer does not represent a cubemap texture.
            ROW, //< Subregions of the texture corresponding to each face of the cubemap are laid out in a single row.
            // COLUMN,
            // ROW_CROSS,
            // COLUMN_CROSS,
        };

        /**
         * @brief The dimensions of the 2D texture.
         * 
         */
        glm::vec2 mDimensions {800, 600};

        /**
         * @brief The type of cubemap layout the texture conforms to, if it is a cubemap.
         * 
         */
        CubemapLayout mCubemapLayout { CubemapLayout::NA };

        /**
         * @brief The type of sampling used with the texture when it is zoomed in to.
         * 
         */
        GLenum mMagFilter { GL_LINEAR };

        /**
         * @brief The type of sampling used with the texture when it is zoomed out from.
         * 
         */
        GLenum mMinFilter { GL_LINEAR };

        /**
         * @brief Horizontally: for UV coordinates beyond the 0.0-1.0 range, which part of the texture is to be sampled from.
         * 
         */
        GLenum mWrapS { GL_CLAMP_TO_EDGE };

        /**
         * @brief Vertically: for UV coordinates beyond the 0.0-1.0 range, which part of the texture is to be sampled from.
         * 
         */
        GLenum mWrapT { GL_CLAMP_TO_EDGE };

        /**
         * @brief The underlying data type representing each channel (also component) of the texture.
         * 
         */
        GLenum mDataType { GL_UNSIGNED_BYTE };

        /**
         * @brief The number of components (or channels) each pixel of the texture contains.
         * 
         */
        GLbyte mComponentCount { 4 };

        /**
         * @brief Whether the intensity of the color of a component maps linearly or exponentially with the value of that component on a pixel.
         * 
         * Web colors are mapped exponentially, whereas lighting calculations are performed in linear space.
         * 
         * Related: SRGB and SRGB_ALPHA in OpenGL.
         * 
         */
        bool mUsesWebColors { false };
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief The class representation of textures on this engine, which are a type of Resource used both within and outside of the engine.
     * 
     */
    class Texture : public Resource<Texture> {
    public:
        /**
         * @brief Constructs a new texture object which takes ownership of an OpenGL texture handle and engine colorbuffer definition created outside of it.
         * 
         * @param textureID The ID of the texture now to be owned by this object.
         * @param colorBufferDefinition The color buffer definition for this object.
         * @param filepath The path to the file the texture was loaded from, if any.
         */
        Texture(
            GLuint textureID,
            const ColorBufferDefinition& colorBufferDefinition,
            const std::string& filepath=""
        );

        /*Copy construction*/
        Texture(const Texture& other);
        /*Copy assignment*/
        Texture& operator=(const Texture& other);

        /*Move construction*/
        Texture(Texture&& other) noexcept;
        /*Move assignment*/
        Texture& operator=(Texture&& other) noexcept;

        /*Destructor belonging to latest subclass*/
        virtual ~Texture();

        /* basic deallocate function */
        virtual void free();

        /**
         * @brief Binds this texture to a texture unit, making it available for use by a shader.
         * 
         * @param textureUnit The unit this texture is to be bound to.
         */
        void bind(GLuint textureUnit) const;

        /* attaches this texture to a (presumably existing and bound) framebuffer*/

        /**
         * @brief Attaches this texture to a (presumably existing and bound) framebuffer, allowing the user of the framebuffer to read from and render to it.
         * 
         * @param attachmentUnit The attachment unit to which this texture will be bound.
         */
        void attachToFramebuffer(GLuint attachmentUnit) const;

        /**
         * @brief Gets the OpenGL texture ID for this texture.
         * 
         * @return GLuint This texture's OpenGL texture id.
         */
        GLuint getTextureID() const;

        /**
         * @brief Gets the width of this texture (per its color buffer definition).
         * 
         * @return GLint This texture's width.
         */
        GLint getWidth() const;

        /**
         * @brief Gets the height of this texture (per its color buffer definition).
         * 
         * @return GLint This texture's height.
         */
        GLint getHeight() const;

        /**
         * @brief Gets the resource type string associated with this resource.
         * 
         * @return std::string The resource type string of this object.
         */
        inline static std::string getResourceTypeName() { return "Texture"; }

        /**
         * @brief Gets the description of this texture.
         * 
         * @return ColorBufferDefinition This texture's color buffer definition.
         */
        ColorBufferDefinition getColorBufferDefinition() { return mColorBufferDefinition; }

    protected:
        void copyImage(const Texture& other);


        /**
         * @brief The OpenGL ID of this texture.
         * 
         */
        GLuint mID {0};

        /**
         * @brief The file this texture was loaded from, if any.
         * 
         */
        std::string mFilepath {""};

        /**
         * @brief The color buffer definition of this texture.
         * 
         */
        ColorBufferDefinition mColorBufferDefinition;

        /**
         * @brief Generates a new texture based on the stored color buffer definition.
         * 
         */
        void generateTexture();

        /**
         * @brief The enum value passed as the "internalFormat" argument of glTexImage2D.
         * 
         * @return GLenum The enum value corresponding to glTexImage2D's "internalFormat" argument.
         */
        GLenum internalFormat();

        /**
         * @brief The enum value passed as the "format" argument of glTexImage2D.
         * 
         * @return GLenum The enum value corresponding to glTexImage2D's "format" argument.
         */
        GLenum externalFormat();

        /**
         * @brief Destroys (OpenGL managed) texture tied to this object.
         * 
         */
        void destroyResource();

        /** 
         * @brief Removes references to (OpenGL managed) texture tied to this object, so that another object or part of the program can take ownership of it.
         */
        void releaseResource();
    };

    /**
     * @ingroup ToyMakerResourceDB ToyMakerRenderSystem
     * @brief A resource constructor which loads a texture from a supported image file located via its file path.
     * 
     * ## Usage:
     * 
     * An example JSON texture resource description.
     * 
     * ```jsonc
     * 
     * {
     *     "name": "Skybox_Texture",
     *     "type": "Texture",
     *     "method": "fromFile",
     *     "parameters": {
     *         "path": "data/textures/skybox.png",
     *         "cubemap_layout": "row"
     *     }
     * }
     * 
     * ```
     * 
     */
    class TextureFromFile: public ResourceConstructor<Texture, TextureFromFile> {
    public:
        TextureFromFile(): ResourceConstructor<Texture, TextureFromFile> {0} {}
        inline static std::string getResourceConstructorName() { return "fromFile"; }
    private:
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };

    /**
     * @ingroup ToyMakerRenderSystem ToyMakerResourceDB
     * @brief Generates a texture based on its color buffer definition.
     * 
     * ## Usage:
     * 
     * A JSON example of a resource loaded from its color buffer definition:
     * 
     * ```jsonc
     * 
     * {
     *     "name": "Plain_White_Texture",
     *     "type": "Texture",
     *     "method": "fromDescription",
     *     "parameters": {
     *         "dimensions": [128, 128],
     *         "cubemap_layout": "na",
     *         "mag_filter": "linear",
     *         "min_filter": "linear",
     *         "wrap_s": "clamp-edge",
     *         "wrap_t": "clamp-edge",
     *         "data_type": "float",
     *         "component_count": 4,
     *         "uses_web_colors": false
     *     }
     * }
     * 
     * ```
     * 
     */
    class TextureFromColorBufferDefinition: public ResourceConstructor<Texture, TextureFromColorBufferDefinition> {
    public:
        TextureFromColorBufferDefinition(): ResourceConstructor<Texture, TextureFromColorBufferDefinition> {0} {}
        inline static std::string getResourceConstructorName() { return "fromDescription"; }
    private:
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief A (quite possibly unnecessary) function to fetch the enum corresponding to the "internalFormat" argument of glTexImage2D based on some color buffer definition.
     * 
     * 
     * @param colorBufferDefinition The color buffer definition of the texture we are presumably trying to allocate on the GPU.
     * 
     * @return GLenum The enum corresponding to glTexImage2D's "internalFormat" argument for this color buffer.
     * 
     */
    inline GLenum deduceInternalFormat(const ColorBufferDefinition& colorBufferDefinition) {
        GLenum internalFormat;

        if(colorBufferDefinition.mDataType == GL_FLOAT && colorBufferDefinition.mComponentCount == 1){ 
            internalFormat = GL_R16F;
        } else if (colorBufferDefinition.mDataType == GL_FLOAT && colorBufferDefinition.mComponentCount == 4) {
            internalFormat = GL_RGBA16F;
        } else if (colorBufferDefinition.mDataType == GL_UNSIGNED_BYTE && colorBufferDefinition.mComponentCount == 1) {
            internalFormat = GL_RED;
        } else if (colorBufferDefinition.mDataType == GL_UNSIGNED_BYTE && colorBufferDefinition.mComponentCount == 4) {

            if(colorBufferDefinition.mUsesWebColors) {
                internalFormat = GL_SRGB_ALPHA;
            } else {
                internalFormat = GL_RGBA;
            }

        } else {
            throw std::invalid_argument("Invalid data type and component count combination provided in texture constructor");
        }

        return internalFormat;
    }

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief A (quite possibly unnecessary) function to fetch the enum corresponding to the "format" argument of glTexImage2D based on some color buffer definition.
     * 
     * 
     * @param colorBufferDefinition The color buffer definition of the texture we are presumably trying to allocate on the GPU.
     * 
     * @return GLenum The enum corresponding to glTexImage2D's "format" argument for this color buffer.
     * 
     */
    inline GLenum deduceExternalFormat(const ColorBufferDefinition& colorBufferDefinition) {
        GLenum externalFormat;

        if(colorBufferDefinition.mDataType == GL_FLOAT && colorBufferDefinition.mComponentCount == 1){ 
            externalFormat = GL_RED;
        } else if (colorBufferDefinition.mDataType == GL_FLOAT && colorBufferDefinition.mComponentCount == 4) {
            externalFormat = GL_RGBA;
        } else if (colorBufferDefinition.mDataType == GL_UNSIGNED_BYTE && colorBufferDefinition.mComponentCount == 1) {
            externalFormat = GL_RED;
        } else if (colorBufferDefinition.mDataType == GL_UNSIGNED_BYTE && colorBufferDefinition.mComponentCount == 4) {
            externalFormat = GL_RGBA;
        } else {
            throw std::invalid_argument("Invalid data type and component count combination provided in texture constructor");
        }

        return externalFormat;
    }


    /** @ingroup ToyMakerSerialization ToyMakerRenderSystem */
    void to_json(nlohmann::json& json, const ColorBufferDefinition& colorBufferDefinition);

    /** @ingroup ToyMakerSerialization ToyMakerRenderSystem */
    void from_json(const nlohmann::json& json, ColorBufferDefinition& colorBufferDefinition);
    NLOHMANN_JSON_SERIALIZE_ENUM(ColorBufferDefinition::CubemapLayout, {
        {ColorBufferDefinition::CubemapLayout::NA, "na"}, 
        {ColorBufferDefinition::CubemapLayout::ROW, "row"},
    });
}

#endif

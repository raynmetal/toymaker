/**
 * @ingroup UrGameUIComponent
 * @file nine_slice_panel.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains class defining this project's implementation of nine-slice (or nine-region) resizable panels.
 * @version 0.3.2
 * @date 2025-09-13
 * 
 * 
 */

#ifndef ZOAPPNINESLICE_H
#define ZOAPPNINESLICE_H

#include <SDL2/SDL.h>
#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

#include "toymaker/engine/shader_program.hpp"
#include "toymaker/engine/core/resource_database.hpp"
#include "toymaker/engine/texture.hpp"

/**
 * @ingroup UrGameUIComponent
 * @brief Resource responsible for resizing a texture using the 9-slice technique, for use in UI elements.
 * 
 * The base texture provided to this panel is divided into 9 regions
 * 
 * - The 4 regions at the corners of the texture are locked, and are not resized along with the panel.
 * 
 * - The regions of the top and bottom edges can be stretched or tiled horizontally.  Similarly, the left and right edge regions can be extended vertically.
 * 
 * - The region in the center can be resized arbitrarily along both axes along with the panel.
 * 
 * To facilitate this, this aspect computes a new albedo texture and static mesh for the StaticModel component of its SimObject whenever it receives a resize request.
 * 
 */
class NineSlicePanel: public ToyMaker::Resource<NineSlicePanel> {
public:
    /**
     * @brief The type of texture sampling to be used on the resizable regions of the base texture.
     * 
     */
    enum ScaleMode: uint8_t {
        STRETCH, //< UV coordinates remain within 0 and 1, even as the panel expands or contracts.
        TILE, //< UV coordinates for the resized panel are computed as multiples of the original pixel size of the panel's base texture.
    };

    /**
     * @brief Constructs a new NineSlicePanel aspect.
     * 
     * @param baseTexture The texture to use as the basis for the panel produced by this aspect.
     * @param contentRegionUV The rectangle, in UV coordinates, of the central region of the rectangle, determining the way the base texture is sliced.
     */
    NineSlicePanel(
        std::shared_ptr<ToyMaker::Texture> baseTexture,
        SDL_FRect contentRegionUV
    );

    /**
     * @brief Destroys the NineSlicePanel object.
     * 
     */
    ~NineSlicePanel();

    /**
     * @brief Gets the resource type string associated with this class.
     * 
     * @return std::string This class' resource type string.
     */
    inline static std::string getResourceTypeName() { return "NineSlicePanel"; }

    /**
     * @brief Generates a new texture based on this panel resource, where the dimensions of the central region of the resource is specified by the caller.
     * 
     * @param contentDimensions The dimensions of the central region of the resource, where content should be placed.
     * @return std::shared_ptr<ToyMaker::Texture> The texture produced.
     */
    std::shared_ptr<ToyMaker::Texture> generateTexture(glm::uvec2 contentDimensions) const;

    /**
     * @brief Gets the width, in pixels, of the panel slices on the left hand side of this resource's base texture.
     * 
     * @return uint32_t The width, in pixels, of left hand side panel texture slices.
     */
    uint32_t getOffsetPixelLeft() const;

    /**
     * @brief Gets the width, in pixels, of the panel slices on the right hand side of this resource's base texture.
     * 
     * @return uint32_t The width, in pixels, of the right hand side panel texture slices.
     */
    uint32_t getOffsetPixelRight() const;

    /**
     * @brief Gets the height, in pixels, of the panel slices on the bottom side of this resource's base texture.
     * 
     * @return uint32_t The height, in pixels, of the bottom side panel texture slices.
     */
    uint32_t getOffsetPixelBottom() const;

    /**
     * @brief Gets the height, in pixels, of the panel slices on the top side of this resource's base texture.
     * 
     * @return uint32_t The height, in pixels, of the top side panel texture slices.
     */
    uint32_t getOffsetPixelTop() const;

private:
    /**
     * @brief The base texture used to generate new textures.
     * 
     */
    std::shared_ptr<ToyMaker::Texture> mTexture {};

    /**
     * @brief The central region of the texture, resized to fit any content placed within a panel generated from this resource.
     * 
     */
    SDL_FRect mContentRegion { .x{0.f}, .y{0.f}, .w{1.f}, .h{1.f} };

    /**
     * @brief The shader responsible for producing panell textures from a base texture.
     * 
     */
    std::shared_ptr<ToyMaker::ShaderProgram> mShaderHandle { nullptr };

    /**
     * @brief The vertex array object used by this resource's panel shader.
     * 
     */
    GLuint mVertexArrayObject {};
};

/**
 * @ingroup UrGameUIComponent
 * @brief Resource constructor for creating new NineSlicePanel resources from their descriptions in JSON.
 * 
 * Its appearance in JSON is as follows:
 * 
 * ```jsonc
 * 
 * {
 *     "name": "Bad_Panel",
 *     "type": "NineSlicePanel",
 *     "method": "fromDescription",
 *     "parameters": {
 *         "base_texture": "Bad_Panel_Texture",
 *         "content_region": [ 0.0235, 0.0235, 0.953, 0.953]
 *     }
 * }
 *
 * ```
 *
 */
class NineSlicePanelFromDescription: public ToyMaker::ResourceConstructor<NineSlicePanel, NineSlicePanelFromDescription> {
public:
    /**
     * @brief creates an instance of this constructor.
     * 
     */
    NineSlicePanelFromDescription(): ToyMaker::ResourceConstructor<NineSlicePanel, NineSlicePanelFromDescription> {0} {}

    /**
     * @brief Gets the resource constructor type string associated with this class.
     * 
     * @return std::string The resource constructor type string associated with this class.
     */
    inline static std::string getResourceConstructorName() { return "fromDescription"; }
private:
    /**
     * @brief Creates a NineSlicePanel resource from its JSON description.
     * 
     * @param methodParameters The description of the NineSlicePanel, in JSON.
     * @return std::shared_ptr<ToyMaker::IResource> A base pointer to the NineSlicePanel object's base resource class instance.
     */
    std::shared_ptr<ToyMaker::IResource> createResource(const nlohmann::json& methodParameters) override;
};

/** @ingroup UrGameUIComponent */
NLOHMANN_JSON_SERIALIZE_ENUM(NineSlicePanel::ScaleMode, {
    {NineSlicePanel::ScaleMode::STRETCH, "stretch"},
    {NineSlicePanel::ScaleMode::TILE, "tile"},
});

#endif

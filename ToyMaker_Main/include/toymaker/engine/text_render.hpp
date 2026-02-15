/**
 * @ingroup ToyMakerText
 * @file text_render.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Classes relating to the creation and use of fonts to render text.
 * @version 0.3.2
 * @date 2025-09-10
 * 
 * 
 */

/**
 * @defgroup ToyMakerText Text
 * 
 * @ingroup ToyMakerEngine ToyMakerResourceDB
 * 
 */

#ifndef FOOLSENGINE_TEXTRENDER_H
#define FOOLSENGINE_TEXTRENDER_H

#include <SDL3_ttf/SDL_ttf.h>

#include "core/resource_database.hpp"
#include "texture.hpp"

namespace ToyMaker{
    class TextFont;

    /**
     * @ingroup ToyMakerText
     * @brief A wrapper class over SDL_ttf, providing methods to generate text textures from text using a font as a resource.
     * 
     */
    class TextFont: public Resource<TextFont> {
    public:
        virtual ~TextFont();

        /**
         * @brief Constructs a new Text Font object
         * 
         * @param fontPath The path to the TTF file containing the desired font.
         * @param pointSize The point size to load the font with, whose purpose I don't quite understand yet.
         */
        explicit TextFont(const std::string& fontPath, uint16_t pointSize);

        /**
         * @brief Gets the resource type string for this class.
         * 
         * @return std::string The resource type string for this class.
         */
        inline static std::string getResourceTypeName() { return "TextFont"; }

        /**
         * @brief Gets the path of the TTF font file from which this resource was loaded.
         * 
         * @return std::string The path to this resource's font.
         */
        inline std::string getFontPath() const { return mFontPath; }

        /**
         * @brief Gets the point size with which the font was loaded.
         * 
         * @return uint16_t The point size this font was loaded with.
         */
        inline uint16_t getPointSize() const { return mPointSize; }

        /**
         * @brief Uses this font to render some text against a solid background.
         * 
         * @param text The text to be rendered.
         * @param textColor The color of the text.
         * @param backgroundColor The color of the text's background.
         * @return std::shared_ptr<Texture> The generated text texture.
         */
        std::shared_ptr<Texture> renderText(
            const std::string& text,
            glm::u8vec3 textColor,
            glm::u8vec3 backgroundColor
        ) const;

        /**
         * @brief Renders a texture for some text with a transparent background, which may run multiple lines, using this font.
         * 
         * @param text The text being rendered.
         * @param textColor The color of the text once rendered.
         * @param wrapLength The number of pixels beyond which text should be wrapped to the next line.
         * @return std::shared_ptr<Texture> The text texture generated.
         */
        std::shared_ptr<Texture> renderTextArea(
            const std::string& text,
            glm::u8vec4 textColor,
            uint32_t wrapLength=0
        ) const;

        /**
         * @brief Renders a texture for some text with a transparent background on a single line using this font.
         * 
         * @param text The text  to be rendered.
         * @param textColor The color of the text once rendered.
         * @return std::shared_ptr<Texture> The rendered text.
         * 
         * @todo Unnecessary; remove.
         */
        std::shared_ptr<Texture> renderText(
            const std::string& text,
            glm::u8vec4 textColor
        ) const;


    private:
        /**
         * @brief The function responsible for actually loading the font from the path specified.
         * 
         * @param fontPath The path of the font being loaded.
         * @param pointSize The point size with which to load the font.
         * @return TTF_Font* A pointer to the underlying SDL_ttf font data.
         */
        static TTF_Font* LoadFont(const std::string& fontPath, uint16_t pointSize);

        /**
         * @brief Utility function for converting an SDL surface into a Texture resource.
         * 
         * @param surface The input surface.
         * @return std::shared_ptr<Texture> The Texture Resource.
         */
        static std::shared_ptr<Texture> MakeTexture(SDL_Surface* surface);

        /**
         * @brief A pointer to the underlying SDL_ttf font resource.
         * 
         */
        TTF_Font* mFont;

        /**
         * @brief The path from which this font was loaded.
         * 
         */
        std::string mFontPath;

        /**
         * @brief The point size with which the font was loaded.
         * 
         */
        uint16_t mPointSize;
    };

    /**
     * @ingroup ToyMakerResourceDB ToyMakerText
     * @brief The ResourceConstructor responsible for loading a TextFont from its file path.
     * 
     * ## Usage:
     * 
     * An example JSON representation of a TextFont resource loaded from a file:
     * 
     * ```jsonc
     * 
     * {
     *     "name": "Roboto_Mono_Regular_24",
     *     "type": "TextFont",
     *     "method": "fromFile",
     *     "parameters": {
     *         "path": "data/fonts/Roboto_Mono/static/RobotoMono-Regular.ttf",
     *         "point_size": 24
     *     }
     * }
     * 
     * ```
     * 
     */
    class TextFontFromFile: public ResourceConstructor<TextFont, TextFontFromFile> {
    public:
        explicit TextFontFromFile(): ResourceConstructor<TextFont, TextFontFromFile>{0} {}
        inline static std::string getResourceConstructorName() { return "fromFile"; }
    private:
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };
}

#endif

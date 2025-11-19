/**
 * @ingroup UrGameUIComponent
 * @file ui_text.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains the definition for UIText, responsible for rendering text textures.
 * @version 0.3.2
 * @date 2025-09-13
 * 
 * 
 */

#ifndef ZOAPPUITEXT_H
#define ZOAPPUITEXT_H

#include "toymaker/engine/sim_system.hpp"
#include "toymaker/engine/text_render.hpp"

namespace ToyMaker {

    /**
     * @ingroup UrGameUIComponent
     * @brief An aspect responsible for rendering text textures and displaying them on a surface in the scene.
     * 
     */
    class UIText: public ToyMaker::SimObjectAspect<UIText> {
    public:
        /**
         * @brief Constructs a new UIText aspect.
         * 
         */
        UIText(): SimObjectAspect<UIText>{0} {}

        /**
         * @brief Gets the aspect type string for this class.
         * 
         * @return std::string This class' aspect type string.
         */
        inline static std::string getSimObjectAspectTypeName() { return "UIText"; }

        /**
         * @brief Creates a new UIText aspect based on its JSON description.
         * 
         * @param jsonAspectProperties The json description of this UIText instance.
         * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed UIText instance.
         */
        static std::shared_ptr<BaseSimObjectAspect> create(const nlohmann::json& jsonAspectProperties);

        /**
         * @brief Constructs a new UIText instance using this one as its blueprint.
         * 
         * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed UIText instance.
         */
        std::shared_ptr<BaseSimObjectAspect> clone() const override;

        /**
         * @brief Renders the initial text associated with this object once it becomes a part of the active scene.
         * 
         */
        void onActivated() override;

        /**
         * @brief Gets the actual text associated with this UIText object.
         * 
         * @return std::string This object's display text.
         */
        inline std::string getText() const { return mText; }

        /**
         * @brief Updates the text rendered by this object.
         * 
         * @param newText The new text used by this object.
         */
        void updateText(const std::string& newText);

        /**
         * @brief Updates the color of the text rendered by this object.
         * 
         * @param newColour This object's display text's new colour.
         */
        void updateColor(glm::u8vec4 newColour);

        /**
         * @brief Updates the scale of this object.
         * 
         * @param scale The new scale for this text object.
         */
        void updateScale(float scale);

        /**
         * @brief Updates the font using which this object's text texture is rendered.
         * 
         * @param textResourceName The new font's text texture resource name.
         */
        void updateFont(const std::string& textResourceName);

        /**
         * @brief Updates the point considered this object's origin, where (0,0) represents the top left corner of the object and (1,1) represents its bottom right corner.
         * 
         * @param anchor This object's new anchor.
         */
        void updateAnchor(glm::vec2 anchor);

    private:
        /**
         * @brief Recomputes the text texture and vertex offsets associated with this object based on its configuration.
         * 
         */
        void recomputeTexture();

        /**
         * @brief The color associated with the text rendered by this object.
         * 
         */
        glm::u8vec4 mColor {0x00, 0x00, 0x00, 0xFF};

        /**
         * @brief The font used to render the text texture used by this object.
         * 
         */
        std::shared_ptr<ToyMaker::TextFont> mFont {};

        /**
         * @brief The text displayed by this object on its associated StaticModel surface.
         * 
         */
        std::string mText {};

        /**
         * @brief The scale, relative to the texture's dimensions, with which the object is made.
         * 
         */
        float mScale { 1e-2 };

        /**
         * @brief The width, in pixels, a single line of text is rendered on before it is wrapped around to the next line in the text texture.
         * 
         */
        uint32_t mMaxWidthPixels { 0 };

        /**
         * @brief The point considered the origin of this object, where (0,0) represents the top left hand corner, and (1,1) the bottom right corner.
         * 
         */
        glm::vec2 mAnchor {0.f, 0.f};
    };
}

#endif

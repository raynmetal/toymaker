/**
 * @ingroup UrGameUIComponent UrGameInteractionLayer
 * @file ui_button.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains the class representation of UI buttons used in this project.
 * @version 0.3.2
 * @date 2025-09-13
 * 
 * 
 */

#ifndef ZOAPPUIBUTTON_H
#define ZOAPPUIBUTTON_H

#include <nlohmann/json.hpp>

#include "toymaker/engine/sim_system.hpp"
#include "toymaker/engine/signals.hpp"

#include "toymaker/builtins/interface_pointer_callback.hpp"
#include "toymaker/builtins/nine_slice_panel.hpp"

namespace ToyMaker {

    /**
     * @ingroup UrGameInteractionLayer UrGameUIComponent
     * @brief A UI component class for creating simple buttons comprised of a resizable panel and some text.
     * 
     * May optionally also have a "highlight" texture configured to overlay over a button in order to represent special application defined states.
     * 
     * Upon receiving a pointer left button event from an implementer of ILeftClickable, emits a "ButtonPressed" or "ButtonReleased" event containing the value configured for the button.
     * 
     * ## Usage
     * 
     * Its appearance in JSON may be as follows:
     * 
     * ```jsonc
     * {
     *     "name": "Ur_Button",
     *     "parent": "/viewport_UI/",
     *     "type": "Scene",
     *     "copy": true,
     *     "overrides":  {
     *         "name": "return",
     *         "components": [
     *             {
     *                 "type": "Placement",
     *                 "orientation": [ 1.0,  0.0, 0.0, 0.0 ],
     *                 "position": [ -683.0, -384.0, 0.0, 1.0 ],
     *                 "scale": [ 1.0, 1.0, 1.0 ]
     *             }
     *         ],
     *         "aspects": [
     *             {
     *                 "type": "UIButton",
     *                 "text": "<- Main Menu",
     *                 "font_resource_name": "Roboto_Mono_Regular_24",
     *                 "color": [255, 255, 255, 255],
     *                 "scale": 1.0,
     *                 "anchor": [0.0, 1.0],
     *                 "value": "Game_Of_Ur_Main_Menu",
     *                 "panel_active": "Bad_Button_Active_Panel",
     *                 "panel_inactive": "Bad_Button_Inactive_Panel",
     *                 "panel_hover": "Bad_Button_Hover_Panel",
     *                 "panel_pressed": "Bad_Button_Pressed_Panel",
     *                 "has_highlight": false
     *             }
     *         ]
     *     }
     * },
     * 
     * ```
     * 
     * To connect with, for example, the "ButtonReleased" event for this button, write this in the connections section of the appropriate scene file.
     * 
     * ```c++
     * 
     * {
     *     "from": "/viewport_UI/return/@UIButton",
     *     "signal": "ButtonReleased",
     * 
     *     "to": "/@UrUINavigation",
     *     "observer": "ButtonClickedObserved"
     * }
     * 
     * ```
     * 
     * ... where `"/@UrUINavigation"` is the path to the aspect of the SimObject responsible for handling this signal, and "ButtonClickedObserved" is the name of its SignalObserver.
     * 
     * The button may also be enabled or disabled by calling UIButton::enableButton() or UIButton::disableButton().
     * 
     */
    class UIButton: public ToyMaker::SimObjectAspect<UIButton>, public IHoverable, public ILeftClickable {
    public:
        /**
         * @brief A list of states this button class supports.
         * 
         */
        enum State: uint8_t {
            ACTIVE, //< This button is active, and ready to be pressed.
            HOVER, //< This button is being hovered over by a pointer.
            PRESSED, //< This button has been pressed and held down.
            INACTIVE, //< This button is disabled and will not respond to pointer events.

            //=============
            TOTAL, //< The tally of all button states.
        };
        UIButton(): SimObjectAspect<UIButton>{0} {}

        /**
         * @brief Gets the aspect type string associated with this class.
         * 
         * @return std::string This class' aspect type string.
         */
        inline static std::string getSimObjectAspectTypeName() { return "UIButton"; }

        /**
         * @brief Creates an instance of this aspect based on its description in JSON.
         * 
         * @param jsonAspectProperties This aspect's description in JSON.
         * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed aspect.
         */
        static std::shared_ptr<BaseSimObjectAspect> create(const nlohmann::json& jsonAspectProperties);

        /**
         * @brief Creates an instance of the aspect using this this aspect as its blueprint.
         * 
         * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed aspect.
         */
        std::shared_ptr<BaseSimObjectAspect> clone() const override;

        /**
         * @brief Sets the state of this button and computes its texture on becoming an active part of the scene.
         * 
         */
        void onActivated() override;

        /**
         * @brief Enables the button so that it will respond to pointer events.
         * 
         */
        void enableButton();

        /**
         * @brief Disables this button, preventing it from responding to pointer events.
         * 
         */
        void disableButton();

        /**
         * @brief Updates the text displayed within the button.
         * 
         * @param newText The new text displayed by this button.
         */
        void updateText(const std::string& newText);

        /**
         * @brief Updates the scale of the text in this button.
         * 
         * @param scale The multiplier by which this button's text texture is scaled.
         */
        void updateTextScale(float scale);

        /**
         * @brief Updates the font used to generate the text texture used by this button.
         * 
         * @param textResourceName The resource name of the new TextFont resource this button should use.
         */
        void updateTextFont(const std::string& textResourceName);

        /**
         * @brief Updates the color of the text rendered by TextFont.
         * 
         * @param textColor The new color with which to render the text, each component specified as a value between 0-255.
         */
        void updateTextColor(glm::u8vec4 textColor);

        /**
         * @brief Updates the point on the button StaticModel considered its anchor, where (0, 0) is the top left corner, and (1, 1) is the bottom right corner.
         * 
         * @param newAnchor The new anchor (origin) for this UI element.
         */
        void updateButtonAnchor(glm::vec2 newAnchor);

        /**
         * @brief The color applied to this object's "highlight" texture.
         * 
         * @param newColor The new color of the highligh sub object of this button.
         */
        void updateHighlightColor(glm::vec4 newColor);

    private:
        /**
         * @brief The current state of this button.
         * 
         */
        State mCurrentState { State::ACTIVE };

        /**
         * @brief Whether this button is being hovered over presently.
         * 
         */
        bool mHovered { false };

        /**
         * @brief The panel textures associated with the different states of this button.
         * 
         */
        std::array<std::shared_ptr<NineSlicePanel>, State::TOTAL> mStatePanels {};

        /**
         * @brief The anchor of the button, considered the StaticModel component's "origin".
         * 
         * (0, 0) represents the top left corner of the button, and (1, 1) represents the bottom right corner.
         * 
         */
        glm::vec2 mAnchor {.5f, .5f};

        /**
         * @brief The value emitted by this button when it has been pressed-and-released.
         * 
         */
        std::string mValue {""};

        /**
         * @brief The text rendered on this button.
         * 
         */
        std::string mTextOverride {""};

        /**
         * @brief The scale of the rendered text used with this button.
         * 
         */
        float mTextScaleOverride {1.f};

        /**
         * @brief The name of the font resource used to render this button's text.
         * 
         */
        std::string mTextFontOverride {""};

        /**
         * @brief The color value with which this button's text is rendered.
         * 
         */
        glm::u8vec4 mTextColorOverride { 0x00, 0x00, 0x00, 0xFF };

        /**
         * @brief The texture used on this object's child object, which acts as an overlay to this button.
         * 
         * This overlay can be used to communicate special information not covered by any currently existing button states.
         * 
         */
        std::shared_ptr<NineSlicePanel> mHighlightPanel {};

        /**
         * @brief The colour which the highlight object will be rendered with.
         * 
         */
        glm::vec4 mHighlightColor {0.f, 0.f, 0.f, 0.f};

        /**
         * @brief The method responsible for recomputing this button's panel and text textures.
         * 
         */
        void recomputeTexture();

        /**
         * @brief The method through which all state changes pass, which is responsible for firing events related to those state changes.
         * 
         * @param newState The new state the button is transitioning to.
         */
        void updateButtonState(UIButton::State newState);

        /**
         * @brief Fires an event based on the button's state.  Related closely with updateButtonState().
         * 
         */
        void fireStateEvent();

        /**
         * @brief Gets the object which owns the text texture associated with this button.
         * 
         * @return std::shared_ptr<ToyMaker::SimObject> The object with this button's text texture.
         */
        std::shared_ptr<ToyMaker::SimObject> getTextObject();
    public:

        /**
         * @brief The signal fired when this button has just been pressed (but not released).
         * 
         */
        ToyMaker::Signal<std::string> mSigButtonPressed { *this, "ButtonPressed" };

        /**
         * @brief The signal fired when this button has just been released after being pressed.
         * 
         */
        ToyMaker::Signal<std::string> mSigButtonReleased { *this, "ButtonReleased" };

        /**
         * @brief The signal fired when a pointer just enters this button.
         * 
         */
        ToyMaker::Signal<std::string> mSigButtonHoveredOver { *this, "ButtonHoveredOver" };

        /**
         * @brief The signal fired when this button is activated.
         * 
         */
        ToyMaker::Signal<> mSigButtonActivated { *this, "ButtonActivated" };

        /**
         * @brief The signal fired when this button is deactivated.
         * 
         */
        ToyMaker::Signal<> mSigButtonDeactivated { *this, "ButtonDeactivated" };

    private:
        /**
         * @brief Handler for the hover event where a pointer just enters this button's area.
         * 
         * @param pointerLocation The location of the intersection between the pointer raycast and this object.
         * @retval true This button handled the pointer enter event.
         * @retval false The button did not handle the pointer enter event.
         */
        bool onPointerEnter(glm::vec4 pointerLocation) override;

        /**
         * @brief Handler for the hover event where a pointer just leaves this button's area.
         * 
         * @retval true This button handled the pointer leave event.
         * @retval false This button didn't handle the pointer leave event.
         */
        bool onPointerLeave() override;

        /**
         * @brief Handler for the hover event where a pointer presses on this button.
         * 
         * @param pointerLocation The location of the intersection of the pointer ray and this object.
         * @retval true This button handled the pointer button press event.
         * @retval false This button did not handle the pointer button press event.
         */
        bool onPointerLeftClick(glm::vec4 pointerLocation) override;

        /**
         * @brief Handler for the hover event where a pointer presses and releases this button.
         * 
         * @param pointerLocation The location of the intersection of the pointer ray and this object.
         * @retval true This button handled the pointer button release event.
         * @retval false This button did not handle the pointer button release event.
         */
        bool onPointerLeftRelease(glm::vec4 pointerLocation) override;
    };


    /** @ingroup UrGameUIComponent UrGameInteractionLayer */
    NLOHMANN_JSON_SERIALIZE_ENUM(UIButton::State, {
        {UIButton::State::ACTIVE, "active"},
        {UIButton::State::HOVER, "hover"},
        {UIButton::State::PRESSED, "pressed"},
        {UIButton::State::INACTIVE, "inactive"},
    });

}

#endif

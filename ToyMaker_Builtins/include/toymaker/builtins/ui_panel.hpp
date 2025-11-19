/**
 * @ingroup UrGameUIComponent
 * @file ui_panel.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains the class responsible for managing, configuring, and displaying a NineSlicePanel resource.
 * @version 0.3.2
 * @date 2025-09-13
 * 
 * 
 */

/**
 * @defgroup UrGameVisualLayer Visual Layer
 * @ingroup UrGame
 * 
 */

/**
 * @defgroup UrGameUIComponent UI Components
 * @ingroup UrGameVisualLayer
 * 
 */

#ifndef ZOAPPUIPANEL_H
#define ZOAPPUIPANEL_H

#include "toymaker/engine/sim_system.hpp"

#include "toymaker/builtins/nine_slice_panel.hpp"

namespace ToyMaker {

    /**
     * @ingroup UrGameUIComponent
     * @brief UI aspect responsible for managing and rendering a NineSlicePanel texture on the UI.
     * 
     * ## Usage:
     * 
     * Its appearance in JSON is as follows:
     * 
     * ```jsonc
     * 
     * {
     *     "type": "UIPanel",
     *     "anchor": [1.0, 0.0],
     *     "content_size": [454.0, 764.0],
     *     "panel_resource_name": "Bad_Panel"
     * }
     * 
     * ```
     * 
     */
    class UIPanel: public ToyMaker::SimObjectAspect<UIPanel> {
    public:
        /**
         * @brief Constructs a new UIPanel aspect.
         * 
         */
        UIPanel(): SimObjectAspect<UIPanel>{0} {}

        /**
         * @brief Gets the aspect type string associated with this class.
         * 
         * @return std::string The aspect type string associated with this class.
         * 
         */
        inline static std::string getSimObjectAspectTypeName() { return "UIPanel"; }

        /**
         * @brief Constructs a new UIPanel instance based on its JSON description.
         * 
         * @param jsonAspectProperties The JSON description of a UIPanel.
         * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed UIPanel instance.
         */
        static std::shared_ptr<BaseSimObjectAspect> create(const nlohmann::json& jsonAspectProperties);

        /**
         * @brief Constructs a new UIPanel instance using this one as a blueprint.
         * 
         * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed UIPanel instance.
         * 
         */
        std::shared_ptr<BaseSimObjectAspect> clone() const override;

        /**
         * @brief Loads and renders the texture of a panel per its configuration.
         * 
         */
        void onActivated() override;

        /**
         * @brief Updates the size of the central region of the NineSlicePanel.
         * 
         * @param contentSize The new content display region of the NineSlicePanel.
         */
        void updateContentSize(glm::vec2 contentSize);

        /**
         * @brief Updates the point considered the new origin for this object, where (0,0) represents the top left corner of the object, and (1,1) the bottom right corner.
         * 
         * 
         * @param anchor The point considered the new origin of this object.
         */
        void updateAnchor(glm::vec2 anchor);

        /**
         * @brief Changes the panel resource used to create this object's panel texture.
         * 
         * @param newPanel The new panel resource used to render this object's texture.
         */
        void updateBasePanel(std::shared_ptr<NineSlicePanel> newPanel);

    private:
        /**
         * @brief The method responsible for actually creating the new texture through the loaded panel resource.
         * 
         */
        void recomputeTexture();

        /**
         * @brief The panel resource used to create this texture.
         * 
         */
        std::shared_ptr<NineSlicePanel> mBasePanel {};

        /**
         * @brief The size of the central area of the nine slice panel in which content can be displayed.
         * 
         */
        glm::vec2 mContentSize {0.f, 0.f};

        /**
         * @brief The point considered the origin of this object, where (0,0) represents the top left corner and (1,1) the bottom right corner.
         * 
         */
        glm::vec2 mAnchor {0.f, 0.f};
    };

}

#endif

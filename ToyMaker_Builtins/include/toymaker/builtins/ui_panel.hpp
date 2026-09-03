/**
 * @ingroup ToyMakerBuiltins
 * @file ui_panel.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains the class responsible for managing, configuring, and displaying a NineSlicePanel resource.
 * @version 0.3.2
 * @date 2025-09-13
 *
 *
 */

#ifndef TOYMAKERBUILTINS_UIPANEL_H
#define TOYMAKERBUILTINS_UIPANEL_H

#include "toymaker/engine/sim_system.hpp"

#include "nine_slice_panel.hpp"
#include "ui_container.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToyMakerBuiltins
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
    class UIPanel: public ToyMaker::SimObjectAspect<UIPanel>, public IUIElement {
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
        virtual void updateContentSize(glm::vec2 contentSize);

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

        /**
         * @brief Returns the current content size for this panel.
         *
         */
        inline glm::vec2 getContentSize() const { return mContentSize; }

        /**
         * @brief Returns the reference coordinate relative to which the position of this element is computed.
         *
         *
         */
        inline glm::vec2 getReferenceCoordinate() const override { return mReferenceCoordinate; }

        /**
         * @brief Returns this element's offsets relative to the owning container's reference coordinate.
         *
         */
        inline glm::vec2 getOffsets() const override { return mOffsets; }

        /**
         * @brief Returns whether the owning container can modify this object's children's positions.
         *
         */
        inline bool allowsDescendantControl() const override { return mAllowsDescendantControl; }

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

        /**
         * @brief The point on the owning container relative to which this elements offsets
         * are computed.
         *
         * @see ToyMaker::UIContainer
         */
        glm::vec2 mReferenceCoordinate { 0.f, 0.f };

        /**
         * @brief The offsets of the anchor of this element relative to the owning container's reference coordinate.
         *
         * @see ToyMaker::UIContainer
         */
        glm::vec2 mOffsets { 0.f, 0.f };

        /**
         * @brief Whether elements falling under this panel can have their positions controlled by the
         * owning container.
         *
         */
        bool mAllowsDescendantControl { false };
    };
}

#endif

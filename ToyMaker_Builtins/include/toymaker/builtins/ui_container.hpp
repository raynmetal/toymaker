#ifndef TOYMAKERBUILTINS_UICONTAINER_H
#define TOYMAKERBUILTINS_UICONTAINER_H

#include "toymaker/engine/sim_system.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToyMakerBuiltins
     * @brief Interface for a single UI element.
     *
     * @see ToyMaker::UIContainer
     *
     * @TODO: Improve reusability by actually having UI elements share
     * certain data and instructions.
     *
     */
    class IUIElement {
    public:
        /**
         * @brief Returns the reference coordinate on the container relative to which
         * this element's offsets are defined.
         *
         * (0, 0) represents the top left hand corner of the container, while
         * (1, 1) represents the bottom-right hand corner of the container.
         *
         * @see ToyMaker::UIContainer
         */
        virtual glm::vec2 getReferenceCoordinate() const = 0;

        /**
         * @brief Returns the offsets relative to the container reference coordinates
         * used to position this element's anchor.
         *
         * @see ToyMaker::UIContainer
         *
         */
        virtual glm::vec2 getOffsets() const = 0;

        /**
         * @brief Whether the container is permitted to reposition this element's
         * children.
         *
         */
        virtual bool allowsDescendantControl() const = 0;
    };

    /**
     * @ingroup ToyMakerBuiltins
     * @brief Container for UI elements, responsible for repositioning elements in case
     * their properties, or its properties, change.
     *
     * Changes size according to its owning viewport.
     *
     * Each UI element decides on a point between (0, 0), and (1, 1), representing
     * the top-left and bottom-right corners of this container respectively,
     * relative to which the element's anchor is offset.  When the container's size
     * changes, the UI element's position in the scene changes accordingly.
     *
     * Elements are marked with IUIElement.
     *
     * @see ToyMaker::IUIElement
     *
     * @TODO: Either flesh this out or replace with a proper UI library.
     *
     * @WARNING: There can only be one of these in one viewport.
     *
     */
    class UIContainer: public ToyMaker::SimObjectAspect<UIContainer> {
    public:
        /**
         * @brief Constructs a new UIContainer aspect.
         *
         */
        UIContainer(): SimObjectAspect<UIContainer>{ 0 } {}

        /**
         * @brief Gets the aspect type string associated with this class.
         *
         * @return std::string The aspect type string associated with this class.
         *
         */
        inline static std::string getSimObjectAspectTypeName() { return "UIContainer"; }

        /**
         * @brief Constructs a new UIContainer instance based on its JSON description.
         *
         * @param jsonAspectProperties The JSON description of the container.
         *
         * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed UIContainer instance.
         *
         */
        static std::shared_ptr<BaseSimObjectAspect> create(const nlohmann::json& jsonAspectProperties);

        /**
         * @brief Constructs a new UIContainer instance using this one as a blueprint.
         *
         * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed UIContainer instance.
         */
        std::shared_ptr<BaseSimObjectAspect> clone() const override;

        /**
         * @brief Subscribes to viewport resizing signals, and adjusts positions of child UI elements.
         *
         */
        void onActivated() override;

    private:
        /**
         * @brief Procedure for when the owning viewport is resized, where the container is scaled
         * to the viewport's dimensions, and child UI elements are repositioned.
         *
         */
        void onViewportResized(const ViewportNode::RenderConfiguration& renderConfiguration);

        /**
         * @brief The total dimensions spanned by this container.
         *
         */
        glm::vec2 mContentSize { 1.f, 1.f };

        /**
         * @brief Observer for viewport resize events, which calls onViewportResized.
         *
         */
        SignalObserver<ViewportNode::RenderConfiguration> mObservedViewportResized {
            *this, "onViewportResized", [this](const auto& renderConfiguration) {
                this->onViewportResized(renderConfiguration);
            }
        };
    };
}

#endif

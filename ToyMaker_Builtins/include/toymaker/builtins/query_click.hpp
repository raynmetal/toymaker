/**
 * @ingroup UrGameInteractionLayer
 * @file query_click.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains the QueryClick class definition, responsible for calling pointer click callbacks for objects in its world that support it.
 * @version 0.3.2
 * @date 2025-09-13
 * 
 */

#ifndef ZOAPPQUERYCLICK_H
#define ZOAPPQUERYCLICK_H

#include "toymaker/engine/camera_system.hpp"
#include "toymaker/engine/sim_system.hpp"
#include "toymaker/engine/input_system/input_system.hpp"

#include "toymaker/builtins/interface_pointer_callback.hpp"

namespace ToyMaker {

    /**
     * @ingroup UrGameInteractionLayer
     * @brief The aspect responsible for conducting raycasts and calling pointer event callbacks on eligible objects.
     * 
     * It has access to object click callbacks through its IUsePointer interface.
     * 
     * Should be attached to a SimObject which is a part of the CameraSystem.
     * 
     * Its appearance in JSON is as follows:
     * 
     * ```jsonc
     * 
     * { "type": "QueryClick" }
     * 
     * ```
     * 
     */
    class QueryClick: public ToyMaker::SimObjectAspect<QueryClick>, public IUsePointer {
    public:

        /**
         * @brief Gets the aspect type string associated with this class.
         * 
         * @return std::string The aspect type string for this class.
         */
        inline static std::string getSimObjectAspectTypeName() { return "QueryClick"; }

        /**
         * @brief Creates a new query click aspect using this one as its blueprint.
         * 
         * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed query click object.
         */
        std::shared_ptr<BaseSimObjectAspect> clone() const override;

        /**
         * @brief Creates a query click object based on its description in JSON.
         * 
         * @param jsonAspectProperties The json description of this aspect.
         * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed aspect.
         */
        static std::shared_ptr<BaseSimObjectAspect> create(const nlohmann::json& jsonAspectProperties);

    protected:

        /**
         * @brief Method responsible for calling hover callbacks when a pointer move event is received.
         * 
         * @param actionData Pointer move event data.
         * @param actionDefinition Pointer move event definition.
         * @retval true The pointer move event was handled by an object handling hover events.
         * @retval false The pointer move event wasn't handled by any object.
         */
        bool onPointerMove(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition);

        /**
         * @brief Method responsible for calling left mouse button press callbacks on eligible objects when a pointer click event is received.
         * 
         * @param actionData The location of the pointer click.
         * @param actionDefinition The definition of the pointer click action.
         * @retval true The action was handled by an object supporting the click callback.
         * @retval false The action was not handled by any object.
         */
        bool onLeftClick(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition);

        /**
         * @brief Method responsible for calling left mouse button release callbacks on eligible objects when a pointer button release event is received.
         * 
         * @param actionData The location of the pointer button release event.
         * @param actionDefinition The definition of the pointer button release action.
         * @retval true The pointer button release action was handled by an objects supporting the appropriate callback.
         * @retval false The pointer button release action was not handledy by any object.
         */
        bool onLeftRelease(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition);

        /**
         * @brief The binding connecting the pointer move action to its handler method on this object.
         * 
         */
        std::weak_ptr<ToyMaker::FixedActionBinding> handlerPointerMove {
            declareFixedActionBinding(
                "UI",
                "PointerMove",
                [this](const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
                    return this->onPointerMove(actionData, actionDefinition);
                }
            )
        };

        /**
         * @brief The binding connecting the pointer left click action to its handler method on this object.
         * 
         */
        std::weak_ptr<ToyMaker::FixedActionBinding> handlerLeftClick {
            declareFixedActionBinding(
                "UI",
                "Tap",
                [this](const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
                    return this->onLeftClick(actionData, actionDefinition);
                }
            )
        };

        /**
         * @brief The binding connecting the pointer left release action to its handler method on this object.
         * 
         */
        std::weak_ptr<ToyMaker::FixedActionBinding> handlerLeftRelease {
            declareFixedActionBinding(
                "UI",
                "Untap",
                [this](const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
                    return this->onLeftRelease(actionData, actionDefinition);
                }
            )
        };

    private:
        /**
         * @brief Constructs a new Query Click aspect.
         * 
         */
        QueryClick(): SimObjectAspect<QueryClick>{0} {}

        /**
         * @brief Converts the location of a pointer event from its normalized viewport coordinates to a camera relative ray into the scene.
         * 
         * @param clickCoordinates The normalized coordinates of the pointer relative to this aspect's camera's viewport.
         * @return ToyMaker::Ray The ray constructed from pointer coordinates.
         */
        ToyMaker::Ray rayFromClickCoordinates(glm::vec2 clickCoordinates);

        /**
         * @brief A list of results from previous pointer queries.
         * 
         * Used in order to compare results from the two most recent frames, to determine whether or not hover events should be fired this frame.
         * 
         * @todo We're storing shared pointers to nodes in the scene tree here.  There's a bug in waiting if we have a reference to a node that was taken off the active scene tree between queries.  I don't really know how to think about the problem just now.
         * 
         */
        std::vector<std::shared_ptr<ToyMaker::SceneNodeCore>> mPreviousQueryResults {};
    };

}

#endif

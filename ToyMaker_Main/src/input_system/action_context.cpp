#include <memory>

#include "toymaker/engine/input_system/input_system.hpp"

using namespace ToyMaker;

ActionData ActionContext::ApplyInput(const ActionDefinition& actionDefinition, const ActionData& actionData, const AxisFilter targetAxis, const UnmappedInputValue& inputValue) {
    // write action state into the actionData variable
    const float valueSign { targetAxis&AxisFilterMask::SIGN? -1.f: 1.f };
    const double newValue { valueSign * inputValue.mAxisValue };
    ActionData newActionData{ actionData };
    newActionData.mCommonData.mTimestamp = inputValue.mTimestamp;

    switch(targetAxis){
        case AxisFilter::SIMPLE:
            assert(
                actionDefinition.mAttributes&InputAttributes::HAS_BUTTON_VALUE
                && "Action must support button values for AxisFilter::SIMPLE to apply"
            );
            newActionData.mCommonData.mActivated = inputValue.mActivated;
        break;

        case AxisFilter::X_POS:
        case AxisFilter::X_NEG:
            assert(
                (actionDefinition.mAttributes&InputAttributes::HAS_STATE_VALUE
                || actionDefinition.mAttributes&InputAttributes::HAS_CHANGE_VALUE)
                && static_cast<uint8_t>(actionDefinition.mAttributes&InputAttributes::N_AXES) >= static_cast<uint8_t>(AxisFilter::X_POS)
                && "Action must support change values or state values and have one or more axes"
            );
            if(
                actionDefinition.mValueType == ActionValueType::STATE
                // affect only if the old value and the new one belong to the 
                // same axis (or is 0.f)
                && valueSign * actionData.mOneAxisActionData.mValue >= 0.f
            ) {
                newActionData.mOneAxisActionData.mValue = newValue;
            } else if (actionDefinition.mValueType == ActionValueType::CHANGE) {
                newActionData.mOneAxisActionData.mValue += newValue;
            }
        break;

        case AxisFilter::Y_POS:
        case AxisFilter::Y_NEG:
            assert(
                (actionDefinition.mAttributes&InputAttributes::HAS_STATE_VALUE
                || actionDefinition.mAttributes&InputAttributes::HAS_CHANGE_VALUE)
                && static_cast<uint8_t>(actionDefinition.mAttributes&InputAttributes::N_AXES) >= static_cast<uint8_t>(AxisFilter::Y_POS)
                && "Action must support change values or state values and have two or more axes"
            );
            if(
                actionDefinition.mValueType == ActionValueType::STATE
                && valueSign * actionData.mTwoAxisActionData.mValue.y >= 0.f
            ) {
                newActionData.mTwoAxisActionData.mValue.y = newValue;
            } else if (actionDefinition.mValueType == ActionValueType::CHANGE) {
                newActionData.mTwoAxisActionData.mValue.y += newValue;
            }
        break;

        case AxisFilter::Z_POS:
        case AxisFilter::Z_NEG:
            assert(
                (actionDefinition.mAttributes&InputAttributes::HAS_STATE_VALUE
                || actionDefinition.mAttributes&InputAttributes::HAS_CHANGE_VALUE)
                && (static_cast<uint8_t>(actionDefinition.mAttributes&InputAttributes::N_AXES) == static_cast<uint8_t>(AxisFilter::Z_POS))
                && "Action must support change or state values and must have three axes"
            );
            if(
                actionDefinition.mValueType== ActionValueType::STATE
                && valueSign * actionData.mThreeAxisActionData.mValue.z >= 0.f
            ) {
                newActionData.mThreeAxisActionData.mValue.z = newValue;
            } else if (actionDefinition.mValueType == ActionValueType::CHANGE) {
                newActionData.mThreeAxisActionData.mValue.z += newValue;
            }
        break;

        default:
            assert(false && "This is an unsupported path and should never execute");
        break;
    }

    // Normalize/clamp non-location action states with magnitudes greater than 1.f.
    if(
        newActionData.mCommonData.mType != ActionType::BUTTON
        // Change values are kept as is
        && !(actionDefinition.mAttributes&InputAttributes::HAS_CHANGE_VALUE)
        // location states are kept at their full range (otherwise we'd miss ~1/3rd of
        // the window)
        && !(actionDefinition.mAttributes&InputAttributes::STATE_IS_LOCATION)
        && glm::length(newActionData.mThreeAxisActionData.mValue) > 1.f
    ) {
        // NOTE: in order for this to work, we need action data to guarantee
        // that unused dimensions have a value of 0.f
        newActionData.mThreeAxisActionData.mValue = glm::normalize(newActionData.mThreeAxisActionData.mValue);
    }

    return newActionData;
}

void ActionContext::registerAction(const ActionName& name, InputAttributesType attributes) {
    assert(mActions.find(ActionDefinition{{mName, name}}) == mActions.end() && "Another action with this name has already been registered");
    assert(
        !(
            (attributes&InputAttributes::HAS_CHANGE_VALUE) && (attributes&InputAttributes::HAS_STATE_VALUE)
        ) && "Action may either have a change value or a state value but not both"
    );

    ActionDefinition actionDefinition {{mName,name}};
    actionDefinition.mAttributes = attributes;
    // TODO: redundant
    actionDefinition.mValueType = attributes&InputAttributes::HAS_CHANGE_VALUE? ActionValueType::CHANGE: ActionValueType::STATE;
    ActionData initialActionData { static_cast<uint8_t>(actionDefinition.mAttributes&InputAttributes::N_AXES) };

    mActions.emplace(std::pair<ActionDefinition, ActionData>{actionDefinition, initialActionData});
    mActionToInputBinds.emplace(std::pair<ActionDefinition, std::set<InputCombo>>{actionDefinition, {}});
}

void ActionContext::registerAction(const nlohmann::json& actionParameters) {
    std::string actionName { actionParameters.at("name").get<std::string>() };
    assert(mActions.find(ActionDefinition{{mName, actionName}}) == mActions.end() && "An action with this name has previously been registered");
    ActionDefinition actionDefinition = actionParameters;
    registerAction(actionDefinition.mName, actionDefinition.mAttributes);
}

void ActionContext::unregisterAction(const ActionName& name) {
    const auto& actionIter {mActions.find(ActionDefinition{{mName, name}})};
    assert(actionIter != mActions.end() && "This action is not registered with this context");
    const auto& actionDefinition { actionIter->first };

    unregisterInputBinds(actionDefinition.mName);
    mActions.erase(actionDefinition);
}

void ActionContext::registerInputBind(const nlohmann::json& inputBindParameters) {
    std::string actionName { inputBindParameters.at("action").get<std::string>() };
    AxisFilter targetAxis = inputBindParameters.at("target_axis");
    InputCombo inputCombo = inputBindParameters.at("input_combo");
    registerInputBind(actionName, targetAxis, inputCombo);
}

void ActionContext::registerInputBind(const ActionName& forAction, AxisFilter targetAxis, const InputCombo& withInput){
    const auto& actionDefinitionIter {mActions.find(ActionDefinition{{mName, forAction}})};
    assert(mInputBindToAction.find(withInput) == mInputBindToAction.end() && "This input combination has already been registered with another action");
    assert(actionDefinitionIter != mActions.end() && "This action has not been registered");

    const ActionDefinition& actionDefinition { actionDefinitionIter->first };
    assert(
        ( // the axis indicated by onAxis must be one of the dimensions possessed by the action
            ((static_cast<uint8_t>(targetAxis)&AxisFilterMask::ID) <= (static_cast<uint8_t>(actionDefinition.mAttributes)&InputAttributes::N_AXES))
        ) && ( 
            // Target axis isn't negative, ...
            !(static_cast<uint8_t>(targetAxis)&AxisFilterMask::SIGN)
            // ... or if it is, then ...
            || (
                // .. the action must support negative state values...
                (static_cast<uint8_t>(actionDefinition.mAttributes)&InputAttributes::HAS_NEGATIVE)
                // ... or support change values (which de facto support negative values).
                || (static_cast<uint8_t>(actionDefinition.mAttributes)&InputAttributes::HAS_CHANGE_VALUE)
            )
        )
        && "The axis specified is not among those available for this action"
    );
    mInputBindToAction.emplace(withInput, std::pair<AxisFilter, ActionDefinition>{targetAxis, actionDefinition});
    mActionToInputBinds[actionDefinition].emplace(withInput);
    mInputManager.registerInputCombo(mName, withInput);
}

void ActionContext::unregisterInputBind(const InputCombo& inputCombo) {
    const auto& inputBindToActionIter { mInputBindToAction.find(inputCombo) };
    assert(inputBindToActionIter != mInputBindToAction.end() && "This input binding does not exist");

    const ActionDefinition& actionDefinition { std::get<1>(inputBindToActionIter->second) };
    mInputBindToAction.erase(inputCombo);
    mActionToInputBinds[actionDefinition].erase(inputCombo);
    mInputManager.unregisterInputCombo(mName, inputCombo);
}

void ActionContext::unregisterInputBinds(const ActionName& forAction) {
    const auto& actionIter { mActions.find(ActionDefinition{{mName, forAction}}) };
    assert(actionIter != mActions.end() && "This action has not been registered");

    while(!mActionToInputBinds[actionIter->first].empty()) {
        const InputCombo inputCombo { *mActionToInputBinds[actionIter->first].begin() };
        mInputBindToAction.erase(inputCombo);
        mActionToInputBinds[actionIter->first].erase(inputCombo);
        mInputManager.unregisterInputCombo(mName, inputCombo);
    }
}

void ActionContext::unregisterInputBinds() {
    for(const auto& actionIter: mActions) {
        unregisterInputBinds(actionIter.first.mName);
    }
}

void ActionContext::resetActionData(const ActionName& forAction, uint32_t timestamp) {
    // TODO: Figure out how to handle this properly. When will this take place anyway?
    const ActionDefinition& actionDefinition { mActions.find(ActionDefinition{{mName, forAction}})->first };

    // Let action listeners know that a reset has occurred
    ActionData actionData { static_cast<uint8_t>(actionDefinition.mAttributes&InputAttributes::N_AXES) };
    actionData.mCommonData.mTimestamp = timestamp;
    actionData.mCommonData.mTriggeredBy = ActionTrigger::RESET;
    mPendingTriggeredActions.push_back(std::pair(actionDefinition, actionData));
    mActions[actionDefinition] = actionData;
}

void ActionContext::resetActionData(uint32_t timestamp) {
    for(const auto& action: mActions) {
        resetActionData(action.first.mName, timestamp);
    }
}

void ActionContext::mapToAction(const UnmappedInputValue& inputValue, const InputCombo& inputCombo) {
    const auto& comboToActionIter { mInputBindToAction.find(inputCombo) };
    assert(
        comboToActionIter != mInputBindToAction.end() 
        && "This input combination has not been bound to any action in this context"
    );

    const ActionDefinition& actionDefinition { mActions.find(comboToActionIter->second.second)->first};
    const AxisFilter& axisFilter { comboToActionIter->second.first };

    // Action state values should be retrieved from memory, while change 
    // values should be made fresh
    ActionData actionData { 
        (actionDefinition.mAttributes&InputAttributes::HAS_CHANGE_VALUE)?
        ActionData { static_cast<uint8_t>(actionDefinition.mAttributes&N_AXES) }:
        mActions[actionDefinition] 
    };
    if(
        !mPendingTriggeredActions.empty()
        && actionDefinition == mPendingTriggeredActions.back().first
    ) {
        actionData = mPendingTriggeredActions.back().second;
        mPendingTriggeredActions.pop_back();
    }

    actionData = ApplyInput(actionDefinition, actionData, axisFilter, inputValue);

    //  Push the newly constructed actionData to the back of
    // our pending action queue
    mPendingTriggeredActions.push_back(std::pair(actionDefinition, actionData));

    // Update the action map with this latest action value
    mActions[actionDefinition] = actionData;
}

std::vector<std::pair<ActionDefinition, ActionData>> ActionContext::getTriggeredActions() {
    std::vector<std::pair<ActionDefinition, ActionData>> triggeredActions {};
    std::swap<std::pair<ActionDefinition, ActionData>> (mPendingTriggeredActions, triggeredActions);
    return triggeredActions;
}

void ActionDispatch::registerActionHandler(const QualifiedActionName& contextActionPair, std::weak_ptr<IActionHandler> actionHandler) {
    mActionHandlers[contextActionPair].insert(actionHandler);
}
void ActionDispatch::unregisterActionHandler(const QualifiedActionName& contextActionPair, std::weak_ptr<IActionHandler> actionHandler) {
    mActionHandlers.at(contextActionPair).erase(actionHandler);
}
void ActionDispatch::unregisterActionHandler(std::weak_ptr<IActionHandler> actionHandler) {
    for(auto& actionHandlerGroup: mActionHandlers) {
        actionHandlerGroup.second.erase(actionHandler);
    }
}

bool ActionDispatch::dispatchAction(const std::pair<ActionDefinition, ActionData>& pendingAction) {
    std::vector<std::weak_ptr<IActionHandler>> handlersToUnregister {};
    bool handled { false };
    for(auto handler: mActionHandlers[pendingAction.first]) {
        if(!handler.expired()) {
            handled = handler.lock()->handleAction(pendingAction.second, pendingAction.first) || handled;
        } else {
            handlersToUnregister.push_back(handler);
        }
    }

    for(auto handler: handlersToUnregister) {
        unregisterActionHandler(handler);
    }

    return handled;
}

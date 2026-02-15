#include <cassert>
#include <algorithm>
#include <iostream>

#include <SDL3/SDL.h>

#include "toymaker/engine/util.hpp"
#include "toymaker/engine/window_context_manager.hpp"
#include "toymaker/engine/input_system/input_system.hpp"

using namespace ToyMaker;

const std::map<InputSourceType, InputAttributesType> ToyMaker::kInputSourceTypeAttributes {
    {{DeviceType::MOUSE, ControlType::POINT}, {
        (2&InputAttributes::N_AXES)
        | InputAttributes::HAS_STATE_VALUE
        | InputAttributes::HAS_CHANGE_VALUE
        | InputAttributes::STATE_IS_LOCATION
    }},
    {{DeviceType::MOUSE, ControlType::BUTTON}, {
        (2&InputAttributes::N_AXES)
        | InputAttributes::HAS_BUTTON_VALUE
        | InputAttributes::HAS_STATE_VALUE
        | InputAttributes::STATE_IS_LOCATION
    }},
    {{DeviceType::MOUSE, ControlType::MOTION}, {
        (2&InputAttributes::N_AXES)
        | InputAttributes::HAS_CHANGE_VALUE
    }},
    {{DeviceType::KEYBOARD, ControlType::BUTTON}, {
        InputAttributes::HAS_BUTTON_VALUE
    }},
    {{DeviceType::TOUCH, ControlType::POINT}, {
        (2&InputAttributes::N_AXES)
        | InputAttributes::HAS_BUTTON_VALUE
        | InputAttributes::HAS_STATE_VALUE
        | InputAttributes::HAS_CHANGE_VALUE
        | InputAttributes::STATE_IS_LOCATION
    }},
    {{DeviceType::CONTROLLER, ControlType::AXIS}, {
        (1&InputAttributes::N_AXES)
        | InputAttributes::HAS_STATE_VALUE
        | InputAttributes::HAS_NEGATIVE
    }},
    {{DeviceType::CONTROLLER, ControlType::RADIO},{
        (2&InputAttributes::N_AXES)
        | InputAttributes::HAS_STATE_VALUE
        | InputAttributes::HAS_NEGATIVE
    }},
    {{DeviceType::CONTROLLER, ControlType::MOTION},{
        (2&InputAttributes::N_AXES)
        | InputAttributes::HAS_CHANGE_VALUE
    }},
    {{DeviceType::CONTROLLER, ControlType::BUTTON}, {
        InputAttributes::HAS_BUTTON_VALUE
    }},
    {{DeviceType::CONTROLLER, ControlType::POINT}, {
        (2&InputAttributes::N_AXES)
        | InputAttributes::HAS_BUTTON_VALUE
        | InputAttributes::HAS_STATE_VALUE
        | InputAttributes::STATE_IS_LOCATION
    }},
    {{DeviceType::NA, ControlType::NA}, 0}
};
bool hasValue(const InputSourceDescription& input, const AxisFilterType filter) {
    uint8_t axisID { static_cast<uint8_t>(filter&AxisFilterMask::ID) };
    uint8_t axisSign { static_cast<uint8_t>(filter&AxisFilterMask::SIGN) };
    uint8_t axisDelta { static_cast<uint8_t>(filter&AxisFilterMask::CHANGE) };

    return (
        input
        && (
            // No axis specified, so in other words it's a button
            (axisID == AxisFilter::SIMPLE && input.mAttributes&HAS_BUTTON_VALUE)

            // An axis is specified, so check whether the desired value
            // supports negatives or represents a change
            || (
                axisID > 0
                && (axisID <= (input.mAttributes&N_AXES))
                && (
                    (axisSign <= (input.mAttributes&HAS_NEGATIVE))
                    || (axisDelta <= (input.mAttributes&HAS_CHANGE_VALUE))
                )
            )
        )
    );
}

bool isValid(const InputFilter& inputFilter) {
    return hasValue(inputFilter.mControl, inputFilter.mAxisFilter);
}

double InputManager::getRawValue(const InputFilter& inputFilter, const SDL_Event& inputEvent) const {
    assert(isValid(inputFilter) && "This is an empty input filter, and does not map to any input value");

    float axisValue {0.f};

    int windowWidth, windowHeight;
    SDL_GetWindowSize(WindowContext::getInstance().getSDLWindow(), &windowWidth, &windowHeight);

    switch(inputFilter.mControl.mDeviceType) {

        // Extract raw value from mouse events
        case DeviceType::MOUSE:
            switch(inputFilter.mControl.mControlType) {
                case ControlType::BUTTON:
                    switch(inputFilter.mAxisFilter) {
                        case AxisFilter::SIMPLE:
                            axisValue = inputEvent.button.down;
                        break;
                        case AxisFilter::X_POS:
                            axisValue = RangeMapperLinear{0.f, static_cast<double>(windowWidth), 0.f, 1.f}(inputEvent.button.x);
                        break;
                        case AxisFilter::Y_POS:
                            axisValue = RangeMapperLinear{0.f, static_cast<double>(windowHeight), 0.f, 1.f}(inputEvent.button.y);
                        break;

                        default:
                            assert(false && "Not a valid mouse event, this line should never be reached");
                        break;
                    }
                break;
                case ControlType::POINT:
                    switch(inputFilter.mAxisFilter) {
                        // Mouse location filter
                        case X_POS:
                            axisValue = RangeMapperLinear{0.f, static_cast<double>(windowWidth), 0.f, 1.f}(inputEvent.motion.x);
                        break;
                        case Y_POS:
                            axisValue = RangeMapperLinear{0.f, static_cast<double>(windowHeight), 0.f, 1.f}(inputEvent.motion.y);
                        break;
                        // Mouse movement filter
                        case X_CHANGE_POS:
                        case X_CHANGE_NEG:
                        {
                            const float sign { inputFilter.mAxisFilter&AxisFilterMask::SIGN? -1.f: 1.f };
                            axisValue = RangeMapperLinear{0.f, static_cast<double>(windowWidth), 0.f, 1.f}(sign * inputEvent.motion.xrel);
                        }
                        break;
                        case Y_CHANGE_POS:
                        case Y_CHANGE_NEG:
                        {
                            const float sign { inputFilter.mAxisFilter&AxisFilterMask::SIGN? -1.f: 1.f };
                            axisValue = RangeMapperLinear{0.f, static_cast<double>(windowHeight), 0.f, 1.f}(sign * inputEvent.motion.yrel);
                        }
                        break;

                        // Everything else is unsupported
                        default:
                            assert(false && "Not a valid mouse event, this line should never be reached");
                        break;
                    }
                break;
                // mouse wheel filters
                case ControlType::MOTION:
                    switch(inputFilter.mAxisFilter) {
                        case X_CHANGE_POS:
                        case X_CHANGE_NEG: {
                            const float sign { inputFilter.mAxisFilter&AxisFilterMask::SIGN? -1.f: 1.f };
                            axisValue = RangeMapperLinear{0.f, 1.f, 0.f, 1.f}(sign * inputEvent.wheel.x);
                        }
                        break;
                        case Y_CHANGE_POS:
                        case Y_CHANGE_NEG: {
                            const float sign { inputFilter.mAxisFilter&AxisFilterMask::SIGN? -1.f: 1.f };
                            axisValue = RangeMapperLinear{0.f, 1.f, 0.f, 1.f}(sign * inputEvent.wheel.y);
                        }
                        break;
                        default:
                            assert(false && "Invalid axis for mouse motion");
                        break;
                    }
                break;
                default:
                    assert(false && "Invalid control type for mouse");
                break;
            }
        break;

        // Extract raw value from keyboard events
        case DeviceType::KEYBOARD: {
           assert(inputFilter.mAxisFilter == AxisFilter::SIMPLE && "Invalid keyboard axis filter, keyboards only support `AxisFilter::SIMPLE`");
            axisValue = inputEvent.key.down;
        }
        break;

        // Extract raw value from controller events
        case DeviceType::CONTROLLER: {
            switch(inputFilter.mControl.mControlType) {
                // controller touchpad events
                case ControlType::POINT:
                    switch(inputFilter.mAxisFilter) {
                        case AxisFilter::SIMPLE:
                            axisValue = inputEvent.gtouchpad.pressure;
                        break;
                        case AxisFilter::X_POS:
                            axisValue = inputEvent.gtouchpad.x;
                        break;
                        case AxisFilter::Y_POS:
                            axisValue = inputEvent.gtouchpad.y;
                        break;
                        default:
                            assert(false && "This device control does not support this type of value");
                        break;
                    }
                break;

                case ControlType::BUTTON:
                    assert(inputFilter.mAxisFilter == AxisFilter::SIMPLE);
                    axisValue = inputEvent.button.down;
                break;

                case ControlType::AXIS:
                    switch(inputFilter.mAxisFilter) {
                        case X_POS:
                        case X_NEG: {
                            const float sign { inputFilter.mAxisFilter&AxisFilterMask::SIGN? -1.f: 1.f};
                            axisValue = RangeMapperLinear{0.f, 32768.f, 0.f, 1.f} (sign * inputEvent.jaxis.value);
                        }
                        break;
                        default:
                            assert(false && "Invalid filter type for this device and control");
                        break;
                    }
                break;

                case ControlType::RADIO:
                    switch(inputFilter.mAxisFilter) {
                        case AxisFilter::X_POS:
                            switch(inputEvent.jhat.value) {
                                case SDL_HAT_RIGHT:
                                case SDL_HAT_RIGHTDOWN:
                                case SDL_HAT_RIGHTUP:
                                    axisValue = 1.f;
                                break;
                                default:
                                    axisValue = 0.f;
                                break;
                            }
                        break;
                        case AxisFilter::Y_POS:
                            switch(inputEvent.jhat.value) {
                                case SDL_HAT_RIGHTUP:
                                case SDL_HAT_UP:
                                case SDL_HAT_LEFTUP:
                                    axisValue = 1.f;
                                break;
                                default:
                                    axisValue = 0.f;
                                break;
                            }
                        break;
                        case AxisFilter::X_NEG:
                            switch(inputEvent.jhat.value) {
                                case SDL_HAT_LEFTUP:
                                case SDL_HAT_LEFT:
                                case SDL_HAT_LEFTDOWN:
                                    axisValue = 1.f;
                                break;
                                default:
                                    axisValue = 0.f;
                                break;
                            }
                        break;
                        case AxisFilter::Y_NEG:
                            switch(inputEvent.jhat.value) {
                                case SDL_HAT_LEFTDOWN:
                                case SDL_HAT_DOWN:
                                case SDL_HAT_RIGHTDOWN:
                                    axisValue = 1.f;
                                break;
                                default:
                                    axisValue = 0.f;
                                break;
                            }
                        break;
                        default:
                            assert(false && "Unsupported axis for this device and control");
                        break;
                    }
                break;

                case ControlType::MOTION:
                    switch(inputFilter.mAxisFilter) {
                        case X_CHANGE_POS:
                        case X_CHANGE_NEG: {
                            const float sign { inputFilter.mAxisFilter&AxisFilterMask::SIGN? -1.f: 1.f };
                            axisValue = RangeMapperLinear{ 0.f, 128.f, 0.f, 1.f }(sign * inputEvent.jball.xrel);
                        }
                        break;
                        case Y_CHANGE_POS:
                        case Y_CHANGE_NEG: {
                            const float sign { inputFilter.mAxisFilter&AxisFilterMask::SIGN? -1.f: 1.f };
                            axisValue = RangeMapperLinear{ 0.f, 128.f, 0.f, 1.f }(sign * inputEvent.jball.yrel);
                        }
                        break;
                        default:
                            assert(false && "Invalid filter for this device and control");
                        break;
                    }
                break;
                default:
                    assert(false && "Invalid or unsupported control type for this device");
                break;
            }
        }
        break;

        // Extract raw value from touch events
        case DeviceType::TOUCH:
            switch(inputFilter.mControl.mControlType){
                case ControlType::POINT:
                    switch(inputFilter.mAxisFilter) {
                        case AxisFilter::SIMPLE:
                            axisValue = inputEvent.tfinger.pressure;
                        break;
                        case AxisFilter::X_POS:
                            axisValue = inputEvent.tfinger.x;
                        break;
                        case AxisFilter::Y_POS:
                            axisValue = inputEvent.tfinger.y;
                        break;
                        case AxisFilter::X_CHANGE_POS:
                        case AxisFilter::X_CHANGE_NEG: {
                            const float sign { inputFilter.mAxisFilter&AxisFilterMask::SIGN? -1.f: 1.f };
                            axisValue = RangeMapperLinear{0.f, 1.f, 0.f, 1.f}(sign * inputEvent.tfinger.dx);
                        }
                        break;
                        case AxisFilter::Y_CHANGE_POS:
                        case AxisFilter::Y_CHANGE_NEG: {
                            const float sign { inputFilter.mAxisFilter&AxisFilterMask::SIGN? -1.f: 1.f };
                            axisValue = RangeMapperLinear{0.f, 1.f, 0.f, 1.f}(sign * inputEvent.tfinger.dy);
                        }
                        break;
                        default:
                            assert(false && "invalid axis filter type for touch");
                        break;
                    }
                break;
                default: 
                    assert(false && "unsupported touch control type");
                break;
            }
        break;
        default:
            assert(false && "Unsupported device type");
        break;
    }
    return axisValue;
}

std::vector<AxisFilter> deriveAxisFilters(InputAttributesType attributes) {
    std::vector<AxisFilter> result {};

    if(attributes&HAS_BUTTON_VALUE) result.push_back(AxisFilter::SIMPLE);

    for(AxisFilterType i{AxisFilter::X_POS}; i <= (attributes&InputAttributes::N_AXES); ++i) {
        result.push_back(static_cast<AxisFilter>(i));

        if(attributes&HAS_NEGATIVE) {
            result.push_back(static_cast<AxisFilter>(i|AxisFilterMask::SIGN));
        }

        if(attributes&HAS_CHANGE_VALUE) {
            result.push_back(static_cast<AxisFilter>(i|AxisFilterMask::CHANGE));
            result.push_back(static_cast<AxisFilter>(i|AxisFilterMask::SIGN|AxisFilterMask::CHANGE));
        }
    }

    return result;
}

InputSourceDescription getInputIdentity(const SDL_Event& inputEvent) {
    InputSourceDescription inputIdentity {};
    switch(inputEvent.type) {
        /**
         * Mouse events
         */
        case SDL_EVENT_MOUSE_BUTTON_UP:
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            inputIdentity.mDeviceType = DeviceType::MOUSE;
            inputIdentity.mControlType = ControlType::BUTTON;
            inputIdentity.mDevice = inputEvent.button.which;
            inputIdentity.mControl = inputEvent.button.button;
        break;
        case SDL_EVENT_MOUSE_MOTION:
            inputIdentity.mDeviceType = DeviceType::MOUSE;
            inputIdentity.mControlType = ControlType::POINT;
            inputIdentity.mDevice = inputEvent.motion.which;
        break;
        case SDL_EVENT_MOUSE_WHEEL:
            inputIdentity.mDeviceType = DeviceType::MOUSE;
            inputIdentity.mControlType = ControlType::MOTION;
            inputIdentity.mDevice = inputEvent.wheel.which;
        break;

        /** 
         * Keyboard events
         */
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            inputIdentity.mDeviceType = DeviceType::KEYBOARD;
            inputIdentity.mControlType = ControlType::BUTTON;
            inputIdentity.mControl = inputEvent.key.key;
        break;
        case SDL_EVENT_FINGER_UP:
        case SDL_EVENT_FINGER_DOWN:
        case SDL_EVENT_FINGER_MOTION:
            inputIdentity.mDeviceType = DeviceType::TOUCH;
            inputIdentity.mControlType = ControlType::POINT;
            inputIdentity.mControl = inputEvent.tfinger.touchID;
        break;

        /**
         * Controller-like events
         */
        case SDL_EVENT_JOYSTICK_AXIS_MOTION:
            inputIdentity.mDeviceType = DeviceType::CONTROLLER;
            inputIdentity.mControlType = ControlType::AXIS;
            inputIdentity.mDevice = inputEvent.jaxis.which;
            inputIdentity.mControl = inputEvent.jaxis.axis;
        break;
        case SDL_EVENT_JOYSTICK_HAT_MOTION:
            inputIdentity.mDeviceType = DeviceType::CONTROLLER;
            inputIdentity.mControlType = ControlType::RADIO;
            inputIdentity.mDevice = inputEvent.jhat.which;
            inputIdentity.mControl = inputEvent.jhat.hat;
        break;
        case SDL_EVENT_JOYSTICK_BALL_MOTION:
            inputIdentity.mDeviceType = DeviceType::CONTROLLER;
            inputIdentity.mControlType = ControlType::MOTION;
            inputIdentity.mControl = inputEvent.jball.ball;
            inputIdentity.mDevice = inputEvent.jball.which;
        break;
        case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
        case SDL_EVENT_JOYSTICK_BUTTON_UP:
            inputIdentity.mDeviceType = DeviceType::CONTROLLER;
            inputIdentity.mControlType = ControlType::BUTTON;
            inputIdentity.mDevice = inputEvent.jbutton.which;
            inputIdentity.mControl = inputEvent.jbutton.button;
        break;
        case SDL_EVENT_GAMEPAD_TOUCHPAD_DOWN:
        case SDL_EVENT_GAMEPAD_TOUCHPAD_UP:
        case SDL_EVENT_GAMEPAD_TOUCHPAD_MOTION:
            inputIdentity.mDeviceType = DeviceType::CONTROLLER;
            inputIdentity.mControlType = ControlType::POINT;
            inputIdentity.mControl = inputEvent.gtouchpad.touchpad;
            inputIdentity.mDevice = inputEvent.gtouchpad.which;
        break;
        default:
            std::cout << "Unsupported input event: \ttype: " << inputEvent.type << "\n";
            // assert (false && "This event is unsupported");
        break;
    }
    if(inputIdentity.mDeviceType != DeviceType::NA) {
        inputIdentity.mAttributes = kInputSourceTypeAttributes.at(
            {inputIdentity.mDeviceType, inputIdentity.mControlType}
        );
    }
    return inputIdentity;
}

ActionContext& InputManager::operator[] (const std::string& actionContext) {
    auto actionContextPair { mActionContexts.find(actionContext) };
    assert(actionContextPair != mActionContexts.end() && "No action context with this name has been registered with the input manager");
    return actionContextPair->second.first;
}

void InputManager::queueInput(const SDL_Event& inputEvent) {
    // variable storing the (internal) identity of the
    // control that created this event
    InputSourceDescription inputIdentity { getInputIdentity(inputEvent) };
    // assert(inputIdentity && "This event is not supported");
    if(!inputIdentity) return;

    // Update the raw values of any input filters that are in use and
    // have changed this frame
    std::vector<InputFilter> updatedInputFilters {};
    for(AxisFilter axisFilter: deriveAxisFilters(inputIdentity.mAttributes)) {
        InputFilter inputFilter {inputIdentity, axisFilter};
        if(mRawInputState.find(inputFilter) != mRawInputState.end()) {
            double newValue { getRawValue(inputFilter, inputEvent) };
            if(mRawInputState[inputFilter] != newValue || inputFilter.mAxisFilter&AxisFilterMask::CHANGE){
                mRawInputState[inputFilter] = newValue;
                updatedInputFilters.push_back(inputFilter);
            }
        }
    }

    std::unordered_map<InputCombo, UnmappedInputValue> finalComboStates {};
    //  Apply filter changes to any mapped combination, adding input combo events
    // to the queue if the combo condition is fulfilled
    for(const InputFilter& filter: updatedInputFilters) {
        for(const InputCombo& combo: mInputFilterToCombos[filter]) {
            assert(combo && "This combo does not have a main control, making it invalid");
            const bool modifier1Held { !combo.mModifier1 || mRawInputState[combo.mModifier1] >= mModifierThreshold };
            const bool modifier2Held { !combo.mModifier2 || mRawInputState[combo.mModifier2] >= mModifierThreshold };

            // Prepare new combo state parameters
            const UnmappedInputValue previousComboState { mInputComboStates[combo] };
            UnmappedInputValue newComboState {};
            if(modifier1Held && modifier2Held) {
                const double upperBound { 1.f };
                const double lowerBound { combo.mDeadzone };
                const double threshold { 
                    (combo.mTrigger == InputCombo::Trigger::ON_CHANGE || combo.mTrigger == InputCombo::Trigger::ON_BUTTON_CHANGE)?
                    0.f:
                    combo.mThreshold
                };

                newComboState.mAxisValue = RangeMapperLinear{lowerBound, upperBound, 0.f, 1.f}(mRawInputState[combo.mMainControl]);
                if(
                    combo.mMainControl.mAxisFilter != AxisFilter::SIMPLE
                    && (
                        combo.mTrigger == InputCombo::Trigger::ON_BUTTON_CHANGE
                        || combo.mTrigger == InputCombo::Trigger::ON_BUTTON_PRESS
                        || combo.mTrigger == InputCombo::Trigger::ON_BUTTON_RELEASE
                    )
                ) {
                    InputFilter buttonControl { combo.mMainControl };
                    buttonControl.mAxisFilter = AxisFilter::SIMPLE;
                    newComboState.mButtonValue = RangeMapperLinear{lowerBound, upperBound, 0.f, 1.f}(mRawInputState[buttonControl]);
                    newComboState.mActivated = newComboState.mButtonValue >= threshold;
                } else {
                    newComboState.mButtonValue = 0.f;
                    newComboState.mActivated = newComboState.mAxisValue >= threshold;
                }
            } else {
                newComboState.mAxisValue= 0.f;
                newComboState.mButtonValue = 0.f;
                newComboState.mActivated = false;
            }
            newComboState.mTimestamp = inputEvent.common.timestamp;

            //  Add input to queue to be consumed by subscribed action contexts when value ...
            if(
                ( // ... just exceeded threshold
                    (combo.mTrigger == InputCombo::Trigger::ON_PRESS || combo.mTrigger == InputCombo::Trigger::ON_BUTTON_PRESS)
                    && newComboState.mActivated 
                    && !previousComboState.mActivated

                ) || ( // ... just dropped below threshold
                    (combo.mTrigger == InputCombo::Trigger::ON_RELEASE || combo.mTrigger == InputCombo::Trigger::ON_BUTTON_RELEASE)
                    && !newComboState.mActivated
                    && previousComboState.mActivated

                ) || ( // ... change just occurred
                    (combo.mTrigger == InputCombo::Trigger::ON_CHANGE || combo.mTrigger == InputCombo::Trigger::ON_BUTTON_CHANGE)
                    && (
                        // ... and input type is change
                        (combo.mMainControl.mAxisFilter&AxisFilterMask::CHANGE && newComboState.mActivated)
                        // ... or a state change occurred (and therefore the new state should 
                        // be forwarded)
                        || (newComboState.mAxisValue != previousComboState.mAxisValue)
                    )
                )
            ) {
                mUnmappedInputs.push(std::pair(combo, newComboState));
            } 

            // Update presently stored combo state
            finalComboStates[combo] = newComboState;
        }
    }

    for(const auto& comboValuePair: finalComboStates) {
        mInputComboStates[comboValuePair.first] = comboValuePair.second;
    }
}

void InputManager::loadInputConfiguration(const nlohmann::json& inputConfiguration) {
    // clear old bindings
    std::vector<std::string> oldActionContexts {};
    for(auto actionContext: mActionContexts) {
        oldActionContexts.push_back(actionContext.first);
    }
    for(const std::string& context: oldActionContexts) {
        unregisterActionContext(context);
    }

    for(const std::string& actionContextName: inputConfiguration.at("action_contexts").get<std::vector<std::string>>()) {
        registerActionContext(actionContextName);
    }
    for(const nlohmann::json& actionDefinition: inputConfiguration.at("actions").get<std::vector<nlohmann::json>>()) {
        registerAction(actionDefinition);
    }
    for(const nlohmann::json& inputBinding: inputConfiguration.at("input_binds").get<std::vector<nlohmann::json>>()) {
        registerInputBind(inputBinding);
    }
}

void InputManager::registerInputBind(const nlohmann::json& inputBindingParameters) {
    mActionContexts.at(inputBindingParameters.at("context").get<std::string>()).first.registerInputBind(inputBindingParameters);
}

void InputManager::registerAction(const nlohmann::json& actionParameters) {
    mActionContexts.at(actionParameters.at("context").get<std::string>()).first.registerAction(actionParameters);
}

void InputManager::registerActionContext(const std::string& name, ActionContextPriority priority) {
    assert(mActionContexts.find(name) == mActionContexts.end()
        && "An action context with this name has already been registered");
    mActionContexts.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(name),
        std::forward_as_tuple(
            std::pair<ActionContext, ActionContextPriority>(
                std::piecewise_construct,
                std::forward_as_tuple(*this, name),
                std::forward_as_tuple(priority)
            )
        )
    );
}

void InputManager::unregisterActionContext(const std::string& actionContextName) {
    assert(mActionContexts.find(actionContextName) != mActionContexts.end()
        && "No action context with this name has been registered before");

    {
        ActionContext& actionContext {
            mActionContexts.at(actionContextName).first
        };
        actionContext.unregisterInputBinds();
    }

    // Erase the last trace of this action context
    mActionContexts.erase(actionContextName);
}

std::vector<std::pair<ActionDefinition, ActionData>> InputManager::getTriggeredActions(uint32_t targetTimeMillis) {
    if(mUnmappedInputs.empty()) {
        return {};
    }

    // Send each pending input event to all action contexts 
    // that are listenining for it
    std::set<ContextName> updatedActionContexts {};
    while(
        !mUnmappedInputs.empty() 
        && mUnmappedInputs.front().second.mTimestamp <= targetTimeMillis
    ) {
        const std::pair<InputCombo, UnmappedInputValue>& inputPair { mUnmappedInputs.front() };
        bool allowPropagate { true };

        // each set of associated actions, in descending order of priority
        for(const std::set<ContextName>& actionSet: mInputComboToActionContexts[inputPair.first]) {
            // each action in this priority level
            for(const ContextName& actionContextName: actionSet) {
                ActionContext& actionContext{ mActionContexts.at(actionContextName).first };
                if(actionContext.enabled()) {
                    actionContext.mapToAction(inputPair.second, inputPair.first);
                    updatedActionContexts.insert(actionContextName);
                    allowPropagate = actionContext.propagateAllowed();
                }
                if(!allowPropagate) break;
            }
            if(!allowPropagate) break;
        }

        // Remove this input from the queue
        mUnmappedInputs.pop();
    }


    // Let each updated context dispatch actions to their subscribed
    // action handlers
    std::vector<std::pair<ActionDefinition, ActionData>> triggeredActions {};
    for(const ContextName& name: updatedActionContexts) {
        std::vector<std::pair<ActionDefinition, ActionData>> contextActions {mActionContexts.at(name).first.getTriggeredActions()};
        triggeredActions.insert(triggeredActions.end(), contextActions.begin(), contextActions.end());
    }
    return triggeredActions;
}

void InputManager::registerInputCombo(const std::string& actionContext, const InputCombo& inputCombo) {
    assert(mActionContexts.find(actionContext) != mActionContexts.end() && "No action context with this name has been registered");
    const ActionContextPriority& priority { mActionContexts.at(actionContext).second };

    // Add associated input filters to records that use them
    std::array<const InputFilter, 3> inputComboFilters {{
        {inputCombo.mMainControl},
        {inputCombo.mModifier1},
        {inputCombo.mModifier2}
    }};

    if(
        (inputCombo.mTrigger == InputCombo::Trigger::ON_BUTTON_CHANGE
        || inputCombo.mTrigger == InputCombo::Trigger::ON_BUTTON_PRESS
        || inputCombo.mTrigger == InputCombo::Trigger::ON_BUTTON_RELEASE)
    ) {
        if(inputCombo.mMainControl.mAxisFilter != AxisFilter::SIMPLE) {
            assert((inputCombo.mMainControl.mControl.mAttributes&InputAttributes::HAS_BUTTON_VALUE) && "The main control for this input combo \
            does not have a button value attribute");
            InputFilter buttonControl { inputCombo.mMainControl };
            buttonControl.mAxisFilter = AxisFilter::SIMPLE;
            mRawInputState.try_emplace(
                buttonControl,
                0.f
            );
            mInputFilterToCombos[buttonControl].emplace(inputCombo);
        } else {
            assert(false && "Only input combos whose main controls are pointer types with optional button values\
                may have button-related triggers.  All other combos may only use default axis triggers");
        }
    }

    for(const InputFilter& inputFilter: inputComboFilters) {
        if(inputFilter) {
            mRawInputState.try_emplace(
                inputFilter,
                0.f
            );
            // TODO: only one of multiple combos gets mapped to a filter. We need all of them.
            mInputFilterToCombos[inputFilter].emplace(inputCombo);
        }
    }

    // Add input combo to records that require it
    mInputComboStates.try_emplace(inputCombo, UnmappedInputValue{});
    mInputComboToActionContexts[inputCombo][ActionContextPriority::TOTAL - 1 - priority].insert(actionContext);
}

void InputManager::unregisterInputCombo(const std::string& actionContext, const InputCombo& inputCombo) {
    assert(mActionContexts.find(actionContext) != mActionContexts.end() && "No action context with this name has been registered");
    assert(mInputComboStates.find(inputCombo) != mInputComboStates.end() 
        && "This input combination has not been registered, or has already\
        been unregistered."
    );

    ActionContextPriority contextPriority {mActionContexts.at(actionContext).second};
    mInputComboToActionContexts[inputCombo][ActionContextPriority::TOTAL - 1 - contextPriority].erase(
        actionContext
    );

    // If we find even one other place where this combo is used, the combo itself and all
    // of its input filters should be kept
    bool keepCombo { false };
    for(const auto& eachSet: mInputComboToActionContexts[inputCombo]) {
        if(!eachSet.empty()) {
            keepCombo=true;
            break;
        }
    }
    if(keepCombo) return;

    // The combo is to be removed; see if any of its 
    // associated input filters (corresponding to individual
    // controls) are in use
    std::vector<InputFilter> inputComboFilters {{
        {inputCombo.mMainControl},
        {inputCombo.mModifier1},
        {inputCombo.mModifier2},
    }};
    // Array to track whether any of the filters that appear under this combo are used
    // in other places
    std::vector<bool> keepFilter {{false, false, false}};
    // Add the button value of this (likely pointer) control to the list of input values we are interested 
    // in, if it is implicitly used as part of our trigger condition
    if(
        (
            inputCombo.mTrigger == InputCombo::Trigger::ON_BUTTON_CHANGE
            || inputCombo.mTrigger == InputCombo::Trigger::ON_BUTTON_PRESS
            || inputCombo.mTrigger == InputCombo::Trigger::ON_BUTTON_RELEASE
        ) && inputCombo.mMainControl.mAxisFilter != AxisFilter::SIMPLE
    ) {
        assert((inputCombo.mMainControl.mControl.mAttributes&InputAttributes::HAS_BUTTON_VALUE) && "The main control for this input combo \
        does not have a button value attribute");
        InputFilter buttonControl { inputCombo.mMainControl };
        buttonControl.mAxisFilter = AxisFilter::SIMPLE;
        inputComboFilters.push_back(buttonControl);
        keepFilter.push_back(false);
    }


    for(std::size_t i{0}; i < inputComboFilters.size(); ++i) {
        //  Empty filter, and therefore does not require
        // processing
        if(!inputComboFilters[i]) {
            keepFilter[i] = true;
            continue;
        }

        //  A size greater than 1 indicates that there's at least
        // one other combo (besides the one being deleted) using
        // this filter
        const std::set<InputCombo>& filterCombos { mInputFilterToCombos[inputComboFilters[i]] };
        if(filterCombos.size() > 1) {
            keepFilter[i] = true;
        }
    }

    //  Remove the segment keeping track of the latest processed 
    // value for this combo
    mInputComboStates.erase(inputCombo);

    //  Remove all events in the input queue that correspond
    // to this input combination
    std::queue<std::pair<InputCombo, UnmappedInputValue>> newInputQueue {};
    while(!mUnmappedInputs.empty()) {
        const auto& currentInput { mUnmappedInputs.back() };
        if(currentInput.first != inputCombo) {
            newInputQueue.push(currentInput);
        }
        mUnmappedInputs.pop();
    }
    std::swap(mUnmappedInputs, newInputQueue);

    // Remove this combo from filters that map to 
    // it, as well as the filter itself if there
    // are no more combos that require it.
    for(std::size_t i{0}; i < keepFilter.size(); ++i) {
        auto& filter { inputComboFilters[i] };
        bool keep { keepFilter[i] };

        mInputFilterToCombos[filter].erase(inputCombo);

        if(!keep) {
            mInputFilterToCombos.erase(filter);
            mRawInputState.erase(filter);
        }
    }
}

void InputManager::unregisterInputCombos(const std::string& actionContext){
    assert(mActionContexts.find(actionContext) != mActionContexts.end() && "No action context with this name has been registered");

    //  Find all combos mapped to this action context
    ActionContextPriority priority { mActionContexts.at(actionContext).second };
    std::vector<InputCombo> combosToUnregister {};
    for(const auto& inputMapping: mInputComboToActionContexts) {
        if(inputMapping.second[priority].find(actionContext) != inputMapping.second[priority].end()) {
            combosToUnregister.push_back(inputMapping.first);
        }
    }

    //  Remove the mapping between each found combo and the context
    for(const InputCombo& combo: combosToUnregister) {
        unregisterInputCombo(actionContext, combo);
    }
}

void InputManager::unregisterInputCombos() {
    for(auto& actionPair: mActionContexts) {
        unregisterInputCombos(actionPair.first);
    }
}

/** @ingroup ToyMakerInputSystem ToyMakerSerialization */
void ToyMaker::to_json(nlohmann::json& json, const ToyMaker::InputAttributesType& inputAttributes) {
    json = {
        {"n_axes", (inputAttributes&InputAttributes::N_AXES)},
        {"has_negative", (inputAttributes&InputAttributes::HAS_NEGATIVE) > 0},
        {"has_change_value", (inputAttributes&InputAttributes::HAS_CHANGE_VALUE) > 0},
        {"has_button_value", (inputAttributes&InputAttributes::HAS_BUTTON_VALUE) > 0},
        {"has_state_value", (inputAttributes&InputAttributes::HAS_STATE_VALUE) > 0},
        {"state_is_location", (inputAttributes&InputAttributes::STATE_IS_LOCATION) > 0},
    };
}

/** @ingroup ToyMakerInputSystem ToyMakerSerialization */
void ToyMaker::from_json(const nlohmann::json& json, ToyMaker::InputAttributesType& inputAttributes) {
    inputAttributes = (
        json.at("n_axes").get<uint8_t>()
        | (json.at("has_negative").get<bool>()?
            InputAttributes::HAS_NEGATIVE: 0)
        | (json.at("has_change_value").get<bool>()?
            InputAttributes::HAS_CHANGE_VALUE: 0)
        | (json.at("has_button_value").get<bool>()?
            InputAttributes::HAS_BUTTON_VALUE: 0)
        | (json.at("has_state_value").get<bool>()?
            InputAttributes::HAS_STATE_VALUE: 0)
        | (json.at("state_is_location").get<bool>()?
            InputAttributes::STATE_IS_LOCATION: 0)
    );
}

/** @ingroup ToyMakerInputSystem ToyMakerSerialization */
void ToyMaker::to_json(nlohmann::json& json, const ToyMaker::InputSourceDescription& inputSourceDescription) {
    json = {
        {"device_type", inputSourceDescription.mDeviceType},
        {"control_type", inputSourceDescription.mControlType},
        {"device", inputSourceDescription.mDevice},
        {"control", inputSourceDescription.mControl},
    };
}

/** @ingroup ToyMakerInputSystem ToyMakerSerialization */
void ToyMaker::from_json(const nlohmann::json& json, ToyMaker::InputSourceDescription& inputSourceDescription) {
    json.at("device_type").get_to(inputSourceDescription.mDeviceType);
    json.at("device").get_to(inputSourceDescription.mDevice);
    json.at("control_type").get_to(inputSourceDescription.mControlType);
    json.at("control").get_to(inputSourceDescription.mControl);
    const InputSourceType inputSourceType { inputSourceDescription.mDeviceType, inputSourceDescription.mControlType };
    inputSourceDescription.mAttributes = kInputSourceTypeAttributes.at(inputSourceType);
}

/** @ingroup ToyMakerInputSystem ToyMakerSerialization */
void ToyMaker::to_json(nlohmann::json& json, const ToyMaker::InputFilter& inputFilter) {
    json = {
        {"input_source", inputFilter.mControl},
        {"filter", inputFilter.mAxisFilter},
    };
}

void ToyMaker::from_json(const nlohmann::json& json, ToyMaker::InputFilter& inputFilter) {
    json.at("input_source").get_to(inputFilter.mControl);
    json.at("filter").get_to(inputFilter.mAxisFilter);
}

void ToyMaker::to_json(nlohmann::json& json, const ToyMaker::InputCombo& inputCombo) {
    json = {
        {"trigger", inputCombo.mTrigger},
        {"main_control", inputCombo.mMainControl},
        {"modifier_1", inputCombo.mModifier1},
        {"modifier_2", inputCombo.mModifier2},
        {"deadzone", inputCombo.mDeadzone},
        {"threshold", inputCombo.mThreshold},
    };
}

void ToyMaker::from_json(const nlohmann::json& json, ToyMaker::InputCombo& inputCombo) {
    json.at("main_control").get_to(inputCombo.mMainControl);
    json.at("modifier_1").get_to(inputCombo.mModifier1);
    json.at("modifier_2").get_to(inputCombo.mModifier2);
    json.at("trigger").get_to(inputCombo.mTrigger);
    json.at("deadzone").get_to(inputCombo.mDeadzone);
    json.at("threshold").get_to(inputCombo.mThreshold);
}

void ToyMaker::to_json(nlohmann::json& json, const ToyMaker::ActionDefinition& actionDefinition) {
    json = {
        {"name", actionDefinition.mName},
        {"attributes", actionDefinition.mAttributes},
        {"value_type", actionDefinition.mValueType},
    };
}

void ToyMaker::from_json(const nlohmann::json& json, ToyMaker::ActionDefinition& actionDefinition) {
    json.at("name").get_to(actionDefinition.mName);
    json.at("attributes").get_to(actionDefinition.mAttributes);
    json.at("value_type").get_to(actionDefinition.mValueType);
}

/**
 * @ingroup ToyMakerInputSystem
 * @file input_system/input_system.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief 
 * @version 0.3.2
 * @date 2025-09-01
 * 
 * The input system, in a nutshell, breaks up all inputs from every source into their constituent single axis values.  Each such value is then remapped to one axis of one action.
 * 
 * @see InputManager
 * @see input_data.hpp
 * 
 */

#ifndef FOOLSENGINE_INPUTSYSTEM_H
#define FOOLSENGINE_INPUTSYSTEM_H

#include <string>
#include <map>
#include <queue>
#include <memory>

#include <nlohmann/json.hpp>
#include <SDL2/SDL.h>

#include "input_data.hpp"

namespace ToyMaker {
    class ActionContext;
    class ActionDispatch;
    class InputManager;

    /**
     * @ingroup ToyMakerInputSystem
     * 
     * @brief The class that acts as the main interface between the rest of the project and the input system.
     * 
     * Processes raw SDL input events into unmapped inputs, and later reports bind value changes to an ActionContext for conversion into corresponding action events.
     */
    class InputManager {
    public:
        /**
         * @brief The priority associated with an action context, which as yet has no real bearing on their evaluation.
         * 
         */
        enum ActionContextPriority: uint8_t {
            VERY_LOW=0,
            LOW=1,
            DEFAULT=2,
            HIGH=3,
            VERY_HIGH=4,
            TOTAL=5,
        };

        /** 
         * @brief Maps an event to its internal representation, if one is available.
         * 
         * Called by the main thread at the start of every event loop.  The kind of events supported are described in input_manager.cpp
         * 
         */
        void queueInput(const SDL_Event& inputEvent);

        /**
         * @brief The full description of an input binding, to be tracked and signaled by the InputManager.
         * 
         * Such an input binding must name:
         * 
         * - One InputCombo (some combination of axis values of different controls from attached input devices)
         * 
         * - One ActionDefinition (the target action the input will (partly) populate)
         * 
         * - One AxisFilter (axis of the action to which the source input combo is mapped)
         * 
         * Example:
         * 
         * ```jsonc
         * {
         *     "action": "Rotate",
         *     "context": "Camera",
         *     "input_combo": {
         *         "deadzone": 0.0,
         *         "main_control": {
         *             "filter": "+dx",
         *             "input_source": {
         *                 "control": 0,
         *                 "control_type": "point",
         *                 "device": 0,
         *                 "device_type": "mouse"
         *             }
         *         },
         *         "modifier_1": {
         *             "filter": "simple",
         *             "input_source": {
         *                 "control": 0,
         *                 "control_type": "na",
         *                 "device": 0,
         *                 "device_type": "na"
         *             }
         *         },
         *         "modifier_2": {
         *             "filter": "simple",
         *             "input_source": {
         *                 "control": 0,
         *                 "control_type": "na",
         *                 "device": 0,
         *                 "device_type": "na"
         *             }
         *         },
         *         "threshold": 0.5,
         *         "trigger": "on-change"
         *     },
         *     "target_axis": "+x"
         * }
         * ```
         * 
         * Modifiers 1 and 2 are non-sources, and are always considered active.  The +dx of the main control, or motion in the +x axis of the mouse, is mapped to the +x axis of the action Camera Rotate.
         * 
         * @see InputCombo
         * @see ActionDefinition
         * @see ActionData
         * 
         * @param inputBindingParameters A JSON description of the relationship between an InputCombo and the action it maps to.
         */
        void registerInputBind(const nlohmann::json& inputBindingParameters);

        /**
         * @brief Registers an action definition against a defined action context.
         * 
         * The action contains also a description of its type, axes, and other things.  See for example:
         * 
         * ```jsonc
         * {
         *     "attributes": {
         *         "has_button_value": false,
         *         "has_change_value": true,
         *         "has_negative": false,
         *         "has_state_value": false,
         *         "n_axes": 2,
         *         "state_is_location": false
         *     },
         *     "context": "Camera",
         *     "name": "Rotate",
         *     "value_type": "change"
         * }
         * ```
         * 
         * @see ActionDefinition
         * @see ActionData
         * 
         * @param actionParameters The JSON representation of an ActionDefinition
         */
        void registerAction(const nlohmann::json& actionParameters);

        /**
         * @brief Registers a new action context with a given name.
         * 
         * Optionally takes an argument indicating the context's priority.  ActionContextPriority determines when InputCombos for this context are evaluated relative to other contexts'.
         * 
         * This matters, for example, when a higher priority context resolving an action should prevent further action contexts from being evaluated.
         * 
         * There is no defined precedence for contexts within the same priority level.
         * 
         */
        void registerActionContext(const ::std::string& name, ActionContextPriority priority=DEFAULT);

        /**
         * @brief Removes the action context associated with this name.
         *
         */
        void unregisterActionContext(const ::std::string& name);

        /**
         * @brief Loads a full input configuration based on its JSON description.
         *
         * @see registerActionContext()
         * @see registerAction()
         * @see registerInputBind()
         * 
         * @param inputConfiguration A JSON description of an input configuration.
         */
        void loadInputConfiguration(const nlohmann::json& inputConfiguration);

        /**
         * @brief Retrieves an action context based on its name.
         * 
         * @param actionContext The name of an ActionContext.
         * @return ActionContext& A reference to the ActionContext.
         */
        ActionContext& operator[] (const ::std::string& actionContext);

        /**
         * @brief Dispatches all mapped inputs received before the target time to action contexts that can handle them.
         *
         */
        ::std::vector<::std::pair<ActionDefinition, ActionData>> getTriggeredActions(uint32_t targetTimeMillis);

    private:

        /**
         * @brief Get the value associated with a single InputFilter, a number between 0 and 1.
         * 
         * Each InputFilter value, taken along with other InputFilter values, makes up the value for an InputCombo used in a binding.
         * 
         * @param inputFilter The InputFilter this value corresponds to.
         * @param inputEvent The event used as the basis for the value for the InputFilter.
         * 
         * @return double The computed value for the InputFilter.
         */
        double getRawValue(const InputFilter& inputFilter, const SDL_Event& inputEvent) const;

        /**
         * @brief Register a listener for a certain input combination on behalf of `actionContext`.
         */
        void registerInputCombo(const ::std::string& actionContext, const InputCombo& inputCombo);

        /**
         * @brief Remove entry for a specific input within an action context.
         */
        void unregisterInputCombo(const ::std::string& actionContext, const InputCombo& inputCombo);

        /**
         *  Remove entry for all inputs bound within an action context.
         */
        void unregisterInputCombos(const ::std::string& actionContext);

        /**
         *  Remove all input binds.
         */
        void unregisterInputCombos();

        /**  
         * @brief All action context names -> contexts.
         */
        ::std::unordered_map<ContextName, ::std::pair<ActionContext, ActionContextPriority>> mActionContexts {};

        /**
         * @brief The current, raw state of the control+axis associated with each input filter, each between 0.f and 1.f (button controls get 0.f and 1.f when unpressed and pressed respectively)
         * 
         */
        ::std::unordered_map<InputFilter, double> mRawInputState {};

        /**
         * @brief All active input combinations associated with a given input.
         * 
         */
        ::std::unordered_map<InputFilter, ::std::set<InputCombo>> mInputFilterToCombos {};

        /** 
         * @brief All action contexts associated with a given input combination, organized by priority.
        */
        ::std::unordered_map<InputCombo, ::std::array<
            ::std::set<ContextName>, ActionContextPriority::TOTAL
        >> mInputComboToActionContexts {};

        /**
         * @brief Input combination values, up to the most recently fired input trigger.
         * 
         * If all modifiers for an InputCombo are active, then the main control alone determines the value of the combo as a whole.  These are computed with InputFilter values.
         * 
         * @see mRawInputState
         */
        ::std::unordered_map<InputCombo, UnmappedInputValue> mInputComboStates {};

        /**
         * @brief Queue of input state changes, to be consumed by whichever action contexts require them.
         * 
         */
        ::std::queue<::std::pair<InputCombo, UnmappedInputValue>> mUnmappedInputs {};

        /**
         * @brief Button threshold for axes or buttons that map to modifiers, beyond which those modifiers are considered active (like buttons).
         * 
         * 
         */
        float mModifierThreshold { .7f };

    friend class ActionContext;
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief Class interface for systems that wish to be notified when action events occur in an action context.
     * 
     * ## Usage
     * ```c++
     * 
     * class BaseSimObjectAspect :
     * // ... other base classes ...,
     * public IActionHandler  // NOTE: Derive this class
     * {
     *     // NOTE: Override handleAction
     *     bool handleAction(const ActionData& actionData, const ActionDefinition& actionDefinition) override;
     * 
     *     //... Rest of class definition
     * };
     * 
     * ```
     * 
     */
    class IActionHandler {
    public:
    private:
        /**
         * @brief The action handling function in any class that implements 
         * this interface. 
         * 
         * @return bool Indicates whether the input that triggered was handled by this handler.
         * 
         */
        virtual bool handleAction(const ActionData& actionData, const ActionDefinition& actionDefinition) {
            (void)actionData; // prevent unused parameter warnings
            (void)actionDefinition; // prevent unused parameter warnings
            return false;
        };

    friend class ActionDispatch;
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief A container for actions that make sense within a given context.
     * 
     * Different contexts might have different requirements, even with the same set of inputs.  For example, it might make sense to have a "slash" action in the game world, but in the context of a game menu, slash cannot have meaning.
     * 
     * Action Contexts turn those requirements into a container object, where the semantics of an input event are indicated by the action and context, and are one degree removed from the inputs themselves.
     * 
     * This can be useful, for example, when a game requires input for character movement across multiple platforms.  For a console, or when a controller is present, it would make sense to query the value of the left analog stick.  However, no such control exists on a keyboard, which is the most common input device for a PC.
     * 
     * Action Contexts, among other things, allow it so that multiple mappings to the same type of input are possible, and the handlers of the input don't have to reason about differences in platforms.  In the example above, that mapping might look something like this:
     * 
     * ```
     *   W    ______
     * A|S|D        \___.--> Character : Move (2 Axes) --> (Move handlers)
     * <keyboard>    /
     *              /
     *             /
     * (( L ))----/
     * <controller>
     * ```
     * 
     * This allows developers to reason about input somewhat uniformly during game development.  The input mappings themselves are a matter of wiring inputs to high level actions during configuration, separate from game logic.
     * 
     * In this case, all a game programmer need know is that they want signed non-location state input on 2 axes, and that they want it for "Move".
     * 
     */
    class ActionContext {
    public:
        /**
         * @brief Construct a new action context.
         * 
         * @param inputManager The input manager in charge of this context.
         * @param name The name of the context.
         */
        ActionContext(InputManager& inputManager, const ContextName& name): mInputManager{inputManager}, mName {name} {}

        /**
         * @brief Construct a new action context.
         * 
         * @param inputManager The input manager in charge of this context.
         * @param name The name of the context.
         */
        ActionContext(InputManager&& inputManager, const ContextName& name) = delete;

        /**
         * @brief Returns the result of applying an unmapped input combo value to its target action-axis.
         * 
         * @param actionDefinition Action definition of target action (but usually just a context-action name string pair).
         * @param actionData The previous value of the data associated with the action.
         * @param targetAxis The axis of the action affected.
         * @param inputValue The value associated with the input combo being applied.
         * @return ActionData The action data after the input has been applied.
         * 
         * @see InputCombo
         */
        static ActionData ApplyInput(const ActionDefinition& actionDefinition, const ActionData& actionData, const AxisFilter targetAxis, const UnmappedInputValue& inputValue);

        /**
         * @brief Returns a list of triggered actions following input mapping in this context.
         * 
         * @return ::std::vector<::std::pair<ActionDefinition, ActionData>> List of triggered actions after inputs are applied.
         */
        ::std::vector<::std::pair<ActionDefinition, ActionData>> getTriggeredActions();

        /**
         * @brief Creates an action and specifies its attributes.
         * 
         * @param name The name of the action.
         * @param attributes The attributes of the action (no. of axes, state or change, etc.)
         */
        void registerAction(const ActionName& name, InputAttributesType attributes);

        /**
         * @brief Creates an action and specifies its attributes based on its JSON description.
         * 
         * @param actionParameters The action's description in JSON.
         * 
         * @see InputManager::registerAction()
         */
        void registerAction(const nlohmann::json& actionParameters);

        /**
         * @brief Removes an action from this context.
         * 
         * @param name The name of the action being removed.
         */
        void unregisterAction(const ActionName& name);

        /**
         * @brief Register a binding from an input-sign-axis-modifiers combination to a specific axis of an action.
         * 
         * @param forAction The action being mapped to.
         * @param targetAxis The axis of the action affected.
         * @param withInput The combo responsible for changing the action's value.
         * 
         * @see InputManager::registerInputBind()
         */
        void registerInputBind(const ActionName& forAction, AxisFilter targetAxis, const InputCombo& withInput);

        /**
         * @brief Register a binding from an input-sign-axis-modifiers combination to a specific axis of an action.
         * 
         * @param inputBindParameters A JSON description of the input bind.
         */
        void registerInputBind(const nlohmann::json& inputBindParameters);

        /**
         * @brief Remove the binding from this input-sign-axis-modifier combination to whatever action it's bound to.
         * 
         * @param inputCombo The combo of the input bind being removed.
         */
        void unregisterInputBind(const InputCombo& inputCombo);

        /**
         * @brief Removes all input binds associated with a particular action.
         * 
         * @param forAction The action whose input binds are being removed.
         */
        void unregisterInputBinds(const ActionName& forAction);

        /**
         * @brief Removes all input combo -> action-axis bindings.
         * 
         */
        void unregisterInputBinds();

        /**
         * @brief Checks whether this context allows propagation to lower priority contexts.
         * 
         * @retval true Allows propagation to lower priority contexts;
         * @retval false Disallows propagation to lower priority contexts;
         */
        inline bool propagateAllowed() { return mPropagateInput; };

        // Enable or disable input propagation to lower priority contexts
        /**
         * @brief Enables or disables input propagation to lower priority contexts.
         * 
         * @param allowPropagate Whether propagation is permitted (for handled actions)
         */
        inline void setPropagateAllowed(bool allowPropagate) { mPropagateInput = allowPropagate; }

        /** 
         * @brief Checks whether this context is active and able to process input events.
         * 
         */
        inline bool enabled() { return mEnabled; }

        /**
         * @brief Enable or disable this context, allowing it to or preventing it from receiving input events.
         */
        inline void setEnabled(bool enable)  { mEnabled = enable; }

    private:
        /**
         * @brief Sets action data for this action to 0.f or false, and queues a corresponding RESET action.
         * 
         * @param forAction The action whose vlaue is being reset.
         * @param timestamp The time of the reset, in milliseconds since application start.
         */
        void resetActionData(const ActionName& forAction, uint32_t timestamp);

        /**
         * @brief Sets all action data to 0.f or false for ALL actions, and queues related RESET actions.
         *
         * @param timestamp The time of the reset, in milliseconds since application start.
         */
        void resetActionData(uint32_t timestamp);

        /**
         * @brief Maps the given input value to its assigned action state.
         * 
         * @param inputValue The value of the input combo to be mapped to an axis of an action belonging to this action's context.
         * @param inputCombo The combo whose value is being mapped.
         */
        void mapToAction(const UnmappedInputValue& inputValue, const InputCombo& inputCombo);

        /* 
         * @brief A reference to the input manager that created this 
         * action context.
         * 
         * (apparently the technical term for this is "dependency injection via constructor")
        */
        InputManager& mInputManager;

        /**
         * @brief The name of this action context
         */
        const ContextName mName;

        /**
         * @brief Determines whether this action context is active and allowed to process any bound input events
         * 
         */
        bool mEnabled { true };

        /**
         * @brief Determines whether, after mapping an input event to its corresponding action, other contexts waiting for the input event are allowed to have a go at processing it also.
         * 
         */
        bool mPropagateInput { false };

        /**
         * @brief All actions defined for this context and their most recently triggered state.
         */
        ::std::unordered_map<ActionDefinition, ActionData> mActions {};

        /**
         * @brief Action state changes that have recently been triggered, in the order that they were triggered.
         */
        ::std::vector<::std::pair<ActionDefinition, ActionData>> mPendingTriggeredActions {};

        /**
         * @brief All input bindings associated with a specific action.
         */
        ::std::unordered_map<ActionDefinition, ::std::set<InputCombo>> mActionToInputBinds {};

        /**
         * @brief Mapping from unmapped input controls, provided by the input manager, to their associated action definitions.
         * 
         */
        ::std::unordered_map<InputCombo, ::std::pair<AxisFilter, ActionDefinition>> mInputBindToAction {};

    friend class InputManager;
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief An object responsible for tracking action listeners for a given project.
     * 
     * Triggered actions created by ActionContext do not actually reach their intended listeners until they have been (manually) sent via an ActionDispatch object.  This affords developers some control over when and where actions are received.
     * 
     * In the engine's scene system, for example, while every ViewportNode has an associated ActionDispatch object, descendant viewports do not see triggered actions unless their parent viewports allow actions to propagate to them.
     * 
     * ## Usage:
     * 
     * ```c++
     * 
     * class ViewportNode: public BaseSceneNode<ViewportNode>, public Resource<ViewportNode> {
     * 
     *     // ...
     * 
     *     ActionDispatch mActionDispatch {};
     * 
     *     // ...
     * 
     * };
     * 
     * // ...
     * 
     * // This viewport method receives an action somehow, and then ...
     * bool ViewportNode::handleAction(std::pair<ActionDefinition, ActionData> pendingAction) {
     * 
     *     // ...
     * 
     *     // ... sends it along to any action listeners it has.
     *     bool actionHandled { false };
     *     actionHandled =  mActionDispatch.dispatchAction(pendingAction);
     * 
     *     // ...
     * 
     * }
     * 
     * ```
     */
    class ActionDispatch {
    public:
        /**
         * @brief Registers a handler for an action.
         * 
         * @param contextActionPair The full name of the action.
         * @param actionHandler A pointer to the action handler.
         */
        void registerActionHandler(const QualifiedActionName& contextActionPair, ::std::weak_ptr<IActionHandler> actionHandler);

        /**
         * @brief Removes a handler for a particular action.
         * 
         * @param contextActionPair The full name of the action.
         * @param actionHandler A pointer to the action handler.
         */
        void unregisterActionHandler(const QualifiedActionName& contextActionPair, ::std::weak_ptr<IActionHandler> actionHandler);

        /**
         * @brief Removes an action handler from all its subscribed actions.
         * 
         * @param actionHandler A pointer to the action handler.
         */
        void unregisterActionHandler(::std::weak_ptr<IActionHandler> actionHandler);

        /**
         * @brief Sends data for an action to all of that action's registered handlers.
         * 
         * @param pendingAction The full action, including its definition and data.
         * @retval true The action was handled by one of this ActionDispatch's subscribers.
         * @retval false The action was not handled by any of this ActionDispatch's subscribers.
         */
        bool dispatchAction(const ::std::pair<ActionDefinition, ActionData>& pendingAction);

    private:

        /**
         * @brief Pointers to all action handler instances waiting for a particular action.
         * 
         */
        ::std::map<QualifiedActionName, ::std::set<::std::weak_ptr<IActionHandler>, ::std::owner_less<::std::weak_ptr<IActionHandler>>>, ::std::less<QualifiedActionName>> mActionHandlers {};
    };
}

#endif

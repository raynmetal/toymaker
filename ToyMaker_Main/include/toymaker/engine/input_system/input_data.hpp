/**
 * @file input_system/input_data.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief A collection of types used by the input system and any entities that have to interact with it.
 * @version 0.3.2
 * @date 2025-08-31
 * 
 * The input system, in a nutshell, breaks up all inputs from every source into their constituent single axis values.  Each such value is then remapped to one axis of one action.
 */

/**
 * @defgroup ToyMakerInputSystem Input System
 * @ingroup ToyMakerEngine
 * 
 */

#ifndef FOOLSENGINE_INPUTSYSTEMDATA_H
#define FOOLSENGINE_INPUTSYSTEMDATA_H

#include <set>
#include <map>
#include <string>
#include <functional>

#include <nlohmann/json.hpp>
#include <glm/glm.hpp>

namespace ToyMaker {

    /**
     * @ingroup ToyMakerInputSystem
     * @brief The name of an action whose meaning is known within a specific context.
     * 
     * @see ContextName
     * 
     */
    using ActionName = ::std::string;

    /**
     * @ingroup ToyMakerInputSystem
     * @brief The name of a context which contains definitions for actions that are valid within it.
     * 
     */
    using ContextName = ::std::string;

    struct InputSourceDescription;

    /**
     * @ingroup ToyMakerInputSystem
     * @brief The type of input device that was responsible for creating the signal which will be mapped to an action.
     * 
     */
    enum class DeviceType: uint8_t {
        NA, /**< No valid input device */
        MOUSE, /**< A continuous pointer device */
        KEYBOARD, /**< A collection of buttons */
        CONTROLLER, /**< Various button, axis, touch inputs */
        TOUCH, /**< As in touch screen devices */
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief A single device may have multiple buttons and other controls, each of which will correspond to a type of input listed here.
     * 
     */
    enum class ControlType: uint8_t {
        NA, /**< No valid (or known) input device control */
        AXIS, /**< Control that emits continuous values b/w 0 and 1, or -1 and 1 (like analog sticks, or controller triggers) */
        MOTION, /**< Control indicating a change in position of something (like a mouse movement, or a touch drag) */
        POINT, /**< Control that maps to a point on the screen, or on the input device itself (like a touch screen tap, or a mouse click) */
        BUTTON, /**< Button, which may either be pressed or not pressed (like a keyboard key) */
        RADIO, /**< A collection of buttons where only one may be active at a time (like a d-pad) */
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief A type with multiple uses.
     * 
     * The bits of this type signify a single axis, in the positive or negative direction.  They also determine whether the value is that of a change, or of a state.
     * 
     * Some examples,
     * 
     * - State: Trigger value, tablet pen pressure, pointer location on an axis
     * 
     * - Change: Mouse movement on an axis, button pressed or unpressed, touch drag
     * 
     */
    typedef uint8_t AxisFilterType;

    /**
     * @ingroup ToyMakerInputSystem
     * @brief A type that is quite possibly unnecessary now that DeviceType and ControlType exist.
     * 
     * But either way.  Lists various attributes of the control that it is associated with. (State/change? Axis/button? Simple? One-axis? Two-axes?)
     * 
     */
    typedef uint8_t InputAttributesValueType;

    /**
     * @ingroup ToyMakerInputSystem
     * @brief A composite type which uniquely identifies a control attached to the platform running this application.
     * 
     */
    typedef ::std::pair<DeviceType, ControlType> InputSourceType;

    /**
     * @ingroup ToyMakerInputSystem
     * @brief A class that, perhaps just as unnecessarily, stores a value of InputAttributesValueType.
     * 
     */
    struct InputAttributesType {
        InputAttributesType() = default;
        InputAttributesType(InputAttributesValueType value) : mValue{value} {}
        operator InputAttributesValueType() const { return mValue; }
        InputAttributesValueType mValue {0};
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief A mapping from each type of input control to the attributes associated with it.
     * 
     */
    extern const ::std::map<InputSourceType, InputAttributesType> kInputSourceTypeAttributes;

    /**
     * @ingroup ToyMakerInputSystem
     * @brief A collection of a few important input attribute value type values and masks.
     * 
     * Each value corresponds to some aspect that an input control might have. The input attributes value, filtered through these enums, tells you of the capabilities of the control.
     * 
     */
    enum InputAttributes: InputAttributesValueType {
        /* 
        *   Mask for the first two bits containing
        * the number of axes in the value produced by
        * an input device
        */
        N_AXES=0x3, /**< The number of axes present on the control (or 0 for buttons), retrieved using this as a mask*/
        HAS_NEGATIVE=0x4, /**< Lines up with the bit representing sign in AxisFilter, indicates that a control has negative values */
        HAS_CHANGE_VALUE=0x8, /**< Does the control report changes? (mouse motion, touch pad drag) */
        HAS_BUTTON_VALUE=0x10, /**< Does the control also act as a button sometimes? */
        HAS_STATE_VALUE=0x20, /**< Does the control have a state? (gyros don't, mouse pointers, touch pads, triggers, analog sticks, buttons all do.) */
        STATE_IS_LOCATION=0x40, /**< Does the control sometimes indicate a location? (like mouse pointers, touch pads, tablet pen hovers) */
    };

    /** 
     * @ingroup ToyMakerInputSystem
     * @brief Identifies a single control, such as a button, trigger, or joystick, on a single device.
     */
    struct InputSourceDescription {
        /**
         * @brief The attributes of the input control, queryable using values from InputAttributes
         * 
         */
        InputAttributesType mAttributes {0};

        /**
         * @brief The ID of a device, assuming several of the same devices can be connected to a single platform.
         * 
         */
        uint8_t mDevice {0};

        /**
         * @brief The ID of the control on a single device, if the device has multiple controls (buttons, triggers, and the like).
         * 
         */
        uint32_t mControl {0};

        /**
         * @brief The type of device described by this object.
         * 
         */
        DeviceType mDeviceType {DeviceType::NA};
        /**
         * @brief The type of control belonging to this device, described by this object.
         * 
         */
        ControlType mControlType {ControlType::NA};

        /**
         * @brief Compares one description to another for equality.
         * 
         * Used mainly to ensure that when used in containers like maps, that different input sources are seen as unique.
         * 
         * @param other The other input source description, being compared to this one.
         * @retval true The input sources are different;
         * @retval false The input sources are the same;
         */
        bool operator==(const InputSourceDescription& other) const {
            return !(*this < other) && !(other < *this);
        }

        /**
         * @brief Compares one description to another, for use by the equality operator.
         * 
         * Also used for sorting.
         * 
         * @param other The input source description being compared to this one.
         * @retval true This input source is "less than";
         * @retval false The input source is not "less than"
         */
        bool operator<(const InputSourceDescription& other) const {
            return (
                static_cast<uint8_t>(mDeviceType) < static_cast<uint8_t>(other.mDeviceType)
                || (
                    mDeviceType == other.mDeviceType && (
                        mDevice < other.mDevice
                        || (
                            mDevice == other.mDevice
                            && (
                                static_cast<uint8_t>(mControlType) < static_cast<uint8_t>(other.mControlType)
                                || mControl < other.mControl
                            )
                        )
                    )
                )
            );
        }

        /**
         * @brief Explicitly defines what are considered truthy and falsey values for this type.
         * 
         * @retval true At least one of mDeviceType or mControlType is defined;
         * @retval false Neither of mDeviceType or mControlType are defined;
         */
        operator bool() const {
            // Must have both a device type and a control type
            // to be considered a valid input source
            return !(
                mDeviceType == DeviceType::NA
                || mControlType == ControlType::NA
            );
        }
    };

    /** 
     * @ingroup ToyMakerInputSystem
     * @brief Enumeration of all possible axis filter values 
     * 
     */
    enum AxisFilter: AxisFilterType {
                          ///     V- lines up with the bit representing sign in actionAttributes
        SIMPLE=0x0,       //<    Sign     Index
        X_POS=0x1,        //< 0b  00       01
        X_NEG=0x5,        //< 0b  01       01
        Y_POS=0x2,        //< 0b  00       10
        Y_NEG=0x6,        //< 0b  01       10
        Z_POS=0x3,        //< 0b  00       11
        Z_NEG=0x7,        //< 0b  01       11
        X_CHANGE_POS=0x9, //< 0b  10       01
        X_CHANGE_NEG=0xD, //< 0b  11       01
        Y_CHANGE_POS=0xA, //< 0b  10       10
        Y_CHANGE_NEG=0xE, //< 0b  11       10
        Z_CHANGE_POS=0xB, //< 0b  10       11
        Z_CHANGE_NEG=0xF, //< 0b  11       11
    };
    
    /**
     * @ingroup ToyMakerInputSystem
     * @brief Important values used with AxisFilterType for determining the type, direction, and sign, of an input.
     * 
     */
    enum AxisFilterMask: AxisFilterType {
        ID=0x3,
        SIGN=0x4,
        CHANGE=0x8,
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief Filter that uniquely defines ONE axis of one control of one input belonging to one device.
     * 
     */
    struct InputFilter {

        /**
         * @brief The control, one of whose axes is being filtered for, uniquely described by this InputFilter.
         * 
         */
        InputSourceDescription mControl {};

        /**
         * @brief The axis of the control (and its value type) being filtered for.
         * 
         * @see mControl
         */
        AxisFilter mAxisFilter { 0x0 };

        /**
         * @brief Compares two input filters for equality.
         * 
         * Mainly used in special containers (like maps), and for sorting algorithms.
         * 
         * @param other The input filter this one is being compared to.
         * @retval true If the filters are the same.
         * @retval false If the filters are not the same.
         */
        bool operator==(const InputFilter& other) const {
            return !(*this < other) && !(other < *this);
        }

        /**
         * @brief A less than operator, mainly used for sorting and the equality operator.
         * 
         * @param other The filter this one is being compared to.
         * @retval true If this filter is "less than"
         * @retval false If this filter is not "less than"
         */
        bool operator<(const InputFilter& other) const {
            return (
                mControl < other.mControl
                || (
                    mControl == other.mControl
                    && (
                        mAxisFilter < other.mAxisFilter
                    )
                )
            );
        }

        /**
         * @brief Provides explicit truthy-falsey mapping for this struct
         * 
         * @retval true If this object describes a valid input;
         * @retval false If this object does not describe a valid input
         * 
         * @see InputSourceDescription::operator bool()
         */
        operator bool() const {
            return mControl;
        }
    };

    /**
     * @ingroup ToyMakerInputSystem
     * 
     * @brief An input combo that whose value is recorded and mapped to an (axis of an) action value of some kind.
     * 
     * These objects are used to inform the InputManager what inputs are being listened for, how these inputs relate to each other, and what event should trigger an Action update.
     * 
     * At the time of writing, each combo supports one "main control" and two "modifier controls", each of which is taken to be one axis of one control of one device.
     * 
     * After conversion, input from any InputCombo is ultimately mapped to a value between 0 and 1.
     * 
     */
    struct InputCombo {
        /**
         * @brief The action on the main control (provided any modifiers are active) that activates this combo.
         * 
         */
        enum Trigger: uint8_t {
            ON_PRESS, //< Main control is pressed
            ON_RELEASE, //< Main control was pressed and is now released
            ON_CHANGE, //< Something has changed w.r.t. the main control
            ON_BUTTON_PRESS, //< (mainly for pointers and analog sticks) Main control that doubles as a button is pressed
            ON_BUTTON_RELEASE, //< (mainly for pointers and analog sticks) Main control that doubles as a button was pressed and is now released
            ON_BUTTON_CHANGE, //< (mainly for pointers and analog sticks) Main control that doubles as a button has just been pressed or released
        };

        /**
         * @brief Axis value corresponding to this combo may be sampled from this control.
         * 
         */
        InputFilter mMainControl {}; 

        /**
         * @brief A single axis of a single input source that must be considered active in order for this combo to be considered active.
         * 
         * Falsey InputFilters are always considered active.
         */
        InputFilter mModifier1 {}; 

        /**
         * @brief A single axis of a single input source that must be considered active in order for this combo to be considered active.
         * 
         * Falsey InputFilters are always considered active.
         */
        InputFilter mModifier2 {}; 

        /**
         * @brief The actual event on the main control that causes the value mapped to this InputCombo to change.
         * 
         */
        Trigger mTrigger { ON_CHANGE };

        /**
         * @brief Some device controls, like analog sticks, wear out over time producing false positives for input events.  Adjusting this value helps to filter out such false positives.
         * 
         */
        double mDeadzone { 0.f };

        /**
         * @brief The threshold (on a main control that produces continuous values, like analog sticks and triggers) beyond which the control is considered pressed, and below which it is considered released.
         * 
         */
        double mThreshold { .7f };

        /**
         * @brief An explicit definition for what set of InputCombo values are considered truthy and which ones are falsey.
         * 
         * @retval true An input combo corresponding to a real input source;
         * @retval false An input combo which is invalid, and therefore "falsey";
         */
        operator bool() const {
            return mMainControl;
        }

        /**
         * @brief Compares to InputCombos for equality, for use in certain containers like maps.
         * 
         * @param other The combo this one is being compared to.
         * @retval true The combos are equivalent.
         * @retval false The combos are not equivalent.
         */
        bool operator==(const InputCombo& other) const {
            return !(*this < other) && !(other < *this);
        }

        /**
         * @brief Mainly supports the equality operator, and allows sorting for InputCombos to work.
         * 
         * @param other The combo this one is being compared to.
         * @retval true The combos are equivalent;
         * @retval false The combos are not equivalent;
         */
        bool operator<(const InputCombo& other) const {
            return (
                mMainControl < other.mMainControl
                || (
                    mMainControl == other.mMainControl
                    && (
                        mModifier1 < other.mModifier1
                        || (
                            mModifier1 == other.mModifier1
                            && (
                                mModifier2 < other.mModifier2
                                || (
                                    mModifier2 == other.mModifier2 
                                    && (
                                        static_cast<uint8_t>(mTrigger) < static_cast<uint8_t>(other.mTrigger)
                                        || (
                                            mTrigger == other.mTrigger 
                                            && (
                                                mThreshold < other.mThreshold
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
            );
        }
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief An input state that hasn't yet been mapped to its corresponding action.
     * 
     */
    struct UnmappedInputValue {
        /**
         * @brief The time at which this input state was recorded.
         * 
         */
        uint32_t mTimestamp {};

        /**
         * @brief Per its combo's main control, whether this value should be considered "active"
         * 
         */
        bool mActivated { false };

        /**
         * @brief The value of the axis of the control of the combo that this value represents.
         * 
         */
        float mAxisValue { 0.f };

        /**
         * @brief In devices where a control also doubles as a button (like analog sticks, pointer clicks), the state of the button when this input was recorded.
         * 
         */
        float mButtonValue { 0.f };
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief The type of value associated with this action.
     * 
     */
    enum class ActionValueType: uint8_t {
        STATE, //< A value that represents the state of an action in the present moment (like mouse positions, or tablet pen pressure)
        CHANGE, //< A value that represents a recent change (like mouse motions, or button presses and releases)
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief An identifier that fully names one action present in the project.
     * 
     */
    using QualifiedActionName = ::std::pair<ContextName, ActionName>;

    /**
     * @ingroup ToyMakerInputSystem
     * @brief The definition of a single action, including whether it represents state or change, whether it supports negative values, and the number of axes it has.
     * 
     * The only parts of the definition used for comparisons is the action's name and context;  its input attributes are mainly used internally, by the InputManager itself.
     */
    struct ActionDefinition {
        /**
         * @brief Construct a new action definition object
         * 
         * @param contextActionNamePair The full name of the action.
         */
        ActionDefinition(const QualifiedActionName& contextActionNamePair):
        mName{contextActionNamePair.second},
        mContext{contextActionNamePair.first} 
        {}
        /**
         * @brief Construct a new (empty) action definition object
         * 
         */
        ActionDefinition()=default;

        /**
         * @brief The name of the action.
         * 
         */
        ::std::string mName {};
        
        /**
         * @brief The same as in an InputSource, describes the type of data (normalized) this action is expected to have.
         * 
         * Its used mainly internally, by an ActionContext, in order to figure out how to build action data values.  This description is defined in some kind of input file.
         * 
         */
        InputAttributesType mAttributes {};

        /**
         * @brief I'm not yet sure where this is used, since mAttributes already exists.
         * 
         * @todo Find out where this is used.
         * 
         */
        ActionValueType mValueType {};

        /**
         * @brief The name of the context the action belongs to.
         * 
         */
        ::std::string mContext {};

        /**
         * @brief Compares two action definitions for equality.
         * 
         * ... Just on the basis of their action and context names.  Mainly in order to make them a unique key in a map container.
         * 
         * @param other The action definition this one is being compared to.
         * @retval true They're the same;
         * @retval false They're not the same;
         */
        inline bool operator==(const ActionDefinition& other) const {
            return !(*this < other) && !(other < *this);
        }

        /**
         * @brief Provides an implementation of the less than operator, mainly for use by the equality operator.
         * 
         * @param other The action definition this one is being compared to.
         * @retval true This one is lesser than the other.
         * @retval false This one is greater than the other.
         */
        inline bool operator<(const ActionDefinition& other) const {
            return (mContext < other.mContext || (
                mContext == other.mContext 
                && mName < other.mName
            ));
        }

        /**
         * @brief Provides implicit casting from QualifiedActionName
         * 
         * @return QualifiedActionName 
         */
        operator QualifiedActionName() const {
            return {mContext, mName};
        }
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief A seemingly redundant type that is a part of the ActionData struct and not the ActionDefinition struct.
     * 
     */
    enum class ActionType {
        BUTTON, //< It's either on or off
        ONE_AXIS, //< It's a single value between 0 and 1 or -1 and 1
        TWO_AXIS, //< It's two values each between 0 and 1 or -1 and 1
        THREE_AXIS, //< It's three values each between 0 and 1 or -1 and 1
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief Helps describe what InputCombo related event was responsible for signaling this action.
     * 
     */
    enum class ActionTrigger: uint8_t {
        UPDATE, //< The trigger condition for the associated input combo was met.
        RESET, //< The trigger condition was met, but has now been failed.
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief A struct containing meta-info that will be present for all types of actions.
     * 
     */
    struct CommonActionData {
        /**
         * @brief The condition which caused this action to be signalled.
         * 
         */
        ActionTrigger mTriggeredBy { ActionTrigger::UPDATE };

        /**
         * @brief The time at which the action was signalled.
         * 
         */
        uint32_t mTimestamp {};

        /**
         * @brief Unused for now, but presumably the duration an active input has been active.
         * 
         */
        uint32_t mDuration { 0 };

        /**
         * @brief I'm not sure about this one.
         * 
         * @todo Find out what this does.
         */
        bool mActivated {false};

        /**
         * @brief The type of value associated with this action.
         * 
         */
        ActionType mType{ ActionType::BUTTON };
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief Actions that ultimately act like a single button value, where mActivated is the state of the button.
     * 
     */
    typedef CommonActionData SimpleActionData;

    /**
     * @ingroup ToyMakerInputSystem
     * @brief Actions that have just one axis of data, eg., the accelerator on a car.
     * 
     */
    struct OneAxisActionData {
        /**
         * @brief Common metadata belonging to this action.
         * 
         */
        CommonActionData mCommonData { .mType{ActionType::ONE_AXIS} };

        /**
         * @brief The actual value of the axis of this action.
         * 
         */
        double mValue { 0.f };
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief Actions that have two axes of data. (Pointer locations, movement direction input, pitch+roll, etc.)
     * 
     */
    struct TwoAxisActionData {
        /**
         * @brief Common action metadata.
         * 
         */
        CommonActionData mCommonData { .mType {ActionType::TWO_AXIS} };

        /**
         * @brief Two float values, normalized (or not, for location states)
         * 
         */
        glm::dvec2 mValue { 0.f };
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief Actions described by 3 axes (I can't think of any examples for this)
     * 
     */
    struct ThreeAxisActionData {
        /**
         * @brief Common action metadata.
         * 
         */
        CommonActionData mCommonData { .mType{ActionType::THREE_AXIS} };

        /**
         * @brief Three float values, normalized (or not, for values representing location)
         * 
         */
        glm::dvec3 mValue { 0.f };
    };

    /**
     * @ingroup ToyMakerInputSystem
     * @brief A union that may contain any one of SimpleActionData, OneAxisActionData, TwoAxisActionData, ThreeAxisActionData
     * 
     */
    union ActionData {
        /**
         * @brief Array of action types so that 0 - BUTTON, 1 - ONE_AXIS, and so on.
         * 
         */
        static constexpr ActionType toType[4] {
            ActionType::BUTTON, ActionType::ONE_AXIS,
            ActionType::TWO_AXIS, ActionType::THREE_AXIS
        };

        /**
         * @brief Construct a new action data object of a particular type.  Called prior to all other ActionData constructors.
         * 
         * @param actionType The type of action data object to construct.
         */
        ActionData(ActionType actionType): mCommonData{ .mType{actionType} } {
            // Regardless of the type, all the data that corresponds
            // to the action value should be initialized with 0
            mThreeAxisActionData.mValue = glm::vec3{0.f};
        }

        /**
         * @brief Construct a new SIMPLE action data object
         * 
         */
        ActionData(): ActionData{ ActionType::BUTTON }  {}

        /**
         * @brief Construct a new SIMPLE action data object, based on already existing SimpleActionData
         * 
         * @param simpleData Preexisting SimpleActionData.
         */
        ActionData(SimpleActionData simpleData): ActionData{ActionType::BUTTON} {
            mSimpleData = simpleData;
        }

        /**
         * @brief Construct a new ONE_AXIS action data object based on already existing OneAxisActionData.
         * 
         * @param oneAxisActionData Preexisting OneAxisActionData.
         */
        ActionData(OneAxisActionData oneAxisActionData): ActionData{ActionType::ONE_AXIS} {
            mOneAxisActionData = oneAxisActionData;
        }

        /**
         * @brief Construct a new action data object based on already existing TwoAxisActionData.
         * 
         * @param twoAxisActionData Preexisting TwoAxisActionData.
         */
        ActionData(TwoAxisActionData twoAxisActionData): ActionData{ActionType::TWO_AXIS} {
            mTwoAxisActionData = twoAxisActionData;
        }
        /**
         * @brief Construct a new THREE_AXIS action data object based on already existing ThreeAxisActionData.
         * 
         * @param threeAxisActionData Preexisting ThreeAxisActionData.
         */
        ActionData(ThreeAxisActionData threeAxisActionData): ActionData{ActionType::THREE_AXIS} {
            mThreeAxisActionData = threeAxisActionData;
        }

        /**
         * @brief Construct a new Action Data object with nAxes axes.
         * 
         * @param nAxes Number of axes for this action data.
         */
        ActionData(uint8_t nAxes): ActionData{ toType[nAxes] } {}


        /**
         * @brief Common or simple data.
         * 
         */
        CommonActionData mCommonData;

        /**
         * @brief Common or simple data.
         * 
         */
        SimpleActionData mSimpleData;

        /**
         * @brief One axis action data.
         * 
         */
        OneAxisActionData mOneAxisActionData;

        /**
         * @brief Two axis action data.
         * 
         */
        TwoAxisActionData mTwoAxisActionData;

        /**
         * @brief Three axis action data.
         * 
         */
        ThreeAxisActionData mThreeAxisActionData;
    };

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    NLOHMANN_JSON_SERIALIZE_ENUM ( DeviceType, {
        {DeviceType::NA, "na"},
        {DeviceType::MOUSE, "mouse"},
        {DeviceType::KEYBOARD, "keyboard"},
        {DeviceType::TOUCH, "touch"},
        {DeviceType::CONTROLLER, "controller"},
    });

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    NLOHMANN_JSON_SERIALIZE_ENUM(ControlType, {
        {ControlType::NA, "na"},
        {ControlType::AXIS, "axis"},
        {ControlType::MOTION, "motion"},
        {ControlType::POINT, "point"},
        {ControlType::BUTTON, "button"},
        {ControlType::RADIO, "radio"},
    });

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    NLOHMANN_JSON_SERIALIZE_ENUM( AxisFilter, {
        {AxisFilter::SIMPLE, "simple"},
        {AxisFilter::X_POS, "+x"},
        {AxisFilter::X_NEG, "-x"},
        {AxisFilter::Y_POS, "+y"},
        {AxisFilter::Y_NEG, "-y"},
        {AxisFilter::Z_POS, "+z"},
        {AxisFilter::Z_NEG, "-z"},
        {AxisFilter::X_CHANGE_POS, "+dx"},
        {AxisFilter::X_CHANGE_NEG, "-dx"},
        {AxisFilter::Y_CHANGE_POS, "+dy"},
        {AxisFilter::Y_CHANGE_NEG, "-dy"},
        {AxisFilter::Z_CHANGE_POS, "+dz"},
        {AxisFilter::Z_CHANGE_NEG, "-dz"},
    });

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    NLOHMANN_JSON_SERIALIZE_ENUM( InputCombo::Trigger, {
        {InputCombo::Trigger::ON_PRESS, "on-press"},
        {InputCombo::Trigger::ON_RELEASE, "on-release"},
        {InputCombo::Trigger::ON_CHANGE, "on-change"},
        {InputCombo::Trigger::ON_BUTTON_PRESS, "on-button-press"},
        {InputCombo::Trigger::ON_BUTTON_RELEASE, "on-button-release"},
        {InputCombo::Trigger::ON_BUTTON_CHANGE, "on-button-change"},
    });

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    NLOHMANN_JSON_SERIALIZE_ENUM( ActionValueType, {
        {ActionValueType::STATE, "state"},
        {ActionValueType::CHANGE, "change"},
    });

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    void to_json(nlohmann::json& json, const ToyMaker::InputAttributesType& inputAttributes);

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    void from_json(const nlohmann::json& json, ToyMaker::InputAttributesType& inputAttributes);

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    void to_json(nlohmann::json& json, const ToyMaker::InputSourceDescription& inputSourceDescription);

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    void from_json(const nlohmann::json& json, ToyMaker::InputSourceDescription& inputSourceDescription);

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    void to_json(nlohmann::json& json, const ToyMaker::InputFilter& inputFilter);

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    void from_json(const nlohmann::json& json, ToyMaker::InputFilter& inputFilter);

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    void to_json(nlohmann::json& json, const ToyMaker::InputCombo& inputCombo);

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    void from_json(const nlohmann::json& json, ToyMaker::InputCombo& inputCombo);

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    void to_json(nlohmann::json& json, const ToyMaker::ActionDefinition& actionDefinition);

    /**
     * @ingroup ToyMakerInputSystem ToyMakerSerialization
     * 
     */
    void from_json(const nlohmann::json& json, ToyMaker::ActionDefinition& actionDefinition);
}

namespace std {
    template<>
    struct hash<ToyMaker::InputSourceDescription> {
        size_t operator() (const ToyMaker::InputSourceDescription& definition) const {
            return (
                (( (hash<uint32_t>{}(definition.mControl))
                ^ (hash<uint8_t>{}(definition.mDevice) << 1) >> 1)
                ^ (hash<uint8_t>{}(static_cast<uint8_t>(definition.mDeviceType)) << 1) >> 1)
                ^ (hash<uint8_t>{}(static_cast<uint8_t>(definition.mControlType) << 1))
            );
        }
    };

    template<>
    struct hash<ToyMaker::ActionDefinition> {
        size_t operator() (const ToyMaker::ActionDefinition& definition) const {
            return hash<std::string>{}(definition.mName);
        }
    };

    template<>
    struct hash<ToyMaker::InputFilter> {
        size_t operator() (const ToyMaker::InputFilter& inputFilter) const {
            return (
                (hash<ToyMaker::InputSourceDescription>{}(inputFilter.mControl)
                ^ (hash<uint8_t>{}(inputFilter.mAxisFilter) << 1))
            );
        }
    };

    template<>
    struct hash<ToyMaker::InputCombo> {
        size_t operator() (const ToyMaker::InputCombo& inputBind) const {
            return (
                (((hash<ToyMaker::InputFilter>{}(inputBind.mMainControl)
                ^ (hash<ToyMaker::InputFilter>{}(inputBind.mModifier1) << 1) >> 1)
                ^ (hash<ToyMaker::InputFilter>{}(inputBind.mModifier2) << 1) >> 1)
                ^ (hash<ToyMaker::InputCombo::Trigger>{}(inputBind.mTrigger) << 1) >> 1)
                ^ (hash<float>{}(inputBind.mThreshold) << 1)
            );
        }
    };
}

#endif

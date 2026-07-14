#include <SDL3/SDL.h>

#include <toymaker/engine/window_context_manager.hpp>
#include <toymaker/engine/input_system/input_system.hpp>
#include <toymaker/engine/signals.hpp>
#include <toymaker/engine/sim_system.hpp>
#include <toymaker/engine/physics/system.hpp>

const float DEFAULT_CAMERA_SPEED { 6.f };
const float MAX_PITCH { 89.f };
const float LOOK_SENSITIVITY { .23f };
const float ZOOM_SENSITIVITY { 1.5f };
const float MAX_FOV { 100.f };
const float MIN_FOV { 40.f };

/**
 * @ingroup Examples
 *
 * @brief Aspect responsible for applying downward gravitational force to object's center
 * of mass.
 */
class FlyCamera: public ToyMaker::SimObjectAspect<FlyCamera> {
public:
    /**
     * @brief Gets the aspect type string associated with this class
     *
     * @return std::string This class' aspect type string.
     *
     */
    inline static std::string getSimObjectAspectTypeName() { return "FlyCamera"; }

    /**
     * @brief Constructs this aspect from its JSON description
     *
     * Example:
     * ```json
     * { "type": "FlyCamera" }
     * ```
     *
     * @param jsonAspectProperties This aspect's description in JSON.
     *
     * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed aspect.
     *
     */
    static std::shared_ptr<ToyMaker::BaseSimObjectAspect> create(const nlohmann::json& jsonAspectProperties);

    /**
     * @brief Uses this aspect's data to construct a new aspect.
     *
     * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed aspect.
     *
     */
    std::shared_ptr<ToyMaker::BaseSimObjectAspect> clone() const override;

private:

    bool mActive { false };
    float mMaxSpeed { DEFAULT_CAMERA_SPEED };
    glm::vec3 mVelocity { 0.f };
    float mLookSensitivity { LOOK_SENSITIVITY };
    float mZoomSensitivity { 1.5f };

    /**
     * @brief Constructs a new FlyCamera object.
     *
     */
    FlyCamera(): ToyMaker::SimObjectAspect<FlyCamera>{0} {}

    void variableUpdate(uint32_t variableStepMillis) override;

    void onMove(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition);

    void onRotate(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition);

    void onToggleControl(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition);

    std::weak_ptr<ToyMaker::FixedActionBinding> handlerMove {
        declareFixedActionBinding(
            "Camera",
            "Move",
            [this](const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
                this->onMove(actionData, actionDefinition);
                return true;
            }
        )
    };

    std::weak_ptr<ToyMaker::FixedActionBinding> handlerRotate {
        declareFixedActionBinding(
            "Camera",
            "Rotate",
            [this](const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
                this->onRotate(actionData, actionDefinition);
                return true;
            }
        )
    };

    std::weak_ptr<ToyMaker::FixedActionBinding> handlerToggleControl {
        declareFixedActionBinding(
            "Camera",
            "UpdateFOV",
            [this](const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
                this->onToggleControl(actionData, actionDefinition);
                return true;
            }
        )
    };

    void updatePitch(glm::quat& current, float dPitch);

    void updateYaw(glm::quat& current, float dYaw);

    void updateFOV(float dFOV);

    void setActive(bool active);
};


std::shared_ptr<ToyMaker::BaseSimObjectAspect> FlyCamera::create(const nlohmann::json& jsonAspectProperties) {
    (void) jsonAspectProperties; // prevent unused parameter warning
    return std::shared_ptr<FlyCamera>(new FlyCamera{});
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> FlyCamera::clone() const {
    return std::shared_ptr<FlyCamera>(new FlyCamera{});
}

void FlyCamera::variableUpdate(uint32_t variableStepMillis) {
    if(!mActive) return;

    ToyMaker::Placement placement { getComponent<ToyMaker::Placement>() };

    const glm::mat4 rotationMatrix { glm::mat4_cast(placement.mOrientation) };
    const glm::vec4 localForward { rotationMatrix * glm::vec4 { 0.f, 0.f, -1.f, 0.f } };
    const glm::vec4 localRight { rotationMatrix * glm::vec4 { 1.f, 0.f, 0.f, 0.f } };

    placement.mPosition += (
        variableStepMillis / 1000.f * (
            mVelocity.z * localForward
            + mVelocity.x * localRight
        )
    );
    updateComponent(placement);
}

void FlyCamera::onToggleControl(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
    setActive(!mActive);
}

void FlyCamera::onRotate(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
    if(!mActive) return;
    ToyMaker::Placement placement { getComponent<ToyMaker::Placement>() };
    updatePitch(placement.mOrientation, actionData.mTwoAxisActionData.mValue.y);
    updateYaw(placement.mOrientation, actionData.mTwoAxisActionData.mValue.x);
    updateComponent(placement);

    // Dirty hack because SDL_SetWindowRelativeMouseMotion doesn't work on my platform somehow
    const auto appWindow {
        ToyMaker::WindowContext::getInstance().getSDLWindow(),
    };
    const auto windowDimensions {
        ToyMaker::WindowContext::getInstance().getDimensions()
    };
    SDL_WarpMouseInWindow(appWindow, .5f * windowDimensions.x, .5f * windowDimensions.y);
}

void FlyCamera::onMove(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
    if(!mActive) return;
    mVelocity.x = mMaxSpeed * actionData.mTwoAxisActionData.mValue.x;
    mVelocity.z = mMaxSpeed * actionData.mTwoAxisActionData.mValue.y;
}

void FlyCamera::updatePitch(glm::quat& current, float dPitch) {
    float dOutPitch { mLookSensitivity * -dPitch };

    const float oldPitch { glm::pitch(current) };
    const float newPitch { oldPitch + dOutPitch };

    // TODO: find a better way to do this
    // Overcomplicated way to constrain pitch to range (-MAX_PITCH, MAX_PITCH)
    if(
        glm::sin(newPitch) > glm::sin(glm::radians(MAX_PITCH)) // approaching complete alignment with the up vector
        || ( // or
            !std::signbit(glm::sin(newPitch)) && !std::signbit(glm::sin(oldPitch)) // positive y axis
            && std::signbit(glm::cos(newPitch)) != std::signbit(glm::cos(oldPitch)) // but flip in x axis
        )
    ) { 
        dOutPitch = (
            glm::cos(oldPitch) < 0.f? 
                glm::radians(180.f) - glm::radians(MAX_PITCH): 
                glm::radians(MAX_PITCH)
        ) - oldPitch;
    }
    else if(
        glm::sin(newPitch) < glm::sin(glm::radians(-MAX_PITCH))
        || (
            std::signbit(glm::sin(newPitch)) && std::signbit(glm::sin(oldPitch)) // negative y axis
            && std::signbit(glm::cos(newPitch)) != std::signbit(glm::cos(oldPitch))  // but flip in x axis
        )
    ) {
        dOutPitch = (
            glm::cos(oldPitch) < 0.f?
                glm::radians(180.f) - glm::radians(-MAX_PITCH):
                glm::radians(-MAX_PITCH)
        ) - oldPitch;
    }

    const glm::quat pitchQuat { glm::vec3{ dOutPitch, 0.f, 0.f } };
    current = glm::normalize(current * pitchQuat);
}

void FlyCamera::updateYaw(glm::quat& current, float dYaw) {
    float dOutYaw { mLookSensitivity * -dYaw };
    const glm::quat yawQuat { glm::vec3{ 0.f, dOutYaw, 0.f } };
    current = glm::normalize(yawQuat * current);
}

void FlyCamera::setActive(bool active) {
    mActive = active;
    const auto appWindow {
        ToyMaker::WindowContext::getInstance().getSDLWindow(),
    };
    if(active) {
        SDL_RaiseWindow(appWindow);
        SDL_SetWindowMouseGrab(appWindow, true);
        SDL_HideCursor();
    } else {
        SDL_SetWindowMouseGrab(appWindow, false);
        SDL_ShowCursor();
    }
}


#include "toymaker/engine/physics/system.hpp"
#include <toymaker/engine/sim_system.hpp>
#include <toymaker/engine/physics/types.hpp>

#include <toymaker/builtins/interface_pointer_callback.hpp>


/**
 * @ingroup Examples
 *
 * @brief Aspect responsible for applying torque to the object it is attached to when clicked,
 * depending on the location clicked.
 */
class Pin: public ToyMaker::SimObjectAspect<Pin> {
public:
    /**
     * @brief Gets the aspect type string associated with this class
     *
     * @return std::string This class' aspect type string.
     *
     */
    inline static std::string getSimObjectAspectTypeName() { return "Pin"; }

    /**
     * @brief Constructs this aspect from its JSON description
     *
     * Example:
     * ```json
     * { "type": "SpinOnClick" }
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
    /**
     * @brief Constructs a new Spin On Click object.
     *
     */
    Pin(): ToyMaker::SimObjectAspect<Pin>{0} {}

    /**
     * @brief Capture object placement and use it to register a constraint with the physics system
     *
     */
    void onActivated() override;

    /**
     * @brief Clear constraint registered for this object
     *
     */
    void onDeactivated() override;

    ToyMaker::PhysicsSystem::ConstraintID mConstraint;
};


std::shared_ptr<ToyMaker::BaseSimObjectAspect> Pin::create(const nlohmann::json& jsonAspectProperties) {
    (void) jsonAspectProperties; // prevent unused parameter warning
    return std::shared_ptr<Pin>(new Pin{});
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> Pin::clone() const {
    return std::shared_ptr<Pin>(new Pin{});
}

void Pin::onActivated() {
    ToyMaker::Placement placement { getComponent<ToyMaker::Placement>() };
    std::vector<
        std::pair<ToyMaker::EntityID, ToyMaker::PinConstraintData>
    > pinnedEntity {};
    pinnedEntity.push_back({
        getEntityID(),
        ToyMaker::PinConstraintData {
            .mOrigin { placement.mPosition },
            .mOrientation { placement.mOrientation },
        }
    });
    mConstraint = getLocalViewport()
        .getWorld().lock()->getSystem<ToyMaker::PhysicsSystem>()
        ->registerConstraint<ToyMaker::PinConstraint>(
            pinnedEntity, .1f
    );
}

void Pin::onDeactivated() {
    getLocalViewport()
        .getWorld().lock()->getSystem<ToyMaker::PhysicsSystem>()
        ->unregisterConstraint(mConstraint);
}

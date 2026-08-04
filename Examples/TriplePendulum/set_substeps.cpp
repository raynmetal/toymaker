#include <toymaker/engine/sim_system.hpp>
#include <toymaker/engine/physics/system.hpp>

/**
 * @ingroup Examples
 *
 * @brief Aspect which sets the number of physics substeps used by the physics system of the object it is attached to.
 *
 * @TODO: It would be nicer if we had a uniform way of accessing per-ECS-system settings in the scene description
 * somehow
 *
 */
class SetSubsteps: public ToyMaker::SimObjectAspect<SetSubsteps> {
public:
    /**
     * @brief Gets the aspect type string associated with this class
     *
     * @return std::string This class' aspect type string.
     *
     */
    inline static std::string getSimObjectAspectTypeName() { return "SetSubsteps"; }

    /**
     * @brief Constructs this aspect from its JSON description
     *
     * Example:
     * ```json
     * { "type": "SetSubsteps" }
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
     * @brief Constructs a new SetSubsteps object.
     *
     */
    SetSubsteps(): ToyMaker::SimObjectAspect<SetSubsteps>{0} {}

    /**
     * @brief Sets the number of substeps used by the physics system controlling its world
     *
     */
    void onActivated() override;
};

void SetSubsteps::onActivated() {
    getSimObject().getWorld().lock()->getSystem<ToyMaker::PhysicsSystem>()->setSubsteps(10);
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> SetSubsteps::create(const nlohmann::json& jsonAspectProperties) {
    (void) jsonAspectProperties; // prevent unused parameter warning
    return std::shared_ptr<SetSubsteps>(new SetSubsteps{});
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> SetSubsteps::clone() const {
    return std::shared_ptr<SetSubsteps>(new SetSubsteps{});
}


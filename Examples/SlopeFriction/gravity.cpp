#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <toymaker/engine/sim_system.hpp>
#include <toymaker/engine/physics/types.hpp>

#include <toymaker/builtins/interface_pointer_callback.hpp>


/**
 * @ingroup Examples
 *
 * @brief Aspect responsible for applying torque to the object it is attached to when clicked,
 * depending on the location clicked.
 */
class Gravity: public ToyMaker::SimObjectAspect<Gravity> {
public:
    /**
     * @brief Gets the aspect type string associated with this class
     *
     * @return std::string This class' aspect type string.
     *
     */
    inline static std::string getSimObjectAspectTypeName() { return "Gravity"; }

    /**
     * @brief Constructs this aspect from its JSON description
     *
     * Example:
     * ```json
     * { "type": "Gravity" }
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
    Gravity(): ToyMaker::SimObjectAspect<Gravity>{0} {}

    void simulationUpdate(uint32_t timestep) override;
};


std::shared_ptr<ToyMaker::BaseSimObjectAspect> Gravity::create(const nlohmann::json& jsonAspectProperties) {
    (void) jsonAspectProperties; // prevent unused parameter warning
    return std::shared_ptr<Gravity>(new Gravity{});
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> Gravity::clone() const {
    return std::shared_ptr<Gravity>(new Gravity{});
}

void Gravity::simulationUpdate(uint32_t timestep) {
    auto physics { getComponent<ToyMaker::PhysicsState>() };
    const auto bounds { getComponent<ToyMaker::ObjectBounds>() };
    physics.applyForceGlobal(
        physics.mMass * glm::vec3 { 0.f, -10.f, 0.f },
        bounds.getPositionWorld(),
        bounds
    );
    updateComponent(physics);
    // std::cout << "angular velocity: " << glm::to_string(physics.mAngularVelocity) << "\n";
    // std::cout << "orientation: " << glm::to_string(bounds.getOrientationWorld()) << "\n";
}

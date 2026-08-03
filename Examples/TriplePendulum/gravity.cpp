#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <toymaker/engine/signals.hpp>
#include <toymaker/engine/sim_system.hpp>
#include <toymaker/engine/physics/system.hpp>

/**
 * @ingroup Examples
 *
 * @brief Aspect which applies a downward gravitational force every simulation frame to the object it's
 * attached to.
 *
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
     * @brief Constructs a new Gravity object.
     *
     */
    Gravity(): ToyMaker::SimObjectAspect<Gravity>{0} {}

    /**
     * @brief Prints details about the collision that was just observed.
     *
     */
    void printCollision(const ToyMaker::PhysicsSystem::SignalCollidedData& collisionData);

    /**
     * @brief Listens for collision events that this object participates in, reports them to 
     * printCollision().
     *
     */
    ToyMaker::SignalObserver<ToyMaker::PhysicsSystem::SignalCollidedData> mObserveCollided {
        *this, "CollisionObserved",
        [this](ToyMaker::PhysicsSystem::SignalCollidedData signalData) {
            printCollision(signalData);
        }
    };

    /**
     * @brief Applies a downward gravitational force to this object's center of mass every
     * simulation frame.
     *
     */
    void simulationUpdate(uint32_t timestep) override;

    /**
     * @brief Connects to collision signal for this entity advertised by the physics system.
     *
     */
    void onActivated() override;
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
        physics.getMass() * glm::vec3 { 0.f, -10.f, 0.f },
        bounds.getPositionWorld(),
        bounds
    );
    updateComponent(physics);
}

void Gravity::onActivated() {
    connect(
        ToyMaker::PhysicsSystem::SignalCollidedPrefix + std::to_string(getEntityID()),
        "CollisionObserved",
        *getWorld().lock()->getSystem<ToyMaker::PhysicsSystem>()
    );
}

void Gravity::printCollision(const ToyMaker::PhysicsSystem::SignalCollidedData& collisionData) {
    std::cout << "Colliding entities: (" 
        << std::to_string(collisionData.first.first()) << ", " 
        << std::to_string(collisionData.first.second()) << ")\n";
    std::cout << "Penetration depth: "
        << std::to_string(collisionData.second.mContactA.mPenetrationDepth) << "\n";
}

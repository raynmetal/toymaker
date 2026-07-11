#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <toymaker/engine/signals.hpp>
#include <toymaker/engine/sim_system.hpp>
#include <toymaker/engine/physics/system.hpp>

/**
 * @ingroup Examples
 *
 * @brief Aspect responsible for spawing a ball once every second.
 *
 */
class BallSpawner: public ToyMaker::SimObjectAspect<BallSpawner> {
public:
    /**
     * @brief Gets the aspect type string associated with this class.
     *
     * @return std::string This class' aspect type string.
     *
     */
    inline static std::string getSimObjectAspectTypeName() {
        return "BallSpawner";
    }

    /**
     * @brief Constructs this aspect from its JSON description.
     *
     * Example:
     *
     * ```json
     * { "type": "BallSpawner" }
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
     * @return std::shared_ptr<BaseSimObjectAspect> The newly constructed aspect.
     *
     */
    std::shared_ptr<ToyMaker::BaseSimObjectAspect> clone() const override;

private:
    /**
     * @brief Constructs a new BallSpawner aspect.
     *
     */
    BallSpawner(): ToyMaker::SimObjectAspect<BallSpawner>{0} {}

    /**
     * @brief Once every X or so seconds, spawns a new ball object.
     *
     */
    void variableUpdate(uint32_t timestep) override;

    /**
     * @brief The time after which this spawner will attempt
     * to create a new ball.
     *
     */
    uint32_t mTimeSpawnThreshold { 3000 };

    /**
     * @brief The time elapsed since the last ball was spawned.
     *
     */
    uint32_t mTimeElapsed { 0 };

    /**
     * @brief The number of balls spawned so far.
     *
     */
    uint32_t mNBalls { 0 };
};

std::shared_ptr<ToyMaker::BaseSimObjectAspect> BallSpawner::create(const nlohmann::json& jsonAspectProperties) {
    (void) jsonAspectProperties;
    return std::shared_ptr<BallSpawner>(new BallSpawner());
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> BallSpawner::clone() const {
    return std::shared_ptr<BallSpawner>(new BallSpawner());
}

void BallSpawner::variableUpdate(uint32_t timestep) {
    mTimeElapsed = std::min(mTimeElapsed + timestep, mTimeSpawnThreshold);

    if(mTimeElapsed == mTimeSpawnThreshold) {
        mTimeElapsed = 0;
        ToyMaker::ObjectBounds bounds { getComponent<ToyMaker::ObjectBounds>() };
        const auto overlappingEntities {
            getWorld().lock()->getSystem<ToyMaker::SpatialQuerySystem>()->findEntitiesOverlappingCoarse(
                ToyMaker::AxisAlignedBounds(bounds.getPositionWorld(), glm::vec3 { 4.2f }),
                0x1
            )
        };
        if(overlappingEntities.size() > 1) {
            return;
        }

        std::cout << "New ball! " << std::to_string(mNBalls) << "\n";
        std::shared_ptr<ToyMaker::SimObject> newNode {
            ToyMaker::SimObject::copy(
                ToyMaker::ResourceDatabase::GetRegisteredResource<ToyMaker::SimObject>("BallAsset")
            )
        };
        newNode->setName("ball_" + std::to_string(mNBalls++));
        getSimObject().addNode(newNode, "/");
    }
}

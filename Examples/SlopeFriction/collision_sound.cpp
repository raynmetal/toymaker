#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <toymaker/engine/signals.hpp>
#include <toymaker/engine/sim_system.hpp>
#include <toymaker/engine/physics/system.hpp>
#include <toymaker/engine/sound/types.hpp>
#include <toymaker/engine/sound/system.hpp>
#include <toymaker/engine/camera_system.hpp>

/**
 * @ingroup Examples
 *
 * @brief Aspect which makes a sound upon receiving a collision signal for this object
 *
 */
class CollisionSound: public ToyMaker::SimObjectAspect<CollisionSound> {
public:
    /**
     * @brief Gets the aspect type string associated with this class
     *
     * @return std::string This class' aspect type string.
     *
     */
    inline static std::string getSimObjectAspectTypeName() { return "CollisionSound"; }

    /**
     * @brief Constructs this aspect from its JSON description
     *
     * Example:
     * ```json
     * { "type": "CollisionSound" }
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
     * @brief Sound played when a collision is detected.
     *
     */
    std::shared_ptr<ToyMaker::Sound> mSound { nullptr };

    /**
     * @brief Channel on which the collision sound will be played
     *
     */
    std::unique_ptr<ToyMaker::SoundChannel> mChannel { nullptr };

    /**
     * @brief Constructs a new CollisionSound object.
     *
     */
    CollisionSound(): ToyMaker::SimObjectAspect<CollisionSound>{0} {}

    /**
     * @brief Prints details about the collision that was just observed.
     *
     */
    void soundCollision(const ToyMaker::PhysicsSystem::SignalCollidedData& collisionData);

    /**
     * @brief Listens for collision events that this object participates in, reports them to 
     * printCollision().
     *
     */
    ToyMaker::SignalObserver<ToyMaker::PhysicsSystem::SignalCollidedData> mObserveCollided {
        *this, "CollisionObserved",
        [this](ToyMaker::PhysicsSystem::SignalCollidedData signalData) {
            soundCollision(signalData);
        }
    };

    /**
     * @brief Connects to collision signal for this entity advertised by the physics system.
     *
     */
    void onActivated() override;
};


std::shared_ptr<ToyMaker::BaseSimObjectAspect> CollisionSound::create(const nlohmann::json& jsonAspectProperties) {
    (void) jsonAspectProperties; // prevent unused parameter warning
    return std::shared_ptr<CollisionSound>(new CollisionSound{});
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> CollisionSound::clone() const {
    return std::shared_ptr<CollisionSound>(new CollisionSound{});
}

void CollisionSound::onActivated() {
    connect(
        ToyMaker::PhysicsSystem::SignalCollidedPrefix + std::to_string(getEntityID()),
        "CollisionObserved",
        *getWorld().lock()->getSystem<ToyMaker::PhysicsSystem>()
    );

    mSound = ToyMaker::ResourceDatabase::GetRegisteredResource<ToyMaker::Sound>("SnapSound");
    mChannel = getSimObject().getWorld().lock()->getSystem<ToyMaker::SoundSystem>()->createChannel();
    mChannel->setSound(*mSound);
    mChannel->setGain(5.f);
    mChannel->play();
}

void CollisionSound::soundCollision(const ToyMaker::PhysicsSystem::SignalCollidedData& collisionData) {
    std::cout << "Colliding entities: ("
        << std::to_string(collisionData.first.first()) << ", "
        << std::to_string(collisionData.first.second()) << ")\n";
    std::cout << "Penetration depth: "
        << std::to_string(collisionData.second.mContactA.mPenetrationDepth) << "\n";

    if(collisionData.second.mContactA.mPenetrationDepth <= .005f) {
        return;
    }

    const auto camera { getLocalViewport().getActiveCamera() };
    const auto viewMatrix { camera->getComponent<ToyMaker::CameraProperties>().mViewMatrix };
    const glm::vec3 viewPosition { viewMatrix * glm::vec4 { collisionData.second.mContactA.mPoint, 1.f } };
    mChannel->setGain(5.f);
    mChannel->setPosition(viewPosition);
    mChannel->play();
    assert(mChannel->isPlaying() && "Collision sound effect isn't playing");
}

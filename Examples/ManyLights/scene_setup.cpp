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
class SceneSetup: public ToyMaker::SimObjectAspect<SceneSetup> {
public:
    /**
     * @brief Gets the aspect type string associated with this class.
     *
     * @return std::string This class' aspect type string.
     *
     */
    inline static std::string getSimObjectAspectTypeName() {
        return "SceneSetup";
    }

    /**
     * @brief Constructs this aspect from its JSON description.
     *
     * Example:
     *
     * ```json
     * { "type": "SceneSetup" }
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
     * @brief Constructs a new SceneSetup aspect.
     *
     */
    SceneSetup(): ToyMaker::SimObjectAspect<SceneSetup>{0} {}

    void onActivated() override;
};

std::shared_ptr<ToyMaker::BaseSimObjectAspect> SceneSetup::create(const nlohmann::json& jsonAspectProperties) {
    (void) jsonAspectProperties;
    return std::shared_ptr<SceneSetup>(new SceneSetup());
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> SceneSetup::clone() const {
    return std::shared_ptr<SceneSetup>(new SceneSetup());
}


void SceneSetup::onActivated() {
    std::vector<std::shared_ptr<ToyMaker::SimObject>> toAdd {};
    const std::size_t iterations { 20 };
    for(std::size_t i { 0 }; i < iterations; ++i) {
        for(std::size_t j { 0 }; j < iterations; ++j) {
            std::shared_ptr<ToyMaker::SimObject> newNode {
                ToyMaker::SimObject::copy(
                    ToyMaker::ResourceDatabase::GetRegisteredResource<ToyMaker::SimObject>(
                        "EagleAsset"
                    )
                )
            };
            ToyMaker::Placement placement { newNode->getComponent<ToyMaker::Placement>() };
            placement.mPosition = {
                -static_cast<float>(iterations) + i * 2.f,
                0.f,
                -static_cast<float>(iterations) + j * 2.f,
                1.f
            };
            newNode->updateComponent(placement);
            newNode->setName("eagle__"
                + std::to_string(i) + "_"
                + std::to_string(j)
            );
            toAdd.push_back(newNode);
        }
    }
    for(auto node: toAdd) {
        getSimObject().addNode(node, "/");
    }
}


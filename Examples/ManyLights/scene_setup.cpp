#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <toymaker/engine/signals.hpp>
#include <toymaker/engine/sim_system.hpp>
#include <toymaker/engine/light.hpp>
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
    const std::size_t iterations { 21 };
    const float spacing { 3.f };

    const float deltaAngle { glm::pi<float>() * .5f / iterations };

    const float initialAngle { deltaAngle * .5f };
    const float initialOffset { -static_cast<float>(iterations) * spacing * .5f  };

    for(std::size_t i { 0 }; i < iterations; ++i) {
        const float phi { initialAngle + i * deltaAngle };

        for(std::size_t j { 0 }; j < iterations; ++j) {
            const float theta { (initialAngle + j * deltaAngle) };

            std::shared_ptr<ToyMaker::SimObject> newNode {
                ToyMaker::SimObject::copy(
                    ToyMaker::ResourceDatabase::GetRegisteredResource<ToyMaker::SimObject>(
                        "EagleAsset"
                    )
                )
            };

            ToyMaker::Placement placement { newNode->getComponent<ToyMaker::Placement>() };
            placement.mPosition = {
                initialOffset + i * spacing,
                0.f,
                initialOffset + j * spacing,
                1.f
            };
            newNode->updateComponent(placement);

            ToyMaker::LightEmissionData light { newNode->getNode("/light/")->getComponent<ToyMaker::LightEmissionData>() };
            const glm::vec3 selectedColor {
                glm::sin(theta) * glm::cos(phi),
                glm::sin(theta) * glm::sin(phi),
                glm::cos(theta),
            };
            light.mDiffuseColor = (
                glm::dot(light.mDiffuseColor, glm::vec4{ selectedColor, 1.f })
                * glm::vec4{ selectedColor, 1.f }
            );
            newNode->getNode("/light/")->updateComponent(light);

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


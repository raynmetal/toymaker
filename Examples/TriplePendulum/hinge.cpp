#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <toymaker/engine/signals.hpp>
#include <toymaker/engine/sim_system.hpp>
#include <toymaker/engine/physics/system.hpp>

/**
 * @ingroup Examples
 *
 * @brief Aspect which registers a set of rotation and distance constraints on activation to simulate a hinge.
 *
 */
class Hinge: public ToyMaker::SimObjectAspect<Hinge> {
public:

    /**
     * @brief The possible planes of rotation relative to this body, enumerated.
     *
     */
    enum Plane: uint8_t {
        XY,
        XZ,
        YZ,
    };

    /**
     * @brief Gets the aspect type string associated with this class
     *
     * @return std::string This class' aspect type string.
     *
     */
    inline static std::string getSimObjectAspectTypeName() { return "Hinge"; }

    /**
     * @brief Constructs this aspect from its JSON description
     *
     * Example:
     * ```json
     * { "type": "Hinge" }
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
     * @brief The point at which the other object is attached to this object
     *
     */
    glm::vec3 mAttachmentPoint { 0.f };

    /**
     * @brief The attachment point of the other object participating in this constraint.
     *
     * @see mAttachmentPoint
     * @see mOther
     */
    glm::vec3 mAttachmentPointOther { 0.f };

    /**
     * @brief Distance offsets of the other object's attachment point relative to this object's attachment point.
     *
     */
    glm::vec3 mOffsets { 0.f };

    /**
     * @brief The plane, relative to this object's orientation, that permits free rotation.
     *
     */
    Plane mFreePlane { XY };

    /**
     * @brief The (path to the) other participant's scene node in this constraint, relative to this node's parent viewport.
     *
     */
    std::string mOther {};

    /**
     * @brief Constraints registered by this aspect with the physics system.
     *
     */
    std::set<ToyMaker::PhysicsSystem::ConstraintID> mConstraints {};

    /**
     * @brief Constructs a new Gravity object.
     *
     */
    Hinge(): ToyMaker::SimObjectAspect<Hinge>{0} {}

    /**
     * @brief Registers a hinge joint composed of 2 rotation constraints and 3 position constraints
     *
     */
    void onActivated() override;

    /**
     * @brief De-registers hinge joint constraints
     *
     */
    void onDeactivated() override;
};

NLOHMANN_JSON_SERIALIZE_ENUM(Hinge::Plane, {
    { Hinge::Plane::XY, "xy" },
    { Hinge::Plane::XZ, "xz" },
    { Hinge::Plane::YZ, "yz" },
});

std::shared_ptr<ToyMaker::BaseSimObjectAspect> Hinge::create(const nlohmann::json& jsonAspectProperties) {
    std::shared_ptr<Hinge> newHinge(new Hinge{});
    newHinge->mAttachmentPoint = {
        jsonAspectProperties.at("attachment")[0],
        jsonAspectProperties.at("attachment")[1],
        jsonAspectProperties.at("attachment")[2],
    };
    newHinge->mAttachmentPointOther = {
        jsonAspectProperties.at("attachment_other")[0],
        jsonAspectProperties.at("attachment_other")[1],
        jsonAspectProperties.at("attachment_other")[2],
    };
    jsonAspectProperties.at("free_plane").get_to(newHinge->mFreePlane);
    newHinge->mOther = jsonAspectProperties.at("other");
    newHinge->mOffsets = {
        jsonAspectProperties.at("offsets")[0],
        jsonAspectProperties.at("offsets")[1],
        jsonAspectProperties.at("offsets")[2],
    };
    return newHinge;
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> Hinge::clone() const {
    std::shared_ptr<Hinge> newHinge(new Hinge{});
    newHinge->mAttachmentPoint = mAttachmentPoint;
    newHinge->mAttachmentPointOther = mAttachmentPointOther;
    newHinge->mFreePlane = mFreePlane;
    newHinge->mOther = mOther;
    newHinge->mOffsets = mOffsets;
    return newHinge;
}

void Hinge::onActivated() {
    std::shared_ptr<ToyMaker::SceneNodeCore> other {
        getSimObject().getLocalViewport()->getNode(mOther)
    };

    // const ToyMaker::Constraint1DOFConfig configDistanceX {
    // };
    // const ToyMaker::Constraint1DOFConfig configDistanceY {
    // };
    // const ToyMaker::Constraint1DOFConfig configDistanceZ {
    // };
    //
    // const ToyMaker::Constraint1DOFConfig configRotation0 {
    // };
    // const ToyMaker::Constraint1DOFConfig configRotation1 {
    // };

    std::cout << "Hinge activated!\n";
}

void Hinge::onDeactivated() {
    std::cout << "Hinge deactivated!\n";
    mConstraints.clear();
}


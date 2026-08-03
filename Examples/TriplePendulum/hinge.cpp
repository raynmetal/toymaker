#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include <toymaker/engine/signals.hpp>
#include <toymaker/engine/sim_system.hpp>
#include <toymaker/engine/physics/system.hpp>
#include <toymaker/engine/util.hpp>

/**
 * @ingroup Examples
 *
 * @brief Aspect which registers a set of rotation and distance constraints on activation to simulate a hinge.
 *
 */
class Hinge: public ToyMaker::SimObjectAspect<Hinge> {
public:

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
     * @brief Rotation taking vector from constraint space to this object's local space
     *
     */
    glm::quat mOrientation { 1.f, 0.f, 0.f, 0.f };

    /**
     * @brief Rotation taking vector from constraint space to attached object's local space.
     *
     */
    glm::quat mOrientationOther { 1.f, 0.f, 0.f, 0.f };

    /**
     * @brief Distance offsets of the other object's attachment point relative to this object's attachment point.
     *
     */
    glm::vec3 mOffsets { 0.f };

    /**
     * @brief The direction vector relative to this object to be brought into alignment with the other object's vector.
     *
     */
    glm::vec3 mAxis { 0.f, 0.f, 1.f };

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
    newHinge->mOrientation = {
        jsonAspectProperties.at("orientation")[0],
        jsonAspectProperties.at("orientation")[1],
        jsonAspectProperties.at("orientation")[2],
        jsonAspectProperties.at("orientation")[3],
    };
    // invert to find constraint->local rotation
    newHinge->mOrientation = glm::inverse(glm::normalize(newHinge->mOrientation));
    newHinge->mOrientationOther = {
        jsonAspectProperties.at("orientation_other")[0],
        jsonAspectProperties.at("orientation_other")[1],
        jsonAspectProperties.at("orientation_other")[2],
        jsonAspectProperties.at("orientation_other")[3],
    };
    // invert to find constraint->local rotation
    newHinge->mOrientationOther = glm::inverse(glm::normalize(newHinge->mOrientationOther));
    newHinge->mAxis = {
        jsonAspectProperties.at("axis")[0],
        jsonAspectProperties.at("axis")[1],
        jsonAspectProperties.at("axis")[2],
    };
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
    newHinge->mOrientation = mOrientation;
    newHinge->mOrientationOther = mOrientation;
    newHinge->mAxis = mAxis;
    newHinge->mOther = mOther;
    newHinge->mOffsets = mOffsets;
    return newHinge;
}

void Hinge::onActivated() {
    std::shared_ptr<ToyMaker::SceneNodeCore> other {
        getSimObject().getLocalViewport()->getNode(mOther)
    };

    const ToyMaker::Constraint1DOFConfig configDistanceX {
        .mAxis { 1.f, 0.f, 0.f },
        .mBoundLower { mOffsets.x },
        .mBoundUpper { mOffsets.x },
    };
    const ToyMaker::Constraint1DOFConfig configDistanceY {
        .mAxis { 0.f, 1.f, 0.f },
        .mBoundLower { mOffsets.y },
        .mBoundUpper { mOffsets.y },
    };
    const ToyMaker::Constraint1DOFConfig configDistanceZ {
        .mAxis { 0.f, 0.f, 1.f },
        .mBoundLower { mOffsets.z },
        .mBoundUpper { mOffsets.z },
    };

    const auto tangents { ToyMaker::getTangents(mAxis) };
    const ToyMaker::Constraint1DOFConfig configRotation0 {
        .mAxis { tangents.first },
        .mBoundLower { 0.f },
        .mBoundUpper { 0.f },
    };
    const ToyMaker::Constraint1DOFConfig configRotation1 {
        .mAxis { tangents.second },
        .mBoundLower { 0.f },
        .mBoundUpper { 0.f },
    };

    // register distance constraints
    auto physics { getSimObject().getWorld().lock()->getSystem<ToyMaker::PhysicsSystem>() };
    const auto entityThis { getSimObject().getEntityID() };
    const auto entityOther { other->getEntityID() };
    mConstraints.insert(
        physics->registerConstraint<ToyMaker::ConstraintDistance1D>(
            configDistanceX,
            {
                { entityThis, {
                    mOrientation,
                    mAttachmentPoint
                } },
                { entityOther, {
                    mOrientationOther,
                    mAttachmentPointOther
                } },
            },
            0.f
        )
    );
    mConstraints.insert(
        physics->registerConstraint<ToyMaker::ConstraintDistance1D>(
            configDistanceY,
            {
                { entityThis, {
                    mOrientation,
                    mAttachmentPoint
                } },
                { entityOther, {
                    mOrientationOther,
                    mAttachmentPointOther
                } },
            },
            0.f
        )
    );
    mConstraints.insert(
        physics->registerConstraint<ToyMaker::ConstraintDistance1D>(
            configDistanceZ,
            {
                { entityThis, {
                    mOrientation,
                    mAttachmentPoint
                } },
                { entityOther, {
                    mOrientationOther,
                    mAttachmentPointOther
                } },
            },
            0.f
        )
    );

    // register rotation constraints
    mConstraints.insert(
        physics->registerConstraint<ToyMaker::ConstraintRotation1D>(
            configRotation0,
            {
                { entityThis, {
                    mOrientation,
                    mAxis
                } },
                { entityOther, {
                    mOrientationOther,
                    mAxis
                } },
            },
            0.f
        )
    );
    mConstraints.insert(
        physics->registerConstraint<ToyMaker::ConstraintRotation1D>(
            configRotation1,
            {
                { entityThis, {
                    mOrientation,
                    mAxis
                } },
                { entityOther, {
                    mOrientationOther,
                    mAxis
                } },
            },
            0.f
        )
    );
    std::cout << "Hinge activated!\n";
}

void Hinge::onDeactivated() {
    std::cout << "Hinge deactivated!\n";
    mConstraints.clear();
}


/**
 * @ingroup ToyMakerPhysics
 * @file physics/system.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief File containing definitions of types used in this engine's physics system implementation
 * @version 0.2.3
 * @date 2026-05-23
 *
 */

/**
 * @defgroup ToyMakerPhysics Physics properties, maths, contraints, system, etc.
 * @ingroup ToyMakerEngine
 *
 */

#ifndef TOYMAKERENGINE_PHYSICSTYPES_H
#define TOYMAKERENGINE_PHYSICSTYPES_H

#include <string>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "../spatial_query/types.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToyMakerPhysics
     *
     * @brief Component representing the physics state of the body
     * it's attached to at some particular point in time.
     *
     * All forces are expressed relative to the frame of the body.
     */
    struct PhysicsLocal {

        /**
         * @brief Fetches the component type string associated with this class.
         *
         * @return std::string The component type string of this class
         */
        inline static std::string getComponentTypeName() { return "PhysicsState"; }

        /**
         * @brief The sum of all the forces acting on this object's center of mass,
         * causing it to move through space.
         *
         * Expressed as a vector in this object's local frame.
         *
         */
        glm::vec3 mForce { 0.f };

        /**
         * @brief Proportional to sum of all forces acting perpendicular to the vector going
         * from the point at which the forces are being applied to this object' center of
         * mass, causing it to rotate about its axis.
         *
         * Expressed as a vector in this object's local frame.
         *
         */
        glm::vec3 mTorque { 0.f };

        /**
         * @brief The velocity of this object.
         *
         * Expressed as a vector in this object's local frame.
         *
         */
        glm::vec3 mVelocity { 0.f };

        /**
         * @brief The angular velocity of this object.
         *
         * Expressed as a vector in this object's local frame.
         *
         */
        glm::vec3 mAngularVelocity { 0.f };

        /**
         * @brief This object's resistance to rotational change.
         *
         * Expressed in terms of the object's local frame, and should be derived from
         * the objects mass and shape.
         *
         */
        glm::vec3 mRotationalInertia { 1.f };

        /**
         * @brief How heavy and resistant to translational change this object is.
         *
         */
        float mMass { 1.f };

        /**
         * @brief Applies a force `force` at position `atPosition`, updating the torque
         * and central force of an object whose state is represented by `physics`.
         *
         * @warning All forces applied to an object are cleared once per simulation frame.
         *
         * @param force The amount of force being applied to an object
         * @param atPosition The world position at which the force is applied
         * @param bounds The bounds of the object to which the force is applied, used
         * to find local frame force and position equivalents
         */
        void applyForceGlobal(const glm::vec3& force, const glm::vec3& atPosition, const ObjectBounds& bounds);

        /**
         * @brief Applies a force `force` at position `atPosition`, updating the torque
         * and central force of an object whose state is represented by `physics`.
         *
         * @warning All forces applied to an object are cleared once per simulation frame.
         *
         * @param force The amount of force being applied to an object
         * @param atPosition The object-local position at which the force is applied
         */
        void applyForceLocal(const glm::vec3& force, const glm::vec3& atPosition);
    };

    /**
     * @ingroup ToyMakerPhysics
     * @brief Base class for constraints
     *
     * Ensures that each constraint provides a method for preprocessing constraint data,
     * and one for applying the constraint.
     *
     * Also contains a table of entities that are participating in the constraint.
     */
    class BaseConstraint {
    private:

        /**
         * @brief Value greater than equal to zero, inverse of the physical stiffness
         * of this constraint.
         *
         * Part of extended position based dynamics implementation.
         *
         * A compliance of 0 implies infinite stiffness.
         *
         */
        float mCompliance { 0.f };

        /**
         *
         * @brief The Lagrange multiplier, computed every substep since the start of the
         * physics simulation till the current time.
         *
         * Allows position and orientation corrections applied by this constraint to be
         * independent of the length of substep.
         *
         */
        float mLagrangeMultiplier { 0.f };

    protected:

        /**
         * @brief Adds a delta to the lagrange value based on some parameters.
         *
         */
        void applyLagrangeDelta(float delta);

        /**
         * @brief Gets the current Lagrange value for this constraint
         *
         */
        float getLagrange() const;

        /**
         * @brief Initializes this constraint.
         */
        BaseConstraint(float compliance) {
            setCompliance(compliance);
        }

    public:
        using ParticipantID = std::size_t;
        using ParticipantTable = std::unordered_map<
            ParticipantID,
            std::pair<
                std::reference_wrapper<ObjectBounds>,
                std::reference_wrapper<PhysicsLocal>
            >
        >;

        /**
         * @brief Gets the current compliance value for this constraint
         *
         */
        float getCompliance() const;

        /**
         * @brief Applies constraint to current set of bounds and physics states.
         *
         * Will update values stored in ObjectBounds and PhysicsLocal to maintain the
         * constraint and update its own compliance parameters
         *
         */
        virtual void applyConstraint(
            const ParticipantTable& states,
            float substepSeconds
        ) = 0;

        /**
         * @brief Sets the compliance value for this constraint
         *
         */
        void setCompliance(float newCompliance);

        /**
         * @brief Sets the Lagrange multiplier to zero
         *
         */
        inline void resetLagrange() { mLagrangeMultiplier = 0; }

        virtual ~BaseConstraint() {};
    };

    /**
     * @brief Subclass implementation for any constraint which takes data of type TParameter
     *
     * @tparam TParameter A parameter specific to this constraint.
     *
     */
    template <typename TParameter>
    class Constraint: public BaseConstraint {

    private:
        /**
         * @brief A set of parameters associated with each entity
         *
         * These parameters store values that augment the values known by the engine -- i.e.,
         * physics state and object bounds -- but are specific to the constraint itself
         */
        std::unordered_map<ParticipantID, TParameter> mParameters {};

    protected:
        /**
         * @brief Initializes this constraint with some initial compliance value.
         *
         */
        Constraint(float compliance): BaseConstraint { compliance } {}

        /**
         * @brief Returns all parameters known to this constraint.
         *
         */
        const std::unordered_map<ParticipantID, TParameter>& getParameters() const;

    public:

        /**
         * @brief Adds a parameter for this constraint.
         *
         */
        void setParameter(ParticipantID participant, const TParameter& parameter);

        /**
         * @brief Gets parameter belonging to a particular constraint participant
         *
         */
        inline TParameter getParameter(ParticipantID participant) const {
            return mParameters.at(participant);
        }

        /**
         * @brief Removes a parameter belonging to a particular constraint participant
         *
         */
        void removeParameter(ParticipantID participant);
    };

    struct CollisionConstraintData {
        Contact mContact;
        float mInverseMass;
        glm::mat3 mRotationalInertia;
    };

    /**
     * @brief Constraint requiring exactly two parameters representing colliding rigid
     * bodies.
     *
     * Repositions objects such that they no longer intersect along the axis of collision.
     *
     */
    class CollisionConstraint: protected Constraint<CollisionConstraintData> {
    private:
        /**
         * @brief Whether or not two objects are currently intersecting (and therefore whether
         * they should be separated)
         *
         */
        bool mCollided { false };

    public:
        /**
         * @brief Initializes constraint with collision data from two potentially intersecting objects
         *
         */
        CollisionConstraint(
            const Collision& collision,
            const PhysicsLocal& physicsA,
            const ObjectBounds& boundsA,
            const PhysicsLocal& objectB,
            const ObjectBounds& boundsB
        );

        /**
         * @brief Updates collision data to value to be used for current substep
         *
         */
        void updateCollisionData(
            const Collision& collision,
            const PhysicsLocal& physicsA,
            const ObjectBounds& boundsA,
            const PhysicsLocal& physicsB,
            const ObjectBounds& boundsB
        );

        /**
         * @brief Separates intersecting/colliding objects.
         *
         */
        void applyConstraint(
            const ParticipantTable& states,
            float substepSeconds
        ) override;
    };

    inline void from_json(
        const nlohmann::json& json,
        PhysicsLocal& physics
    ) {
        assert(json.at("type") == PhysicsLocal::getComponentTypeName() && "Incorrect type property for an physics property component");
        const float mass { json.at("mass") };
        physics = { .mMass { mass } };

        if(json.find("velocity") != json.end()) {
            physics.mVelocity = glm::vec3 {
                json.at("velocity")[0],
                json.at("velocity")[1],
                json.at("velocity")[2],
            };
        }
        if(json.find("angular_velocity") != json.end()) {
            physics.mAngularVelocity = glm::vec3 {
                json.at("angular_velocity")[0],
                json.at("angular_velocity")[1],
                json.at("angular_velocity")[2],
            };
        }
        if(json.find("force") != json.end()) {
            physics.mForce = glm::vec3 {
                json.at("force")[0],
                json.at("force")[1],
                json.at("force")[2],
            };
        }
        if(json.find("torque") != json.end()) {
            physics.mForce = glm::vec3 {
                json.at("torque")[0],
                json.at("torque")[1],
                json.at("torque")[2],
            };
        }
    }

    inline void to_json(
        nlohmann::json& json,
        const PhysicsLocal& physics
    ) {
        json = {
            { "type", PhysicsLocal::getComponentTypeName() },
            { "mass", physics.mMass },
        };
    }

    template<typename TParameter>
    void Constraint<TParameter>::setParameter(BaseConstraint::ParticipantID participant, const TParameter& parameter) {
        mParameters[participant] = parameter;
    }

    template<typename TParameter>
    void Constraint<TParameter>::removeParameter(BaseConstraint::ParticipantID participant) {
        mParameters.erase(participant);
    }

    template <typename TParameter>
    const std::unordered_map<BaseConstraint::ParticipantID, TParameter>& Constraint<TParameter>::getParameters() const {
        return mParameters;
    }
}

#endif


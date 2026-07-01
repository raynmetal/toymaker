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

#include <array>
#include <string>
#include <utility>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "../spatial_query/types.hpp"

namespace ToyMaker {
    struct PhysicsState;

    /**
     * @ingroup ToyMakerPhysics
     *
     * @brief Computes the generalized inverse mass used for positional corrections applied by
     * constraints.
     *
     * @param object The bounds indicating the current position and orientation of the object in world space
     * @param inverseMass 1 divided by the mass of the object.
     * @param inverseRotationalInertia The inverse of the rotational inertia of this object in its local frame
     * @param correctionPoint Location of the point of application of the correction
     * @param correctionGradient The direction of the greatest increase in error with respect to
     * the desired object state in terms of its position.
     *
     */
    float computeGeneralizedInverseMassPositional(
        const ObjectBounds& object,
        float inverseMass,
        const glm::vec3& inverseRotationalInertia,
        const glm::vec3& correctionPoint,
        const glm::vec3& correctionGradient
    );

    /**
     * @ingroup ToyMakerPhysics
     *
     * @brief Computes the generalized inverse mass used by constraints to apply strictly rotational
     * corrections
     *
     * @param inverseMass 1 divided by the mass of the object.
     * @param inverseRotationalInertia The inverse of the rotational inertia of the object in its local frame.
     * @param correction The vector representing the desired rotational correction, usually a product
     * of an impulse vector and its offset from the object's center-of-mass.  Moving along its direction 
     * _increases_ the magnitude of the correction required
     *
     */
    float computeGeneralizedInverseMassRotational(
        const ObjectBounds& object,
        const glm::vec3& inverseRotationalInertia,
        const glm::vec3& correctionRotational
    );

    /**
     * @ingroup ToyMakerPhysics
     *
     * @brief Returns an objects rotation tensor in the global frame given its tensor in the local frame
     * according to its current orientation.
     *
     */
    glm::mat3 computeInertiaRotationalWorld(const glm::vec3& rotationalInertiaLocal, const glm::quat& orientation);

    /**
     * @ingroup ToyMakerPhysics
     *
     * @brief Returns object bounds in state it would be post application of
     * a positional impulse.
     *
     */
    ObjectBounds impulseApplied(
        ObjectBounds object,
        float massInverse,
        const glm::vec3& rotationalInertiaInverse,
        const glm::vec3& impulsePositional,
        const glm::vec3& impulsePoint
    );

    /**
     * @ingroup ToyMakerPhysics
     *
     * @brief Returns object bounds in state it would be post application of
     * a rotational impulse.
     *
     */
    ObjectBounds impulseApplied(
        ObjectBounds object,
        const glm::vec3& rotationalInertiaInverse,
        const glm::vec3& impulseRotational
    );

    /**
     * @ingroup ToyMakerPhysics
     *
     * @brief Returns new physics state after application of global impulse
     *
     */
    PhysicsState impulseApplied(
        const ObjectBounds& object,
        PhysicsState physics,
        const glm::vec3& impulsePositional,
        const glm::vec3& impulsePoint
    );

    /**
     * @ingroup ToyMakerPhysics
     *
     * @brief Returns new physics state after application of global angular impulse
     */
    PhysicsState impulseApplied(
        const ObjectBounds& object,
        PhysicsState physics,
        const glm::vec3& impulseRotational
    );

    /**
     * @ingroup ToyMakerPhysics
     *
     * @brief Component representing the physics state of the body
     * it's attached to at some particular point in time.
     *
     * All vectors are expressed in terms of world space coordinates.
     *
     * Rotational inertia alone is stored in the object's local frame.
     */
    struct PhysicsState {
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
         * Expressed as a vector in the world frame.
         *
         */
        glm::vec3 mForce { 0.f };

        /**
         * @brief Proportional to sum of all forces acting perpendicular to the vector going
         * from the point at which the forces are being applied to this object' center of
         * mass, causing it to rotate about its axis.
         *
         * Expressed as a vector in the world frame.
         *
         */
        glm::vec3 mTorque { 0.f };

        /**
         * @brief The velocity of this object.
         *
         * Expressed as a vector in the world frame.
         *
         */
        glm::vec3 mVelocity { 0.f };

        /**
         * @brief The angular velocity of this object.
         *
         * Expressed as a vector in the world frame.
         *
         */
        glm::vec3 mAngularVelocity { 0.f };

        /**
         * @brief The inverse of this object's resistance to rotational change.
         *
         * Expressed in terms of the object's local frame, and should be derived from
         * the objects mass and shape.
         *
         */
        glm::vec3 mRotationalInertiaInverse { 1.f };

        /**
         * @brief The inverse of this object's mass.
         *
         */
        float mMassInverse { 1.f };

        /**
         * @brief The friction coefficient of the force that prevents relative motion between the
         * surface of two objects when they are stationary.
         *
         */
        float mCoefficientFrictionStatic { 0.f };

        /**
         * @brief The friction coefficient of the force that hinders motion between the surface of
         * two objects when they are moving relative to each other.
         *
         */
        float mCoefficientFrictionDynamic { 0.f };

        /**
         * @brief The fraction of the net kinetic energy prior to a collision retained by a pair of objects after
         * the collision has taken place.
         *
         * Used to limit the separation velocity between the contact points of two objects participating in
         * a collision.
         */
        float mCoefficientRestitution { .8f };

        /**
         * @brief Applies a force `force` at position `atPosition`, updating the torque
         * and central force of an object whose state is represented by `physics`.
         *
         * @warning All forces applied to an object are cleared once per simulation frame.
         *
         * @param force The force being applied to the object
         * @param atPosition The world position at which the force is applied
         */
        void applyForceGlobal(const glm::vec3& force, const glm::vec3& atPosition, const ObjectBounds& bounds);

        /**
         * @brief Applies a force `force` at position `atPosition`, updating the torque
         * and central force of an object whose state is represented by `physics`.
         *
         * @warning All forces applied to an object are cleared once per simulation frame.
         *
         * @param force The force being applied to the object
         * @param atPosition The object-local position at which the force is applied
         * @param bounds The bounds of the object to which the force is applied, used
         * to find global frame force and position equivalents
         */
        void applyForceLocal(const glm::vec3& force, const glm::vec3& atPosition, const ObjectBounds& bounds);

        /**
         * @brief Gets the mass of this object
         *
         */
        inline float getMass() const {
            if(mMassInverse == 0.f) {
                return std::numeric_limits<float>::max();
            }
            return 1.f / mMassInverse;
        }

        /**
         * @brief Sets the mass of this object
         *
         */
        inline void setMass(float mass) {
            assert(isNumber(mass) && isPositiveStrict(mass) && "Mass must be a valid positive number");
            if(mass == std::numeric_limits<float>::max()) {
                mMassInverse = 0.f;
                return;
            }
            mMassInverse = 1.f / mass;
        }

        /**
         * @brief Gets the rotational inertia for this object along each axis in the object's local frame
         *
         */
        inline glm::vec3 getRotationalInertia() const {
            return glm::vec3 {
                mRotationalInertiaInverse.x == 0.f? std::numeric_limits<float>::max(): 1.f / mRotationalInertiaInverse.x,
                mRotationalInertiaInverse.y == 0.f? std::numeric_limits<float>::max(): 1.f / mRotationalInertiaInverse.y,
                mRotationalInertiaInverse.z == 0.f? std::numeric_limits<float>::max(): 1.f / mRotationalInertiaInverse.z,
            };
        }

        /**
         * @brief Sets the rotational inertia for this object along each axis in the object's local frame
         *
         */
        inline void setRotationalInertia(const glm::vec3& rotationalInertia) {
            assert(
                isNumber(rotationalInertia) && isPositiveStrict(rotationalInertia)
                && "Rotational inertia must be valid positive number for each axis"
            );
            mRotationalInertiaInverse = 1.f / rotationalInertia;
        }

        /**
         * @brief Sets the coefficient of restitution for this object.
         *
         */
        inline void setCoefficientRestitution(float newCoefficient) {
            assert(
                isNonNegative(newCoefficient) && newCoefficient <= 1.f
                && "Restitution coefficient must be non negative and cannot exceed 1"
            );
            mCoefficientRestitution = newCoefficient;
        }
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

    protected:

        /**
         * @brief Initializes this constraint.
         */
        BaseConstraint(float compliance) { setCompliance(compliance); }

    public:
        using ParticipantID = std::size_t;
        using ParticipantTable = std::unordered_map<
            ParticipantID,
            std::pair<
                std::reference_wrapper<ObjectBounds>,
                std::reference_wrapper<PhysicsState>
            >
        >;

        /**
         * @brief Gets the current compliance value for this constraint
         *
         */
        float getCompliance() const;

        /**
         * @brief Applies positional constraint to current set of bounds and physics states.
         *
         * Will update values stored in ObjectBounds and PhysicsLocal to maintain the
         * constraint and update its own compliance parameters
         *
         */
        virtual void applyConstraintPosition(
            const ParticipantTable& states,
            float substepSeconds
        ) {}

        /**
         * @brief Applies velocity-based constraint to current set of bounds and physics states.
         *
         * Will update values stored in ObjectBounds and PhysicsLocal to maintain the
         * constraint and update its own compliance parameters
         *
         */
        virtual void applyConstraintVelocity(
            const ParticipantTable& states,
            float substepSeconds
        ) {}

        /**
         * @brief Resets all lagrange multipliers, preparing them for a new sequence of
         * constraint solve substeps.
         *
         */
        virtual void resetLagrange() {}

        /**
         * @brief Sets the compliance value for this constraint
         *
         */
        void setCompliance(float newCompliance);

        virtual ~BaseConstraint() {};
    };


    /**
     * @brief Any constraint storing LagrangeCount correction multipliers.
     *
     * @tparameter LagrangeCount The number of Lagrange multipliers in usage by this constraint
     */
    template<uint8_t LagrangeCount>
    class Constraint: public BaseConstraint {
    private:
        /**
         * @brief Private implementation for each lagrange multiplier index in need of
         * a reset
         *
         */
        template<uint8_t... ints>
        void resetLagrange(std::integer_sequence<uint8_t, ints...> index);

        /**
         *
         * @brief The Lagrange multiplier, computed every substep since the start of the
         * physics simulation till the current time.
         *
         * Allows position and orientation corrections applied by this constraint to be
         * independent of the length of substep.
         *
         */
        std::array<float, LagrangeCount> mLagrangeMultipliers { 0.f };

    protected:
        /**
         * @brief Initializes this constraint with some initial compliance value.
         *
         */
        Constraint(float compliance): BaseConstraint { compliance } {}

    public:
        /**
         * @brief Adds a delta to a lagrange value located at `index`
         *
         */
        void applyLagrangeDelta(float delta, uint8_t index);

        /**
         * @brief Gets the current Lagrange multiplier values for this constraint
         *
         */
        const std::array<float, LagrangeCount>& getLagrange() const;

        /**
         * @brief Sets all lagrange multipliers to 0 in preparation for the next physics update
         */
        void resetLagrange() override;
    };

    /**
     * @brief Subclass implementation for any constraint which takes data of type TParameter
     *
     * @tparam TParameter A parameter specific to this constraint.
     * @tparam LagrangeCount The number of Lagrange multipliers used by this constraint
     *
     */
    template <typename TParameter, uint8_t LagrangeCount>
    class ParametrizedConstraint: public Constraint<LagrangeCount> {
    private:
        /**
         * @brief A set of parameters associated with each entity
         *
         * These parameters store values that augment the values known by the engine -- i.e.,
         * physics state and object bounds -- but are specific to the constraint itself
         */
        std::unordered_map<BaseConstraint::ParticipantID, TParameter> mParameters {};

    protected:
        /**
         * @brief Initializes this constraint with some initial compliance value.
         *
         */
        ParametrizedConstraint(float compliance): Constraint<LagrangeCount> { compliance } {}

    public:
        /**
         * @brief Adds a parameter for this constraint.
         *
         */
        void setParameter(BaseConstraint::ParticipantID participant, const TParameter& parameter);

        /**
         * @brief Gets parameter belonging to a particular constraint participant
         *
         */
        inline TParameter getParameter(BaseConstraint::ParticipantID participant) const {
            return mParameters.at(participant);
        }

        /**
         * @brief Removes a parameter belonging to a particular constraint participant
         *
         */
        void removeParameter(BaseConstraint::ParticipantID participant);

        /**
         * @brief Returns all parameters known to this constraint.
         *
         */
        const std::unordered_map<BaseConstraint::ParticipantID, TParameter>& getParameters() const;
    };


    /**
     * @ingroup ToyMakerPhysics
     *
     * @brief Constraint data used for the computation of a fixed constraint corrections.
     *
     */
    struct PinConstraintData {
        /**
         * @brief The desired origin for the object affected by this constraint.
         *
         */
        glm::vec3 mOrigin;

        /**
         * @brief The desired orientation for the object affected by this constraint.
         *
         */
        glm::quat mOrientation;
    };

    /**
     * @brief Constraint requiring exactly two parameters representing colliding rigid
     * bodies.
     *
     * Repositions objects such that they no longer intersect along the axis of collision.
     *
     */
    class CollisionConstraint: protected Constraint<2> {
    private:
        /**
         * @brief Whether or not two objects are currently intersecting (and therefore whether
         * they should be separated)
         *
         */
        bool mCollided { false };

        /**
         * @brief The contact point of object A participating in this constraint in world space.
         *
         */
        glm::vec3 mLastPointContactA { 0.f };

        /**
         * @brief The contact point of object A relative to its own frame.
         *
         */
        glm::vec3 mRelativePointContactA { 0.f };

        /**
         * @brief The contact point of object B participating in this constraint in world space.
         *
         */
        glm::vec3 mLastPointContactB { 0.f };

        /**
         * @brief The contact point of object B relative to its own frame.
         *
         */
        glm::vec3 mRelativePointContactB { 0.f };

        /**
         * @brief Upon collision, points in the direction object B would need to move in order to
         * be separated from object A
         *
         */
        glm::vec3 mContactNormal { 0.f };

        /**
         * @brief The velocity of A's point of contact relative to B at the time the collision took
         * place
         *
         */
        float mCollisionVelocity { 0.f };

    public:

        /**
         * @brief Initializes constraint with collision data from two potentially intersecting objects
         *
         */
        CollisionConstraint(
            const Collision& collision,
            const PhysicsState& physicsA,
            const ObjectBounds& boundsA,
            const PhysicsState& objectB,
            const ObjectBounds& boundsB
        );

        /**
         * @brief Caches collision related information shared across position and velocity corrections
         *
         */
        void updateCollisionData(
            const Collision& collision,
            const PhysicsState& physicsA,
            const ObjectBounds& boundsA,
            const PhysicsState& physicsB,
            const ObjectBounds& boundsB
        );

        /**
         * @brief Separates intersecting/colliding objects and applies static friction.
         *
         */
        void applyConstraintPosition(
            const ParticipantTable& states,
            float substepSeconds
        ) override;

        /**
         * @brief Applies dynamic friction
         *
         */
        void applyConstraintVelocity(
            const ParticipantTable& states,
            float substepSeconds
        ) override;
    };

    class PinConstraint: public ParametrizedConstraint<PinConstraintData, 2> {
    public:
        PinConstraint(const std::vector<PinConstraintData>& data, float compliance) :
        ParametrizedConstraint<PinConstraintData, 2> { compliance }
        {
            assert(data.size() == 1 && "Pin constraint takes exactly one participant");
            setParameter(0, data[0]);
        }

        /**
         * @brief Moves object to pin location and orientation
         *
         */
        void applyConstraintPosition(
            const ParticipantTable& states,
            float substepSeconds
        ) override;
    };

    inline void from_json(
        const nlohmann::json& json,
        PhysicsState& physics
    ) {
        assert(json.at("type") == PhysicsState::getComponentTypeName() && "Incorrect type property for an physics property component");
        physics = {};

        float mass;
        if(json.at("mass").is_string() && json.at("mass") == "infinity") {
            mass = std::numeric_limits<float>::max();
        } else {
            mass = json.at("mass");
        }
        physics.setMass(mass);

        if(json.find("velocity") != json.end()) {

            physics.mVelocity = glm::vec3 {
                json.at("velocity")[0],
                json.at("velocity")[1],
                json.at("velocity")[2],
            };
            assert(isNumber(physics.mVelocity) && isFinite(physics.mVelocity) && "Velocity must be sensible");
        }
        if(json.find("angular_velocity") != json.end()) {
            physics.mAngularVelocity = glm::vec3 {
                json.at("angular_velocity")[0],
                json.at("angular_velocity")[1],
                json.at("angular_velocity")[2],
            };
            assert(isNumber(physics.mAngularVelocity) && isFinite(physics.mAngularVelocity) && "Angular velocity must be sensible");
        }
        if(json.find("force") != json.end()) {
            physics.mForce = glm::vec3 {
                json.at("force")[0],
                json.at("force")[1],
                json.at("force")[2],
            };
            assert(isNumber(physics.mForce) && isFinite(physics.mForce) && "Force must be sensible");
        }
        if(json.find("torque") != json.end()) {
            physics.mForce = glm::vec3 {
                json.at("torque")[0],
                json.at("torque")[1],
                json.at("torque")[2],
            };
            assert(isNumber(physics.mTorque) && isFinite(physics.mTorque) && "Torque must be sensible");
        }
        if(json.find("coefficient_friction_static") != json.end()) {
            physics.mCoefficientFrictionStatic = json.at("coefficient_friction_static");
            assert(physics.mCoefficientFrictionStatic >= 0.f && "Coefficient friction must be non-negative");
        }
        if(json.find("coefficient_friction_dynamic") != json.end()) {
            physics.mCoefficientFrictionDynamic = json.at("coefficient_friction_dynamic");
            assert(physics.mCoefficientFrictionDynamic >= 0.f && "Coefficient friction must be non-negative");
        }
        if(json.find("coefficient_restitution") != json.end()) {
            physics.setCoefficientRestitution(json.at("coefficient_restitution"));
        }
    }

    inline void to_json(
        nlohmann::json& json,
        const PhysicsState& physics
    ) {
        const float mass { physics.getMass() };
        json = {
            { "type", PhysicsState::getComponentTypeName() },
            mass != std::numeric_limits<float>::max()?
                nlohmann::json::object({ "mass", mass }):
                nlohmann::json::object({ "mass", "infinity" }),
            { "coefficient_friction_static", physics.mCoefficientFrictionStatic },
            { "coefficient_friction_dynamic", physics.mCoefficientFrictionDynamic },
            { "coefficient_restitution", physics.mCoefficientRestitution },
        };
    }

    template <uint8_t LagrangeCount>
    inline const std::array<float, LagrangeCount>& Constraint<LagrangeCount>::getLagrange() const {
        return mLagrangeMultipliers;
    }

    template <uint8_t LagrangeCount>
    inline void Constraint<LagrangeCount>::applyLagrangeDelta(float delta, uint8_t index) {
        mLagrangeMultipliers[index] += delta;
    }

    template <uint8_t LagrangeCount>
    inline void Constraint<LagrangeCount>::resetLagrange() {
        resetLagrange(std::make_integer_sequence<uint8_t, LagrangeCount>());
    }

    template<uint8_t LagrangeCount>
    template<uint8_t ...indices>
    inline void Constraint<LagrangeCount>::resetLagrange(std::integer_sequence<uint8_t, indices...> sequence) {
        ((mLagrangeMultipliers[indices] = 0.f), ...);
    }

    template<typename TParameter, uint8_t LagrangeCount>
    inline void ParametrizedConstraint<TParameter, LagrangeCount>::setParameter(BaseConstraint::ParticipantID participant, const TParameter& parameter) {
        mParameters[participant] = parameter;
    }

    template<typename TParameter, uint8_t LagrangeCount>
    inline void ParametrizedConstraint<TParameter, LagrangeCount>::removeParameter(BaseConstraint::ParticipantID participant) {
        mParameters.erase(participant);
    }

    template <typename TParameter, uint8_t LagrangeCount>
    inline const std::unordered_map<BaseConstraint::ParticipantID, TParameter>& ParametrizedConstraint<TParameter, LagrangeCount>::getParameters() const {
        return mParameters;
    }

}

#endif


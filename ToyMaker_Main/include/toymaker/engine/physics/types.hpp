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

    inline void from_json(
        const nlohmann::json& json,
        PhysicsState& physics
    ) {
        assert(json.at("type") == PhysicsState::getComponentTypeName() && "Incorrect type property for an physics property component");
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
        const PhysicsState& physics
    ) {
        json = {
            { "type", PhysicsState::getComponentTypeName() },
            { "mass", physics.mMass },
        };
    }
}

#endif


/**
 * @ingroup ToyMakerCore
 * @file util.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Contains a couple of classes not tied to any part of the engine in particular, but useful to those parts all the same.
 * @version 0.3.2
 * @date 2025-09-11
 * 
 * 
 */

#ifndef TOYMAKERENGINE_UTIL_H
#define TOYMAKERENGINE_UTIL_H

#include <glm/glm.hpp>

namespace ToyMaker {
    /**
     * @ingroup ToyMakerCore ToyMakerSceneSystem
     * @brief Converts a position, orientation and scale into its model matrix equivalent.
     *
     * @param position The position offset to apply to a mesh.
     * @param orientation The rotation applied to a mesh, expressed as a quaternion.
     * @param scale The factor along each direction by which to scale a mesh.
     * @return glm::mat4 The matrix representation of the argument values.
     */
    glm::mat4 buildModelMatrix(glm::vec4 position, glm::quat orientation, glm::vec3 scale = glm::vec3{1.f, 1.f, 1.f});

    /**
     * @ingroup ToyMakerCore
     *
     * @brief Returns the square of the length of a 3 component vector
     *
     */
    inline float squareDistance(const glm::vec3& vector) {
        return glm::dot(vector, vector);
    }

    /**
     * @ingroup ToyMakerCore
     * @brief Tests whether a given number is finite.
     *
     * @param number The number being tested.
     * @retval true The number is finite;
     * @retval false The number is not finite;
     */
    inline bool isFinite(float number) {
        return std::isfinite(number);
    }

    /**
     * @ingroup ToyMakerCore
     * @brief Tests whether a set of 3 numbers is finite.
     *
     * @param vector The numbers being tested.
     * @retval true The numbers are (all) finite;
     * @retval false One or more numbers are not finite;
     */
    inline bool isFinite(const glm::vec3& vector) {
        return isFinite(vector.x) && isFinite(vector.y) && isFinite(vector.z);
    }

    /**
     * @ingroup ToyMakerCore
     * @brief Tests whether a number is strictly positive.
     *
     * @param number The number being tested.
     * @retval true The number is strictly positive;
     * @retval false The number is not positive;
     */
    inline bool isPositiveStrict(float number) {
        return number > 0.f;
    }

    /**
     * @ingroup ToyMakerCore
     * @brief Tests whether a set of 3 numbers is strictly positive.
     *
     * @param vector The numbers being tested.
     * @retval true The numbers are (all) strictly positive;
     * @retval false One or more numbers are not positive;
     */
    inline bool isPositiveStrict(const glm::vec3& vector) {
        return isPositiveStrict(vector.x) && isPositiveStrict(vector.y) && isPositiveStrict(vector.z);
    }

    /**
     * @ingroup ToyMakerCore
     * @brief Tests whether a float is really a number (as opposed to a special error representation)
     *
     */
    inline bool isNumber(float number) {
        return !std::isnan(number);
    };

    /**
     * @ingroup ToyMakerCore
     * @brief Tests whether a set of 3 numbers are really numbers (as opposed to special error representations)
     *
     */
    inline bool isNumber(const glm::vec3& vector) {
        return isNumber(vector.x) && isNumber(vector.y) && isNumber(vector.z);
    };

    /**
     * @ingroup ToyMakerCore
     * @brief Tests whether a number is non-negative.
     *
     * @param number The number being tested.
     * @retval true The number is non-negative;
     * @retval false The number is negative;
     */
    inline bool isNonNegative(float number) {
        return number >= 0.f;
    }

    /**
     * @ingroup ToyMakerCore
     * @brief Tests whether a set of numbers is non-negative.
     *
     * @param vector The numbers being tested.
     * @retval true The numbers are (all) non-negative.
     * @retval false One or more of the numbers are negative.
     */
    inline bool isNonNegative(const glm::vec3& vector) {
        return isNonNegative(vector.x) && isNonNegative(vector.y) && isNonNegative(vector.z);
    }

    /**
     * @ingroup ToyMakerCore
     *
     * @brief Returns the rotation between two vectors represented as a quaternion.
     *
     */
    glm::quat getRotation(const glm::vec3& from, const glm::vec3& to);

    /**
     * @brief Given some arbitrary non-zero vector, returns a normalized vector that is orthogonal
     * to it.
     *
     */
    glm::vec3 getOrthogonal(const glm::vec3& from);

    /**
     * @ingroup ToyMakerSpatialQuerySystem
     * @brief Returns a pair of normalized tangents to an input vector.
     * 
     * See [Erin Catto's blog post](https://box2d.org/posts/2014/02/computing-a-basis/) for the method used here to
     * derive consistent tangents to a vector efficiently.
     * 
     */
    std::pair<glm::vec3, glm::vec3> getTangents(const glm::vec3& vector);

    /**
     * @ingroup ToyMakerCore
     * @brief Determines whether a particular matrix is valid and finite
     *
     */
    bool isSensible(const glm::mat3& matrix);

    /**
     * @ingroup ToyMakerCore ToyMakerSceneSystem
     * @brief Given a transform, returns scale matrix that was used to compose the transform
     *
     * @warning This only works for transforms with positive scale values.
     */
    glm::mat4 getScaleMatrix(const glm::mat4& fromTransform);

    /**
     * @ingroup ToyMakerCore ToyMakerSceneSystem
     * @brief Given a transform, returns rotation matrix that was used to compose the transform
     *
     * @warning This only works for scene objects with positive scale values.
     */
    glm::mat4 getRotationMatrix(const glm::mat4& fromTransform);

    /**
     * @ingroup ToyMakerCore ToyMakerSceneSystem
     * @brief Given a transform, returns translation matrix that was used to compose the transform
     *
     */
    glm::mat4 getTranslationMatrix(const glm::mat4& fromTransform);

    /**
     * @ingroup ToyMakerCore
     * @brief A simple linear interpolation implementation between a fixed input and output range.
     * 
     * ## Usage:
     * 
     * ```c++
     * 
     * axisValue = RangeMapperLinear{
     *     // NOTE: extremes of the input range.
     *     0.f, static_cast<double>(windowWidth),
     * 
     *     // NOTE: extremes of the output range.
     *     0.f, 1.f
     * 
     * // NOTE: the value mapped from the input range to the output range.
     * }(inputEvent.motion.x);
     * 
     * ```
     * 
     * @todo Does this really need to be a class when it seems as though every usage instantiates and calls this "functor" in the same step?  What was I thinking?
     * 
     */
    class RangeMapperLinear {
    public:
        /**
         * @brief Constructs a range mapper with a fixed pair of input and output ranges.
         * 
         * @param inputLowerBound The start of the input range.
         * @param inputUpperBound The end of the input range.
         * @param outputLowerBound The start of the output range.
         * @param outputUpperBound The end of the output range.
         */
        RangeMapperLinear(
            double inputLowerBound, double inputUpperBound,
            double outputLowerBound, double outputUpperBound
        );

        /**
         * @brief The operation that actually converts a value from its input range into its output range.
         * 
         * @param value The value in the input range.
         * @return double The same value mapped to the output range.
         */
        double operator() (double value) const;
    private:
        /**
         * @brief The start of the input range.
         * 
         */
        double mInputLowerBound;
        /**
         * @brief The end of the input range.
         * 
         */
        double mInputUpperBound;

        /**
         * @brief The start of the output range.
         * 
         */
        double mOutputLowerBound;

        /**
         * @brief The end of the output range.
         * 
         */
        double mOutputUpperBound;
    };
}

#endif

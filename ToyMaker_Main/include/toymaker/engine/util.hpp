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

#ifndef FOOLSENGINE_UTIL_H
#define FOOLSENGINE_UTIL_H

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

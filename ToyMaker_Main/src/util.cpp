#include <cassert>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include "toymaker/engine/util.hpp"

using namespace ToyMaker;

glm::mat4 ToyMaker::buildModelMatrix(glm::vec4 position, glm::quat orientation, glm::vec3 scale) {
    glm::mat4 rotateMatrix { glm::normalize(orientation) };
    glm::mat4 translateMatrix { glm::translate(glm::mat4(1.f), glm::vec3(position)) };
    glm::mat4 scaleMatrix { glm::scale(glm::mat4(1.f), scale) };
    return translateMatrix * rotateMatrix * scaleMatrix;
}

RangeMapperLinear::RangeMapperLinear(
    double inputLowerBound, double inputUpperBound,
    double outputLowerBound, double outputUpperBound
): mInputLowerBound {inputLowerBound}, mInputUpperBound{inputUpperBound},
   mOutputLowerBound {outputLowerBound} , mOutputUpperBound {outputUpperBound}
{
    assert(mOutputUpperBound > mOutputLowerBound && "The output upper bound must be greater than the lower bound");
    assert(mInputUpperBound > mInputLowerBound && "The input upper bound must be greater than the lower bound");
    // TODO: assert that (input upperbound - input lowerbound) is
    // within double's range
}

double RangeMapperLinear::operator() (double value) const {
    // clamp the value to the input range
    if(value > mInputUpperBound) value = mInputUpperBound;
    else if(value < mInputLowerBound) value = mInputLowerBound;

    // simple linear interpolation for the result
    return (
        (value - mInputLowerBound)
            /(mInputUpperBound - mInputLowerBound)
            * (mOutputUpperBound - mOutputLowerBound)
        + mOutputLowerBound
    );
}

glm::mat4 ToyMaker::getScaleMatrix(const glm::mat4& fromTransform) {
    return {
        glm::vec4 { glm::length(fromTransform[0]), 0.f, 0.f, 0.f },
        glm::vec4 { 0.f, glm::length(fromTransform[1]), 0.f, 0.f },
        glm::vec4 { 0.f, 0.f, glm::length(fromTransform[1]), 0.f },
        glm::vec4 { 0.f, 0.f, 0.f,                           1.f }
    };
}

glm::mat4 ToyMaker::getRotationMatrix(const glm::mat4& fromTransform) {
    return glm::inverse(getTranslationMatrix(fromTransform))
        * fromTransform
        * glm::inverse(ToyMaker::getScaleMatrix(fromTransform));
}

glm::mat4 ToyMaker::getTranslationMatrix(const glm::mat4& fromTransform) {
    return {
        glm::vec4 { 1.f, 0.f, 0.f, 0.f },
        glm::vec4 { 0.f, 1.f, 0.f, 0.f },
        glm::vec4 { 0.f, 0.f, 1.f, 0.f },
        fromTransform[3],
    };
}


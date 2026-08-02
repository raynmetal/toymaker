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

glm::quat ToyMaker::getRotation(const glm::vec3& from, const glm::vec3& to) {
    // implementation taken from [here.](https://stackoverflow.com/a/11741520/5677302)

    assert(isFinite(from) && isFinite(to) && "One or both of the input vectors is not finite.");
    assert(squareDistance(from) && squareDistance(to) && "One or both input vectors is a zero vector.");
    const float scaledCos { glm::dot(from, to) };
    const float sumLengths { glm::sqrt(squareDistance(from) + squareDistance(to)) };

    // 180 degree rotation where `from + to` gives us 0, which can't be normalized
    if(scaledCos / sumLengths == -1) {
        return glm::quat { 0.f, glm::normalize(getOrthogonal(from)) };
    }

    return glm::normalize(glm::quat { scaledCos + sumLengths, glm::cross(from, to) });
}

float ToyMaker::getAngle(const glm::vec3 &from, const glm::vec3 &to, const glm::vec3 &axis) {
    if(
        !squareDistance(from) || !squareDistance(to) || !squareDistance(axis)
        || !isFinite(from) || !isFinite(to) || !isFinite(axis)
    ) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    constexpr float pi { glm::pi<float>() };
    const glm::vec3 axisNormalized { glm::normalize(axis) };
    const glm::vec3 fromNormalized { glm::normalize(from) };
    const glm::vec3 toNormalized { glm::normalize(to) };
    float angle { std::asin(
        glm::dot(glm::cross(fromNormalized, toNormalized), axisNormalized)
    ) };
    // fix angle if we are in one of the left 2 quadrants
    if(glm::dot(from, to) < 0.f) {
        angle = pi - angle;
    }
    // fix angle if we're in bottom 2 quadrants going anti-clockwise
    if(angle > pi) {
        angle -= 2.f * pi;
    }
    // fix angle if we're in top 2 quadrants going clockwise
    if(angle < -pi) {
        angle += 2.f * pi;
    }

    return angle;
}

glm::vec3 ToyMaker::getOrthogonal(const glm::vec3& from) {
    assert(isFinite(from) && "Vector must be finite");
    assert(squareDistance(from) && "Zero vectors are invalid as basis for tangents");

    return getTangents(from).first;
}

std::pair<glm::vec3, glm::vec3> ToyMaker::getTangents(const glm::vec3& vector) {
    assert(isFinite(vector) && "Vector must be finite");
    assert(squareDistance(vector) && "Zero vectors are invalid as basis for tangents");
    const auto normalized { glm::normalize(vector) };
    const glm::vec3 tangentPrelim { ((glm::abs(normalized.x) >= .57735f) ?
        glm::vec3{ normalized.y, -normalized.x, 0.f } : glm::vec3{ 0.f, normalized.z, -normalized.y }
    ) };
    const glm::vec3 tangent2 {
        glm::normalize(glm::cross(tangentPrelim, normalized))
    };

    return { glm::normalize(glm::cross(normalized, tangent2)), tangent2 };
}

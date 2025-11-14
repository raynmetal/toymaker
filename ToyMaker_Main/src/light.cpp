#include <cmath>
#include <cassert>

#include <glm/glm.hpp>

#include "toymaker/engine/light.hpp"

using namespace ToyMaker;

float LightEmissionData::CalculateRadius(const glm::vec4& diffuseColor, float decayLinear, float decayQuadratic, float intensityCutoff) {
    float intensityMax = diffuseColor.r;
    if(diffuseColor.g > intensityMax) intensityMax = diffuseColor.g;
    if(diffuseColor.b > intensityMax) intensityMax = diffuseColor.b;
    float radiusCutoff {
        (
            -decayLinear
            + std::sqrt(
                decayLinear*decayLinear - 4.f * decayQuadratic * (1.f - intensityMax / intensityCutoff)
            )
        )
        / (2.f * decayQuadratic)
    };
    return radiusCutoff;
}

LightEmissionData LightEmissionData::MakeDirectionalLight(const glm::vec3& diffuse, const glm::vec3& specular, const glm::vec3& ambient){
    return LightEmissionData{
        .mType{ LightEmissionData::directional },
        .mDiffuseColor { diffuse, 1.f },
        .mSpecularColor { specular, 1.f },
        .mAmbientColor{ ambient, 1.f },
        .mDecayLinear {0.f},
        .mDecayQuadratic {0.f},
        .mCosCutoffInner {0.f},
        .mCosCutoffOuter {0.f}
    };
}

LightEmissionData LightEmissionData::MakePointLight(const glm::vec3& diffuse, const glm::vec3& specular, const glm::vec3& ambient, float linearConst, float quadraticConst){
    return LightEmissionData {
        .mType { LightEmissionData::point },
        .mDiffuseColor { diffuse, 1.f, },
        .mSpecularColor { specular, 1.f },
        .mAmbientColor{ ambient, 1.f },
        .mDecayLinear {linearConst},
        .mDecayQuadratic {quadraticConst},
        .mCosCutoffInner {0.f},
        .mCosCutoffOuter {0.f},
        .mRadius {CalculateRadius({diffuse, 1.f}, linearConst, quadraticConst, .05f)}
    };
}

LightEmissionData LightEmissionData::MakeSpotLight(float innerAngle, float outerAngle, const glm::vec3& diffuse, const glm::vec3& specular, const glm::vec3& ambient, float linearConst, float quadraticConst) {
    return LightEmissionData{
        .mType { LightEmissionData::spot },
        .mDiffuseColor { diffuse, 1.f },
        .mSpecularColor { specular, 1.f },
        .mAmbientColor{ ambient, 1.f },
        .mDecayLinear {linearConst},
        .mDecayQuadratic {quadraticConst},
        .mCosCutoffInner {glm::cos(glm::radians(innerAngle))},
        .mCosCutoffOuter {glm::cos(glm::radians(outerAngle))},
        .mRadius {CalculateRadius({diffuse, 1.f}, linearConst, quadraticConst, .05f)}
    };
}

LightInstanceAllocator::LightInstanceAllocator(const std::vector<LightEmissionData>& lightEmissionDataList, const std::vector<glm::mat4>& lightModelMatrices)
:
    BaseInstanceAllocator { LightInstanceLayout },  mLightData {}
{
    assert(lightEmissionDataList.size() == lightModelMatrices.size() && "One to one mapping between emission and placement required");
    mLightData.resize(lightEmissionDataList.size());
    for(std::size_t i{0}; i < lightEmissionDataList.size(); ++i) {
        mLightData[i].first.first = lightModelMatrices[i] * glm::vec4(0.f, 0.f, 0.f, 1.f);
        mLightData[i].first.second = (
            glm::normalize(
                lightModelMatrices[i]
                * glm::vec4(0.f, 0.f, -1.f, 0.f) // new direction of the local -Z axis
            )
        );
        mLightData[i].second = lightEmissionDataList[i];
    }
}

void LightInstanceAllocator::upload() {
    if(isUploaded()) return;

    glGenBuffers(1, &mVertexBufferIndex);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexBufferIndex);
        glBufferData(GL_ARRAY_BUFFER, mLightData.size() * sizeof(LightPackedData), mLightData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

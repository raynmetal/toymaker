/**
 * @ingroup ToyMakerRenderSystem
 * @file light.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief A file that contains definitions for different types of lights, as components to entities, supported by the engine.
 * @version 0.3.2
 * @date 2025-09-03
 * 
 * 
 */

#ifndef FOOLSENGINE_LIGHT_H
#define FOOLSENGINE_LIGHT_H

#include <utility>
#include <queue>
#include <map>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <nlohmann/json.hpp>

#include "core/ecs_world.hpp"
#include "shapegen.hpp"
#include "instance.hpp"
#include "scene_components.hpp"

namespace ToyMaker {
    struct LightEmissionData;

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief A version of light data where the first element represents the light's position and direction, and the second represents its emission properties.
     * 
     */
    using LightPackedData = std::pair<std::pair<glm::vec4, glm::vec4>, LightEmissionData>;

    /**
     * @ingroup ToyMakerRenderSystem ToyMakerECSComponent
     * @brief A struct, used as a component, describing the emissive properties of the light it represents per the Blinn-Phong shading model.
     * 
     * Its appearance in json might be as follows:
     * 
     * ```jsonc
     * 
     * {
     *    "ambient": [0.04, 0.1, 0.04],
     *    "diffuse": [2.4, 8.7, 2.4],
     *    "specular": [ 2.0, 2.0, 2.0],
     *    "linearConst": 0.10,
     *    "quadraticConst": 0.10,
     *    "lightType": "point",
     *    "type": "LightEmissionData"
     * } 
     * 
     * ```
     * 
     * @see ECSWorld::registerComponentTypes()
     * 
     */
    struct LightEmissionData {
        /**
         * @brief Creates a directional source of light, which (in a scene) faces one direction and experiences no attenuation.
         * 
         * @param diffuse The diffuse component of a light per the Blinn-Phong model.
         * 
         * @param specular The specular component of a light per the Blinn-Phong model.
         * 
         * @param ambient The ambient component of a light per the Blinn-Phong model.
         * 
         * @return LightEmissionData The constructed light data object.
         */
        static LightEmissionData MakeDirectionalLight(const glm::vec3& diffuse, const glm::vec3& specular, const glm::vec3& ambient);

        /**
         * @brief Creates a point source of light which has a position and experiences attenuation, but has no direction.
         * 
         * @param diffuse The diffuse component of the light per the Blinn-Phong model.
         * @param specular The specular component of the light per the Blinn-Phong model.
         * @param ambient The ambient component of the light per the Blinn-Phong model.
         * @param linearConst The factor governing linear reduction in intensity of light with distance.
         * @param quadraticConst The factor governing quadratic reduction in intensity of light with distance.
         * @return LightEmissionData The constructed light data object.
         */
        static LightEmissionData MakePointLight(const glm::vec3& diffuse, const glm::vec3& specular, const glm::vec3& ambient, float linearConst, float quadraticConst);

        /**
         * @brief Creates a spotlight which has a position, experiences attenuation, and has a direction.
         * 
         * @param diffuse The diffuse component of the light per the Blinn-Phong model.
         * @param specular The specular component of the light per the Blinn-Phong model.
         * @param ambient The ambient component of the light per the Blinn-Phong model.
         * @param linearConst The factor governing linear reduction in intensity of light with distance.
         * @param quadraticConst The factor governing quadratic reduction in intensity of light with distance.
         * @param innerAngle The angle, in degrees, made by the inner cone of the spotlight (where intensity is strongest) relative to the direction vector of the light.
         * @param outerAngle The angle, in degrees, made by the outer cone of the spotlight (beyond which no light is present) relative to the direction vector of the light.
         * @return LightEmissionData The constructed light data object.
         */
        static LightEmissionData MakeSpotLight(
            float innerAngle, float outerAngle, const glm::vec3& diffuse, const glm::vec3& specular, const glm::vec3& ambient,
            float linearConst, float quadraticConst
        );

        /**
         * @brief Integers representing different types of light sources.
         * 
         */
        enum LightType:int {
            directional=0, //< No attenuation, no position, only direction
            point=1, //< Has attenuation, has position, but no direction
            spot=2, //< Has attenuation, position, and direction
        };

        /**
         * @brief The type of light described by this object.
         * 
         */
        LightType mType;

        /**
         * @brief The color of the diffuse component of the light represented by this object.
         * 
         * The diffuse color is what we would think of as "the color" of an object.  In light, it is the color of the light affecting object surfaces pointing towards the light source.
         */
        glm::vec4 mDiffuseColor;

        /**
         * @brief The color of the specular component of the light represented by this object.
         * 
         * The specular color is the color on portions of the surface where light bounces off an object and towards the camera.
         * 
         */
        glm::vec4 mSpecularColor;

        /**
         * @brief The color of the ambient component of the light represented by this object.
         * 
         * Represents this source's contribution to indirect lighting affecting an object, reflected from other objects in the scene.  This factor just approximates indirect ambient light, and doesn't model real ambient lighting.
         * 
         */
        glm::vec4 mAmbientColor;


        /**
         * @brief A linear factor governing the attenuation in light intensity with distance from source, per the Blinn-Phong model.
         * 
         */
        GLfloat mDecayLinear;

        /**
         * @brief A quadractic factor governing the attenuation in light intensity with distance from source, per the Blinn-Phong model.
         * 
         */
        GLfloat mDecayQuadratic;


        /**
         * @brief The cos of the angle between the surface of the inner cone of a spot light (within which light intensity is highest), and the direction vector of the light.
         * 
         */
        GLfloat mCosCutoffInner;

        /**
         * @brief The cos of the angle between the surface of the outer cone of a spot light (beyond which light intensity drops to 0), and the direction vector of the light.
         * 
         */
        GLfloat mCosCutoffOuter;

        /**
         * @brief The computed radius of the light beyond which the light is no longer active, based on its emission data.
         * 
         * @see CalculateRadius
         */
        GLfloat mRadius {0.f};

        /**
         * @brief A function that computes the cutoff radius for a light source based on its emissive properties.
         * 
         * @param diffuseColor The diffuse color of the light.
         * @param decayLinear The linear factor governing light intensity decay.
         * @param decayQuadratic The quadratic factor governing light intensity decay.
         * @param intensityCutoff The nth fraction of the max intensity of the light beyond which the light is considered inactive.  Eg., intensityCutoff = 40.f => intensityAtRadius = maxIntensity/40.f.
         * @return float 
         */
        static float CalculateRadius(const glm::vec4& diffuseColor, float decayLinear, float decayQuadratic, float intensityCutoff);

        /**
         * @brief The component type string associated with this object.
         * 
         * @return std::string The component type string of this object.
         * 
         * @see ECSWorld::registerComponentTypes()
         */
        inline static std::string getComponentTypeName() { return "LightEmissionData"; }
    };

    /** @ingroup ToyMakerSerialization ToyMakerRenderSystem */
    NLOHMANN_JSON_SERIALIZE_ENUM( LightEmissionData::LightType, {
        {LightEmissionData::LightType::directional, "directional"},
        {LightEmissionData::LightType::point, "point"},
        {LightEmissionData::LightType::spot, "spot"},
    });

    /** @ingroup ToyMakerSerialization ToyMakerRenderSystem */
    inline void from_json(const nlohmann::json& json, LightEmissionData& lightEmissionData) {
        assert(json.at("type") == LightEmissionData::getComponentTypeName() && "Type mismatch. Light component json must have type LightEmissionData");
        json.at("lightType").get_to(lightEmissionData.mType);
        switch(lightEmissionData.mType) {
            case LightEmissionData::LightType::directional:
                lightEmissionData = LightEmissionData::MakeDirectionalLight(
                    glm::vec3{
                        json.at("diffuse")[0].get<float>(),
                        json.at("diffuse")[1].get<float>(),
                        json.at("diffuse")[2].get<float>(),
                    },
                    glm::vec3{
                        json.at("specular")[0].get<float>(),
                        json.at("specular")[1].get<float>(),
                        json.at("specular")[2].get<float>(),
                    },
                    glm::vec3{
                        json.at("ambient")[0].get<float>(),
                        json.at("ambient")[1].get<float>(),
                        json.at("ambient")[2].get<float>(),
                    }
                );
            break;
            case LightEmissionData::LightType::point:
                lightEmissionData = LightEmissionData::MakePointLight(
                    glm::vec3{
                        json.at("diffuse")[0].get<float>(),
                        json.at("diffuse")[1].get<float>(),
                        json.at("diffuse")[2].get<float>(),
                    },
                    glm::vec3{
                        json.at("specular")[0].get<float>(),
                        json.at("specular")[1].get<float>(),
                        json.at("specular")[2].get<float>(),
                    },
                    glm::vec3{
                        json.at("ambient")[0].get<float>(),
                        json.at("ambient")[1].get<float>(),
                        json.at("ambient")[2].get<float>(),
                    },
                    json.at("linearConst").get<float>(),
                    json.at("quadraticConst").get<float>()
                );
            break;
            case LightEmissionData::LightType::spot:
                lightEmissionData = LightEmissionData::MakeSpotLight(
                    json.at("innerAngle").get<float>(),
                    json.at("outerAngle").get<float>(),
                    glm::vec3{
                        json.at("diffuse")[0].get<float>(),
                        json.at("diffuse")[1].get<float>(),
                        json.at("diffuse")[2].get<float>(),
                    },
                    glm::vec3{
                        json.at("specular")[0].get<float>(),
                        json.at("specular")[1].get<float>(),
                        json.at("specular")[2].get<float>(),
                    },
                    glm::vec3{
                        json.at("ambient")[0].get<float>(),
                        json.at("ambient")[1].get<float>(),
                        json.at("ambient")[2].get<float>(),
                    },
                    json.at("linearConst").get<float>(),
                    json.at("quadraticConst").get<float>()
                );
            break;
        }
    }
    /** @ingroup ToyMakerSerialization ToyMakerRenderSystem */
    inline void to_json(nlohmann::json& json, const LightEmissionData& lightEmissionData) {
        switch(lightEmissionData.mType) {
            case LightEmissionData::LightType::directional:
                json = {
                    {"type", LightEmissionData::getComponentTypeName()},
                    {"lightType", lightEmissionData.mType},
                    {"diffuse", {
                        lightEmissionData.mDiffuseColor.r,
                        lightEmissionData.mDiffuseColor.g,
                        lightEmissionData.mDiffuseColor.b,
                    }},
                    {"specular", {
                        lightEmissionData.mSpecularColor.r,
                        lightEmissionData.mSpecularColor.g,
                        lightEmissionData.mSpecularColor.b,
                    }},
                    {"ambient", {
                        lightEmissionData.mAmbientColor.r,
                        lightEmissionData.mAmbientColor.g,
                        lightEmissionData.mAmbientColor.b,
                    }},
                };
            break;
            case LightEmissionData::LightType::point:
                json = {
                    {"type", LightEmissionData::getComponentTypeName()},
                    {"lightType", lightEmissionData.mType},
                    {"diffuse", {
                        lightEmissionData.mDiffuseColor.r,
                        lightEmissionData.mDiffuseColor.g,
                        lightEmissionData.mDiffuseColor.b,
                    }},
                    {"specular", {
                        lightEmissionData.mSpecularColor.r,
                        lightEmissionData.mSpecularColor.g,
                        lightEmissionData.mSpecularColor.b,
                    }},
                    {"ambient", {
                        lightEmissionData.mAmbientColor.r,
                        lightEmissionData.mAmbientColor.g,
                        lightEmissionData.mAmbientColor.b,
                    }},
                    {"linearConst", lightEmissionData.mDecayLinear},
                    {"quadraticConst", lightEmissionData.mDecayQuadratic},
                };
            break;
            case LightEmissionData::LightType::spot:
                json = {
                    {"type", LightEmissionData::getComponentTypeName()},
                    {"lightType", lightEmissionData.mType},
                    {"diffuse", {
                        lightEmissionData.mDiffuseColor.r,
                        lightEmissionData.mDiffuseColor.g,
                        lightEmissionData.mDiffuseColor.b,
                    }},
                    {"specular", {
                        lightEmissionData.mSpecularColor.r,
                        lightEmissionData.mSpecularColor.g,
                        lightEmissionData.mSpecularColor.b,
                    }},
                    {"ambient", {
                        lightEmissionData.mAmbientColor.r,
                        lightEmissionData.mAmbientColor.g,
                        lightEmissionData.mAmbientColor.b,
                    }},
                    {"linearConst", lightEmissionData.mDecayLinear},
                    {"quadraticConst", lightEmissionData.mDecayQuadratic},
                    {"innerAngle", glm::degrees(glm::acos(lightEmissionData.mCosCutoffInner))},
                    {"outerAngle", glm::degrees(glm::acos(lightEmissionData.mCosCutoffOuter))},
                };
            break;
        }
    }

    /**
     * @ingroup ToyMakerRenderSystem
     * @brief The layout for built-in light sources when used as instance attributes.
     * 
     */
    static InstanceLayout LightInstanceLayout {{
        {"attrLightPlacement_mPosition", RUNTIME, 4, GL_FLOAT},
        {"attrLightPlacement_mDirection", RUNTIME, 4, GL_FLOAT},

        {"attrLightEmission_mType", RUNTIME, 1, GL_INT},
        {"attrLightEmission_mDiffuseColor", RUNTIME, 4, GL_FLOAT},
        {"attrLightEmission_mSpecularColor", RUNTIME, 4, GL_FLOAT},
        {"attrLightEmission_mAmbientColor", RUNTIME, 4, GL_FLOAT},
        {"attrLightEmission_mDecayLinear", RUNTIME, 1, GL_FLOAT},
        {"attrLightEmission_mDecayQuadratic", RUNTIME, 1, GL_FLOAT},
        {"attrLightEmission_mCosCutoffInner", RUNTIME, 1, GL_FLOAT},
        {"attrLightEmission_mCosCutoffOuter", RUNTIME, 1, GL_FLOAT},
        {"attrLightEmission_mRadius", RUNTIME, 1, GL_FLOAT}
    }};


    /**
     * @ingroup ToyMakerRenderSystem
     * @brief The allocator associated with built in light sources used as attributes.
     * 
     */
    class LightInstanceAllocator : public BaseInstanceAllocator {
    public:
        LightInstanceAllocator(const std::vector<LightEmissionData>& lightEmissionDataList, const std::vector<glm::mat4>& lightModelMatrices);

    protected:
        virtual void upload() override;

    private:
        std::vector<LightPackedData> mLightData;
    };

    /**
     * @ingroup ToyMakerECSComponent ToyMakerRenderSystem
     * @brief Interpolates light emission properties between previous and next simulation states using linear interpolation.
     * 
     * @tparam Specialization for LightEmissionData.
     * @param previousState The state of the light at the end of the last simulation step.
     * @param nextState The state of the light at the end of this simulation step.
     * @param simulationProgress Progress towards the new state from the old state, a number between 0.0 and 1.0
     * @return LightEmissionData The interpolated light emission data.
     */
    template<>
    inline LightEmissionData Interpolator<LightEmissionData>::operator() (
        const LightEmissionData& previousState,
        const LightEmissionData& nextState,
        float simulationProgress
    ) const {
        simulationProgress = mProgressLimits(simulationProgress);
        LightEmissionData interpolatedState { previousState };

        interpolatedState.mDiffuseColor += simulationProgress * (nextState.mDiffuseColor - previousState.mDiffuseColor);
        interpolatedState.mSpecularColor += simulationProgress * (nextState.mSpecularColor - previousState.mSpecularColor);
        interpolatedState.mAmbientColor += simulationProgress * (nextState.mAmbientColor - previousState.mAmbientColor);
        interpolatedState.mDecayLinear += simulationProgress * (nextState.mDecayLinear - previousState.mDecayLinear);
        interpolatedState.mDecayQuadratic += simulationProgress * (nextState.mDecayQuadratic - previousState.mDecayQuadratic);
        interpolatedState.mCosCutoffInner += simulationProgress * (nextState.mCosCutoffInner - previousState.mCosCutoffInner);
        interpolatedState.mCosCutoffOuter += simulationProgress * (nextState.mCosCutoffOuter - previousState.mCosCutoffOuter);

        return interpolatedState;
    }
}

#endif

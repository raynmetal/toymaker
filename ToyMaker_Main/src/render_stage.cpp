#include <iostream>
#include <memory>

#include "toymaker/engine/core/resource_database.hpp"
#include "toymaker/engine/texture.hpp"
#include "toymaker/engine/shader_program.hpp"
#include "toymaker/engine/shapegen.hpp"
#include "toymaker/engine/framebuffer.hpp"
#include "toymaker/engine/light.hpp"

#include "toymaker/engine/render_stage.hpp"

using namespace ToyMaker;

BaseRenderStage::BaseRenderStage(
    const std::string& shaderFilepath
) : 
mShaderHandle {nullptr}
{
    if(!ResourceDatabase::HasResourceDescription(shaderFilepath)){
        nlohmann::json shaderDescription {
            {"name", shaderFilepath},
            {"type", ShaderProgram::getResourceTypeName()},
            {"method", ShaderProgramFromFile::getResourceConstructorName()},
            {"parameters", {
                {"path", shaderFilepath}
            }}
        };
        ResourceDatabase::AddResourceDescription(shaderDescription);
    }
    mShaderHandle = ResourceDatabase::GetRegisteredResource<ShaderProgram>(shaderFilepath);
    glGenVertexArrays(1, &mVertexArrayObject);
}

BaseRenderStage::~BaseRenderStage() {
    if(mVertexArrayObject){
        glDeleteVertexArrays(1, &mVertexArrayObject);
    }
}

void BaseRenderStage::setTargetViewport(const SDL_Rect& targetViewport) {
    mTargetViewport = targetViewport;
}

void BaseRenderStage::useViewport() {
    glViewport(mTargetViewport.x, mTargetViewport.y, mTargetViewport.w, mTargetViewport.h);
}

void BaseRenderStage::attachMesh(
    const std::string& name,
    std::shared_ptr<StaticMesh> meshHandle
) {
    mMeshAttachments.insert_or_assign(name, meshHandle);
}

std::shared_ptr<StaticMesh> BaseRenderStage::getMesh(const std::string& name) {
    return mMeshAttachments.at(name);
}

void BaseRenderStage::attachTexture(
    const std::string& name,
    std::shared_ptr<Texture> textureHandle
) {
    mTextureAttachments.insert_or_assign(name, textureHandle);
}

std::shared_ptr<Texture> BaseRenderStage::getTexture(const std::string& name) {
    return mTextureAttachments.at(name);
}

void BaseRenderStage::attachMaterial(
    const std::string& name,
    std::shared_ptr<Material> materialHandle
) {
    mMaterialAttachments.insert_or_assign(name, materialHandle);
}

std::shared_ptr<Material> BaseRenderStage::getMaterial(const std::string& name) {
    return mMaterialAttachments.at(name);
}

void BaseRenderStage::submitToRenderQueue(OpaqueRenderUnit renderUnit) {
    mOpaqueMeshQueue.push(renderUnit);
}

void BaseRenderStage::submitToRenderQueue(LightRenderUnit renderLightUnit) {
    mLightQueue.push(renderLightUnit);
}

BaseOffscreenRenderStage::BaseOffscreenRenderStage(
    const std::string& shaderFilepath, const nlohmann::json& templateFramebufferDescription
):
BaseRenderStage{ shaderFilepath }
{
    mTemplateFramebufferDescription = templateFramebufferDescription;
}

bool BaseOffscreenRenderStage::hasAttachedRBO() const {
    return mFramebufferHandle->hasAttachedRBO();
}

bool BaseOffscreenRenderStage::hasOwnRBO() const {
    return mFramebufferHandle->hasOwnRBO();
}

RBO& BaseOffscreenRenderStage::getOwnRBO() {
    return mFramebufferHandle->getOwnRBO();
}

void BaseOffscreenRenderStage::attachRBO(RBO& rbo) {
    mFramebufferHandle->attachRBO(rbo);
}

void BaseOffscreenRenderStage::detachRBO() {
    mFramebufferHandle->detachRBO();
}

std::size_t BaseOffscreenRenderStage::attachTextureAsTarget(std::shared_ptr<Texture> texture) {
    return mFramebufferHandle->addTargetColorBufferHandle(texture);
}

std::size_t BaseOffscreenRenderStage::attachTextureAsTarget(const std::string& targetName, std::shared_ptr<Texture> texture) {
    const std::size_t attachmentID { attachTextureAsTarget(texture) };
    declareRenderTarget(targetName, attachmentID);
    return attachmentID;
}

void BaseOffscreenRenderStage::declareRenderTarget(const std::string& name, unsigned int index) {
    mRenderTargets.insert_or_assign(name, index);
}

std::shared_ptr<Texture> BaseOffscreenRenderStage::getRenderTarget(const std::string& name) {
    return mFramebufferHandle->getTargetColorBufferHandles()[mRenderTargets.at(name)];
}

void GeometryRenderStage::setup(const glm::u16vec2& textureDimensions) {
    setTargetViewport({0,0, textureDimensions.x, textureDimensions.y});
    nlohmann::json framebufferDescription = mTemplateFramebufferDescription;
    framebufferDescription["parameters"]["dimensions"][0] = textureDimensions.x;
    framebufferDescription["parameters"]["dimensions"][1] = textureDimensions.y;
    for(nlohmann::json& colorBufferDefinition: framebufferDescription["parameters"]["colorBufferDefinitions"]) {
        colorBufferDefinition["dimensions"][0] = textureDimensions.x;
        colorBufferDefinition["dimensions"][1] = textureDimensions.y;
    }
    mFramebufferHandle = ResourceDatabase::ConstructAnonymousResource<Framebuffer>(framebufferDescription);

    mRenderTargets.clear();
    declareRenderTarget("geometryPosition", 0);
    declareRenderTarget("geometryNormal", 1);
    declareRenderTarget("geometryAlbedoSpecular", 2);

    mShaderHandle->use();
    mShaderHandle->setUniformBlock("Matrices", 0);
    if(!ResourceDatabase::HasResourceDescription("PlainWhite")) {
        ResourceDatabase::AddResourceDescription({
            {"name", "PlainWhite"},
            {"type", Texture::getResourceTypeName()},
            {"method", TextureFromColorBufferDefinition::getResourceConstructorName()},
            {"parameters", 
                ColorBufferDefinition{}
            }
        });
    }
    std::shared_ptr<Texture> plainWhite = ResourceDatabase::GetRegisteredResource<Texture>("PlainWhite");
    Material::RegisterTextureHandleProperty("textureAlbedo", plainWhite);
    Material::RegisterTextureHandleProperty("textureSpecular", plainWhite);
    Material::RegisterTextureHandleProperty("textureNormal", plainWhite);
    Material::RegisterFloatProperty("specularExponent", 14.f);
    Material::RegisterIntProperty("usesTextureAlbedo", false);
    Material::RegisterIntProperty("usesTextureSpecular", false);
    Material::RegisterIntProperty("usesTextureNormal", false);
    Material::RegisterVec4Property("colorMultiplier", glm::vec4{1.f, 1.f, 1.f, 1.f});
}

void GeometryRenderStage::validate() {
    /*
     * Three colour buffers corresponding to position, normal, albedospec (for now)
     */
    assert(3 == std::const_pointer_cast<const Framebuffer>(mFramebufferHandle)->getTargetColorBufferHandles().size());
    assert(std::const_pointer_cast<const Framebuffer>(mFramebufferHandle)->hasOwnRBO());
    mFramebufferHandle->bind();
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE && "Could not complete creation of framebuffer");
    mFramebufferHandle->unbind();

    /*
     * TODO: more geometry pass related assertions
     */
}

void GeometryRenderStage::execute() {
    mShaderHandle->use();
    useViewport();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
    glEnable(GL_FRAMEBUFFER_SRGB);

    mFramebufferHandle->bind();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const VertexLayout vertexLayout {{
            { "position", LOCATION_POSITION, 4, GL_FLOAT },
            { "normal", LOCATION_NORMAL, 4, GL_FLOAT },
            { "tangent", LOCATION_TANGENT, 4, GL_FLOAT },
            { "color", LOCATION_COLOR, 4, GL_FLOAT },
            { "UV1", LOCATION_UV1, 2, GL_FLOAT }
        }};

        while(!mOpaqueMeshQueue.empty()) {
            std::vector<glm::mat4> instanceData{};

            OpaqueRenderUnit first { mOpaqueMeshQueue.top() };
            instanceData.push_back(mOpaqueMeshQueue.top().mModelMatrix);
            mOpaqueMeshQueue.pop();

            while (
                !mOpaqueMeshQueue.empty() 
                && mOpaqueMeshQueue.top().mMeshHandle == first.mMeshHandle
                && mOpaqueMeshQueue.top().mMaterialHandle == first.mMaterialHandle
            ) {
                instanceData.push_back(mOpaqueMeshQueue.top().mModelMatrix);
                mOpaqueMeshQueue.pop();
            }

            // TODO: what is this mVertexArrayObject good for anyway, now?
            mShaderHandle->use();
            glBindVertexArray(mVertexArrayObject);
                first.mMeshHandle->bind(vertexLayout);
                BuiltinModelMatrixAllocator modelMatrixAllocator{instanceData};
                modelMatrixAllocator.bind(BuiltinModelMatrixLayout);

                mShaderHandle->setUFloat(
                    "uMaterial.mSpecularExponent",
                    first.mMaterialHandle->getFloatProperty("specularExponent")
                );

                first.mMaterialHandle->getTextureProperty("textureAlbedo")->bind(0);
                mShaderHandle->setUInt("uMaterial.mTextureAlbedo", 0);
                mShaderHandle->setUBool(
                    "uMaterial.mUsingAlbedoMap",
                    first.mMaterialHandle->getIntProperty("usesTextureAlbedo")
                );

                first.mMaterialHandle->getTextureProperty("textureSpecular")->bind(1);
                mShaderHandle->setUInt("uMaterial.mTextureSpecular", 1);
                mShaderHandle->setUBool(
                    "uMaterial.mUsingSpecularMap",
                    first.mMaterialHandle->getIntProperty("usesTextureSpecular")
                );

                first.mMaterialHandle->getTextureProperty("textureNormal")->bind(2);
                mShaderHandle->setUInt("uMaterial.mTextureNormal", 2);
                mShaderHandle->setUBool(
                    "uMaterial.mUsingNormalMap", 
                    first.mMaterialHandle->getIntProperty("usesTextureNormal")
                );

                mShaderHandle->setUVec4("uMaterial.mColorMultiplier",
                    first.mMaterialHandle->getVec4Property("colorMultiplier")
                );

                glDrawElementsInstanced(
                    GL_TRIANGLES, first.mMeshHandle->getElementCount(),
                    GL_UNSIGNED_INT, nullptr, instanceData.size()
                );
            glBindVertexArray(0);
        }

    mFramebufferHandle->unbind();
}

void LightingRenderStage::setup(const glm::u16vec2& textureDimensions) {
    setTargetViewport({0,0, textureDimensions.x, textureDimensions.y});
    nlohmann::json framebufferDescription = mTemplateFramebufferDescription;
    framebufferDescription["parameters"]["dimensions"][0] = textureDimensions.x;
    framebufferDescription["parameters"]["dimensions"][1] = textureDimensions.y;
    for(nlohmann::json& colorBufferDefinition: framebufferDescription["parameters"]["colorBufferDefinitions"]) {
        colorBufferDefinition["dimensions"][0] = textureDimensions.x;
        colorBufferDefinition["dimensions"][1] = textureDimensions.y;
    }
    mFramebufferHandle = ResourceDatabase::ConstructAnonymousResource<Framebuffer>(framebufferDescription);

    mRenderTargets.clear();
    declareRenderTarget("litScene", 0);
    declareRenderTarget("brightCutoff", 1);
    mShaderHandle->use();
    mShaderHandle->setUniformBlock("Matrices", 0);

    Material::RegisterFloatProperty("brightCutoff", 1.f);
    Material::RegisterIntProperty("screenWidth", 800);
    Material::RegisterIntProperty("screenHeight", 600);
}

void LightingRenderStage::validate() {
    assert(mTextureAttachments.find("positionMap") != mTextureAttachments.end());
    assert(mTextureAttachments.find("normalMap") != mTextureAttachments.end());
    assert(mTextureAttachments.find("albedoSpecularMap") != mTextureAttachments.end());
    assert(mFramebufferHandle->hasOwnRBO());
    assert(std::const_pointer_cast<const Framebuffer>(mFramebufferHandle)->getTargetColorBufferHandles().size() >= 1);
    mFramebufferHandle->bind();
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE && "Could not complete creation of framebuffer");
    mFramebufferHandle->unbind();

    /*
     * TODO: more assertions related to the lighting stage
     */
}

void LightingRenderStage::execute() {
    std::shared_ptr<Material> lightMaterial{ getMaterial("lightMaterial") };
    mShaderHandle->use();
    mShaderHandle->setUInt(
        "uScreenWidth",
        lightMaterial->getIntProperty("screenWidth")
    );
    mShaderHandle->setUInt(
        "uScreenHeight",
        lightMaterial->getIntProperty("screenHeight")
    );
    useViewport();

    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE);

    mFramebufferHandle->bind();
        glClear(GL_COLOR_BUFFER_BIT);

        while(!mLightQueue.empty()) {
            std::vector<glm::mat4> modelMatrices {};
            std::vector<LightEmissionData> lightEmissionList {};

            LightRenderUnit first {mLightQueue.top()};
            mLightQueue.pop();

            modelMatrices.push_back(first.mModelMatrix);
            lightEmissionList.push_back(first.mLightAttributes);

            while(
                !mLightQueue.empty() 
                && mLightQueue.top().mMeshHandle == first.mMeshHandle
            ) {
                const LightRenderUnit renderLightUnit {mLightQueue.top()};
                mLightQueue.pop();

                modelMatrices.push_back(renderLightUnit.mModelMatrix);
                lightEmissionList.push_back(renderLightUnit.mLightAttributes);
            }

            mShaderHandle->use();
            const std::array<const std::string, 3> gBufferAliases {
                "positionMap", "normalMap", "albedoSpecularMap"
            };
            for(int i{0}; i < 3; ++i) {
                mTextureAttachments.at(gBufferAliases[i])->bind(i);
            }
            mShaderHandle->setUInt("uGeometryPositionMap", 0);
            mShaderHandle->setUInt("uGeometryNormalMap", 1);
            mShaderHandle->setUInt("uGeometryAlbedoSpecMap", 2);
            glBindVertexArray(mVertexArrayObject);
                first.mMeshHandle->bind(VertexLayout{{
                    {"position", LOCATION_POSITION, 4, GL_FLOAT}
                }});
                BuiltinModelMatrixAllocator modelMatrixAllocator{ modelMatrices };
                LightInstanceAllocator lightInstanceAllocator{ lightEmissionList, modelMatrices };
                modelMatrixAllocator.bind(BuiltinModelMatrixLayout);
                lightInstanceAllocator.bind({{
                    {"attrLightPlacement_mPosition", RUNTIME, 4, GL_FLOAT},
                    {"attrLightPlacement_mDirection", RUNTIME, 4, GL_FLOAT},

                    {"attrLightEmission_mType", RUNTIME, 1, GL_INT},
                    {"attrLightEmission_mDiffuseColor", RUNTIME, 4, GL_FLOAT},
                    {"attrLightEmission_mSpecularColor", RUNTIME, 4, GL_FLOAT},
                    {"attrLightEmission_mAmbientColor", RUNTIME, 4, GL_FLOAT},
                    {"attrLightEmission_mDecayLinear", RUNTIME, 1, GL_FLOAT},
                    {"attrLightEmission_mDecayQuadratic", RUNTIME, 1, GL_FLOAT},
                    {"attrLightEmission_mCosCutoffInner", RUNTIME, 1, GL_FLOAT},
                    {"attrLightEmission_mCosCutoffOuter", RUNTIME, 1, GL_FLOAT}
                }});
                glDrawElementsInstanced(
                    GL_TRIANGLES, first.mMeshHandle->getElementCount(),
                    GL_UNSIGNED_INT, nullptr, lightEmissionList.size()
                );
            glBindVertexArray(0);
        }
    mFramebufferHandle->unbind();
}

void SkyboxRenderStage::setup(const glm::u16vec2& textureDimensions) {
    setTargetViewport({0, 0, textureDimensions.x, textureDimensions.y});
    nlohmann::json framebufferDescription = mTemplateFramebufferDescription;
    framebufferDescription["parameters"]["dimensions"][0] = textureDimensions.x;
    framebufferDescription["parameters"]["dimensions"][1] = textureDimensions.y;
    for(nlohmann::json& colorBufferDefinition: framebufferDescription["parameters"]["colorBufferDefinitions"]) {
        colorBufferDefinition["dimensions"][0] = textureDimensions.x;
        colorBufferDefinition["dimensions"][1] = textureDimensions.y;
    }
    mFramebufferHandle = ResourceDatabase::ConstructAnonymousResource<Framebuffer>(framebufferDescription);
    mRenderTargets.clear();
    declareRenderTarget("litSceneWithSkybox", 0);
}

void SkyboxRenderStage::validate() {
    assert(1 == std::const_pointer_cast<const Framebuffer>(mFramebufferHandle)->getTargetColorBufferHandles().size() && "The skybox render stage\
    must have exactly one color attachment");
    assert(hasAttachedRBO() && "An RBO must be attached to the skybox render stage");
    assert(mTextureAttachments.find("skybox") != mTextureAttachments.end() && "Skybox render stage requires a cubemap texture to be attached");
    assert(mMeshAttachments.find("unitCube") != mMeshAttachments.end() && "Skybox must have cube mesh attached");
    mFramebufferHandle->bind();
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE && "Could not complete creation of framebuffer");
    mFramebufferHandle->unbind();
}

void SkyboxRenderStage::execute() {
    mShaderHandle->use();
    useViewport();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_BLEND);

    mFramebufferHandle->bind();
        const VertexLayout vertexLayout {{
            { "position", LOCATION_POSITION, 4, GL_FLOAT },
            { "UV1", LOCATION_UV1, 2, GL_FLOAT },
        }};

        glBindVertexArray(mVertexArrayObject);
            std::shared_ptr<StaticMesh> unitCube { mMeshAttachments.at("unitCube") };
            unitCube->bind(vertexLayout);

            mTextureAttachments.at("skybox")->bind(0);
            mShaderHandle->setUInt("textureSkybox", 0);
            getRenderTarget("litSceneWithSkybox")->attachToFramebuffer(0);

            glDrawElementsInstanced(
                GL_TRIANGLES, unitCube->getElementCount(),
                GL_UNSIGNED_INT, nullptr, 1
            );
        glBindVertexArray(0);
    mFramebufferHandle->unbind();
}

void BlurRenderStage::setup(const glm::u16vec2& textureDimensions) {
    setTargetViewport({0,0, textureDimensions.x, textureDimensions.y});
    nlohmann::json framebufferDescription = mTemplateFramebufferDescription;
    framebufferDescription["parameters"]["dimensions"][0] = textureDimensions.x;
    framebufferDescription["parameters"]["dimensions"][1] = textureDimensions.y;
    for(nlohmann::json& colorBufferDefinition: framebufferDescription["parameters"]["colorBufferDefinitions"]) {
        colorBufferDefinition["dimensions"][0] = textureDimensions.x;
        colorBufferDefinition["dimensions"][1] = textureDimensions.y;
    }
    mFramebufferHandle = ResourceDatabase::ConstructAnonymousResource<Framebuffer>(framebufferDescription);
    Material::RegisterIntProperty("nBlurPasses", 12);

    attachMesh("screenMesh", ResourceDatabase::GetRegisteredResource<StaticMesh>("screenRectangleMesh"));
    mRenderTargets.clear();
    declareRenderTarget("pingBuffer", 0);
    declareRenderTarget("pongBuffer", 1);
    attachMaterial("screenMaterial", std::make_shared<Material>());
}

void BlurRenderStage::validate() {
    assert(2 == std::const_pointer_cast<const Framebuffer>(mFramebufferHandle)->getTargetColorBufferHandles().size());
    assert(mMeshAttachments.find("screenMesh") != mMeshAttachments.end());
    assert(mTextureAttachments.find("unblurredImage") != mTextureAttachments.end());
    mFramebufferHandle->bind();
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE && "Could not complete creation of framebuffer");
    mFramebufferHandle->unbind();

    /*
     * TODO: assert various other things related to rendering a blur
     */
}

void BlurRenderStage::execute() {
    mShaderHandle->use();
    useViewport();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_BLEND);

    mFramebufferHandle->bind();
        glClear(GL_COLOR_BUFFER_BIT);
        const std::vector<GLenum> drawbufferEnums {
            {GL_COLOR_ATTACHMENT0},
            {GL_COLOR_ATTACHMENT1}
        };
        glDrawBuffers(2, drawbufferEnums.data()); // clear both buffers
        const int nPasses {  mMaterialAttachments
            .at("screenMaterial")->getIntProperty("nBlurPasses")
        };
        mTextureAttachments.at("unblurredImage")->bind(0);
        mShaderHandle->setUInt("uGenericTexture", 0);
        mShaderHandle->setUBool("uHorizontal", false);
        std::shared_ptr<StaticMesh> screenMeshHandle { mMeshAttachments.at("screenMesh") };

        // In the first pass, draw to the pong buffer, then 
        // alternate between ping and pong
        std::vector<std::shared_ptr<Texture>> pingpongBuffers {
            mFramebufferHandle->getTargetColorBufferHandles()
        };

        glDrawBuffer(GL_COLOR_ATTACHMENT1);
        // Blur our image along one axis
        glBindVertexArray(mVertexArrayObject);
            screenMeshHandle->bind({{
                {"position", LOCATION_POSITION, 4, GL_FLOAT},
                {"UV1", LOCATION_UV1, 2, GL_FLOAT}
            }});
            for(int i{0}; i < nPasses; ++i) {
                glDrawElementsInstanced(
                    GL_TRIANGLES, screenMeshHandle->getElementCount(), 
                    GL_UNSIGNED_INT, nullptr, 1
                );

                // Prepare for the next pass, flipping color buffers
                // and axis
                pingpongBuffers[(1+i)%2]->bind(0);
                mShaderHandle->setUBool("uHorizontal", (1+i)%2);
                glDrawBuffer(GL_COLOR_ATTACHMENT0 + i%2);
            }
        glBindVertexArray(0);
    mFramebufferHandle->unbind();
}

void TonemappingRenderStage::setup(const glm::u16vec2& textureDimensions) {
    setTargetViewport({0,0, textureDimensions.x, textureDimensions.y});
    nlohmann::json framebufferDescription = mTemplateFramebufferDescription;
    framebufferDescription["parameters"]["dimensions"][0] = textureDimensions.x;
    framebufferDescription["parameters"]["dimensions"][1] = textureDimensions.y;
    for(nlohmann::json& colorBufferDefinition: framebufferDescription["parameters"]["colorBufferDefinitions"]) {
        colorBufferDefinition["dimensions"][0] = textureDimensions.x;
        colorBufferDefinition["dimensions"][1] = textureDimensions.y;
    }
    mFramebufferHandle = ResourceDatabase::ConstructAnonymousResource<Framebuffer>(framebufferDescription);

    Material::RegisterFloatProperty("exposure", 1.f);
    Material::RegisterFloatProperty("gamma", 2.4f);
    Material::RegisterIntProperty("combine", true);

    mRenderTargets.clear();
    declareRenderTarget("tonemappedScene", 0);
    attachMesh("screenMesh", ResourceDatabase::GetRegisteredResource<StaticMesh>("screenRectangleMesh"));
    attachMaterial("screenMaterial", std::make_shared<Material>());
}
void TonemappingRenderStage::validate() {
    assert(mMeshAttachments.find("screenMesh") != mMeshAttachments.end());
    assert(mTextureAttachments.find("litScene") != mTextureAttachments.end());
    assert(mTextureAttachments.find("bloomEffect") != mTextureAttachments.end());
    assert(std::const_pointer_cast<const Framebuffer>(mFramebufferHandle)->getTargetColorBufferHandles().size() >= 1);
    mFramebufferHandle->bind();
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE && "Could not complete creation of framebuffer");
    mFramebufferHandle->unbind();
}

void TonemappingRenderStage::execute() {
    mShaderHandle->use();
    useViewport();
    std::shared_ptr<Material> screenMaterial {getMaterial("screenMaterial")};

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_BLEND);
    mFramebufferHandle->bind();
        glClear(GL_COLOR_BUFFER_BIT);
        mTextureAttachments.at("litScene")->bind(0);
        /** TODO: Implement proper bloom some time in the future */
        // mTextureAttachments.at("bloomEffect")->bind(1);
        mShaderHandle->setUInt("uGenericTexture", 0);
        // mShaderHandle->setUInt("uGenericTexture1", 1);
        mShaderHandle->setUFloat("uExposure", 
            screenMaterial->getFloatProperty("exposure")
        );
        mShaderHandle->setUFloat("uGamma", 
            screenMaterial->getFloatProperty("gamma")
        );
        mShaderHandle->setUInt("uCombine", 
            screenMaterial->getIntProperty("combine")
        );
        glBindVertexArray(mVertexArrayObject);
            mMeshAttachments.at("screenMesh")->bind({{
                {"position", LOCATION_POSITION, 4, GL_FLOAT},
                {"UV1", LOCATION_UV1, 2, GL_FLOAT}
            }});
            glDrawElementsInstanced(
                GL_TRIANGLES,
                mMeshAttachments.at("screenMesh")->getElementCount(),
                GL_UNSIGNED_INT,
                nullptr,
                1
            );
        glBindVertexArray(0);
    mFramebufferHandle->unbind();
}

void AdditionRenderStage::setup(const glm::u16vec2& textureDimensions) {
    setTargetViewport({0, 0, textureDimensions.x, textureDimensions.y});
    nlohmann::json framebufferDescription = mTemplateFramebufferDescription;
    framebufferDescription["parameters"]["dimensions"][0] = textureDimensions.x;
    framebufferDescription["parameters"]["dimensions"][1] = textureDimensions.y;
    for(nlohmann::json& colorBufferDefinition: framebufferDescription["parameters"]["colorBufferDefinitions"]) {
        colorBufferDefinition["dimensions"][0] = textureDimensions.x;
        colorBufferDefinition["dimensions"][1] = textureDimensions.y;
    }
    mFramebufferHandle = ResourceDatabase::ConstructAnonymousResource<Framebuffer>(framebufferDescription);

    mRenderTargets.clear();
    declareRenderTarget("textureSum", 0);
    attachMesh("screenMesh", ResourceDatabase::GetRegisteredResource<StaticMesh>("screenRectangleMesh"));
    attachMaterial("screenMaterial", std::make_shared<Material>());
}

void AdditionRenderStage::validate() {
    assert(mMeshAttachments.find("screenMesh") != mMeshAttachments.end());
    assert(std::const_pointer_cast<const Framebuffer>(mFramebufferHandle)->getTargetColorBufferHandles().size() >= 1);
    mFramebufferHandle->bind();
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE && "Could not complete creation of framebuffer");
    mFramebufferHandle->unbind();
}

void AdditionRenderStage::execute() {
    mShaderHandle->use();
    useViewport();
    std::shared_ptr<Material> screenMaterial {getMaterial("screenMaterial")};

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glEnable(GL_BLEND);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);

    mFramebufferHandle->bind();
        glClear(GL_COLOR_BUFFER_BIT);

        std::string textureAddendName{ "textureAddend_0" };
        while(mTextureAttachments.find(textureAddendName) != mTextureAttachments.end()) {
            std::size_t nCombineTextures { 0 };
            // batch textures in groups of 4
            while(nCombineTextures < 4 * (nCombineTextures/4 + 1) && mTextureAttachments.find(textureAddendName) != mTextureAttachments.end()) {
                std::shared_ptr<Texture> currentTextureAttachment { mTextureAttachments.at(textureAddendName) };
                const std::string samplerUniformName {
                    std::string{"uCombineTextures["}
                    + std::to_string(nCombineTextures)
                    + std::string{"]"}
                };
                currentTextureAttachment->bind(nCombineTextures);
                mShaderHandle->setUInt(samplerUniformName, nCombineTextures);

                textureAddendName = std::string{"textureAddend_"} + std::to_string(++nCombineTextures);
            }
            mShaderHandle->setUInt("nCombine", nCombineTextures);
            glBindVertexArray(mVertexArrayObject);
                mMeshAttachments.at("screenMesh")->bind({{
                    {"position", LOCATION_POSITION, 4, GL_FLOAT},
                    {"UV1", LOCATION_UV1, 2, GL_FLOAT},
                }});
                glDrawElementsInstanced(
                    GL_TRIANGLES,
                    mMeshAttachments.at("screenMesh")->getElementCount(),
                    GL_UNSIGNED_INT,
                    nullptr,
                    1
                );
            glBindVertexArray(0);
        }

    mFramebufferHandle->unbind();
    mTextureAttachments.clear();
}

void ScreenRenderStage::setup(const glm::u16vec2& targetDimensions) {
    setTargetViewport({0,0, targetDimensions.x, targetDimensions.y});
    attachMesh("screenMesh", ResourceDatabase::GetRegisteredResource<StaticMesh>("screenRectangleMesh"));
}

void ScreenRenderStage::validate() {
    assert(mMeshAttachments.find("screenMesh") != mMeshAttachments.end());
    assert(mTextureAttachments.find("renderSource") != mTextureAttachments.end());
}

void ScreenRenderStage::execute() {
    mShaderHandle->use();
    useViewport();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_BLEND);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindVertexArray(mVertexArrayObject);
        mTextureAttachments.at("renderSource")->bind(0);
        mShaderHandle->setUInt("uGenericTexture", 0);
        mMeshAttachments.at("screenMesh")->bind({{
            {"position", LOCATION_POSITION, 4, GL_FLOAT},
            {"UV1", LOCATION_UV1, 2, GL_FLOAT}
        }});
        glDrawElementsInstanced(
            GL_TRIANGLES,
            mMeshAttachments.at("screenMesh")->getElementCount(),
            GL_UNSIGNED_INT,
            nullptr,
            1
        );
    glBindVertexArray(0);
}

void ResizeRenderStage::setup(const glm::u16vec2& textureDimensions) {
    setTargetViewport({0, 0, textureDimensions.x, textureDimensions.y});
    nlohmann::json framebufferDescription = mTemplateFramebufferDescription;
    framebufferDescription["parameters"]["dimensions"][0] = textureDimensions.x;
    framebufferDescription["parameters"]["dimensions"][1] = textureDimensions.y;
    for(nlohmann::json& colorBufferDefinition: framebufferDescription["parameters"]["colorBufferDefinitions"]) {
        colorBufferDefinition["dimensions"][0] = textureDimensions.x;
        colorBufferDefinition["dimensions"][1] = textureDimensions.y;
    }
    mFramebufferHandle = ResourceDatabase::ConstructAnonymousResource<Framebuffer>(framebufferDescription);
    attachMesh("screenMesh", ResourceDatabase::GetRegisteredResource<StaticMesh>("screenRectangleMesh"));
    mRenderTargets.clear();
    declareRenderTarget("resizedTexture", 0);
}

void ResizeRenderStage::validate() {
    assert(mMeshAttachments.find("screenMesh") != mMeshAttachments.end());
    assert(mTextureAttachments.find("renderSource") != mTextureAttachments.end());
    mFramebufferHandle->bind();
        assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE && "Could not complete creation of framebuffer");
    mFramebufferHandle->unbind();
}

void ResizeRenderStage::execute() {
    mShaderHandle->use();
    useViewport();
    glDisable(GL_BLEND);
    glDisable(GL_FRAMEBUFFER_SRGB);
    glDisable(GL_DEPTH_TEST);
    mFramebufferHandle->bind();
        glClear(GL_COLOR_BUFFER_BIT);
        glBindVertexArray(mVertexArrayObject);
            mTextureAttachments.at("renderSource")->bind(0);
            mShaderHandle->setUInt("uGenericTexture", 0);
            mMeshAttachments.at("screenMesh")->bind({{
                {"position", LOCATION_POSITION, 4, GL_FLOAT},
                {"UV1", LOCATION_UV1, 2, GL_FLOAT}
            }});
            glDrawElementsInstanced(
                GL_TRIANGLES,
                mMeshAttachments.at("screenMesh")->getElementCount(),
                GL_UNSIGNED_INT,
                nullptr,
                1
            );
        glBindVertexArray(0);
    mFramebufferHandle->unbind();
}

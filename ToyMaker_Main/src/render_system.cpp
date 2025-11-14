#include <iostream>
#include <array>

#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

#include "toymaker/engine/core/resource_database.hpp"
#include "toymaker/engine/core/ecs_world.hpp"
#include "toymaker/engine/window_context_manager.hpp"

#include "toymaker/engine/light.hpp"
#include "toymaker/engine/camera_system.hpp"
#include "toymaker/engine/render_stage.hpp"
#include "toymaker/engine/model.hpp"

#include "toymaker/engine/render_system.hpp"

using namespace ToyMaker;

constexpr float MAX_GAMMA  { 3.f };
constexpr float MIN_GAMMA { 1.6f };
constexpr float MAX_EXPOSURE { 15.f };
constexpr float MIN_EXPOSURE { 0.f };

void RenderSystem::LightQueue::onInitialize() {
    if(!ResourceDatabase::HasResourceDescription("sphereLight-10lat-5long")) {
        nlohmann::json sphereLightDescription {
            {"name", "sphereLight-10lat-5long"},
            {"type", StaticMesh::getResourceTypeName()},
            {"method", StaticMeshSphereLatLong::getResourceConstructorName()},
            {"parameters", {
                {"nLatitudes", 10},
                {"nMeridians", 5}
            }}
        };
        ResourceDatabase::AddResourceDescription(sphereLightDescription);
    }
    mSphereMesh = ResourceDatabase::GetRegisteredResource<StaticMesh>("sphereLight-10lat-5long");
}

void RenderSystem::onInitialize() {
    // Set up a uniform buffer for shared matrices
    // TODO: Is there a generalization or abstraction for the problem of uniform buffers that
    // could be useful going forward?
    glGenBuffers(1, &mMatrixUniformBufferIndex);
    glBindBuffer(GL_UNIFORM_BUFFER, mMatrixUniformBufferIndex);
        glBufferData(
            GL_UNIFORM_BUFFER,
            2*sizeof(glm::mat4),
            NULL,
            GL_STATIC_DRAW
        );
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    if(!ResourceDatabase::HasResourceDescription("screenRectangleMesh")) {
        nlohmann::json rectangleMeshDefinition {
            {"name", "screenRectangleMesh"},
            {"type", StaticMesh::getResourceTypeName()},
            {"method", StaticMeshRectangleDimensions::getResourceConstructorName()},
            {"parameters", {
                {"width", 2.f},
                {"height", 2.f},
            }}
        };
        ResourceDatabase::AddResourceDescription(rectangleMeshDefinition);
    }
}

void RenderSystem::execute(float simulationProgress) {
    assert(mRenderSets.find(mActiveRenderSetID) != mRenderSets.end() && "No render set corresponding to the currently set active render set exists");

    if(mRenderSets.at(mActiveRenderSetID).mRenderType != RenderSet::RenderType::ADDITION) {
        assert(
            getEnabledEntities().find(mRenderSets.at(mActiveRenderSetID).mActiveCamera) != getEnabledEntities().end()
            && "The camera specified for this render set is not enabled or does not exist"
        );
        updateCameraMatrices(simulationProgress);
    }

    // Execute each rendering stage in its proper order
    switch(mRenderSets.at(mActiveRenderSetID).mRenderType) {
        case RenderSet::RenderType::BASIC_3D:
            // Bind the shared matrix uniform buffer to the camera matrix binding point
            // TODO: Is there a generalization or abstraction for the problem of uniform buffers that
            // could be useful going forward?
            glBindBufferRange(
                GL_UNIFORM_BUFFER,
                mMatrixUniformBufferBinding,
                mMatrixUniformBufferIndex,
                0,
                2*sizeof(glm::mat4)
            );

            mWorld.lock()->getSystem<OpaqueQueue>()->enqueueTo(*mRenderSets.at(mActiveRenderSetID).mGeometryRenderStage, simulationProgress);
            mRenderSets.at(mActiveRenderSetID).mGeometryRenderStage->execute();
            mWorld.lock()->getSystem<LightQueue>()->enqueueTo(*mRenderSets.at(mActiveRenderSetID).mLightingRenderStage, simulationProgress);
            mRenderSets.at(mActiveRenderSetID).mLightingRenderStage->execute();
            if(mSkyboxTexture) {
                mRenderSets.at(mActiveRenderSetID).mSkyboxRenderStage->execute();
            }
            /** Implement a proper bloom some time in the future */
            // mRenderSets.at(mActiveRenderSetID).mBlurRenderStage->execute();
            mRenderSets.at(mActiveRenderSetID).mTonemappingRenderStage->execute();
        break;

        case RenderSet::RenderType::ADDITION:
            for(auto& textureSource: mRenderSets.at(mActiveRenderSetID).mRenderSources) {
                mRenderSets
                    .at(mActiveRenderSetID)
                    .mAdditionRenderStage->attachTexture(textureSource.first, textureSource.second);
            }

            mRenderSets.at(mActiveRenderSetID).mAdditionRenderStage->execute();
        break;
    }

    if(GLenum openglError = glGetError()) {
        std::cout << "OpenGL error: " << openglError << ", " << glewGetErrorString(openglError) << std::endl;
        assert(!openglError && "Error during render system execution step");
    }

    mRenderSets.at(mActiveRenderSetID).mRerendered = true;
}

void RenderSystem::updateCameraMatrices(float simulationProgress) {
    CameraProperties cameraProps { getComponent<CameraProperties>(mRenderSets[mActiveRenderSetID].mActiveCamera, simulationProgress) };
    // Send shared matrices to the uniform buffer
    // TODO: Is there a generalization or abstraction for the problem of uniform buffers that
    // could be useful going forward?
    glBindBuffer(GL_UNIFORM_BUFFER, mMatrixUniformBufferIndex);

        glBufferSubData(
            GL_UNIFORM_BUFFER,
            0,
            sizeof(glm::mat4),
            glm::value_ptr(cameraProps.mProjectionMatrix)
        );

        glBufferSubData(
            GL_UNIFORM_BUFFER,
            sizeof(glm::mat4),
            sizeof(glm::mat4),
            glm::value_ptr(cameraProps.mViewMatrix)
        );

    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

RenderSetID RenderSystem::createRenderSet(glm::u16vec2 renderDimensions, glm::u16vec2 targetDimensions, const SDL_Rect& viewportDimensions, RenderSet::RenderType renderType) {
    RenderSetID newRenderSetID;

    if(!mDeletedRenderSetIDs.empty()) {
        newRenderSetID = *mDeletedRenderSetIDs.begin();
        mDeletedRenderSetIDs.erase(newRenderSetID);

    } else {
        assert(mNextRenderSetID + 1 < kMaxRenderSetIDs && "Maximum allocatable render sets have been created");
        newRenderSetID = mNextRenderSetID++;
    }

    RenderSet&& newRenderSet {};
    newRenderSet.mGeometryRenderStage = std::make_shared<GeometryRenderStage>("data/shader/geometryShader.json" );
    newRenderSet.mLightingRenderStage = std::make_shared<LightingRenderStage>("data/shader/lightingShader.json");
    newRenderSet.mSkyboxRenderStage = std::make_shared<SkyboxRenderStage>("data/shader/skyboxShader.json");
    newRenderSet.mBlurRenderStage = std::make_shared<BlurRenderStage>("data/shader/gaussianblurShader.json");
    newRenderSet.mTonemappingRenderStage = std::make_shared<TonemappingRenderStage>("data/shader/tonemappingShader.json");
    newRenderSet.mResizeRenderStage = std::make_shared<ResizeRenderStage>("data/shader/basicShader.json");
    newRenderSet.mScreenRenderStage = std::make_shared<ScreenRenderStage>("data/shader/basicShader.json");
    newRenderSet.mAdditionRenderStage = std::make_shared<AdditionRenderStage>("data/shader/combineShader.json");

    newRenderSet.mLightMaterialHandle = ResourceDatabase::ConstructAnonymousResource<Material>({
        {"type", Material::getResourceTypeName()},
        {"method", MaterialFromDescription::getResourceConstructorName()},
        {"parameters", {
            {"properties", nlohmann::json::array()},
        }}
    });

    // TODO: Make it so that we can control which camera is used by the render
    // system, and what portion of the screen it renders to
    newRenderSet.setRenderProperties(renderDimensions, targetDimensions, viewportDimensions, renderType);
    newRenderSet.setGamma(newRenderSet.mGamma);
    newRenderSet.setExposure(newRenderSet.mExposure);
    mRenderSets[newRenderSetID] = std::move(newRenderSet);

    if(mSkyboxTexture) {
        setSkybox(mSkyboxTexture);
    }
    return newRenderSetID;
}

void RenderSystem::useRenderSet(RenderSetID renderSet) {
    assert(mRenderSets.find(renderSet) != mRenderSets.end() && "No render set corresponding to the id provided was found");
    mActiveRenderSetID = renderSet;
}

void RenderSystem::setRenderProperties(glm::u16vec2 renderDimensions, glm::u16vec2 targetDimensions, const SDL_Rect& viewportDimensions, RenderSet::RenderType renderType) {
    mRenderSets.at(mActiveRenderSetID).setRenderProperties(renderDimensions, targetDimensions, viewportDimensions, renderType);
    if(mSkyboxTexture) {
        setSkybox(mSkyboxTexture);
    }
}

void RenderSet::setRenderProperties(glm::u16vec2 renderDimensions, glm::u16vec2 targetDimensions, const SDL_Rect& viewportDimensions, RenderType renderType) {
    mRenderType = renderType;

    mGeometryRenderStage->setup(renderDimensions);
    mLightingRenderStage->setup(renderDimensions);
    mSkyboxRenderStage->setup(renderDimensions);
    mBlurRenderStage->setup(renderDimensions);
    mTonemappingRenderStage->setup(renderDimensions);
    mAdditionRenderStage->setup(renderDimensions);

    mResizeRenderStage->setup(targetDimensions);
    mScreenRenderStage->setup(targetDimensions);

    mResizeRenderStage->setTargetViewport(viewportDimensions);

    mLightMaterialHandle->updateIntProperty("screenWidth", renderDimensions.x);
    mLightMaterialHandle->updateIntProperty("screenHeight", renderDimensions.y);

    // Debug: list of screen textures that may be rendered
    mScreenTextures.clear();

    mScreenTextures.push_back({mAdditionRenderStage->getRenderTarget("textureSum")});
    mScreenTextures.push_back({mGeometryRenderStage->getRenderTarget("geometryPosition")});
    mScreenTextures.push_back({mGeometryRenderStage->getRenderTarget("geometryNormal")});
    mScreenTextures.push_back({mGeometryRenderStage->getRenderTarget("geometryAlbedoSpecular")});
    mScreenTextures.push_back({mLightingRenderStage->getRenderTarget("litScene")}); 
    mScreenTextures.push_back({mLightingRenderStage->getRenderTarget("brightCutoff")});
    mScreenTextures.push_back({mBlurRenderStage->getRenderTarget("pingBuffer")});
    mScreenTextures.push_back({mTonemappingRenderStage->getRenderTarget("tonemappedScene")});

    /**
     * TODO: remove this hacky render debug system, replace it with something that can be configured from file 
     */
    if(mRenderType == RenderType::ADDITION) {
        mCurrentScreenTexture = 0;
    }

    // Last pieces of pipeline setup, where we connect all the
    // render stages together
    mLightingRenderStage->attachTexture("positionMap", mGeometryRenderStage->getRenderTarget("geometryPosition"));
    mLightingRenderStage->attachTexture("normalMap", mGeometryRenderStage->getRenderTarget("geometryNormal"));
    mLightingRenderStage->attachTexture("albedoSpecularMap", mGeometryRenderStage->getRenderTarget("geometryAlbedoSpecular"));
    mLightingRenderStage->attachMaterial("lightMaterial", mLightMaterialHandle);
    mBlurRenderStage->attachTexture("unblurredImage", mLightingRenderStage->getRenderTarget("brightCutoff"));
    mTonemappingRenderStage->attachTexture("litScene", mLightingRenderStage->getRenderTarget("litScene"));
    mTonemappingRenderStage->attachTexture("bloomEffect", mBlurRenderStage->getRenderTarget("pingBuffer"));
    mResizeRenderStage->attachTexture("renderSource", mScreenTextures[mCurrentScreenTexture]);
    mScreenRenderStage->attachTexture("renderSource", mResizeRenderStage->getRenderTarget("resizedTexture"));

    // Functions containing a set of asserts, ensuring that valid connections have
    // been made between the rendering stages
    mGeometryRenderStage->validate();
    mLightingRenderStage->validate();
    mBlurRenderStage->validate();
    mTonemappingRenderStage->validate();
    mResizeRenderStage->validate();
    mAdditionRenderStage->validate();
    mScreenRenderStage->validate();

    glClearColor(0.f, 0.f, 0.f, 0.f);
    mRerendered = true;
}

void RenderSystem::deleteRenderSet(RenderSetID renderSet) {
    if(mRenderSets.find(renderSet) == mRenderSets.end()) { return; }
    mRenderSets.erase(renderSet);
    mDeletedRenderSetIDs.insert(renderSet);
}

void RenderSystem::renderNextTexture() {
    mRenderSets.at(mActiveRenderSetID).renderNextTexture();
}
void RenderSet::renderNextTexture() {
    mCurrentScreenTexture = (mCurrentScreenTexture + 1) % mScreenTextures.size();
    mRerendered=true;
}

std::shared_ptr<Texture> RenderSystem::getCurrentScreenTexture() {
    return mRenderSets.at(mActiveRenderSetID).getCurrentScreenTexture();
}

std::shared_ptr<Texture> RenderSet::getCurrentScreenTexture() {
    if(mRerendered) {
        copyAndResize();
        mRerendered = false;
    }
    return mResizeRenderStage->getRenderTarget("resizedTexture");
}

void RenderSystem::renderToScreen() {
    getCurrentScreenTexture();
    mRenderSets.at(mActiveRenderSetID).mScreenRenderStage->execute();
    WindowContext::getInstance().swapBuffers();
}

void RenderSystem::copyAndResize() { 
    mRenderSets.at(mActiveRenderSetID).copyAndResize();
}

void RenderSet::copyAndResize() {
    mResizeRenderStage->attachTexture("renderSource", mScreenTextures[mCurrentScreenTexture]);
    mResizeRenderStage->validate();
    mResizeRenderStage->execute();
}

void RenderSystem::OpaqueQueue::enqueueTo(BaseRenderStage& renderStage, float simulationProgress) {
    for(EntityID entity: getEnabledEntities()) {
        std::shared_ptr<StaticModel> modelHandle { getComponent<std::shared_ptr<StaticModel>>(entity) };
        Transform entityTransform { getComponent<Transform>(entity, simulationProgress) };
        const std::vector<std::shared_ptr<StaticMesh>>& meshList { modelHandle->getMeshHandles() };
        const std::vector<std::shared_ptr<Material>>& materialList { modelHandle->getMaterialHandles() };

        for(std::size_t i{0}; i < meshList.size(); ++i) {
            renderStage.submitToRenderQueue({
                meshList[i],
                materialList[i],
                entityTransform.mModelMatrix
            });
        }
    }
}

void RenderSystem::LightQueue::enqueueTo(BaseRenderStage& renderStage, float simulationProgress) {
    std::shared_ptr<StaticMesh> screenRectangleMesh { 
        ResourceDatabase::GetRegisteredResource<StaticMesh>("screenRectangleMesh")
    };
    for(EntityID entity: getEnabledEntities()) {
        const Transform entityTransform { getComponent<Transform>(entity, simulationProgress)};
        const LightEmissionData lightEmissionData { getComponent<LightEmissionData>(entity, simulationProgress) };

        renderStage.submitToRenderQueue(LightRenderUnit {
            (lightEmissionData.mType==LightEmissionData::LightType::directional)?
                screenRectangleMesh:
                mSphereMesh,
            lightEmissionData,
            (lightEmissionData.mType==LightEmissionData::LightType::directional)?
                glm::inverse(glm::transpose(entityTransform.mModelMatrix)):
                glm::scale(entityTransform.mModelMatrix, glm::vec3{lightEmissionData.mRadius})
        });
    }
}

void RenderSystem::setSkybox(std::shared_ptr<Texture> skyboxTexture) {
    mSkyboxTexture = skyboxTexture;
    if(mSkyboxTexture) {
        for(auto renderSet: mRenderSets) {
            std::shared_ptr<Texture> litSceneTexture {
                renderSet.second.mLightingRenderStage->getRenderTarget("litScene")
            };
            renderSet.second.mSkyboxRenderStage->setup(glm::vec2{litSceneTexture->getWidth(), litSceneTexture->getHeight()});
            renderSet.second.mSkyboxRenderStage->attachRBO(renderSet.second.mGeometryRenderStage->getOwnRBO());
            const std::size_t targetID { renderSet.second.mSkyboxRenderStage->attachTextureAsTarget(litSceneTexture) };
            assert(targetID == 0 && "Too many texture targets specified for skybox render stage");
            renderSet.second.mSkyboxRenderStage->attachTexture("skybox", mSkyboxTexture);
            renderSet.second.mSkyboxRenderStage->attachMesh("unitCube", ResourceDatabase::ConstructAnonymousResource<StaticMesh>({
                {"type", StaticMesh::getResourceTypeName()},
                {"method", StaticMeshCuboidDimensions::getResourceConstructorName()},
                {"parameters", {
                    {"depth", 2.f},
                    {"width", 2.f},
                    {"height", 2.f},
                    {"layout", mSkyboxTexture->getColorBufferDefinition().mCubemapLayout},
                    {"flip_texture_y", true},
                }},
            }));
            renderSet.second.mSkyboxRenderStage->validate();
            renderSet.second.mRerendered = true;
        }
    } else {
        for(auto renderSet: mRenderSets) {
            renderSet.second.mRerendered = true;
        }
    }
}

void RenderSystem::addOrAssignRenderSource(const std::string& name, std::shared_ptr<Texture> renderSource)  {
    mRenderSets[mActiveRenderSetID].addOrAssignRenderSource(name, renderSource);
}

void RenderSet::addOrAssignRenderSource(const std::string& name, std::shared_ptr<Texture> renderSource)  {
    assert(renderSource && "Null pointers are invalid render sources");
    mRenderSources[name] = renderSource;
}

void RenderSystem::removeRenderSource(const std::string& name)  {
    mRenderSets[mActiveRenderSetID].removeRenderSource(name);
}

void RenderSet::removeRenderSource(const std::string& name) {
    mRenderSources.erase(name);
}

void RenderSystem::setCamera(EntityID cameraEntity) {
    mRenderSets.at(mActiveRenderSetID).setCamera(cameraEntity);
}

void RenderSet::setCamera(EntityID cameraEntity) {
    mActiveCamera = cameraEntity;
}

void RenderSystem::setGamma(float gamma) { 
    mRenderSets.at(mActiveRenderSetID).setGamma(gamma);
}

void RenderSet::setGamma(float gamma) {
    if(gamma > MAX_GAMMA) gamma = MAX_GAMMA;
    else if (gamma < MIN_GAMMA) gamma = MIN_GAMMA;

    mTonemappingRenderStage->getMaterial("screenMaterial")->updateFloatProperty(
        "gamma", gamma
    );

    mGamma = gamma;
}

float RenderSystem::getGamma() {
    return mRenderSets.at(mActiveRenderSetID).getGamma();
}

float RenderSet::getGamma() {
    return mGamma;
}

void RenderSystem::setExposure(float exposure) {
    mRenderSets.at(mActiveRenderSetID).setExposure(exposure);
}

void RenderSet::setExposure(float exposure) {
    if(exposure > MAX_EXPOSURE) exposure = MAX_EXPOSURE;
    else if (exposure < MIN_EXPOSURE) exposure = MIN_EXPOSURE;

    mTonemappingRenderStage->getMaterial("screenMaterial")->updateFloatProperty(
        "exposure", exposure
    );

    mExposure = exposure;
}

float RenderSystem::getExposure() {
    return mRenderSets[mActiveRenderSetID].getExposure();
}

float RenderSet::getExposure() {
    return mExposure;
}

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "toymaker/engine/window_context_manager.hpp"
#include "toymaker/engine/render_system.hpp"

#include "toymaker/builtins/render_debug_viewer.hpp"

using namespace ToyMaker;

bool RenderDebugViewer::onUpdateGamma(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
    (void)actionDefinition; // prevent unused parameter warning
    getLocalViewport().updateGamma(
        getLocalViewport().getGamma() 
        + actionData.mOneAxisActionData.mValue*mGammaStep
    );
    return true;
}
bool RenderDebugViewer::onUpdateExposure(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
    (void)actionDefinition; // prevent unused parameter warning
    getLocalViewport().updateExposure(
        getLocalViewport().getExposure()
        + actionData.mOneAxisActionData.mValue*mExposureStep
    );
    return true;
}
bool RenderDebugViewer::onRenderNextTexture(const ToyMaker::ActionData& actionData, const ToyMaker::ActionDefinition& actionDefinition) {
    (void)actionData; // prevent unused parameter warning
    (void)actionDefinition; // prevent unused parameter warning
    getLocalViewport().viewNextDebugTexture();
    return true;
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> RenderDebugViewer::clone() const {
    return std::shared_ptr<RenderDebugViewer>(new RenderDebugViewer {});
}

std::shared_ptr<ToyMaker::BaseSimObjectAspect> RenderDebugViewer::create(const nlohmann::json& jsonAspectProperties) {
    (void)jsonAspectProperties; // prevent unused parameter warning
    return std::shared_ptr<RenderDebugViewer>(new RenderDebugViewer {});
}

void RenderDebugViewer::printWindowProps() {
    ToyMaker::WindowContext& windowCtxManager { ToyMaker::WindowContext::getInstance() };
    std::cout << "Window State:\n"
        << "\tdisplay index: " << windowCtxManager.getDisplayID() << "\n"
        << "\ttitle: " << windowCtxManager.getTitle() << "\n"
        << "\tmaximized: " << windowCtxManager.isMaximized() << "\n"
        << "\tminimized: " << windowCtxManager.isMinimized() << "\n"
        << "\tresizable: " << windowCtxManager.isResizable() << "\n"
        << "\thidden: " << windowCtxManager.isHidden() << "\n"
        << "\tshown: " << windowCtxManager.isShown() << "\n"
        << "\tmouse focus: " << windowCtxManager.hasMouseFocus() << "\n"
        << "\tmouse capture: " << windowCtxManager.hasCapturedMouse() << "\n"
        << "\tkey focus: " << windowCtxManager.hasKeyFocus() << "\n"
        << "\tfullscreen: " << windowCtxManager.isFullscreen() << "\n"
        << "\tborderless: " << windowCtxManager.isBorderless() << "\n"
        << "\texclusive fullscreen: " << windowCtxManager.isExclusiveFullscreen() << "\n"
        << "\twindow position: " << glm::to_string(windowCtxManager.getPosition()) << "\n"
        << "\twindow dimensions: " << glm::to_string(windowCtxManager.getDimensions()) << "\n"
        << "\tmaximum window dimensions: " << glm::to_string(windowCtxManager.getDimensionsMaximum()) << "\n"
        << "\tminimum window dimensions: " << glm::to_string(windowCtxManager.getDimensionsMinimum()) << "\n"
    ;
}

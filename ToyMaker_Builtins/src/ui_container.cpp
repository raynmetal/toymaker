#include "toymaker/builtins/ui_panel.hpp"

#include "toymaker/builtins/ui_container.hpp"

using namespace ToyMaker;

std::shared_ptr<BaseSimObjectAspect> UIContainer::create(const nlohmann::json& jsonAspectProperties) {
    std::shared_ptr<UIContainer> container { std::make_shared<UIContainer>() };
    return container;
}

std::shared_ptr<BaseSimObjectAspect> UIContainer::clone() const {
    std::shared_ptr<UIContainer> container { std::make_shared<UIContainer>() };
    return container;
}

void UIContainer::onActivated() {
    auto& viewport { getLocalViewport() };
    mObservedViewportResized.connectTo(viewport.mRenderConfigurationUpdated);
    const auto renderConfig { viewport.getRenderConfiguration() };
    onViewportResized(renderConfig);
}

void UIContainer::onViewportResized(const ToyMaker::ViewportNode::RenderConfiguration& config) {
    mContentSize = config.mComputedDimensions;

    // resize the panel associated with this container.
    if(getSimObject().hasAspect<UIPanel>()) {
        UIPanel& panel { getSimObject().getAspect<UIPanel>() };
        panel.updateContentSize(mContentSize);
        panel.updateAnchor({.5f, .5f});
    }

    // collect whichever nodes should be visited first
    std::queue<std::shared_ptr<SceneNodeCore>> toVisit {};
    for(auto node: getSimObject().getChildren()) {
        std::shared_ptr<ToyMaker::SimObject> simObject {
            std::dynamic_pointer_cast<ToyMaker::SimObject>(node)
        };

        // guard: only sim object elements appear here
        if(!simObject || !simObject->hasAspectWithInterface<IUIElement>()) {
            continue;
        }

        toVisit.push(node);
    }

    // compute a few layout constants
    const ToyMaker::Transform transform { getComponent<Transform>() };
    const glm::mat4 scale { transform.getMatrixScale() };
    assert(scale == glm::mat4 { 1.f } && "Scale matrix must be identity matrix");
    const glm::vec3 position { transform.getMatrixTranslation()[3] };
    const glm::mat3 rotation { transform.getMatrixRotation() };
    const glm::vec3 up { rotation * glm::vec3 { 0.f, 1.f, 0.f } };
    const glm::vec3 right { rotation * glm::vec3 { 1.f, 0.f, 0.f } };
    const glm::vec3 out { rotation * glm::vec3 { 0.f, 0.f, 1.f } };
    const glm::vec3 origin { position - (
        .5f * mContentSize.x * right
        - .5f * mContentSize.y * up
    ) };

    // visit child ui elements in breadth-first order
    while(!toVisit.empty()) {
        auto current { toVisit.front() };
        auto currentSim { std::static_pointer_cast<ToyMaker::SimObject>(current) };
        toVisit.pop();

        for(auto aspect: currentSim->getAspectsWithInterface<IUIElement>()) {
            const glm::vec2 referenceCoordinate { aspect.get().getReferenceCoordinate() };
            const glm::vec3 offsets { aspect.get().getOffsets() };
            const glm::vec2 referenceCoordinateContent { mContentSize * referenceCoordinate };
            const glm::vec3 referenceCoordinateWorld { origin
                + referenceCoordinateContent.x * right
                - referenceCoordinateContent.y * up
            };
            const glm::vec3 elementPosition { referenceCoordinateWorld
                + offsets.x * right
                - offsets.y * up
                + offsets.z * out
            };

            ToyMaker::Placement placement { current->getComponent<ToyMaker::Placement>() };
            placement.mPosition = { elementPosition, 1.f };
            current->updateComponent<ToyMaker::Placement>(placement);

            if(!aspect.get().allowsDescendantControl()) {
                continue;
            }

            // add repositionable ui elements to the visit list
            for(auto child: getSimObject().getChildren()) {
                std::shared_ptr<ToyMaker::SimObject> simObject {
                    std::dynamic_pointer_cast<ToyMaker::SimObject>(child)
                };

                // guard: only sim object elements appear here
                if(!simObject || !simObject->hasAspectWithInterface<IUIElement>()) {
                    continue;
                }

                toVisit.push(child);
            }
        }
    }
}


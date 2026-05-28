#include <glm/glm.hpp>

#include <toymaker/engine/scene_components.hpp>

using namespace ToyMaker;

glm::mat4 Transform::getMatrixTranslation() const {
    return {
        glm::vec4 { 1.f, 0.f, 0.f, 0.f },
        glm::vec4 { 0.f, 1.f, 0.f, 0.f },
        glm::vec4 { 0.f, 0.f, 1.f, 0.f },
        mModelMatrix[3]
    };
}




#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

#include <SDL2/SDL.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#include "toymaker/engine/application.hpp"

int main(int argc, char* argv[]) {
    (void)argc; // prevent unused parameter warnings
    (void)argv; // prevent unused parameter warnings

    if(std::shared_ptr<ToyMaker::Application> app { ToyMaker::Application::instantiate("data/project.json") }) {
        app->execute();

    } else return 1;

    return 0;
}

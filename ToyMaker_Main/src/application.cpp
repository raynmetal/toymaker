#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include <thread>
#include <filesystem>

#include <SDL2/SDL.h>
#include <nlohmann/json.hpp>

#include "toymaker/engine/core/ecs_world.hpp"
#include "toymaker/engine/core/resource_database.hpp"

/**
 * @note Since I'm packaging this as a static library, any premade resource type, resource constructor, sim aspect type, or anything derived from a template ought to be included here.
 */

#include "toymaker/engine/window_context_manager.hpp"
#include "toymaker/engine/input_system/input_system.hpp"
#include "toymaker/engine/scene_system.hpp"
#include "toymaker/engine/sim_system.hpp"
#include "toymaker/engine/scene_loading.hpp"
#include "toymaker/engine/material.hpp"
#include "toymaker/engine/render_system.hpp"
#include "toymaker/engine/text_render.hpp"

#include "toymaker/engine/application.hpp"

using namespace ToyMaker;

constexpr uint32_t kMaxSimStep { 5000 };
constexpr uint32_t kMinSimStep { 1000/120 };
constexpr uint32_t kSleepThreshold { 10 };

std::weak_ptr<Application> Application::s_pInstance {};
bool Application::s_instantiated { false };

void printSceneHierarchyData(std::shared_ptr<ToyMaker::SceneNodeCore> rootNode) {
    std::vector<std::pair<std::shared_ptr<ToyMaker::SceneNodeCore>, int>> nodeAndLevel {{rootNode, 0}};
    while(!nodeAndLevel.empty()) {
        std::shared_ptr<ToyMaker::SceneNodeCore> currentNode { nodeAndLevel.back().first };
        int indentation{ nodeAndLevel.back().second };
        nodeAndLevel.pop_back();

        for(std::shared_ptr<ToyMaker::SceneNodeCore> child: currentNode->getChildren()) {
            nodeAndLevel.push_back( { child, indentation+1 });
        }

        std::string indentString { "" };
        for(int i{0}; i < indentation; ++i) {
            indentString += "|  ";
        }

        std::cout << indentString << currentNode->getName() 
            << " (World: " << currentNode->getWorldID() << ", Entity: " << currentNode->getEntityID()
            << "):\n" << indentString << "| \\ parent ID: " << currentNode->getComponent<SceneHierarchyData>().mParent 
            << ",\n" << indentString << "| \\ next sibling ID: " << currentNode->getComponent<SceneHierarchyData>().mSibling
            << ",\n" << indentString << "| \\ first child ID: " << currentNode->getComponent<SceneHierarchyData>().mChild 
            << "\n";
    }
}

Application::Application(const std::string& projectPath) {
    s_instantiated = true;

    std::filesystem::path currentResourcePath { projectPath };
    const std::filesystem::path projectRootDirectory { currentResourcePath.parent_path() };

    std::ifstream jsonFileStream;

    jsonFileStream.open(currentResourcePath.string());
    nlohmann::json projectJSON { nlohmann::json::parse(jsonFileStream) };
    jsonFileStream.close();

    mSimulationStep = projectJSON[0].at("simulation_step").get<uint32_t>();
    {
        char assertionMessage[100];
        sprintf(assertionMessage, "Simulation step must be between %dms and %dms", kMinSimStep, kMaxSimStep);
        assert(mSimulationStep >= kMinSimStep && mSimulationStep <= kMaxSimStep && assertionMessage);
    }


    mSceneSystem = ECSWorld::getSingletonSystem<SceneSystem>();

    initialize(projectJSON[0].at("window_configuration"));

    const std::string& inputFile{ projectJSON[0].at("input_map_path").get<std::string>() };
    currentResourcePath = projectRootDirectory / inputFile;
    jsonFileStream.open(currentResourcePath.string());
    const nlohmann::json inputJSON { nlohmann::json::parse(jsonFileStream) };
    jsonFileStream.close();

    const std::string& rootSceneFile { projectJSON[0].at("root_scene_path").get<std::string>() };
    currentResourcePath = projectRootDirectory / rootSceneFile;
    mInputManager.loadInputConfiguration(inputJSON[0]);
    ResourceDatabase::AddResourceDescription(
        nlohmann::json {
            {"name", "root_scene"},
            {"type", "SimObject"},
            {"method", "fromSceneFile"},
            {"parameters", {
                {"path", currentResourcePath.string()},
            }},
        }
    );

    mSceneSystem.lock()->onApplicationInitialize(projectJSON[0].at("root_viewport_render_configuration"));
    mSceneSystem.lock()->addNode(
        ResourceDatabase::GetRegisteredResource<SimObject>("root_scene"),
        "/"
    );
}

void Application::execute() {
    WindowContext& windowContext { WindowContext::getInstance() };
    std::shared_ptr<SceneSystem> sceneSystem { mSceneSystem.lock() };

    SignalObserver<> onWindowResized { mSignalTracker, "onWindowResized", [this, &sceneSystem, &windowContext]() {
        sceneSystem->getRootViewport().requestDimensions(windowContext.getDimensions());
    }};
    onWindowResized.connectTo(windowContext.mSigWindowResized);


    // Timing related variables
    uint32_t previousTicks { SDL_GetTicks() };
    uint32_t simulationTicks { previousTicks };
    uint32_t nextUpdateTicks { previousTicks };

    // Application loop begins
    SDL_Event event;
    bool quit {false};
    sceneSystem->onApplicationStart();
    sceneSystem->getRootViewport().requestDimensions(WindowContext::getInstance().getDimensions());
    // printSceneHierarchyData(sceneSystem->getNode("/root/"));
    while(true) {
        //Handle events before anything else
        while(SDL_PollEvent(&event)) {
            // TODO: We probably want to locate this elsewhere? I'm not sure. Let's see.
            switch(event.type) {
                case SDL_QUIT:
                    quit=true;
                break;
                case SDL_WINDOWEVENT:
                    windowContext.handleWindowEvent(event.window);
                break;
                default:
                    mInputManager.queueInput(event);
                break;
            }
            if(quit) break;
        }
        if(quit) break;

        // update time related variables
        const uint32_t currentTicks { SDL_GetTicks() };
        const uint32_t variableStep { currentTicks - previousTicks };
        previousTicks = currentTicks;

        // apply simulation updates, if possible
        while(currentTicks - simulationTicks >= mSimulationStep) {
            const uint32_t updatedSimulationTicks = simulationTicks + mSimulationStep;
            sceneSystem->simulationStep(mSimulationStep, mInputManager.getTriggeredActions(updatedSimulationTicks));
            simulationTicks = updatedSimulationTicks;
        }
        const uint32_t simulationLagMillis { currentTicks - simulationTicks};
        const uint32_t simulationTimeOffset { mSimulationStep - simulationLagMillis };

        // calculate progress towards the next simulation step, apply variable update
        const float simulationProgress { static_cast<float>(simulationLagMillis) / mSimulationStep};
        sceneSystem->variableStep(simulationProgress, simulationLagMillis, variableStep, mInputManager.getTriggeredActions(currentTicks));

        // render a frame (or, well, leave it up to the root viewport configuration really)
        const uint32_t renderTimeOffset { sceneSystem->render(simulationProgress, variableStep) };

        // TODO: Implement this for better support for low power devices
        // NOTE: keep these around on the off chance we want it later, for eg., if
        // we want to use 
        //  `SDL_Delay(time)` 
        // or 
        //  `std::this_thread::sleep_for(std::chrono::milliseconds(time))`
#ifdef ZO_DUMB_DELAY
        const uint32_t frameEndTime { SDL_GetTicks() };
        nextUpdateTicks = currentTicks + glm::min(renderTimeOffset, simulationTimeOffset);
        if(frameEndTime + kSleepThreshold < nextUpdateTicks) {
            const uint32_t delayTime { (nextUpdateTicks - frameEndTime)/kSleepThreshold * kSleepThreshold };
            SDL_Delay(delayTime);
        }
#endif

    }
    sceneSystem->onApplicationEnd();
}

Application::~Application() {
    cleanup();
}

std::shared_ptr<Application> Application::instantiate(const std::string& projectPath) {
    assert(!s_instantiated && s_pInstance.expired() && "Application may only be initialized once");
    std::shared_ptr<Application> instance { new Application{projectPath} };
    s_pInstance = instance;
    s_instantiated = true;
    return instance;
}

Application& Application::getInstance() {
    assert(!s_pInstance.expired() && "Application has not been instantiated");
    return *s_pInstance.lock();
}

void Application::initialize(const nlohmann::json& windowProperties) {
    WindowContext::initialize(windowProperties);

    // IMPORTANT: call get instance, just to initialize the window
    // TODO: stupid name bound to trip me up sooner or later. Replace it.
    ResourceDatabase::GetInstance();

    Material::Init();
}

void Application::cleanup() {
    Material::Clear();
}

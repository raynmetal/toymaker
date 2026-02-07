/**
 * @file application.hpp
 * @author Zoheb Shujauddin (zoheb2424@gmail.com)
 * @brief Header for the engine's application class, which does project/application level initialization and cleanup before and after the game loop runs.
 * @version 0.3.2
 * @date 2025-09-02
 * 
 */

#ifndef FOOLSENGINE_APPLICATION_H
#define FOOLSENGINE_APPLICATION_H

#include <string>

#include "core/resource_database.hpp"
#include "core/ecs_world.hpp"

#include "signals.hpp"
#include "scene_system.hpp"
#include "render_system.hpp"
#include "input_system/input_system.hpp"
#include "window_context_manager.hpp"

namespace ToyMaker {

    /**
     * @ingroup ToyMakerEngine
     * @brief Class that handles setup for the rest of the engine and application.
     * 
     * This class is responsible for initializing the various packages used by the engine, reading window and other project configurations from `project.json`, and loading and initializing the default or first scene in the project.
     * 
     * Here's an example of a valid project json file:
     * 
     * ```jsonc
     * 
     * {
     *     "window_configuration": {
     *         "application_title": "Game of Ur",
     *         "window_width": 800,
     *         "window_height": 600 
     *     },
     *
     *     "root_viewport_render_configuration": {
     *         "base_dimensions": [1366, 768],
     *         "update_mode": "on-render-cap-fps",
     *         "resize_type": "texture-dimensions",
     *         "resize_mode": "fixed-aspect",
     *         "render_type": "addition",
     *         "fps_cap": 60,
     *         "render_scale": 1.0
     *     },
     *
     *     "simulation_step": 33,
     *
     *     "input_map_path": "input_bindings.json",
     *     "root_scene_path": "ur_root.json"
     * }
     *
     * ``` 
     * 
     */
    class Application {
        template <typename TObject, typename=void>
        class getByPath_Helper;

    public:
        /**
         * @brief Destroys the application object.
         * 
         */
        ~Application();

        /**
         * @brief Gets the (sole) instance of Application for this project.
         * 
         * @return Application& A reference to this project's Application object.
         */
        static Application& getInstance();

        /**
         * @brief Runs the application loop after setup.
         * 
         */
        void execute();

        /**
         * @brief Creates the single Application object used by the project and returns a reference to it.
         * 
         * @param projectPath The path to the file containing project level configuration data.
         * @return std::shared_ptr<Application> A reference to the instantiated application object.
         */
        static std::shared_ptr<Application> instantiate(const std::string& projectPath);

        /**
         * @brief Gets an object of a specific type by its scene path.
         * 
         * @tparam TObject The type of object being retrieved.
         * @param path The path to that object.
         * @return TObject The object retrieved.
         */
        template <typename TObject>
        TObject getObject(const std::string& path="");

        /**
         * @brief Gets the signal tracker associated with the application.
         * 
         * @return SignalTracker& A reference to the application's SignalTracker.
         * 
         * @see mSignalTracker
         */
        inline SignalTracker& getSignalTracker() { return mSignalTracker; }

    private:
        /**
         * @brief Constructs a new Application object
         * 
         * @param projectPath The path to the project JSON file containing the parameters to be used by this Application.
         */
        Application(const std::string& projectPath);

        /**
         * @brief Base template for engine object getter, used by Application::getObject().
         * 
         * @tparam TObject The type of object to be retrieved.
         * @tparam Enable A type that allows the get function to be SFINAED for unsupported objects.
         */
        template <typename TObject, typename Enable>
        class getByPath_Helper {
            /**
             * @brief Create the getByPath_Helper object.
             * 
             * @param application The application which owns this helper, which provides the methods used for object retrieval.
             */
            getByPath_Helper(Application* application): mApplication { application } {}

            /**
             * @brief The actual method used when getting an application object.
             * 
             * @param path The path to the object, whose format depends on the type of object being retrieved.
             * @return TObject The retrieved object.
             */
            TObject get(const std::string& path="");

            /**
             * @brief A pointer to the application object, which provides the methods used by the helper to actually fetch application objects.
             * 
             */
            Application* mApplication;
        friend class Application;
        };

        /**
         * @brief Initializes this project's 3rd party packages, creates an application window.
         * 
         * @param windowProperties The properties of the window desired by the application.
         */
        void initialize(const nlohmann::json& windowProperties);

        /**
         * @brief Clean up of 3rd party packages and window resources, if required.
         * 
         */
        void cleanup();

        /**
         * @brief The signal tracker associated with this object, which broadcasts Application events and receives events Application is interested in.
         * 
         */
        SignalTracker mSignalTracker{};

        /**
         * @brief The simulation step for the application specified in the app's project file.
         * 
         */
        uint32_t mSimulationStep { 1000/30 }; // simulation stepsize in ms

        /**
         * @brief The input manager associated with this object.
         * 
         * Responsible for receiving and translating SDL events into a form useable by the application.
         * 
         */
        InputManager mInputManager {};

        /**
         * @brief A pointer to the (sole) instance of the Application for this project.
         * 
         */
        static std::weak_ptr<Application> s_pInstance;

        /**
         * @brief A small static helper variable to determine whether  application instantiation has occurred, preventing it from recurring at a later time.
         * 
         */
        static bool s_instantiated;

        /**
         * @brief A pointer to this project's scene system, valid throughout the project.
         * 
         */
        std::weak_ptr<SceneSystem> mSceneSystem {};
    };

    template <typename TObject>
    TObject Application::getObject(const std::string& path) {
        return getByPath_Helper<TObject>{this}.get(path);
    }

    template <typename TObject, typename Enable>
    TObject Application::getByPath_Helper<TObject, Enable>::get(const std::string& path) {
        static_assert(false && "No getter for this object type is known.");
        return TObject {}; // prevent compiler warnings about no return type
    }

    template <typename TObject>
    struct Application::getByPath_Helper<
        TObject,
        typename std::enable_if_t<
            SceneNodeCore::getByPath_Helper<TObject>::s_valid
        >
    > {
        getByPath_Helper(Application* application): mApplication {application} {}
        TObject get(const std::string& path) {
            return mApplication->mSceneSystem.lock()->getByPath<TObject>(path);
        }
        Application* mApplication;
    };

    template <>
    inline InputManager& Application::getByPath_Helper<InputManager&, void>::get(const std::string& path){
        assert(path == "" && "Getter for InputManager does not accept any path parameter");
        return mApplication->mInputManager;
    }
}

#endif

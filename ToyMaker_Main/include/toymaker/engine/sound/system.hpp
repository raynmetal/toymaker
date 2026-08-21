#ifndef TOYMAKER_SOUNDSYSTEM_H
#define TOYMAKER_SOUNDSYSTEM_H

#include "../core/ecs_world.hpp"

/**
 * @defgroup ToyMakerSoundSystem Sound System
 * @ingroup ToyMakerEngine
 *
 */

namespace ToyMaker {

    /**
     * @ingroup SoundSystem
     *
     * @brief System responsible for servicing sound playback commands from the game and managing platform
     * audio hardware.
     *
     * @NOTE: Mainly just a simple wrapper over SDL_mixer, since I haven't fully worked out quite how
     * I'd like to manage this
     *
     */
    class SoundSystem: public System<SoundSystem, std::tuple<>, std::tuple<>> {
    public:

        /**
         * @brief Constructs a new SoundSystem object.
         *
         * @param world The world this SoundSystem will belong to, which is always the prototype ECSWorld.
         */
        inline explicit SoundSystem(std::weak_ptr<ECSWorld> world):
        System<SoundSystem, std::tuple<>, std::tuple<>> { world }
        {}

        /**
         * @brief The system type string associated with the SoundSystem.
         *
         * @return std::string This system's system type string.
         */
        static std::string getSystemTypeName() { return "SoundSystem"; }

        /**
         * @brief Informs this System's ECSWorld that the SoundSystem is a singleton, i.e., there should
         * not be more than one instance of it in the entire project, regardless of how many ECSWorlds
         * are present.
         *
         * @retval true SoundSystem is a singleton System;
         *
         * @TODO: This is obviously stupid and I should do something about it.
         *
         */
        bool isSingleton() const override { return true; }
    };
}

#endif

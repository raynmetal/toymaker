#ifndef TOYMAKER_SOUNDSYSTEM_H
#define TOYMAKER_SOUNDSYSTEM_H

#include "../core/ecs_world.hpp"

#include "types.hpp"

/**
 * @defgroup ToyMakerSoundSystem Sound System
 * @ingroup ToyMakerEngine
 *
 */

namespace ToyMaker {

    /**
     * @ingroup ToyMakerSoundSystem
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
         *
         */
        inline explicit SoundSystem(std::weak_ptr<ECSWorld> world):
        System<SoundSystem, std::tuple<>, std::tuple<>> { world }
        {}

        /**
         * @brief The system type string associated with the SoundSystem.
         *
         * @return std::string This system's system type string.
         *
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

        /**
         * @brief Creates a sound mixer device object to be managed by this system
         *
         * @TODO: Clarify relationship between singleton systems and ECS.  Right now, this call is made manually
         * by Application, explicitly once per singleton system.
         *
         */
        void onApplicationInitialize();

        /**
         * @brief Called during the application shutdown procedure, destroying owned sound mixer device.
         *
         * @TODO: Clarify relationship between singleton systems and ECS.  Right now, this call is made manually
         * by Application, explicitly once per singleton system.
         *
         */
        void onApplicationEnd();

        /**
         * @brief Returns a single sound channel, to be managed by the caller, mixed with other channels by this
         * system.
         *
         * As a general rule, use this for sounds that play continuously.
         *
         */
        std::unique_ptr<SoundChannel> createChannel();

        /**
         * @brief Returns a value from 0 to 1, representing the factor by which the mixed sound is multiplied.
         *
         * 0 means silence, while 1 means the mixed sound is left untouched.
         *
         */
        float getMasterGain() const;

        /**
         * @brief Sets the volume of the mixed output sound.
         *
         * 0 means silence, while 1 means the mixed sound is untouched.
         *
         */
        void setMasterGain(float newVolume);

    private:
        /**
         * @brief The sound mixer managed by this system.
         *
         * Allocated once SoundSystem::onApplicationInitialize is called.
         *
         */
        std::unique_ptr<SoundMixer> mMixer {};
    };
}

#endif

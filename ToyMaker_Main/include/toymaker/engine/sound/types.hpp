#ifndef TOYMAKER_SOUNDTYPES_H
#define TOYMAKER_SOUNDTYPES_H

#include <SDL3_mixer/SDL_mixer.h>
#include <glm/glm.hpp>

#include "../core/resource_database.hpp"

namespace ToyMaker {

    class SoundChannel;
    class SoundMixer;

    /**
     * @ingroup ToyMakerSoundSystem
     * @brief Class representing a single playable piece of audio.
     *
     */
    class Sound: public Resource<Sound> {
    public:
        /**
         * @brief The id string representing this kind of resource.
         *
         */
        inline static std::string getResourceTypeName() { return "Sound"; }

        /**
         * @brief Initializer for this sound.
         *
         */
        Sound(MIX_Audio* sound);

        /** @brief Move constructor */
        Sound(Sound&& other);
        /** @brief Copy constructor */
        Sound(const Sound& other) = delete;
        /** @brief Move assignment */
        Sound& operator=(Sound&& other);
        /** @brief Copy assignment */
        Sound& operator=(const Sound& other) = delete;

        /**
         * @brief Returns the duration of this sound in millis, with -1 indicating the audio is infinite.
         *
         */
        int32_t getDuration() const;

    private:

        /**
         * @brief The underlying SDL3_mixer audio object
         *
         */
        std::unique_ptr<MIX_Audio, decltype(&MIX_DestroyAudio)> mAudio;

    friend class SoundChannel;
    };

    /**
     * @ingroup ToyMakerSoundSystem
     *
     * @brief Class representing a single voice to be mixed with other voices that may play simultaneiously to
     * produce the final sound output.
     *
     * Defines properties to control the playback of the audio being read from -- which segment of the
     * audio to play, how many times the audio should be looped, whether the audio should fade in
     * and out.
     *
     * @NOTE: Currently no more than an abstraction of SDL_mixer tracks.
     *
     */
    class SoundChannel {
    public:
        /**
         * @brief Constructs a channel owned and used by `mixer`
         *
         */
        SoundChannel(const SoundMixer& mixer);

        /**
         * @brief Destroys sound channel and any related properties it owns.
         *
         */
        inline ~SoundChannel() {
            SDL_DestroyProperties(mProperties);
        }

        /** @brief Move constructor */
        SoundChannel(SoundChannel&& other);
        /** @brief Copy constructor */
        SoundChannel(const SoundChannel& other) = delete;
        /** @brief Move assignment */
        SoundChannel& operator=(SoundChannel&& other);
        /** @brief Copy assignment */
        SoundChannel& operator=(const SoundChannel& other) = delete;

        /**
         * @brief Starts playing sound attached to this channel.
         *
         */
        void play();

        /**
         * @brief Start playback with newly configured properties right away.
         *
         */
        void restart();

        /**
         * @brief Pauses sound attached to this channel.
         *
         */
        void pause();

        /**
         * @brief Stops playing sound attached to this channel, fading over the duration specified.
         *
         */
        void stop(uint32_t millis);

        /**
         * @brief Whether or not this channel is currently playing any sound.
         *
         */
        bool isPlaying() const;

        /**
         * @brief Whether or not this channel is paused.
         *
         */
        bool isPaused() const;

        /**
         * @brief Sets the 3D position associated with this channel.
         *
         */
        void setPosition(const glm::vec3& position);

        /**
         * @brief Unsets the 3D position associated with this channel, making audio independent of
         * sound source.
         *
         */
        void unsetPosition();

        /**
         * @brief Sets which sound this channel may play
         *
         */
        void setSound(const Sound& sound);

        /**
         * @brief Unsets whichever sound this channel was reading from earlier.
         *
         */
        void unsetSound();

        /**
         * @brief Whether this channel will have direction related effects applied when mixed into
         * the final sound output.
         *
         */
        bool isChannelPositional() const;

        /**
         * @brief When this channel plays, at which point in the input sound clip to start from.
         *
         */
        void setPlayStart(uint32_t millis);

        /**
         * @brief When this channel plays, what frame on the track is considered the end of the sound clip.
         *
         */
        void setPlayEnd(uint32_t millis);

        /**
         * @brief The number of times the sound should loop before ending, with -1 indicating infinity.
         *
         */
        void setLoopCount(int32_t count);

        /**
         * @brief The timestamp in milliseconds at which the sound is restarted after the end of the first
         * iteration, and every subsequent iteration, of the playback loop.
         *
         */
        void setLoopStart(uint32_t millis);

        /**
         * @brief At the first playback iteration, the period over which the sound goes from quiet to its
         * full volume.
         *
         */
        void setFadeInDuration(uint32_t millis);

        /**
         * @brief Sets the factor by which the sound from this channel will be multiplied.
         *
         * Accepts any value greater than 0.
         */
        void setGain(float gain);

        /**
         * @brief Gets the factor by which the sound from this channel is multiplied.
         *
         */
        float getGain() const;

    private:
        /**
         * @brief The underlying SDL3_mixer track object object
         *
         */
        std::unique_ptr<MIX_Track, decltype(&MIX_DestroyTrack)> mTrack;

        /**
         * @brief The SDL properties controlling the playback for this track.
         *
         */
        SDL_PropertiesID mProperties;
    };

    /**
     * @ingroup ToyMakerSoundSystem
     *
     * @brief Class managing a group of sound channels, as many as permitted by the underlying hardware,
     * which this object mixes to produce a single audio output.
     *
     */
    class SoundMixer {
    public:
        /**
         * @brief Creates a mixer for the default audio playback device.
         *
         */
        SoundMixer();

        /**
         * @brief Returns the gain applied to the mixed sound output applied by this mixer.
         *
         */
        float getGain() const;

        /**
         * @brief Sets the factor by which the sound returned by this mixer is multiplied.
         *
         */
        void setGain(float gain);

    private:
        /**
         * @brief The SDL3_mixer object this class is a wrapper over.
         *
         */
        std::unique_ptr<MIX_Mixer, decltype(&MIX_DestroyMixer)> mMixer;

    friend class SoundChannel;
    };

    /**
     * @ingroup ToyMakerSoundSystem ToyMakerSerialization
     *
     * @brief JSON serialization of a sound resource loaded from a file.
     *
     */
    class SoundFromFile: public ResourceConstructor<Sound, SoundFromFile> {
    public:
        /**
         * @brief Creates a SoundFromFile object.
         */
        SoundFromFile();

        /**
         * @brief Gets the resource constructor type string for this constructor.
         *
         * @return std::string This object's resource constructor type string.
         *
         */
        inline static std::string getResourceConstructorName() { return "fromFile"; }
    private:

        /**
         * @brief Creates a Sound resource based on its parameters.
         *
         * @param methodParameters The parameters associated with this model object.
         *
         * @return std::shared_ptr<IResource> A reference to the constructed resource.
         *
         */
        std::shared_ptr<IResource> createResource(const nlohmann::json& methodParameters) override;
    };

}

#endif


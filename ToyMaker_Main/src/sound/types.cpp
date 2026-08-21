#include <iostream>

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "toymaker/engine/sound/types.hpp"

using namespace ToyMaker;

Sound::Sound(MIX_Audio* sound):
    Resource<Sound>{ 0 },
    mAudio { sound, MIX_DestroyAudio }
{}

int32_t Sound::getDuration() const {
    const auto frames { MIX_GetAudioDuration(mAudio.get()) };
    return MIX_AudioFramesToMS(mAudio.get(), frames);
}

SoundFromFile::SoundFromFile(): ResourceConstructor<Sound, SoundFromFile>{0} {}

SoundMixer::SoundMixer():
    mMixer {
        MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr),
        MIX_DestroyMixer
    }
{
    assert(mMixer != nullptr && "Failed to create mixer");
}

SoundChannel::SoundChannel(std::shared_ptr<SoundMixer> mixer):
    mTrack {
        MIX_CreateTrack(mixer->mMixer.get()),
        MIX_DestroyTrack
    },
    mProperties { SDL_CreateProperties() }
{
    assert(mTrack != nullptr && "Failed to create sound channel");
    assert(mProperties != 0 && "Failed to create sound channel properties");
}

void SoundChannel::play() {
    const bool success {
        isPaused()?
        MIX_ResumeTrack(mTrack.get()):
        (
            !isPlaying() ?
            MIX_PlayTrack(mTrack.get(), mProperties):
            true
        )
    };
    assert(success && "Could not start/resume playback of audio attached to this channel");
}

void SoundChannel::restart() {
    const bool success { MIX_PlayTrack(mTrack.get(), mProperties) };
    assert(success && "Could not restart playback of audio attached to this channel");
}

void SoundChannel::pause() {
    const bool success { MIX_PauseTrack(mTrack.get()) };
    assert(success && "Could not pause this channel");
}

void SoundChannel::stop(uint32_t millis) {
    // guard: must be playing or paused, otherwise do nothing
    if(!isPlaying() && !isPaused()) {
        return;
    }

    const auto frames {
        MIX_TrackMSToFrames(mTrack.get(), millis)
    };
    assert(frames >= 0 && "Error in converting milliseconds to sample frames");

    const bool success {
        MIX_StopTrack(mTrack.get(), frames)
    };
    assert(success && "Failed to stop track");
}

bool SoundChannel::isPlaying() const {
    return MIX_TrackPlaying(mTrack.get());
}

bool SoundChannel::isPaused() const {
    return MIX_TrackPaused(mTrack.get());
}

void SoundChannel::setPosition(const glm::vec3& position) {
    const MIX_Point3D point { position.x, position.y, position.z };
    MIX_SetTrack3DPosition(mTrack.get(), &point);
}

void SoundChannel::unsetPosition() {
    MIX_SetTrack3DPosition(mTrack.get(), nullptr);
}

bool SoundChannel::isChannelPositional() const {
    MIX_Point3D point;
    const bool success { MIX_GetTrack3DPosition(mTrack.get(), &point) };
    assert(success && "Could not retrieve 3D position for this channel");
    return glm::vec3{ point.x, point.y, point.z } != glm::vec3{ 0.f };
}

void SoundChannel::setPlayStart(uint32_t millis) {
    const bool success { SDL_SetNumberProperty(mProperties, MIX_PROP_PLAY_START_MILLISECOND_NUMBER, millis) };
    assert(success && "Could not set play start milliseconds");
}

void SoundChannel::setPlayEnd(uint32_t millis) {
    const bool success {
        SDL_SetNumberProperty(mProperties, MIX_PROP_PLAY_MAX_MILLISECONDS_NUMBER, millis)
    };
    assert(success && "Could not set play end milliseconds");
}

void SoundChannel::setSound(const Sound& sound) {
    const bool success {
        MIX_SetTrackAudio(
            mTrack.get(), sound.mAudio.get()
        )
    };
    assert(success && "Could not attach this sound to this channel");
}

void SoundChannel::unsetSound() {
    const bool success {
        MIX_SetTrackAudio(
            mTrack.get(), nullptr
        )
    };
    assert(success && "Could not unset sound attached to this channel");
}

void SoundChannel::setLoopCount(int32_t count) {
    const bool success {
        SDL_SetNumberProperty(mProperties, MIX_PROP_PLAY_LOOPS_NUMBER, count)
    };
    assert(success && "Could not set play end milliseconds");
}

void SoundChannel::setLoopStart(uint32_t millis) {
    const bool success {
        SDL_SetNumberProperty(mProperties, MIX_PROP_PLAY_LOOP_START_MILLISECOND_NUMBER, millis)
    };
    assert(success && "Could not set loop start milliseconds");
}

void SoundChannel::setFadeInDuration(uint32_t millis) {
    const bool success {
        SDL_SetNumberProperty(mProperties, MIX_PROP_PLAY_FADE_IN_MILLISECONDS_NUMBER, millis)
    };
    assert(success && "Could not set fade in milliseconds");
}

std::shared_ptr<IResource> SoundFromFile::createResource(const nlohmann::json& methodParameters) {
    const std::string soundPath { methodParameters.at("path").get<std::string>() };
    MIX_Audio* pAudio { MIX_LoadAudio(nullptr, soundPath.c_str(), false) }; 
    assert(pAudio && "Failed to load sound from sound file");
    if(!pAudio) {
        std::cerr << SDL_GetError() << "\n";
    }
    return std::make_shared<Sound>(pAudio);
}


module;

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

module Engine.Sound.SoundManager:functions;

import Engine.Sound.SoundManager;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DataEntry.EntryType;

export namespace Engine::Sound {
[[nodiscard]] auto SoundManager::LoadSoundInternal(const std::string& resname, const std::string& label) -> int
{
    auto et = DM_.get()->GetEntry(resname);
    if (!et) {
        return 1918;
    }
    if (et->Type.load() != static_cast<uint8_t>(Engine::Utils::Data::EntryType::Sound)) {
        return 1;
    }
    int succ = et->Read([&label, this, &et](const std::shared_ptr<uint8_t[]>& data) -> int {
        if (!data || !mixer) {
            return 2;
        }
        SDL_IOStream* sio = SDL_IOFromConstMem(data.get(), et->GetSize());
        if (!sio) {
            return 3;
        }
        MIX_Audio* aud = MIX_LoadAudio_IO(mixer.get(), sio, false, true);
        if (!aud) {
            return 4;
        }
        audios[label] = std::shared_ptr<MIX_Audio>(aud, MIX_DestroyAudio);
        return 0;
    });
    return succ;
}

[[nodiscard]] auto SoundManager::CreateTrackInternal(const std::string& label) -> int
{
    MIX_Track* tra = MIX_CreateTrack(mixer.get());
    if (!tra) {
        return 1;
    }
    tracks[label] = std::shared_ptr<MIX_Track>(tra, MIX_DestroyTrack);
    return 0;
}

[[nodiscard]] auto SoundManager::SetTrackAudioInternal(const std::string& tl, const std::string& al) -> int
{
    auto tl_it = tracks.find(tl);
    auto al_it = audios.find(al);
    if (tl_it == tracks.end() || al_it == audios.end()) {
        return 1;
    }
    MIX_SetTrackAudio(tl_it->second.get(), al_it->second.get());
    return 0;
}

[[nodiscard]] auto SoundManager::PlayTrackInternal(const std::string& label) -> int
{
    auto it = tracks.find(label);
    if (it == tracks.end()) {
        return 1;
    }
    MIX_PlayTrack(it->second.get(), 0);
    return 0;
}

[[nodiscard]] auto SoundManager::PlayLoopTrackInternal(const std::string& label, int _c) -> int
{
    auto it = tracks.find(label);
    if (it == tracks.end()) {
        return 1;
    }
    MIX_PlayTrack(it->second.get(), 0);
    MIX_SetTrackLoops(it->second.get(), _c);
    return 0;
}

[[nodiscard]] auto SoundManager::StopTrackPlayingInternal(const std::string& label, int64_t fade) -> int
{
    auto it = tracks.find(label);
    if (it == tracks.end()) {
        return 1;
    }
    MIX_StopTrack(it->second.get(), fade);
    return 0;
}

[[nodiscard]] auto SoundManager::Init() -> int
{
    MIX_Mixer* mama = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!mama) {
        return 1;
    }
    mixer = std::shared_ptr<MIX_Mixer>(mama, MIX_DestroyMixer);
    return 0;
}

auto SoundManager::BindDM(std::shared_ptr<::Engine::Utils::Data::DataManager> dm) -> void
{
    DM_ = dm; // NOLINT
}

[[nodiscard]] auto SoundManager::LoadSound(const std::string& resname, const std::string& label) -> int
{
    std::scoped_lock<std::mutex> lock(mtx_);
    return LoadSoundInternal(resname, label);
}

[[nodiscard]] auto SoundManager::CreateTrack(const std::string& label) -> int
{
    std::scoped_lock<std::mutex> lock(mtx_);
    return CreateTrackInternal(label);
}

[[nodiscard]] auto SoundManager::PlayTrack(const std::string& label) -> int
{
    std::scoped_lock<std::mutex> lock(mtx_);
    return PlayTrackInternal(label);
}

[[nodiscard]] auto SoundManager::SetTrackAudio(const std::string& tl, const std::string& al) -> int
{
    std::scoped_lock<std::mutex> lock(mtx_);
    return SetTrackAudioInternal(tl, al);
}

[[nodiscard]] auto SoundManager::EraseTrack(const std::string& trackname) -> int
{
    std::scoped_lock<std::mutex> lock(mtx_);
    if (tracks.find(trackname) != tracks.end()) {
        tracks.erase(trackname);
        return 0;
    }
    return 1;
}

[[nodiscard]] auto SoundManager::PlaySoundEffect(const std::string& resname) -> int
{
    std::scoped_lock<std::mutex> lock(mtx_);

    if (audios.find(resname) == audios.end()) {
        int ret = LoadSoundInternal(resname, resname);
        if (ret != 0)
            return ret;
    }

    std::string trackLabel = "SFX@" + resname + "::C-" + std::to_string(effectCounter++);
    int ret = CreateTrackInternal(trackLabel);
    if (ret != 0)
        return ret;

    ret = SetTrackAudioInternal(trackLabel, resname);
    if (ret != 0)
        return ret;
    struct CallBackkData {
        SoundManager* mgr;
        std::string label;
    };
    auto* cbData = new CallBackkData { .mgr = this, .label = trackLabel }; // NOLINT
    if (!MIX_SetTrackStoppedCallback(tracks[trackLabel].get(), [](void* userdata, MIX_Track* track) -> void {
            if (!userdata) {
                return;
            }
            auto cb = reinterpret_cast<CallBackkData*>(userdata);
            if (!cb->mgr) {
                delete cb; // NOLINT
                return;
            }
            auto _(cb->mgr->EraseTrack(cb->label));
            delete cb; // NOLINT
        },
                                     cbData)) {
        delete cbData; // NOLINT
        return 99;
    }

    return PlayTrackInternal(trackLabel);
}

[[nodiscard]] auto SoundManager::GetTrackList() -> std::vector<std::string>
{
    std::vector<std::string> vec { };
    std::scoped_lock<std::mutex> lock(mtx_);
    vec.resize(tracks.size());
    int c = 0;
    for (auto& i : tracks) {
        vec.at(c) = i.first;
        c++;
    }
    return vec;
}

[[nodiscard]] auto SoundManager::PlayLoopTrack(const std::string& label, int _c) -> int
{
    std::scoped_lock<std::mutex> lock(mtx_);
    return PlayLoopTrackInternal(label, _c);
}

[[nodiscard]] auto SoundManager::StopTrackPlaying(const std::string& label, int64_t fade) -> int
{
    std::scoped_lock<std::mutex> lock(mtx_);
    return StopTrackPlayingInternal(label, fade);
}
}

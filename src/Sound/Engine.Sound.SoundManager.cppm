module;

#include "SDL3/SDL_iostream.h"
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <atomic> // 新增
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

export module Engine.Sound.SoundManager;

import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DataEntry.EntryType;

export namespace Engine::Sound {
class SoundManager {
private:
    std::shared_ptr<MIX_Mixer> mixer;
    std::unordered_map<std::string, std::shared_ptr<MIX_Audio>> audios;
    std::unordered_map<std::string, std::shared_ptr<MIX_Track>> tracks;
    std::shared_ptr<::Engine::Utils::Data::DataManager> DM_;
    std::mutex mtx_;
    std::atomic<int> effectCounter { 0 };

    [[nodiscard]] auto LoadWAVInternal(const std::string& resname, const std::string& label) -> int
    {
        auto et = DM_.get()->GetEntry(resname);
        if (et->Type.load() != static_cast<uint8_t>(Engine::Utils::Data::EntryType::wav)) {
            return 1;
        }
        int succ = et->Read([&label, this, &et](const std::shared_ptr<uint8_t[]>& data) -> int {
            if (!data || !mixer) {
                printf("02");
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

    [[nodiscard]] auto CreateTrackInternal(const std::string& label) -> int
    {
        MIX_Track* tra = MIX_CreateTrack(mixer.get());
        if (!tra) {
            return 1;
        }
        tracks[label] = std::shared_ptr<MIX_Track>(tra, MIX_DestroyTrack);
        return 0;
    }

    [[nodiscard]] auto SetTrackAudioInternal(const std::string& tl, const std::string& al) -> int
    {
        auto tl_it = tracks.find(tl);
        auto al_it = audios.find(al);
        if (tl_it == tracks.end() || al_it == audios.end()) {
            return 1;
        }
        MIX_SetTrackAudio(tl_it->second.get(), al_it->second.get());
        return 0;
    }

    [[nodiscard]] auto PlayTrackInternal(const std::string& label) -> int
    {
        auto it = tracks.find(label);
        if (it == tracks.end()) {
            return 1;
        }
        MIX_PlayTrack(it->second.get(), 0);
        return 0;
    }

public:
    [[nodiscard]] auto Init() -> int
    {
        MIX_Mixer* mama = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
        if (!mama) {
            return 1;
        }
        mixer = std::shared_ptr<MIX_Mixer>(mama, MIX_DestroyMixer);
        return 0;
    }

    auto BindDM(std::shared_ptr<::Engine::Utils::Data::DataManager> dm) -> void
    {
        DM_ = dm;
    }

    // ---------- 线程安全的公共接口 ----------
    [[nodiscard]] auto LoadWAV(const std::string& resname, const std::string& label) -> int
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return LoadWAVInternal(resname, label);
    }

    [[nodiscard]] auto CreateTrack(const std::string& label) -> int
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return CreateTrackInternal(label);
    }

    [[nodiscard]] auto PlayTrack(const std::string& label) -> int
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return PlayTrackInternal(label);
    }

    [[nodiscard]] auto SetTrackAudio(const std::string& tl, const std::string& al) -> int
    {
        std::lock_guard<std::mutex> lock(mtx_);
        return SetTrackAudioInternal(tl, al);
    }

    [[nodiscard]] auto PlaySoundEffect(const std::string& resname) -> int
    {
        std::lock_guard<std::mutex> lock(mtx_);

        if (audios.find(resname) == audios.end()) {
            int ret = LoadWAVInternal(resname, resname);
            if (ret != 0)
                return ret;
        }

        std::string trackLabel = "effect_" + std::to_string(effectCounter++);
        int ret = CreateTrackInternal(trackLabel);
        if (ret != 0)
            return ret;

        ret = SetTrackAudioInternal(trackLabel, resname);
        if (ret != 0)
            return ret;

        return PlayTrackInternal(trackLabel);
    }
};

}

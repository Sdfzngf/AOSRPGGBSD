module;

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

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

    [[nodiscard]] auto LoadSoundInternal(const std::string& resname, const std::string& label) -> int;

    [[nodiscard]] auto CreateTrackInternal(const std::string& label) -> int;

    [[nodiscard]] auto SetTrackAudioInternal(const std::string& tl, const std::string& al) -> int;

    [[nodiscard]] auto PlayTrackInternal(const std::string& label) -> int;

    [[nodiscard]] auto PlayLoopTrackInternal(const std::string& label, int _c) -> int;

    [[nodiscard]] auto StopTrackPlayingInternal(const std::string& label, int64_t fade) -> int;

public:
    [[nodiscard]] auto Init() -> int;

    auto BindDM(std::shared_ptr<::Engine::Utils::Data::DataManager> dm) -> void;

    [[nodiscard]] auto LoadSound(const std::string& resname, const std::string& label) -> int;

    [[nodiscard]] auto CreateTrack(const std::string& label) -> int;

    [[nodiscard]] auto PlayTrack(const std::string& label) -> int;

    [[nodiscard]] auto SetTrackAudio(const std::string& tl, const std::string& al) -> int;

    [[nodiscard]] auto EraseTrack(const std::string& trackname) -> int;

    [[nodiscard]] auto PlaySoundEffect(const std::string& resname) -> int;

    [[nodiscard]] auto GetTrackList() -> std::vector<std::string>;

    [[nodiscard]] auto PlayLoopTrack(const std::string& label, int _c) -> int;

    [[nodiscard]] auto StopTrackPlaying(const std::string& label, int64_t fade) -> int;
};

}

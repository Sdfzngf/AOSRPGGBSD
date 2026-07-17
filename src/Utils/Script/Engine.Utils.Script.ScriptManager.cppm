/**
 * @brief 脚本管理器
 *
 */
module;

#include <cstdint>
#include <memory>
#include <unordered_map>

export module Engine.Utils.Script.ScriptManager;

export import Engine.Utils.Script.Lua;

import Engine.Utils.Data.DB;
import Engine.Utils.Data.DataEntry.EntryType;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DataManager;

export namespace Engine::Utils::Script {
class ScriptManager {
private:
    LuaState L; // わたしがLです
    std::unordered_map<std::string, LuaState> Workers;
    std::shared_ptr<::Engine::Utils::Data::DataManager> SDM;

public:
    auto BindDataManager(std::shared_ptr<::Engine::Utils::Data::DataManager>& dm) -> void
    {
        SDM = dm;
    }
    auto BindDataManager(const std::atomic<std::shared_ptr<::Engine::Utils::Data::DataManager>>& dm) -> void
    {
        SDM = dm;
    }
    constexpr auto RunScript(const std::shared_ptr<::Engine::Utils::Data::DataEntry>& DE) -> void
    {
        if (!DE) {
            return;
        }
        if (DE->Type.load() == static_cast<uint32_t>(::Engine::Engine::Utils::Data::EntryType::Script)) {
            uint32_t sz = DE->Size.load();
            DE->Read([this, sz](const std::shared_ptr<uint8_t[]>& data) -> void {
                L.DoBuffer(reinterpret_cast<const char*>(data.get()), sz);
            });
        }
    }
    constexpr auto RunScript(const std::string& et) -> void
    {
        if (!SDM) {
            return;
        }
        RunScript(SDM.get()->GetEntry(et));
    }
    auto OpenLibs() -> void
    {
        L.OpenLibs();
    }
};
}; // namespace Engine::Utils::Script

/**
 * @brief 脚本管理器
 *
 */
module;

#include <cstdint>
#include <memory>

export module Engine.Utils.Script.ScriptManager;

export import Engine.Utils.Script.Lua;

import Engine.Utils.Data.DB;
import Engine.Utils.Data.DataEntry.EntryType;
import Engine.Utils.Data.DataEntry;

export namespace Engine::Utils::Script {
class ScriptManager {
public:
    LuaState L; // わたしがLです
    auto RunScript(const std::shared_ptr<::Engine::Utils::Data::DataEntry>& DE) -> void
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
};
}; // namespace Engine::Utils::Script

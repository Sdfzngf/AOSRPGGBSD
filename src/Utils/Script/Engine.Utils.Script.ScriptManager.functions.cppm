/**
 * @brief 脚本管理器函数实现（模块分区）
 *
 * 将需要 sol 类型的实现从接口文件分离，避免在模块接口中 include sol 头文件。
 */
module;

#include <memory>
#include <sol/sol.hpp>
#include <nlohmann/json.hpp>
#include <cstring>

module Engine.Utils.Script.ScriptManager:functions;

import Engine.Utils.Script.ScriptManager;
import Engine.Utils.Script.Worker;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Logger;

export namespace Engine::Utils::Script {

auto ScriptManager::SetupMainDMAPI() -> void
{
    auto& state = L.get_state();
    auto dm_table = state.create_table();

    dm_table.set_function("create_worker", [this](const std::string& name, const std::string& entry_key) -> bool {
        return CreateWorker(name, entry_key);
    });

    dm_table.set_function("read", [this](const std::string& key) -> sol::object {
        if (!SDM) return sol::lua_nil;
        auto entry = SDM->GetEntry(key);
        if (!entry) return sol::lua_nil;
        std::string json_str;
        entry->Read([&](const std::shared_ptr<uint8_t[]>& data) {
            json_str.assign(reinterpret_cast<const char*>(data.get()), entry->GetSize());
        });
        if (json_str.empty()) return sol::lua_nil;
        try {
            auto parsed = nlohmann::json::parse(json_str);
            return Worker::JsonToSol_LuaState(parsed, L);
        } catch (...) { return sol::lua_nil; }
    });

    dm_table.set_function("write", [this](const std::string& key, sol::table tbl) {
        if (!SDM) return;
        try {
            auto j = Worker::SolTableToJson(tbl);
            std::string json_str = j.dump();
            auto entry = SDM->GetEntry(key);
            if (entry) {
                auto new_data = std::make_shared<uint8_t[]>(json_str.size());
                memcpy(new_data.get(), json_str.data(), json_str.size());
                entry->SetData(new_data, json_str.size());
            } else {
                auto new_entry = std::make_shared<::Engine::Utils::Data::DataEntry>();
                new_entry->SetName(key);
                new_entry->New(json_str.size());
                new_entry->Write([&](const std::shared_ptr<uint8_t[]>& data) {
                    memcpy(data.get(), json_str.data(), json_str.size());
                });
                SDM->InsertEntry(key, new_entry);
            }
        } catch (const std::exception& e) {
            ::Engine::Utils::Logger::Log(std::string("[Main] dm:write error: ") + e.what(),
                                       ::Engine::Utils::Logger::LogLevel::ERROR);
        }
    });

    dm_table.set_function("list_keys", [this]() -> std::vector<std::string> {
        if (!SDM) return {};
        return SDM->GetList();
    });

    state["dm"] = dm_table;
}

}; // namespace Engine::Utils::Script
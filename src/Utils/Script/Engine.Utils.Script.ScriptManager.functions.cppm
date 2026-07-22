/**
 * @brief 脚本管理器函数实现（模块分区）
 *
 * 将需要 sol 类型的实现从接口文件分离，避免在模块接口中 include sol 头文件。
 */
module;

#include <cstring>
#include <memory>
#include <nlohmann/json.hpp>
#include <sol/inheritance.hpp>
#include <sol/sol.hpp>
#include <thread>

module Engine.Utils.Script.ScriptManager:functions;

import Engine.Utils.Script.ScriptManager;
import Engine.Utils.Script.Worker;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Logger;
import Engine.GUI.GUIManager;
import Engine.GUI.GUIManager.Cmd;

using namespace ::Engine::GUI;

export namespace Engine::Utils::Script {

auto ScriptManager::SetupMainDMAPI() -> void
{
    auto& state = L.get_state();
    auto dm_table = state.create_table();

    dm_table.set_function("create_worker", [this](const std::string& name, const std::string& entry_key) -> bool {
        return CreateWorker(name, entry_key);
    });

    dm_table.set_function("read", [this](const std::string& key) -> sol::object {
        if (!SDM)
            return sol::lua_nil;
        auto entry = SDM->GetEntry(key);
        if (!entry)
            return sol::lua_nil;
        std::string json_str;
        entry->Read([&](const std::shared_ptr<uint8_t[]>& data) -> void {
            json_str.assign(reinterpret_cast<const char*>(data.get()), entry->GetSize());
        });
        if (json_str.empty())
            return sol::lua_nil;
        try {
            auto parsed = nlohmann::json::parse(json_str);
            return Worker::JsonToSol_LuaState(parsed, L);
        } catch (...) {
            return sol::lua_nil;
        }
    });
    dm_table.set_function("getlist", [this]() -> std::vector<std::string> {
        return SDM.get()->GetList();
    });

    dm_table.set_function("write", [this](const std::string& key, const sol::table& tbl) -> void {
        if (!SDM)
            return;
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
                new_entry->Write([&](const std::shared_ptr<uint8_t[]>& data) -> void {
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
        if (!SDM)
            return { };
        return SDM->GetList();
    });

    state["dm"] = dm_table;
}

auto ScriptManager::SetupGUILuaAPI() -> void
{
    auto& state = L.get_state();
    auto gui_table = state.create_table();

    gui_table.set_function("set_background",
                           [this](uint8_t r, uint8_t g, uint8_t b, uint8_t a, sol::optional<int> z_order) -> void {
                               if (!SGM)
                                   return;
                               CmdSetBackground cmd { .r = r, .g = g, .b = b, .a = a, .z_order = z_order.value_or(0) };
                               SGM->PushCommand(cmd);
                           });
    gui_table.set_function("rect",
                           [this](float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a, sol::optional<int> z_order) -> void {
                               if (!SGM)
                                   return;
                               CmdRect cmd { .x = x, .y = y, .w = w, .h = h, .r = r, .g = g, .b = b, .a = a, .z_order = z_order.value_or(0) };
                               SGM->PushCommand(cmd);
                           });

    gui_table.set_function("debug_text",
                           [this](const std::string& s, float x, float y, uint8_t r, uint8_t g, uint8_t b, uint8_t a, float size, sol::optional<int> z_order) -> void {
                               if (!SGM)
                                   return;
                               CmdDebugText cmd { .s = s, .x = x, .y = y, .r = r, .g = g, .b = b, .a = a, .size = size, .z_order = z_order.value_or(0) };
                               SGM->PushCommand(cmd);
                           });
    gui_table.set_function("text",
                           [this](const std::string& te, const std::string& fname,
                                  float _x, float _y,
                                  uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a,
                                  uint8_t _br, uint8_t _bg, uint8_t _bb, uint8_t _ba,
                                  float _ptsize,
                                  int _quality,
                                  double _angle, float _acenter_x, float _acenter_y,
                                  sol::optional<int> z_order) -> void {
                               if (!SGM)
                                   return;
                               CmdText cmd { .s = te, .font = fname, .x = _x, .y = _y, .r = _r, .g = _g, .b = _b, .a = _a, .br = _br, .bg = _bg, .bb = _bb, .ba = _ba, .size = _ptsize, .quality = _quality, .angle = _angle, .acenter_x = _acenter_x, .acenter_y = _acenter_y, .z_order = z_order.value_or(0) };
                               SGM->PushCommand(cmd);
                           });
    gui_table.set_function("draw_svg", [this](const std::string& _resname, float _x, float _y, int _w, int _h, float _angle, float _acenter_x, float _acenter_y, sol::optional<int> _z) -> void {
        if (!SGM)
            return;
        CmdDrawSVG cmd { .resname = _resname, .x = _x, .y = _y, .w = _w, .h = _h, .angle = _angle, .acenter_x = _acenter_x, .acenter_y = _acenter_y, .z_order = _z.value_or(0) };
        SGM->PushCommand(cmd);
    });
    gui_table.set_function("set_title", [this](const std::string& title, sol::optional<int> z_order) -> void {
        if (!SGM)
            return;
        CmdSetTitle cmd { .title = title, .z_order = z_order.value_or(0) };
        SGM->PushCommand(cmd);
    });

    gui_table.set_function("set_logical_size",
                           [this](int w, int h, sol::optional<int> z_order) -> void {
                               if (!SGM)
                                   return;
                               CmdSetLogicalSize cmd { .w = w, .h = h, .z_order = z_order.value_or(0) };
                               SGM->PushCommand(cmd);
                           });
    gui_table.set_function("disable_logical_size", [this]() -> void {
        if (!SGM)
            return;
        CmdDisableLogicalSize cmd { };
        SGM->PushCommand(cmd);
    });

    gui_table.set_function("clear_queue", [this]() {
        if (!SGM)
            return;
        SGM->ClearQueue();
    });

    state["gui"] = gui_table;
}

}; // namespace Engine::Utils::Script

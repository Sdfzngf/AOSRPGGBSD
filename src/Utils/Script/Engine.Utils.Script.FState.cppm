/**
 * @brief Lua API 公共注入函数
 *
 * 将 Worker 和 ScriptManager 中重复的 dm/gui 绑定代码提取到此模块，
 * 通过参数化（shared_ptr 传入）实现复用。
 */
module;

#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>

export module Engine.Utils.Script.FState;

import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Logger;
import Engine.GUI.GUIManager;
import Engine.GUI.GUIManager.Cmd;
import Engine.Sound.SoundManager;

using namespace ::Engine::GUI;
using json = nlohmann::json;

export namespace Engine::Utils::Script {
/// 将 nlohmann::json 转为 sol Lua 值
inline auto JsonToSol(const json& j, sol::state& state) -> sol::object
{
    switch (j.type()) {
    case json::value_t::null:
        return sol::lua_nil;
    case json::value_t::boolean:
        return sol::make_object(state, j.get<bool>());
    case json::value_t::number_integer:
    case json::value_t::number_unsigned:
        return sol::make_object(state, j.get<int64_t>());
    case json::value_t::number_float:
        return sol::make_object(state, j.get<double>());
    case json::value_t::string:
        return sol::make_object(state, j.get<std::string>());
    case json::value_t::array: {
        sol::table tbl = state.create_table();
        int idx = 1;
        for (const auto& item : j)
            tbl[idx++] = JsonToSol(item, state);
        return tbl;
    }
    case json::value_t::object: {
        sol::table tbl = state.create_table();
        for (auto it = j.begin(); it != j.end(); ++it)
            tbl[it.key()] = JsonToSol(it.value(), state);
        return tbl;
    }
    default:
        return sol::lua_nil;
    }
}

/// 将 sol::table 递归转换为 nlohmann::json
inline auto SolTableToJson(const sol::table& tbl) -> json
{
    bool is_array = true;
    for (auto& kv : tbl) {
        if (kv.first.get_type() != sol::type::number) {
            is_array = false;
            break;
        }
    }

    if (is_array && tbl.size() > 0) {
        json result = json::array();
        for (auto& kv : tbl) {
            sol::object val = kv.second;
            switch (val.get_type()) {
            case sol::type::boolean:
                result.push_back(val.as<bool>());
                break;
            case sol::type::number:
                result.push_back(val.as<double>());
                break;
            case sol::type::string:
                result.push_back(val.as<std::string>());
                break;
            case sol::type::table:
                result.push_back(SolTableToJson(val.as<sol::table>()));
                break;
            case sol::type::nil:
                result.push_back(nullptr);
                break;
            default:
                break;
            }
        }
        return result;
    } else {
        json result = json::object();
        for (auto& kv : tbl) {
            std::string k = kv.first.as<std::string>();
            sol::object val = kv.second;
            switch (val.get_type()) {
            case sol::type::boolean:
                result[k] = val.as<bool>();
                break;
            case sol::type::number:
                result[k] = val.as<double>();
                break;
            case sol::type::string:
                result[k] = val.as<std::string>();
                break;
            case sol::type::table:
                result[k] = SolTableToJson(val.as<sol::table>());
                break;
            case sol::type::nil:
                result[k] = nullptr;
                break;
            default:
                break;
            }
        }
        return result;
    }
}

/// 注入 dm:read / dm:write / dm:list_keys 等公共方法
inline auto SetupDMBaseAPI(sol::table& dm_table,
                           const std::shared_ptr<::Engine::Utils::Data::DataManager>& dm,
                           sol::state& lua_state,
                           const std::string& log_tag = "Main") -> void
{
    dm_table.set_function("read", [dm, &lua_state, log_tag](const std::string& key) -> sol::object {
        auto entry = dm->GetEntry(key);
        if (!entry)
            return sol::lua_nil;
        std::string json_str;
        entry->Read([&](const std::shared_ptr<uint8_t[]>& data) -> void {
            json_str.assign(reinterpret_cast<const char*>(data.get()), entry->GetSize());
        });
        if (json_str.empty())
            return sol::lua_nil;
        try {
            return JsonToSol(json::parse(json_str), lua_state);
        } catch (const std::exception& e) {
            Engine::Utils::Logger::Log(
                std::string("[") + log_tag + "] dm:read JSON error: " + e.what(),
                Engine::Utils::Logger::LogLevel::ERROR);
            return sol::lua_nil;
        }
    });

    dm_table.set_function("write", [dm, log_tag](const std::string& key, const sol::table& tbl) -> void {
        try {
            json j = SolTableToJson(tbl);
            std::string json_str = j.dump();
            auto entry = dm->GetEntry(key);
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
                dm->InsertEntry(key, new_entry);
            }
        } catch (const std::exception& e) {
            Engine::Utils::Logger::Log(
                std::string("[") + log_tag + "] dm:write error: " + e.what(),
                Engine::Utils::Logger::LogLevel::ERROR);
        }
    });

    dm_table.set_function("list_keys", [dm]() -> std::vector<std::string> {
        return dm->GetList();
    });

    dm_table.set_function("getlist", [dm]() -> std::vector<std::string> {
        return dm->GetList();
    });
}

// ── gui API ──

/// 注入全局 gui 表（所有渲染命令推送函数）
inline void SetupGUIAPI(sol::state& state,
                        const std::function<void(RenderCommand)>& emit,
                        const std::function<void()>& clear)
{
    auto gui_table = state.create_table();

    gui_table.set_function("set_background",
                           [emit](uint8_t r, uint8_t g, uint8_t b, uint8_t a, sol::optional<int> z_order) -> void {
                                                              emit(CmdSetBackground { .r = r, .g = g, .b = b, .a = a, .z_order = z_order.value_or(0) });
                           });

    gui_table.set_function("rect",
                           [emit](float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a, sol::optional<int> z_order) -> void {
                                                              emit(CmdRect { .x = x, .y = y, .w = w, .h = h, .r = r, .g = g, .b = b, .a = a, .z_order = z_order.value_or(0) });
                           });

    gui_table.set_function("debug_text",
                           [emit](const std::string& s, float x, float y, uint8_t r, uint8_t g, uint8_t b, uint8_t a, float size, sol::optional<int> z_order) -> void {
                                                              emit(CmdDebugText { .s = s, .x = x, .y = y, .r = r, .g = g, .b = b, .a = a, .size = size, .z_order = z_order.value_or(0) });
                           });

    gui_table.set_function("text",
                           [emit](const std::string& te, const std::string& fname,
                                float _x, float _y,
                                uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a,
                                uint8_t _br, uint8_t _bg, uint8_t _bb, uint8_t _ba,
                                float _ptsize, int _quality,
                                double _angle, float _acenter_x, float _acenter_y,
                                sol::optional<int> z_order) -> void {
                                                              emit(CmdText { .s = te, .font = fname, .x = _x, .y = _y, .r = _r, .g = _g, .b = _b, .a = _a, .br = _br, .bg = _bg, .bb = _bb, .ba = _ba, .size = _ptsize, .quality = _quality, .angle = _angle, .acenter_x = _acenter_x, .acenter_y = _acenter_y, .z_order = z_order.value_or(0) });
                           });

    gui_table.set_function("draw_svg",
                           [emit](const std::string& _resname, float _x, float _y, int _w, int _h,
                                float _angle, float _acenter_x, float _acenter_y, sol::optional<int> _z) -> void {
                                                              emit(CmdDrawSVG { .resname = _resname, .x = _x, .y = _y, .w = _w, .h = _h, .angle = _angle, .acenter_x = _acenter_x, .acenter_y = _acenter_y, .z_order = _z.value_or(0) });
                           });

    gui_table.set_function("set_title",
                           [emit](const std::string& title, sol::optional<int> z_order) -> void {
                                                              emit(CmdSetTitle { .title = title, .z_order = z_order.value_or(0) });
                           });

    gui_table.set_function("set_logical_size",
                           [emit](int w, int h, sol::optional<int> z_order) -> void {
                                                              emit(CmdSetLogicalSize { .w = w, .h = h, .z_order = z_order.value_or(0) });
                           });

    gui_table.set_function("disable_logical_size", [emit]() -> void {
                CmdDisableLogicalSize cmd { };
        emit(cmd);
    });

    gui_table.set_function("clear_queue",
                           [clear]() -> void {
                               clear();
                           });

    state["gui"] = gui_table;
}

inline auto SetUpSndAPI(sol::state& state,
                        const std::shared_ptr<::Engine::Sound::SoundManager>& mama) -> void
{
    auto snd_table = state.create_table();
    snd_table.set_function("load_sound", [&mama](const std::string& resname, const std::string& label) -> int {
        return mama->LoadSound(resname, label);
    });

    snd_table.set_function("create_track", [&mama](const std::string& sound_label) -> int {
        return mama->CreateTrack(sound_label);
    });

    snd_table.set_function("play_track", [&mama](const std::string& track_label) -> int {
        return mama->PlayTrack(track_label);
    });

    snd_table.set_function("set_track_audio", [&mama](const std::string& tl, const std::string& al) -> int {
        return mama->SetTrackAudio(tl, al);
    });

    snd_table.set_function("erase_track", [&mama](const std::string& trackname) -> int {
        return mama->EraseTrack(trackname);
    });

    snd_table.set_function("play_sfx", [&mama](const std::string& resname) -> int {
        return mama->PlaySoundEffect(resname);
    });

    snd_table.set_function("get_track_list", [&mama] -> std::vector<std::string> {
        return mama->GetTrackList();
    });

    snd_table.set_function("play_loop_track", [&mama](const std::string& label, int _c) -> int {
        return mama->PlayLoopTrack(label, _c);
    });

    snd_table.set_function("stop_track_playing", [&mama](const std::string& label, int64_t fade) -> int {
        return mama->StopTrackPlaying(label, fade);
    });

    state["snd"] = snd_table;
}

}; // namespace Engine::Utils::Script

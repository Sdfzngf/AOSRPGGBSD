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
import Engine.Utils.Script.FState;
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

    // 公共 dm API（read/write/list_keys/getlist）
    SetupDMBaseAPI(dm_table, SDM, state, "Main");

    state["dm"] = dm_table;
}

auto ScriptManager::SetupGUILuaAPI() -> void
{
    auto& state = L.get_state();
    SetupGUIAPI(state, SGM);
}

}; // namespace Engine::Utils::Script

/**
 * @brief Worker 后台线程模块
 *
 * 每个 Worker 运行在独立线程上，持有独立 LuaState。
 * 通过 DataManager 共享数据（JSON 格式），通过 spawn_worker 创建子 Worker。
 */
module;

#include <atomic>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>

export module Engine.Utils.Script.Worker;

import Engine.Utils.Script.Lua;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Logger;

using json = nlohmann::json;

export namespace Engine::Utils::Script {

class Worker {
public:
    /// spawn_callback: (worker_name, entry_key) → shared_ptr<Worker>（nullptr 表示失败）
    using SpawnCallback = std::function<std::shared_ptr<Worker>(const std::string&, const std::string&)>;

    Worker(const std::string& name,
           std::shared_ptr<::Engine::Utils::Data::DataManager> dm,
           const std::string& entry_key,
           SpawnCallback spawn_cb)
        : name_(name)
        , dm_(std::move(dm))
        , spawn_callback_(std::move(spawn_cb))
    {
        running_.store(true);
        thread_ = std::thread(&Worker::ThreadFunc, this, entry_key);
    }

    ~Worker()
    {
        should_exit_.store(true);
        if (thread_.joinable()) {
            thread_.detach();
        }
    }

    Worker(const Worker&) = delete;
    auto operator=(const Worker&) -> Worker& = delete;
    Worker(Worker&&) = delete;
    auto operator=(Worker&&) -> Worker& = delete;

    void Join()
    {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    bool IsRunning() const { return running_.load(); }
    const std::string& GetName() const { return name_; }

    /// 将 nlohmann::json 转为 sol Lua 值（供 ScriptManager 复用）
    static sol::object JsonToSol_LuaState(const json& j, LuaState& lua)
    {
        return JsonToSolImpl(j, lua.get_state());
    }

    /// 将 sol::table 递归转换为 nlohmann::json（供 ScriptManager 复用）
    static json SolTableToJson(sol::table tbl)
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
                case sol::type::boolean: result.push_back(val.as<bool>()); break;
                case sol::type::number: result.push_back(val.as<double>()); break;
                case sol::type::string: result.push_back(val.as<std::string>()); break;
                case sol::type::table: result.push_back(SolTableToJson(val.as<sol::table>())); break;
                case sol::type::nil: result.push_back(nullptr); break;
                default: break;
                }
            }
            return result;
        } else {
            json result = json::object();
            for (auto& kv : tbl) {
                std::string k = kv.first.as<std::string>();
                sol::object val = kv.second;
                switch (val.get_type()) {
                case sol::type::boolean: result[k] = val.as<bool>(); break;
                case sol::type::number: result[k] = val.as<double>(); break;
                case sol::type::string: result[k] = val.as<std::string>(); break;
                case sol::type::table: result[k] = SolTableToJson(val.as<sol::table>()); break;
                case sol::type::nil: result[k] = nullptr; break;
                default: break;
                }
            }
            return result;
        }
    }

private:
    void ThreadFunc(const std::string& entry_key)
    {
        try {
            LuaState lua;
            lua.OpenLibs();
            SetupLuaAPI(lua);

            auto entry = dm_->GetEntry(entry_key);
            if (!entry) {
                Engine::Utils::Logger::Log(
                    std::string("[Worker ") + name_ + "]: entry not found: " + entry_key,
                    Engine::Utils::Logger::LogLevel::ERROR);
                running_.store(false);
                return;
            }

            uint32_t sz = entry->GetSize();
            entry->Read([&lua, sz](const std::shared_ptr<uint8_t[]>& data) {
                lua.DoBuffer(reinterpret_cast<const char*>(data.get()), sz);
            });
        } catch (const std::exception& e) {
            Engine::Utils::Logger::Log(
                std::string("[Worker ") + name_ + "]: exception: " + e.what(),
                Engine::Utils::Logger::LogLevel::ERROR);
        } catch (...) {
            Engine::Utils::Logger::Log(
                std::string("[Worker ") + name_ + "]: unknown exception",
                Engine::Utils::Logger::LogLevel::ERROR);
        }
        running_.store(false);
    }

    void SetupLuaAPI(LuaState& lua)
    {
        auto& state = lua.get_state();
        auto dm_table = state.create_table();

        // ── dm:read(key) → Lua table / nil ──
        dm_table.set_function("read", [this, &lua](const std::string& key) -> sol::object {
            auto entry = dm_->GetEntry(key);
            if (!entry) {
                return sol::lua_nil;
            }

            std::string json_str;
            entry->Read([&](const std::shared_ptr<uint8_t[]>& data) {
                uint32_t sz = entry->GetSize();
                json_str.assign(reinterpret_cast<const char*>(data.get()), sz);
            });

            if (json_str.empty()) {
                return sol::lua_nil;
            }

            try {
                auto parsed = json::parse(json_str);
                return JsonToSol_LuaState(parsed, lua);
            } catch (const std::exception& e) {
                Engine::Utils::Logger::Log(
                    std::string("[Worker ") + name_ + "]: dm:read JSON parse error: " + e.what(),
                    Engine::Utils::Logger::LogLevel::ERROR);
                return sol::lua_nil;
            }
        });

        // ── dm:write(key, table) ──
        dm_table.set_function("write", [this](const std::string& key, sol::table tbl) {
            try {
                json j = SolTableToJson(tbl);
                std::string json_str = j.dump();

                auto entry = dm_->GetEntry(key);
                if (entry) {
                    // 已存在的条目：直接替换数据
                    auto new_data = std::make_shared<uint8_t[]>(json_str.size());
                    memcpy(new_data.get(), json_str.data(), json_str.size());
                    entry->SetData(new_data, json_str.size());
                } else {
                    // 新建条目
                    auto new_entry = std::make_shared<::Engine::Utils::Data::DataEntry>();
                    new_entry->SetName(key);
                    new_entry->New(json_str.size());
                    new_entry->Write([&](const std::shared_ptr<uint8_t[]>& data) {
                        memcpy(data.get(), json_str.data(), json_str.size());
                    });
                    dm_->InsertEntry(key, new_entry);
                }
            } catch (const std::exception& e) {
                Engine::Utils::Logger::Log(
                    std::string("[Worker ") + name_ + "]: dm:write error: " + e.what(),
                    Engine::Utils::Logger::LogLevel::ERROR);
            }
        });

        // ── dm:spawn_worker(name, entry_key) → handle table / nil ──
        dm_table.set_function("spawn_worker", [this, &lua](const std::string& worker_name, const std::string& entry_key) -> sol::object {
            auto worker = spawn_callback_(worker_name, entry_key);
            if (!worker) {
                return sol::lua_nil;
            }
            // 返回 handle table：{ join = fn, is_running = fn }
            auto handle = lua.get_state().create_table();
            auto w_ptr = std::make_shared<std::shared_ptr<Worker>>(std::move(worker));
            handle["join"] = [w_ptr]() {
                (*w_ptr)->Join();
            };
            handle["is_running"] = [w_ptr]() -> bool {
                return (*w_ptr)->IsRunning();
            };
            handle["name"] = [w_ptr]() -> std::string {
                return (*w_ptr)->GetName();
            };
            return handle;
        });

        // ── dm:get_workername() ──
        dm_table.set_function("getworkername", [this]() -> std::string {
            return name_;
        });

        // ── dm:should_exit() ──
        dm_table.set_function("should_exit", [this]() -> bool {
            return should_exit_.load();
        });

        // ── dm:list_keys() → 列出所有 DataManager 中的 key ──
        dm_table.set_function("list_keys", [this]() -> std::vector<std::string> {
            return dm_->GetList();
        });

        state["dm"] = dm_table;
    }

    /// 将 nlohmann::json 转为 sol Lua 值（内部实现，接受 sol::state&）
    static sol::object JsonToSolImpl(const json& j, sol::state& state)
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
            for (const auto& item : j) {
                tbl[idx++] = JsonToSolImpl(item, state);
            }
            return tbl;
        }
        case json::value_t::object: {
            sol::table tbl = state.create_table();
            for (auto it = j.begin(); it != j.end(); ++it) {
                tbl[it.key()] = JsonToSolImpl(it.value(), state);
            }
            return tbl;
        }
        default:
            return sol::lua_nil;
        }
    }

    std::string name_;
    std::shared_ptr<::Engine::Utils::Data::DataManager> dm_;
    SpawnCallback spawn_callback_;
    std::atomic<bool> running_{false};
    std::atomic<bool> should_exit_{false};
    std::thread thread_;
};

}; // namespace Engine::Utils::Script
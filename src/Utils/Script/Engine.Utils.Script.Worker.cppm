/**
 * @brief Worker 后台线程模块
 *
 * 每个 Worker 运行在独立线程上，持有独立 LuaState。
 * 通过 DataManager 共享数据（JSON 格式），通过 spawn_worker 创建子 Worker。
 */
module;

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <sol/sol.hpp>
#include <string>
#include <thread>

export module Engine.Utils.Script.Worker;

import Engine.Utils.Script.Lua;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Logger;
import Engine.GUI.GUIManager;

using namespace ::Engine::GUI;
using json = nlohmann::json;

export namespace Engine::Utils::Script {

class Worker {
public:
    /// spawn_callback: (worker_name, entry_key) → shared_ptr<Worker>（nullptr 表示失败）
    using SpawnCallback = std::function<std::shared_ptr<Worker>(const std::string&, const std::string&)>;

    Worker(std::string name,
           std::shared_ptr<::Engine::Utils::Data::DataManager> dm,
           std::shared_ptr<::Engine::GUI::GUIManager> gm,
           const std::string& entry_key,
           SpawnCallback spawn_cb)
        : name_(std::move(name))
        , dm_(std::move(dm))
        , gm_(std::move(gm))
        , spawn_callback_(std::move(spawn_cb))
    {
        running_.store(true);
        thread_ = std::thread(&Worker::ThreadFunc, this, entry_key);

        // 等待 Worker 初始化完毕（进入帧循环或退出）
        while (!init_done_.load()) {
            std::this_thread::yield();
        }
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

    auto Join() -> void
    {
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    [[nodiscard]] auto IsRunning() const -> bool { return running_.load(); }
    [[nodiscard]] auto GetName() const -> const std::string& { return name_; }
    [[nodiscard]] auto IsFrameMode() const -> bool { return frame_mode_.load(); }

    /// 主线程每帧调用，唤醒帧模式 Worker 并等待其完成渲染
    auto TickFrame(double dt) -> void
    {
        if (!frame_mode_.load())
            return;
        {
            std::lock_guard lock(frame_mtx_);
            frame_dt_ = dt;
            frame_ready_ = true;
        }
        frame_cv_.notify_one();

        // 等待 Worker 完成帧渲染（阻塞主线程，确保命令已推入队列）
        std::unique_lock lock(frame_done_mtx_);
        frame_done_cv_.wait(lock, [this] -> bool { return !frame_busy_.load(); });
    }

    /// 通知帧模式 Worker 退出（Shutdown 时调用）
    auto SignalExit() -> void
    {
        should_exit_.store(true);
        if (frame_mode_.load()) {
            frame_cv_.notify_one();
        }
    }

    /// 将 nlohmann::json 转为 sol Lua 值（供 ScriptManager 复用）
    static auto JsonToSol_LuaState(const json& j, LuaState& lua) -> sol::object
    {
        return JsonToSolImpl(j, lua.get_state());
    }

    /// 将 sol::table 递归转换为 nlohmann::json（供 ScriptManager 复用）
    static auto SolTableToJson(const sol::table& tbl) -> json
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

private:
    /// Lua 可调用的 sleep_frame C 函数
    static auto SleepFrameCFunc(lua_State* L) -> int
    {
        // upvalue 1 是 Worker*（lightuserdata）
        auto* self = static_cast<Worker*>(lua_touserdata(L, lua_upvalueindex(1)));
        if (self->should_exit_.load()) {
            lua_pushnumber(L, 0.0);
            return 1;
        }
        self->frame_mode_.store(true);

        // yield: 主线程 resume 时会传入 dt
        return lua_yield(L, 0);
    }

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
                init_done_.store(true);
                return;
            }

            // 加载脚本并创建协程
            uint32_t sz = entry->GetSize();
            sol::state_view sv = lua.get_state();
            sol::load_result load;
            entry->Read([&](const std::shared_ptr<uint8_t[]>& data) -> void {
                load = sv.load_buffer(reinterpret_cast<const char*>(data.get()), sz, "script");
                if (!load.valid()) {
                    sol::error err = load;
                    throw err;
                }
            });

            // 用协程执行脚本（自动检测帧模式）
            sol::coroutine cr(load);
            auto r1 = cr();
            if (!r1.valid()) {
                sol::error err = r1;
                Engine::Utils::Logger::Log(
                    std::string("[Worker ") + name_ + "]: Lua error: " + err.what(),
                    Engine::Utils::Logger::LogLevel::ERROR);
                running_.store(false);
                init_done_.store(true);
                return;
            }
            if (r1.status() == sol::call_status::ok) {
                // 普通 Worker：脚本执行完没 yield，直接退出
                running_.store(false);
                init_done_.store(true);
                return;
            }

            // 脚本 yield 了 → 帧模式
            frame_mode_.store(true);
            init_done_.store(true);

            while (running_.load() && !should_exit_.load()) {
                // 等待主线程唤醒
                {
                    std::unique_lock lock(frame_mtx_);
                    frame_cv_.wait(lock, [this] { return frame_ready_ || should_exit_.load(); });
                    if (should_exit_.load())
                        break;
                    frame_ready_ = false;
                }

                // 标记忙碌，resume 协程执行帧逻辑
                frame_busy_.store(true);

                auto r2 = cr(frame_dt_);

                // 帧逻辑完成（Lua 已 yield，命令已推入队列）
                frame_busy_.store(false);
                frame_done_cv_.notify_one();

                if (!r2.valid()) {
                    sol::error err = r2;
                    Engine::Utils::Logger::Log(
                        std::string("[Worker ") + name_ + "]: Lua runtime error: " + err.what(),
                        Engine::Utils::Logger::LogLevel::ERROR);
                    break;
                }
                if (r2.status() == sol::call_status::ok) {
                    break; // 脚本正常结束
                }
            }
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
        init_done_.store(true);
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
            entry->Read([&](const std::shared_ptr<uint8_t[]>& data) -> void {
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

        dm_table.set_function("getlist", [this]() -> std::vector<std::string> {
            return dm_.get()->GetList();
        });

        // ── dm:write(key, table) ──
        dm_table.set_function("write", [this](const std::string& key, const sol::table& tbl) -> void {
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
                    new_entry->Write([&](const std::shared_ptr<uint8_t[]>& data) -> void {
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
            handle["join"] = [w_ptr]() -> void {
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

        // ── gui 表（Worker 线程安全，推入命令队列） ──
        auto gui_table = state.create_table();

        gui_table.set_function("set_background",
                               [this](uint8_t r, uint8_t g, uint8_t b, uint8_t a, sol::optional<int> z_order) {
                                   if (!gm_)
                                       return;
                                   CmdSetBackground cmd { .r = r, .g = g, .b = b, .a = a, .z_order = z_order.value_or(0) };
                                   gm_->PushCommand(cmd);
                               });

        gui_table.set_function("rect",
                               [this](float x, float y, float w, float h, uint8_t r, uint8_t g, uint8_t b, uint8_t a, sol::optional<int> z_order) {
                                   if (!gm_)
                                       return;
                                   CmdRect cmd { .x = x, .y = y, .w = w, .h = h, .r = r, .g = g, .b = b, .a = a, .z_order = z_order.value_or(0) };
                                   gm_->PushCommand(cmd);
                               });

        gui_table.set_function("debug_text",
                               [this](const std::string& s, float x, float y, uint8_t r, uint8_t g, uint8_t b, uint8_t a, float size, sol::optional<int> z_order) {
                                   if (!gm_)
                                       return;
                                   CmdText cmd { .s = s, .x = x, .y = y, .r = r, .g = g, .b = b, .a = a, .size = size, .z_order = z_order.value_or(0) };
                                   gm_->PushCommand(cmd);
                               });
        gui_table.set_function("text",
                               [this](const std::string& te, const std::string& fname,
                                      float _x, float _y,
                                      uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a,
                                      uint8_t _br, uint8_t _bg, uint8_t _bb, uint8_t _ba,
                                      float ptsize,
                                      int quality, sol::optional<int> z_order) -> void {
                                   if (!gm_)
                                       return;
                                   CmdText cmd { .s = te, .font = fname, .x = _x, .y = _y, .r = _r, .g = _g, .b = _b, .a = _a, .br = _br, .bg = _bg, .bb = _bb, .ba = _ba, .size = ptsize, .quality = quality, .z_order = z_order.value_or(0) };
                                   gm_->PushCommand(cmd);
                               });

        gui_table.set_function("set_title",
                               [this](const std::string& title, sol::optional<int> z_order) {
                                   if (!gm_)
                                       return;
                                   CmdSetTitle cmd { .title = title, .z_order = z_order.value_or(0) };
                                   gm_->PushCommand(cmd);
                               });

        gui_table.set_function("set_logical_size",
                               [this](int w, int h, sol::optional<int> z_order) {
                                   if (!gm_)
                                       return;
                                   CmdSetLogicalSize cmd { .w = w, .h = h, .z_order = z_order.value_or(0) };
                                   gm_->PushCommand(cmd);
                               });

        gui_table.set_function("clear_queue", [this]() {
            if (!gm_)
                return;
            gm_->ClearQueue();
        });

        state["gui"] = gui_table;

        // ── sleep_frame() 全局函数（帧协程支持）──
        lua_State* L = state.lua_state();
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, SleepFrameCFunc, 1);
        lua_setglobal(L, "sleep_frame");

        state["dm"] = dm_table;
    }

    /// 将 nlohmann::json 转为 sol Lua 值（内部实现，接受 sol::state&）
    static auto JsonToSolImpl(const json& j, sol::state& state) -> sol::object
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
    std::shared_ptr<::Engine::GUI::GUIManager> gm_;
    SpawnCallback spawn_callback_;
    std::atomic<bool> running_ { false };
    std::atomic<bool> should_exit_ { false };
    std::atomic<bool> init_done_ { false };
    std::atomic<bool> frame_mode_ { false };
    std::mutex frame_mtx_;
    std::condition_variable frame_cv_;
    double frame_dt_ { 0.0 };
    bool frame_ready_ { false };
    std::atomic<bool> frame_busy_ { false };
    std::mutex frame_done_mtx_;
    std::condition_variable frame_done_cv_;
    std::thread thread_;
};

}; // namespace Engine::Utils::Script

/**
 * @brief Worker 后台线程模块
 *
 * 每个 Worker 运行在独立线程上，持有独立 LuaState。
 * 通过 DataManager 共享数据（JSON 格式），通过 spawn_worker 创建子 Worker。
 */
module;

#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
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
#include <vector>

export module Engine.Utils.Script.Worker;

import Engine.Utils.Script.Lua;
import Engine.Utils.Script.FState;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Logger;
import Engine.GUI.GUIManager;
import Engine.GUI.GUIManager.Cmd;
import Engine.Sound.SoundManager;
import Engine.Basics.Thread;

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
           std::shared_ptr<::Engine::Sound::SoundManager> mama,
           const std::string& entry_key,
           SpawnCallback spawn_cb)
        : name_(std::move(name))
        , dm_(std::move(dm))
        , gm_(std::move(gm))
        , mama_(std::move(mama))
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

    /// 主线程每帧调用，唤醒帧模式 Worker（异步，不等待）
    auto TickFrame(double dt) -> void
    {
        if (!frame_mode_.load())
            return;
        {
            std::lock_guard lock(frame_sync_mtx_);
            frame_dt_ = dt;
            frame_ready_ = true;
        }
        frame_cv_.notify_one();
    }

    /// 通知帧模式 Worker 退出（Shutdown 时调用）
    auto SignalExit() -> void
    {
        should_exit_.store(true);
        if (frame_mode_.load()) {
            frame_cv_.notify_one();
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
            Engine::Basics::Thread::IAmWorker();
            Engine::Basics::Thread::MyNameIs(name_);

            auto entry = dm_->GetEntry(entry_key);
            if (!entry) {
                Engine::Utils::Logger::Log(
                    std::string("entry not found: ") + entry_key,
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
                    std::string("Lua error: ") + err.what(),
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
                {
                    std::unique_lock lock(frame_sync_mtx_);
                    frame_cv_.wait(lock, [this] -> bool { return frame_ready_ || should_exit_.load(); });
                    if (should_exit_.load())
                        break;
                    frame_ready_ = false; // 消费帧信号
                }

                // 清空上一帧残留命令，然后执行帧（命令写本地缓冲）
                frame_cmds_.clear();
                auto r2 = cr(frame_dt_);

                // 帧末提交完整帧快照（主线程持续渲染最近一次快照）
                if (gm_) {
                    gm_->SetWorkerSnapshot(name_, std::move(frame_cmds_));
                    frame_cmds_.clear();
                }

                if (!r2.valid()) {
                    sol::error err = r2;
                    Engine::Utils::Logger::Log(
                        std::string("Lua runtime error: ") + err.what(),
                        Engine::Utils::Logger::LogLevel::ERROR);
                    break;
                }
                if (r2.status() == sol::call_status::ok) {
                    break; // 脚本正常结束
                }
            }
        } catch (const std::exception& e) {
            Engine::Utils::Logger::Log(
                std::string("exception: ") + e.what(),
                Engine::Utils::Logger::LogLevel::ERROR);
        } catch (...) {
            Engine::Utils::Logger::Log(
                std::string("unknown exception"),
                Engine::Utils::Logger::LogLevel::ERROR);
        }
        // 确保主线程不因 Worker 退出而死锁
        {
            std::lock_guard lock(frame_sync_mtx_);
            frame_ready_ = false;
            frame_cv_.notify_one();
        }
        frame_mode_.store(false);
        running_.store(false);
        init_done_.store(true);
        // 清理该 Worker 的快照槽，避免残留旧帧
        if (gm_) {
            gm_->ClearWorkerSnapshot(name_);
        }
    }

    void SetupLuaAPI(LuaState& lua)
    {
        auto& state = lua.get_state();
        auto dm_table = state.create_table();

        // ── 公共 dm API ──
        SetupDMBaseAPI(dm_table, dm_, state, name_);

        // ── Worker 专用 dm API ──
        dm_table.set_function("spawn_worker", [this, &lua](const std::string& worker_name, const std::string& entry_key) -> sol::object {
            auto worker = spawn_callback_(worker_name, entry_key);
            if (!worker)
                return sol::lua_nil;
            auto handle = lua.get_state().create_table();
            auto w_ptr = std::make_shared<std::shared_ptr<Worker>>(std::move(worker));
            handle["join"] = [w_ptr]() -> void { (*w_ptr)->Join(); };
            handle["is_running"] = [w_ptr]() -> bool { return (*w_ptr)->IsRunning(); };
            handle["name"] = [w_ptr]() -> std::string { return (*w_ptr)->GetName(); };
            return handle;
        });

        dm_table.set_function("getworkername", [this]() -> std::string { return name_; });
        dm_table.set_function("should_exit", [this]() -> bool { return should_exit_.load(); });

        // ── 公共 gui API（命令写入帧内本地缓冲，帧末批量提交）──
        SetupGUIAPI(state, [this](RenderCommand cmd) { frame_cmds_.push_back(std::move(cmd)); }, [this]() { frame_cmds_.clear(); });
        SetUpSndAPI(state, mama_);

        state.set_function("getkeystats", [this, &state]() -> sol::table {
            sol::table stats = state.create_table();
            if (!gm_)
                return stats;

            const auto snapshot = gm_->GetKeyStats();
            for (int scancode = 0; scancode < SDL_SCANCODE_COUNT; ++scancode) {
                const char* name = SDL_GetScancodeName(static_cast<SDL_Scancode>(scancode));
                if (!name || name[0] == '\0')
                    continue;

                std::string raw = name;
                std::string lowered = raw;
                std::transform(lowered.begin(), lowered.end(), lowered.begin(), [](unsigned char ch) {
                    return static_cast<char>(std::tolower(ch));
                });
                stats[raw] = snapshot.keyboard[static_cast<size_t>(scancode)];
                stats[lowered] = snapshot.keyboard[static_cast<size_t>(scancode)];
            }

            static constexpr std::array<const char*, 8> mouse_names = {
                "mouse_left",
                "mouse_middle",
                "mouse_right",
                "mouse_x1",
                "mouse_x2",
                "mouse_button5",
                "mouse_button6",
                "mouse_button7"
            };

            for (std::size_t i = 0; i < mouse_names.size(); ++i)
                stats[mouse_names[i]] = snapshot.mouse[i];

            return stats;
        });

        // ── sleep_frame ──
        lua_State* L = state.lua_state();
        lua_pushlightuserdata(L, this);
        lua_pushcclosure(L, SleepFrameCFunc, 1);
        lua_setglobal(L, "sleep_frame");

        state["dm"] = dm_table;
    }

    std::string name_;
    std::shared_ptr<::Engine::Utils::Data::DataManager> dm_;
    std::shared_ptr<::Engine::GUI::GUIManager> gm_;
    std::shared_ptr<::Engine::Sound::SoundManager> mama_;
    SpawnCallback spawn_callback_;
    std::atomic<bool> running_ { false };
    std::atomic<bool> should_exit_ { false };
    std::atomic<bool> init_done_ { false };
    std::atomic<bool> frame_mode_ { false };
    std::mutex frame_sync_mtx_;
    std::condition_variable frame_cv_;
    double frame_dt_ { 0.0 };
    bool frame_ready_ { false };
    std::vector<RenderCommand> frame_cmds_; // 帧内本地命令缓冲（仅 Worker 线程访问）
    std::thread thread_;
};

}; // namespace Engine::Utils::Script

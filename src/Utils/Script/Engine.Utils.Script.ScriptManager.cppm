/**
 * @brief 脚本管理器
 *
 */
module;

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

export module Engine.Utils.Script.ScriptManager;

export import Engine.Utils.Script.Lua;
import Engine.Utils.Script.Worker;
import Engine.Utils.Data.DB;
import Engine.Utils.Data.DataEntry.EntryType;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DataManager;
import Engine.Utils.Logger;
import Engine.GUI.GUIManager;
import Engine.GUI.GUIManager.Cmd;
import Engine.Sound.SoundManager;

export namespace Engine::Utils::Script {

class ScriptManager {
private:
    LuaState L; // 主线程 LuaState
    std::unordered_map<std::string, std::shared_ptr<Worker>> Workers;
    mutable std::mutex workers_mtx;
    std::shared_ptr<::Engine::Utils::Data::DataManager> SDM;
    std::shared_ptr<::Engine::GUI::GUIManager> SGM;
    std::shared_ptr<::Engine::Sound::SoundManager> MaMa;

public:
    auto BindDataManager(std::shared_ptr<::Engine::Utils::Data::DataManager>& dm) -> void
    {
        SDM = dm;
    }
    auto BindDataManager(const std::atomic<std::shared_ptr<::Engine::Utils::Data::DataManager>>& dm) -> void
    {
        SDM = dm;
    }

    auto BindGUIManager(std::shared_ptr<::Engine::GUI::GUIManager>& gm) -> void
    {
        SGM = gm;
    }

    auto BindGUIManager(const std::atomic<std::shared_ptr<::Engine::GUI::GUIManager>>& gm) -> void
    {
        SGM = gm;
    }

    auto BindSndManager(const std::atomic<std::shared_ptr<::Engine::Sound::SoundManager>>& mama) -> void
    {
        MaMa = mama;
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

    /// 主线程 Lua 环境注入 dm API（实现见 functions 分区）
    auto SetupMainDMAPI() -> void;

    /// 主线程 Lua 环境注入 gui API（实现见 functions 分区）
    auto SetupGUILuaAPI() -> void;

    auto SetupSndLuaAPI() -> void;

    /// 主线程 fire-and-forget 创建 Worker，返回是否成功
    auto CreateWorker(const std::string& name, const std::string& entry_key) -> bool
    {
        if (!SDM) {
            ::Engine::Utils::Logger::Log("[ScriptManager] CreateWorker: DataManager not bound",
                                         ::Engine::Utils::Logger::LogLevel::ERROR);
            return false;
        }

        {
            std::lock_guard lock(workers_mtx);
            if (Workers.contains(name)) {
                ::Engine::Utils::Logger::Log(
                    std::string("[ScriptManager] CreateWorker: worker already exists: ") + name,
                    ::Engine::Utils::Logger::LogLevel::ERROR);
                return false;
            }
        }

        // 锁外创建 Worker（构造函数会阻塞等待 init_done_）
        std::shared_ptr<Worker> w;
        try {
            w = std::make_shared<Worker>(
                name,
                SDM,
                SGM,
                MaMa,
                entry_key,
                [this](const std::string& child_name, const std::string& child_entry_key) -> std::shared_ptr<Worker> {
                    return WorkerSpawn(child_name, child_entry_key);
                });
        } catch (const std::exception& e) {
            ::Engine::Utils::Logger::Log(
                std::string("[ScriptManager] CreateWorker failed: ") + e.what(),
                ::Engine::Utils::Logger::LogLevel::ERROR);
            return false;
        }

        {
            std::lock_guard lock(workers_mtx);
            if (Workers.contains(name)) {
                return false; // 竞态：其他线程抢先创建了同名 Worker
            }
            Workers[name] = w;
        }
        return true;
    }

    /// 查询 Worker 是否运行中
    auto IsWorkerRunning(const std::string& name) -> bool
    {
        std::lock_guard lock(workers_mtx);
        auto it = Workers.find(name);
        if (it == Workers.end()) {
            return false;
        }
        return it->second->IsRunning();
    }

    /// 等待 Worker 完成
    auto JoinWorker(const std::string& name) -> void
    {
        std::shared_ptr<Worker> w;
        {
            std::lock_guard lock(workers_mtx);
            auto it = Workers.find(name);
            if (it == Workers.end()) {
                return;
            }
            w = it->second;
        }
        if (w) {
            w->Join();
        }
    }

    /// 每帧唤醒所有帧模式 Worker
    auto TickFrameWorkers(double dt) -> void
    {
        // 先复制列表再释放锁：TickFrame 会阻塞等待 Worker 完成，
        // 若持锁会导致 Worker 内 spawn_worker 无法获取 workers_mtx 而死锁
        std::vector<std::shared_ptr<Worker>> ws;
        {
            std::lock_guard lock(workers_mtx);
            ws.reserve(Workers.size());
            for (auto& [name, w] : Workers) {
                if (w->IsFrameMode()) {
                    ws.push_back(w);
                }
            }
        }
        for (auto& w : ws) {
            w->TickFrame(dt);
        }
    }

    /// 关闭所有 Worker（优雅退出 + 超时强制）
    auto ShutdownWorkers() -> void
    {
        std::vector<std::shared_ptr<Worker>> workers_copy;
        {
            std::lock_guard lock(workers_mtx);
            for (auto& [name, w] : Workers) {
                workers_copy.push_back(w);
            }
        }

        // 先通知所有 Worker 退出（唤醒帧模式 Worker）
        for (auto& w : workers_copy) {
            if (w)
                w->SignalExit();
        }

        // 给 Worker 3 秒优雅退出
        for (auto& w : workers_copy) {
            if (w && w->IsRunning()) {
                // Wait up to 3s
                auto start = std::chrono::steady_clock::now();
                while (w->IsRunning()) {
                    auto elapsed = std::chrono::steady_clock::now() - start;
                    if (elapsed > std::chrono::seconds(3)) {
                        ::Engine::Utils::Logger::Log(
                            std::string("[ScriptManager] Worker timeout, detaching: ") + w->GetName(),
                            ::Engine::Utils::Logger::LogLevel::WARN);
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                // 超时后或正常结束后不再 join——Worker 析构会 detach
            }
        }

        {
            std::lock_guard lock(workers_mtx);
            Workers.clear();
        }
    }

private:
    /// Worker 内 spawn 的回调实现
    auto WorkerSpawn(const std::string& name, const std::string& entry_key) -> std::shared_ptr<Worker>
    {
        {
            std::lock_guard lock(workers_mtx);
            if (Workers.contains(name)) {
                return nullptr;
            }
        }

        // 锁外创建（构造函数阻塞等待 init_done_，避免递归 spawn 时死锁）
        std::shared_ptr<Worker> child;
        try {
            child = std::make_shared<Worker>(
                name,
                SDM,
                SGM,
                MaMa,
                entry_key,
                [this](const std::string& gname, const std::string& gkey) -> std::shared_ptr<Worker> {
                    return WorkerSpawn(gname, gkey);
                });
        } catch (...) {
            return nullptr;
        }

        {
            std::lock_guard lock(workers_mtx);
            if (Workers.contains(name)) {
                return nullptr; // 竞态：同名已存在
            }
            Workers[name] = child;
        }
        return child;
    }
};

}; // namespace Engine::Utils::Script

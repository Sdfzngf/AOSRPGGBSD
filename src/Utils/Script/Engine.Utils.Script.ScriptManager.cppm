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

export namespace Engine::Utils::Script {

class ScriptManager {
private:
    LuaState L; // 主线程 LuaState
    std::unordered_map<std::string, std::shared_ptr<Worker>> Workers;
    mutable std::mutex workers_mtx;
    std::shared_ptr<::Engine::Utils::Data::DataManager> SDM;
    std::shared_ptr<::Engine::GUI::GUIManager> SGM;

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

    /// 主线程 fire-and-forget 创建 Worker，返回是否成功
    auto CreateWorker(const std::string& name, const std::string& entry_key) -> bool
    {
        if (!SDM) {
            ::Engine::Utils::Logger::Log("[ScriptManager] CreateWorker: DataManager not bound",
                                         ::Engine::Utils::Logger::LogLevel::ERROR);
            return false;
        }

        std::lock_guard lock(workers_mtx);
        if (Workers.contains(name)) {
            ::Engine::Utils::Logger::Log(
                std::string("[ScriptManager] CreateWorker: worker already exists: ") + name,
                ::Engine::Utils::Logger::LogLevel::ERROR);
            return false;
        }

        try {
            auto w = std::make_shared<Worker>(
                name,
                SDM,
                SGM,
                entry_key,
                [this](const std::string& child_name, const std::string& child_entry_key) -> std::shared_ptr<Worker> {
                    // Worker 内 spawn 的回调
                    std::lock_guard lock2(workers_mtx);
                    if (Workers.contains(child_name)) {
                        return nullptr;
                    }
                    try {
                        auto child = std::make_shared<Worker>(
                            child_name,
                            SDM,
                            SGM,
                            child_entry_key,
                            [this](const std::string& gname, const std::string& gkey) -> std::shared_ptr<Worker> {
                                return WorkerSpawn(gname, gkey);
                            });
                        Workers[child_name] = child;
                        return child;
                    } catch (...) {
                        return nullptr;
                    }
                });
            Workers[name] = w;
            return true;
        } catch (const std::exception& e) {
            ::Engine::Utils::Logger::Log(
                std::string("[ScriptManager] CreateWorker failed: ") + e.what(),
                ::Engine::Utils::Logger::LogLevel::ERROR);
            return false;
        }
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
        std::lock_guard lock(workers_mtx);
        for (auto& [name, w] : Workers) {
            if (w->IsFrameMode()) {
                w->TickFrame(dt);
            }
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
        if (Workers.contains(name)) {
            return nullptr;
        }
        try {
            auto child = std::make_shared<Worker>(
                name,
                SDM,
                SGM,
                entry_key,
                [this](const std::string& gname, const std::string& gkey) -> std::shared_ptr<Worker> {
                    return WorkerSpawn(gname, gkey);
                });
            Workers[name] = child;
            return child;
        } catch (...) {
            return nullptr;
        }
    }
};

}; // namespace Engine::Utils::Script

/**
 * @brief 数据管理器模块，注释内容较多，建议查看函数实现部分的注释
 *
 */
module;

#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <sys/stat.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#define ccb(a, b)                   \
    if constexpr (EnableCallback) { \
        progresscallback(a, b);     \
    }

export module Engine.Utils.Data.DataManager;

import Engine.Utils.Logger;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DB;
import Engine.i18n;
import Engine.Basics.Memory.MemoryStream;
import Engine.Basics.Memory;

using Engine::i18n::locale;
using Engine::Utils::Data::DB_Header;
using Engine::Utils::Logger::Log;

export namespace Engine::Utils::Data {
/**
 * @brief 当前DB版本号
 *
 */
const uint8_t _version = 1;

/**
 * @brief 快照元数据（轻量，供 ListSnapshots 返回）
 *
 */
struct SnapshotInfo {
    std::string name; ///< 快照名称
    std::chrono::system_clock::time_point created_at; ///< 创建时间戳
    std::string description; ///< 描述
    size_t entry_count; ///< 条目数
    std::vector<std::string> keys; ///< 条目 key 列表
};

/**
 * @brief 快照实体（内部使用）
 *
 */
struct Snapshot {
    std::string name;
    std::chrono::system_clock::time_point created_at;
    std::string description;
    std::vector<std::string> keys;
    std::unordered_map<std::string, std::shared_ptr<DataEntry>> entries; ///< key → DataEntry 浅拷贝
};

class DataManager {
private:
    // 数据条目容器
    std::unordered_map<std::string, std::shared_ptr<DataEntry>> Data;
    // 读写锁
    mutable std::shared_mutex mtx;

    // ── 快照相关成员 ──
    std::unordered_map<std::string, Snapshot> snapshots_; ///< 所有快照
    std::optional<std::string> active_rollback_; ///< 当前活动的延迟回滚快照名
    std::unordered_set<std::string> rollback_pending_keys_; ///< 尚未触发替换的 key

    /**
     * @brief 从 .snap 文件解析快照（供 LoadSnapshot / LoadAndRestore 复用）
     *
     * @param path 快照文件路径
     * @return std::optional<Snapshot> 解析成功返回快照，失败返回 nullopt
     */
    auto ReadSnapshotFromFile(const std::string& path) -> std::optional<Snapshot>;

    /**
     * @brief 确保 Data[key] 是唯一的（不被任何快照共享）
     *
     * 若 shared_ptr<DataEntry> 的 use_count > 1（快照持有引用），
     * 则深拷贝整个 DataEntry 并替换。
     *
     * @param key 条目名
     */
    auto EnsureUnique(const std::string& key) -> void;

public:
    auto InsertEntry(const std::string& key, std::shared_ptr<DataEntry> entry) -> int;
    auto InsertEntry(std::shared_ptr<DataEntry> entry) -> int;

    /**
     * @brief 获取数据条目（含延迟回滚触发）
     *
     * 若 key 处于活动回滚的 pending 集合中，则先替换为快照版本再返回。
     *
     * @param key 数据条目的名称
     * @return std::shared_ptr<DataEntry> 指向数据条目的智能指针，如果未找到则返回nullptr
     */
    auto GetEntry(const std::string& key) -> std::shared_ptr<DataEntry>;

    auto RemoveEntry(const std::string& key) -> bool;

    /**
     * @brief 清空数据
     *
     */
    auto Clear() -> void
    {
        std::unique_lock<std::shared_mutex> lock(mtx);
        Data.clear();
    }

    /**
     * @brief 获取所有条目的名称
     */
    auto GetList() -> std::vector<std::string>
    {
        std::shared_lock lock(mtx);
        std::vector<std::string> entrys;
        entrys.reserve(Data.size());
        for (const auto& j : Data) {
            entrys.push_back(j.first);
        }
        return entrys;
    }

    // ── 快照接口 ──

    /**
     * @brief 创建 CoW 快照
     *
     * 对指定 keys 建立快照，存储 shared_ptr<DataEntry> 浅拷贝（O(k)）。
     * 若处于活动回滚期间且 key 仍在 pending 中，则取活动回滚快照的旧数据。
     *
     * @param name 全局唯一快照名
     * @param keys 要快照的条目 key 列表
     * @param desc 可选描述
     * @return true 成功
     * @return false 名称重复
     */
    auto CreateSnapshot(const std::string& name, const std::vector<std::string>& keys, const std::string& desc = "") -> bool;

    /**
     * @brief 启动延迟回滚（O(1)）
     *
     * 标记 active_rollback_，将快照覆盖的 key 填入 pending 集合。
     * 实际替换推迟到 GetEntry / Write 首次访问时触发。
     * 同一时刻只能有一个活动回滚。
     *
     * @param name 快照名
     * @return true 成功
     * @return false 快照不存在或已有活动回滚
     */
    auto Rollback(const std::string& name) -> bool;

    /**
     * @brief 删除快照，释放 shared_ptr 引用
     *
     * @param name 快照名
     * @return true 成功
     * @return false 快照不存在
     */
    auto DeleteSnapshot(const std::string& name) -> bool;

    /**
     * @brief 将快照持久化到独立文件
     *
     * @param name 快照名
     * @param path 输出文件路径
     * @return true 成功
     * @return false 快照不存在或写入失败
     */
    auto SaveSnapshot(const std::string& name, const std::string& path) -> bool;

    /**
     * @brief 从持久化文件加载快照到内存
     *
     * @param path 快照文件路径
     * @return true 成功
     * @return false 文件不存在或格式错误
     */
    auto LoadSnapshot(const std::string& path) -> bool;

    /**
     * @brief 列出所有快照的元数据
     *
     * @return std::vector<SnapshotInfo>
     */
    auto ListSnapshots() -> std::vector<SnapshotInfo>;

    /**
     * @brief 取消当前活动回滚（清除 pending 状态）
     *
     */
    auto CancelRollback() -> void;

    // ── 存档便捷接口 ──

    /**
     * @brief 创建全量快照（自动涵盖当前所有条目）
     *
     * @param name 全局唯一快照名
     * @param desc 可选描述
     * @return true 成功
     * @return false 名称重复
     */
    auto CreateSnapshotAll(const std::string& name, const std::string& desc = "") -> bool;

    /**
     * @brief 加载快照文件并立即恢复到该状态（一步到位，非延迟）
     *
     * 读取 .snap 文件，解析后直接替换所有条目，同时保存快照到内存。
     *
     * @param path 快照文件路径
     * @return true 成功
     * @return false 文件不存在或格式错误
     */
    auto LoadAndRestore(const std::string& path) -> bool;

    /**
     * @brief 纯只读预览快照文件元数据（不加载到内存，不加锁）
     *
     * @param path 快照文件路径
     * @return std::optional<SnapshotInfo> 成功返回元数据，失败返回 nullopt
     */
    static auto PeekSnapshot(const std::string& path) -> std::optional<SnapshotInfo>;

    /**
     * @brief 带 CoW 保护的写入操作
     *
     * 自动处理延迟回滚触发和写时复制，然后委托 DataEntry::Write。
     *
     * @tparam F 回调类型
     * @param key 条目名
     * @param func 写入回调
     * @return 回调的返回值
     */
    template <typename F>
    auto Write(const std::string& key, F&& func) -> decltype(auto)
    {
        std::unique_lock lock(mtx);

        // 1. 处理延迟回滚
        if (active_rollback_.has_value() && rollback_pending_keys_.contains(key)) {
            auto& snap = snapshots_[*active_rollback_];
            auto entry_it = snap.entries.find(key);
            if (entry_it != snap.entries.end()) {
                Data[key] = entry_it->second;
            }
            rollback_pending_keys_.erase(key);

            // pending 全部处理完毕 → 自动清除活动回滚
            if (rollback_pending_keys_.empty()) {
                active_rollback_.reset();
            }
        }

        // 2. 确保 CoW 唯一性
        EnsureUnique(key);

        // 3. 委托写入
        auto entry = Data.at(key);
        return entry->Write(std::forward<F>(func));
    }

    /**
     * @brief 从内存中挂载数据库
     *
     * @tparam EnableCallback 是否启用回调函数，默认为false
     * @param mem 指向内存的智能指针
     * @param progresscallback 进度回调函数
     * @param buffsize 整个内存块的大小
     * @return int 返回值
     */
    template <bool EnableCallback = false>
    auto MountDB_memory(const std::shared_ptr<uint8_t[]>& mem, size_t buffsize, const std::function<void(std::string, float)>& progresscallback = nullptr) -> int
    {
        std::unique_lock<std::shared_mutex> lock(mtx);

        ccb(std::string(locale("尝试读取DB_Header")), 0);
        Log([]() -> std::string { return std::string(locale("尝试读取DB_Header")); });
        DB_Header header;
        Engine::Basics::Memory::MemoryStream ms(mem, buffsize);
        if ((ms >> header) != 0) {
            Log(std::string(locale("加载数据库时发生错误，数据库头部不完整")));
            ccb(std::string(locale("加载失败")), 0);
            return 3;
        }
        Log([&header, &buffsize]() -> std::string { return std::format("DB_Header::numberOfEntrys={},buffsize={}", header.numberOfEntrys, buffsize); });
        switch (header.version) {
        case _version:
            break;
        default:
            unsigned char a = header.version;
            Log([a]() -> std::string { return std::format(locale("不支持的DB版本: {}"), a); });
            ccb(std::string(locale("加载失败")), 0);
            return 2;
        }
        if (header.end_flag != 1) {
            Log(std::string(locale("数据库头部结束标记无效")));
            ccb(std::string(locale("加载失败")), 0);
            return 2;
        }
        try {
            /**
             * @brief 循环读取条目
             *
             */
            for (uint32_t i = 0; i < header.numberOfEntrys; i++) {
                ccb(std::format(locale("正在加载数据条目 {} / {}"), i + 1, header.numberOfEntrys), header.numberOfEntrys == 0 ? 100.0f : float(i + 1) / header.numberOfEntrys * 100);
                DB_DataEntry_Header entryheader;
                if ((ms >> entryheader) != 0) {
                    throw std::runtime_error("DB entry header read failed");
                }
                if (entryheader.NameSize > ms.Remaining()) {
                    throw std::runtime_error("DB entry name size exceeds remaining buffer");
                }
                std::shared_ptr<uint8_t[]> namep = std::make_shared<uint8_t[]>(entryheader.NameSize);
                Engine::Basics::Memory::MemoryBlock name { .block = namep, .size = entryheader.NameSize };
                if ((ms >> name) != 0) {
                    throw std::runtime_error("DB entry name read failed");
                }
                if (entryheader.Size > ms.Remaining()) {
                    throw std::runtime_error("DB entry data size exceeds remaining buffer");
                }
                std::shared_ptr<uint8_t[]> datap = std::make_shared<uint8_t[]>(entryheader.Size);
                Engine::Basics::Memory::MemoryBlock data { .block = datap, .size = entryheader.Size };
                if ((ms >> data) != 0) {
                    throw std::runtime_error("DB entry data read failed");
                }
                std::string name_s(reinterpret_cast<const char*>(namep.get()), entryheader.NameSize);
                Log([&name_s, &i, &header]() -> std::string { return std::format(locale("加载数据条目\"{}\"，进度 {} / {}"), name_s, i + 1, header.numberOfEntrys); });
                Data[name_s] = std::make_shared<DataEntry>(name_s, data.size, entryheader.Type, data.block);
            }
        } catch (...) {
            Log(std::string(locale("加载数据库时发生错误，可能是由于内存中的数据格式不正确导致的")));
            ccb(std::string(locale("加载失败")), 0);
            return 3;
        }
        ccb(std::string(locale("加载完成")), 100);
        Log([]() -> std::string { return std::string(locale("加载完成")); });
        return 0;
    }

    /**
     * @brief 从文件挂载数据库
     *
     * @tparam EnableCallback 是否启用回调函数，默认为false
     * @param path 数据库文件路径
     * @param progresscallback 进度回调函数
     * @return int 返回值
     */
    template <bool EnableCallback = false>
    auto MountDB(const std::string& path, const std::function<void(std::string, float)>& progresscallback = nullptr) -> int
    {
        ccb(std::format(locale("尝试将文件 \"{}\" 加载到内存"), path), 0);
        Log([&path]() -> std::string { return std::format(locale("尝试将文件 \"{}\" 加载到内存"), path); });
        std::ifstream file;
        file.open(path, std::ifstream::binary);
        if (!file) {
            Log([path]() -> std::string { return std::format(locale("无法打开文件\"{}\""), path); });
            ccb(std::format(locale("无法打开文件\"{}\""), path), 0);
            return 1;
        }
        struct stat statbuf { };

        if (stat(path.c_str(), &statbuf) != 0 || statbuf.st_size < 0) {
            Log([path]() -> std::string { return std::format(locale("无法读取文件大小\"{}\""), path); });
            ccb(std::format(locale("无法读取文件大小\"{}\""), path), 0);
            return 1;
        }
        auto fileSize = static_cast<size_t>(statbuf.st_size);
        std::shared_ptr<uint8_t[]> mem = std::make_shared<uint8_t[]>(fileSize);
        file.read(reinterpret_cast<char*>(mem.get()), static_cast<std::streamsize>(fileSize));
        if (!file && fileSize != 0) {
            Log([path]() -> std::string { return std::format(locale("读取文件\"{}\"到内存时失败"), path); });
            ccb(std::format(locale("读取文件\"{}\"到内存时失败"), path), 0);
            return 1;
        }
        ccb(std::format(locale("成功将文件 \"{}\" 加载到内存"), path), 10);
        Log([&path]() -> std::string { return std::format(locale("成功将文件 \"{}\" 加载到内存"), path); });
        file.close();
        return MountDB_memory<EnableCallback>(mem, fileSize, progresscallback);
    }

    /**
     * @brief 保存数据库到文件
     *
     * @tparam EnableCallback 是否启用回调函数，默认为false
     * @param path 数据库文件路径
     * @param progresscallback 进度回调函数
     * @return int 返回值
     */
    template <bool EnableCallback = false>
    auto SaveDB(const std::string& path, const std::string& desc, const std::function<void(std::string, float)>& progresscallback = nullptr) -> int
    {
        std::unique_lock<std::shared_mutex> lock(mtx);
        ccb(std::string(locale("尝试保存当前数据库")), 1);
        std::fstream file(path, std::ios::binary | std::ios::out | std::ios::trunc);
        if (!file) {
            ccb(std::format(locale("无法打开文件\"{}\""), path), 1);
            Log([path]() -> std::string { return std::format(locale("无法打开文件\"{}\""), path); });
            return 1;
        }
        DB_Header dbh;
        if (desc.length() >= 503) {
            ccb(std::format(locale("DB文件的描述过长\"{}\""), path), 1);
            Log([path]() -> std::string { return std::format(locale("DB文件的描述过长\"{}\""), path); });
            return 2;
        }
        auto fsize = Data.size();
        dbh.SetDesc(desc.c_str(), desc.length());
        dbh.numberOfEntrys = fsize;
        dbh.version = _version;
        file.write(reinterpret_cast<char*>(&dbh), sizeof(dbh));
        int i = 1;
        for (const auto& pai : Data) {
            ccb(std::format(locale("保存数据 {} / {}"), i, dbh.numberOfEntrys), float(i + 1) / dbh.numberOfEntrys * 100);
            DB_DataEntry_Header entryh;
            if (pai.first.size() > std::numeric_limits<uint8_t>::max()) {
                ccb(std::format(locale("数据条目名称过长\"{}\""), pai.first), 1);
                Log([&pai]() -> std::string { return std::format(locale("数据条目名称过长\"{}\""), pai.first); });
                return 2;
            }
            entryh.NameSize = pai.first.size();
            entryh.Size = pai.second.get()->GetSize();
            entryh.Type = pai.second.get()->Type;
            std::string nn = pai.first;
            Log([&nn, &i, &dbh]() -> std::string { return std::format(locale("保存数据条目\"{}\"，进度 {} / {}"), nn, i, dbh.numberOfEntrys); });
            file.write(reinterpret_cast<const char*>(&entryh), sizeof(entryh));
            file.write(nn.c_str(), entryh.NameSize);
            file.write(reinterpret_cast<const char*>(pai.second.get()->Data.load().get()), entryh.Size);
            i++;
        }
        file.close();
        ccb(std::string(locale("保存完成")), 100);
        Log([]() -> std::string { return std::string(locale("保存完成")); });
        return 0;
    }
};
} // namespace Engine::Utils::Data

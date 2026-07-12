/**
 * @brief 数据管理器模块
 *
 */
module;

#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <sys/stat.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>

module Engine.Utils.Data.DataManager:functions;

import Engine.Utils.Data.DataManager;
import Engine.Utils.Logger;
import Engine.i18n;

using Engine::i18n::locale;
using Engine::Utils::Logger::Log;

export namespace Engine::Utils::Data {

/**
 * @brief 确保 Data[key] 是唯一的（不被任何快照共享）
 *
 * 若 shared_ptr<DataEntry> 的 use_count > 1（快照持有引用），
 * 则深拷贝整个 DataEntry 并替换 Data[key]。
 *
 * @param key 条目名
 */
auto DataManager::EnsureUnique(const std::string& key) -> void
{
    auto it = Data.find(key);
    if (it == Data.end()) {
        return;
    }
    if (it->second.use_count() <= 1) {
        return;
    }

    // use_count > 1：有快照（或其他持有者）共享此 DataEntry → 深拷贝
    auto old = it->second;
    auto newEntry = std::make_shared<DataEntry>();

    // 拷贝元数据
    auto namePtr = old->Name.load();
    newEntry->Name.store(namePtr);
    newEntry->Size.store(old->Size.load());
    newEntry->Type.store(old->Type.load());

    // 深拷贝数据
    uint32_t sz = old->Size.load();
    auto newData = std::make_shared<uint8_t[]>(sz);
    old->Read([&](const std::shared_ptr<uint8_t[]>& data) {
        memcpy(newData.get(), data.get(), sz);
    });
    newEntry->Data.store(newData);

    Data[key] = newEntry;
}

/**
 * @brief 插入数据条目
 *
 * @param key 名称
 * @param entry 指向数据条目的智能指针
 * @return int 一直为0，暂时没什么意义
 */
auto DataManager::InsertEntry(const std::string& key, std::shared_ptr<DataEntry> entry) -> int
{
    std::unique_lock lock(mtx);
    Data[key] = std::move(entry);
    return 0;
}
auto DataManager::InsertEntry(std::shared_ptr<DataEntry> entry) -> int
{
    std::unique_lock lock(mtx);
    Data[*entry.get()->Name.load().get()] = std::move(entry);
    return 0;
}

/**
 * @brief 获取数据条目（含延迟回滚触发）
 *
 * 若 key 处于活动回滚的 pending 集合中，则触发替换为快照版本。
 * 先以共享锁查找；若命中 pending 则升级为独占锁执行替换。
 *
 * @param key 数据条目的名称
 * @return std::shared_ptr<DataEntry> 指向数据条目的智能指针，如果未找到则返回nullptr
 */
auto DataManager::GetEntry(const std::string& key) -> std::shared_ptr<DataEntry>
{
    // 先以共享锁尝试查找
    {
        std::shared_lock lock(mtx);
        // 快速路径：不在 pending 中，直接返回
        if (!active_rollback_.has_value() || !rollback_pending_keys_.contains(key)) {
            auto it = Data.find(key);
            if (it != Data.end()) {
                return it->second;
            }
            return nullptr;
        }
    }

    // 慢路径：在 pending 中，需要独占锁执行替换
    std::unique_lock lock(mtx);

    // 双重检查：可能已被其他线程处理
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

    auto it = Data.find(key);
    if (it != Data.end()) {
        return it->second;
    }
    return nullptr;
}

/**
 * @brief 移除数据条目
 *
 * @param key 数据条目的名称
 * @return true 如果成功移除数据条目
 * @return false 如果未找到数据条目
 */
auto DataManager::RemoveEntry(const std::string& key) -> bool
{
    std::unique_lock lock(mtx);
    return Data.erase(key) > 0;
}

// ── 快照方法实现 ──

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
auto DataManager::CreateSnapshot(const std::string& name, const std::vector<std::string>& keys, const std::string& desc) -> bool
{
    std::unique_lock lock(mtx);
    if (snapshots_.contains(name)) {
        Log([&name]() -> std::string { return std::format(locale("快照名称已存在: {}"), name); });
        return false;
    }

    Snapshot snap;
    snap.name = name;
    snap.created_at = std::chrono::system_clock::now();
    snap.description = desc;
    snap.keys = keys;

    for (const auto& key : keys) {
        // 活动回滚期间：pending key 应取活动回滚快照的旧数据
        if (active_rollback_.has_value() && rollback_pending_keys_.contains(key)) {
            auto& active_snap = snapshots_[*active_rollback_];
            auto entry_it = active_snap.entries.find(key);
            if (entry_it != active_snap.entries.end()) {
                snap.entries[key] = entry_it->second;
                continue;
            }
        }
        // 正常路径：取当前数据
        auto it = Data.find(key);
        if (it != Data.end()) {
            snap.entries[key] = it->second;
        }
    }

    snapshots_[name] = std::move(snap);
    Log([&name]() -> std::string { return std::format(locale("快照已创建: {}"), name); });
    return true;
}

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
auto DataManager::Rollback(const std::string& name) -> bool
{
    std::unique_lock lock(mtx);
    auto it = snapshots_.find(name);
    if (it == snapshots_.end()) {
        Log([&name]() -> std::string { return std::format(locale("回滚失败：快照不存在: {}"), name); });
        return false;
    }
    if (active_rollback_.has_value()) {
        Log([&name, this]() -> std::string { return std::format(locale("回滚失败：已有活动回滚 {}，当前请求: {}"), *active_rollback_, name); });
        return false;
    }

    active_rollback_ = name;
    for (const auto& key : it->second.keys) {
        rollback_pending_keys_.insert(key);
    }
    Log([&name, &it]() -> std::string { return std::format(locale("已启动延迟回滚: {}，pending {} 个条目"), name, it->second.keys.size()); });
    return true;
}

/**
 * @brief 删除快照，释放 shared_ptr 引用
 *
 * @param name 快照名
 * @return true 成功
 * @return false 快照不存在
 */
auto DataManager::DeleteSnapshot(const std::string& name) -> bool
{
    std::unique_lock lock(mtx);
    auto it = snapshots_.find(name);
    if (it == snapshots_.end()) {
        return false;
    }

    snapshots_.erase(it);
    Log([&name]() -> std::string { return std::format(locale("快照已删除: {}"), name); });
    return true;
}

/**
 * @brief 将快照持久化到独立文件（.snap 格式）
 *
 * 文件格式：
 *   magic      4 bytes  "DSNP"
 *   version    1 byte   (1)
 *   name_len   1 byte
 *   name       N bytes  (UTF-8)
 *   desc_len   2 bytes
 *   desc       N bytes  (UTF-8)
 *   timestamp  8 bytes  (ms since epoch)
 *   entry_count 4 bytes
 *
 *   每个 entry:
 *     key_len   1 byte
 *     key       N bytes
 *     type      4 bytes
 *     data_size 4 bytes
 *     data      N bytes
 *
 * @param name 快照名
 * @param path 输出文件路径
 * @return true 成功
 * @return false 快照不存在或写入失败
 */
auto DataManager::SaveSnapshot(const std::string& name, const std::string& path) -> bool
{
    std::shared_lock lock(mtx);
    auto it = snapshots_.find(name);
    if (it == snapshots_.end()) {
        Log([&name]() -> std::string { return std::format(locale("保存快照失败：快照不存在: {}"), name); });
        return false;
    }

    const auto& snap = it->second;
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        Log([&path]() -> std::string { return std::format(locale("保存快照失败：无法打开文件: {}"), path); });
        return false;
    }

    // Header
    const char magic[4] = { 'D', 'S', 'N', 'P' };
    file.write(magic, 4);

    uint8_t version = 1;
    file.write(reinterpret_cast<const char*>(&version), 1);

    uint8_t nameLen = static_cast<uint8_t>(snap.name.size());
    file.write(reinterpret_cast<const char*>(&nameLen), 1);
    file.write(snap.name.data(), nameLen);

    uint16_t descLen = static_cast<uint16_t>(snap.description.size());
    file.write(reinterpret_cast<const char*>(&descLen), 2);
    file.write(snap.description.data(), descLen);

    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
        snap.created_at.time_since_epoch())
                  .count();
    file.write(reinterpret_cast<const char*>(&ts), 8);

    uint32_t entryCount = static_cast<uint32_t>(snap.entries.size());
    file.write(reinterpret_cast<const char*>(&entryCount), 4);

    // Entries
    for (const auto& [key, entry] : snap.entries) {
        uint8_t keyLen = static_cast<uint8_t>(key.size());
        file.write(reinterpret_cast<const char*>(&keyLen), 1);
        file.write(key.data(), keyLen);

        uint32_t type = entry->Type.load();
        file.write(reinterpret_cast<const char*>(&type), 4);

        uint32_t dataSize = entry->Size.load();
        file.write(reinterpret_cast<const char*>(&dataSize), 4);

        entry->Read([&](const std::shared_ptr<uint8_t[]>& data) {
            file.write(reinterpret_cast<const char*>(data.get()), dataSize);
        });
    }

    file.close();
    Log([&name, &path]() -> std::string { return std::format(locale("快照 {} 已保存到: {}"), name, path); });
    return true;
}

/**
 * @brief 从 .snap 文件解析快照（私有辅助）
 *
 * 加锁由调用方负责。
 *
 * @param path 快照文件路径
 * @return std::optional<Snapshot> 成功返回快照，失败返回 nullopt
 */
auto DataManager::ReadSnapshotFromFile(const std::string& path) -> std::optional<Snapshot>
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        Log([&path]() -> std::string { return std::format(locale("读取快照文件失败：无法打开: {}"), path); });
        return std::nullopt;
    }

    // 读取并验证 magic
    char magic[4];
    file.read(magic, 4);
    if (!file || magic[0] != 'D' || magic[1] != 'S' || magic[2] != 'N' || magic[3] != 'P') {
        Log([&path]() -> std::string { return std::format(locale("读取快照文件失败：格式无效: {}"), path); });
        return std::nullopt;
    }

    // 版本
    uint8_t version = 0;
    file.read(reinterpret_cast<char*>(&version), 1);
    if (!file || version != 1) {
        Log([&path]() -> std::string { return std::format(locale("读取快照文件失败：不支持的版本: {}"), path); });
        return std::nullopt;
    }

    // 名称
    uint8_t nameLen = 0;
    file.read(reinterpret_cast<char*>(&nameLen), 1);
    std::string name(nameLen, '\0');
    file.read(name.data(), nameLen);

    // 描述
    uint16_t descLen = 0;
    file.read(reinterpret_cast<char*>(&descLen), 2);
    std::string desc(descLen, '\0');
    file.read(desc.data(), descLen);

    // 时间戳
    int64_t ts = 0;
    file.read(reinterpret_cast<char*>(&ts), 8);

    // 条目数
    uint32_t entryCount = 0;
    file.read(reinterpret_cast<char*>(&entryCount), 4);

    Snapshot snap;
    snap.name = name;
    snap.description = desc;
    snap.created_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ts));

    for (uint32_t i = 0; i < entryCount; i++) {
        uint8_t keyLen = 0;
        file.read(reinterpret_cast<char*>(&keyLen), 1);
        std::string key(keyLen, '\0');
        file.read(key.data(), keyLen);
        snap.keys.push_back(key);

        uint32_t type = 0;
        file.read(reinterpret_cast<char*>(&type), 4);

        uint32_t dataSize = 0;
        file.read(reinterpret_cast<char*>(&dataSize), 4);

        auto data = std::make_shared<uint8_t[]>(dataSize);
        file.read(reinterpret_cast<char*>(data.get()), dataSize);

        auto entry = std::make_shared<DataEntry>(key, dataSize, type, data);
        snap.entries[key] = entry;
    }

    if (!file && !file.eof()) {
        Log([&path]() -> std::string { return std::format(locale("读取快照文件失败：读取出错: {}"), path); });
        return std::nullopt;
    }

    return snap;
}

/**
 * @brief 从持久化文件加载快照到内存
 *
 * @param path 快照文件路径
 * @return true 成功
 * @return false 文件不存在或格式错误
 */
auto DataManager::LoadSnapshot(const std::string& path) -> bool
{
    std::unique_lock lock(mtx);
    auto snap = ReadSnapshotFromFile(path);
    if (!snap.has_value()) {
        return false;
    }

    auto name = snap->name;
    snapshots_[name] = std::move(*snap);
    Log([&name, &path]() -> std::string { return std::format(locale("快照 {} 已从文件加载: {}"), name, path); });
    return true;
}

/**
 * @brief 列出所有快照的元数据
 *
 * @return std::vector<SnapshotInfo>
 */
auto DataManager::ListSnapshots() -> std::vector<SnapshotInfo>
{
    std::shared_lock lock(mtx);
    std::vector<SnapshotInfo> result;
    result.reserve(snapshots_.size());
    for (const auto& [name, snap] : snapshots_) {
        SnapshotInfo info;
        info.name = snap.name;
        info.created_at = snap.created_at;
        info.description = snap.description;
        info.entry_count = snap.entries.size();
        info.keys = snap.keys;
        result.push_back(std::move(info));
    }
    return result;
}

/**
 * @brief 取消当前活动回滚（清除 pending 状态）
 *
 */
auto DataManager::CancelRollback() -> void
{
    std::unique_lock lock(mtx);
    if (active_rollback_.has_value()) {
        Log([this]() -> std::string { return std::format(locale("已取消活动回滚: {}"), *active_rollback_); });
        active_rollback_.reset();
        rollback_pending_keys_.clear();
    }
}

// ── 存档便捷接口实现 ──

/**
 * @brief 创建全量快照（自动涵盖当前所有条目）
 *
 * @param name 全局唯一快照名
 * @param desc 可选描述
 * @return true 成功
 * @return false 名称重复
 */
auto DataManager::CreateSnapshotAll(const std::string& name, const std::string& desc) -> bool
{
    auto keys = GetList();
    return CreateSnapshot(name, keys, desc);
}

/**
 * @brief 加载快照文件并立即恢复到该状态（一步到位，非延迟）
 *
 * 读取 .snap 文件，解析后直接替换所有条目，同时保存快照到内存。
 *
 * @param path 快照文件路径
 * @return true 成功
 * @return false 文件不存在或格式错误
 */
auto DataManager::LoadAndRestore(const std::string& path) -> bool
{
    std::unique_lock lock(mtx);
    auto snap = ReadSnapshotFromFile(path);
    if (!snap.has_value()) {
        Log([&path]() -> std::string { return std::format(locale("加载存档失败：{}"), path); });
        return false;
    }

    // 立即替换所有条目
    for (const auto& [key, entry] : snap->entries) {
        Data[key] = entry;
    }

    auto name = snap->name;
    Log([&name, &path]() -> std::string { return std::format(locale("存档 {} 已加载并恢复: {}"), name, path); });

    // 清除活动回滚状态（如果存在）
    active_rollback_.reset();
    rollback_pending_keys_.clear();

    snapshots_[name] = std::move(*snap);
    return true;
}

/**
 * @brief 纯只读预览快照文件元数据（静态方法，不加载到内存，不加锁）
 *
 * @param path 快照文件路径
 * @return std::optional<SnapshotInfo> 成功返回元数据，失败返回 nullopt
 */
auto DataManager::PeekSnapshot(const std::string& path) -> std::optional<SnapshotInfo>
{
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }

    // 读取并验证 magic
    char magic[4];
    file.read(magic, 4);
    if (!file || magic[0] != 'D' || magic[1] != 'S' || magic[2] != 'N' || magic[3] != 'P') {
        return std::nullopt;
    }

    // 版本
    uint8_t version = 0;
    file.read(reinterpret_cast<char*>(&version), 1);
    if (!file || version != 1) {
        return std::nullopt;
    }

    // 名称
    uint8_t nameLen = 0;
    file.read(reinterpret_cast<char*>(&nameLen), 1);
    std::string name(nameLen, '\0');
    file.read(name.data(), nameLen);

    // 描述
    uint16_t descLen = 0;
    file.read(reinterpret_cast<char*>(&descLen), 2);
    std::string desc(descLen, '\0');
    file.read(desc.data(), descLen);

    // 时间戳
    int64_t ts = 0;
    file.read(reinterpret_cast<char*>(&ts), 8);

    // 条目数
    uint32_t entryCount = 0;
    file.read(reinterpret_cast<char*>(&entryCount), 4);

    SnapshotInfo info;
    info.name = name;
    info.description = desc;
    info.created_at = std::chrono::system_clock::time_point(std::chrono::milliseconds(ts));
    info.entry_count = entryCount;

    // 只读取 key，跳过数据
    for (uint32_t i = 0; i < entryCount; i++) {
        uint8_t keyLen = 0;
        file.read(reinterpret_cast<char*>(&keyLen), 1);
        std::string key(keyLen, '\0');
        file.read(key.data(), keyLen);
        info.keys.push_back(key);

        // 跳过 type + data_size + data
        uint32_t type = 0;
        file.read(reinterpret_cast<char*>(&type), 4);
        uint32_t dataSize = 0;
        file.read(reinterpret_cast<char*>(&dataSize), 4);
        file.seekg(dataSize, std::ios::cur);
    }

    return info;
}
} // namespace Engine::Utils::Data

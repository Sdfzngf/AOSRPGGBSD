/**
 * @brief 数据管理器模块
 *
 */
module;

#include <cassert>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sys/stat.h>
#include <unordered_map>
#include <utility>

module Engine.Utils.Data.DataManager:functions;

import Engine.Utils.Data.DataManager;

export namespace Engine::Utils::Data {

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
 * @brief 获取数据条目
 *
 * @param key 数据条目的名称
 * @return std::shared_ptr<DataEntry> 指向数据条目的智能指针，如果未找到则返回nullptr
 */
auto DataManager::GetEntry(const std::string& key) const -> std::shared_ptr<DataEntry>
{
    std::shared_lock lock(mtx);
    auto it = Data.find(key);
    if (it != Data.end())
        return it->second;
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
}

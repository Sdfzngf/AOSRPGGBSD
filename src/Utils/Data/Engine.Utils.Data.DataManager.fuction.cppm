/**
 * @brief 数据管理器模块
 * 
 */
module;

#include <cassert>
#include <memory>
#include <unordered_map>
#include <utility>
#include <shared_mutex>
#include <mutex>
#include <sys/stat.h>

module Engine.Utils.Data.DataManager:fuctions;

import Engine.Utils.Data.DataManager;

export namespace Engine {
    namespace Utils {
        namespace Data {
            
            /**
             * @brief 插入数据条目
             * 
             * @param key 名称
             * @param entry 指向数据条目的智能指针
             * @return int 一直为0，暂时没什么意义
             */
            int DataManager::InsertEntry(const std::string& key, std::shared_ptr<DataEntry> entry) {
                std::unique_lock lock(mtx);
                Data[key] = std::move(entry);
                return 0;
            }
            int DataManager::InsertEntry(std::shared_ptr<DataEntry> entry) {
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
            std::shared_ptr<DataEntry> DataManager::GetEntry(const std::string& key) const {
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
            bool DataManager::RemoveEntry(const std::string& key) {
                std::unique_lock lock(mtx);
                return Data.erase(key) > 0;
            }
        }
    }
}

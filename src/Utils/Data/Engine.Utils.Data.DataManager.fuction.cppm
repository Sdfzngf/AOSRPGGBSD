/**
 * @brief 数据管理器模块
 * 
 */
module;

#include <cassert>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <utility>
#include <shared_mutex>
#include <mutex>
#include <fstream>
#include <format>
#include <sys/stat.h>
#include <functional>
#define ccb(a,b) if constexpr (EnableCallback) {\
progresscallback(a,b);\
}

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

            /**
             * @brief 从内存中挂载数据库
             * 
             * @tparam EnableCallback 是否启用回调函数，默认为false
             * @param mem 指向内存的智能指针
             * @param progresscallback 进度回调函数
             * @return int 返回值
             */
            template<bool EnableCallback=false>
            int DataManager::MountDB_memory(const std::shared_ptr<uint8_t[]> mem,std::function<void(std::string,float)> progresscallback){
                std::unique_lock<std::shared_mutex> lock(mtx);
                ccb(std::string(locale("尝试读取DB_Header")),0);
                DB_Header header;
                memcpy(&header, mem.get(), sizeof(header));
                switch (header.version){
                    case __version:
                        break;
                    default:
                        char a=header.version;
                        Log([a](){return std::format(locale("不支持的DB版本: {}"),a);});
                        ccb(std::string(locale("加载失败")),0);
                        return 2;
                }

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
            template<bool EnableCallback=false>
            int DataManager::MountDB(std::string path,std::function<void(std::string,float)> progresscallback){
                ccb(std::format(locale("尝试将文件 \"{}\" 加载到内存"),path),0);
                std::ifstream file;
                file.open(path,std::ifstream::binary);
                if(!file){
                    Log([path](){return std::format(locale("无法打开文件\"{}\""),path);});
                    return 1;
                }
                struct stat statbuf;

                stat(path.c_str(), &statbuf);
                std::shared_ptr<uint8_t[]> mem = std::make_shared<uint8_t[]>(statbuf.st_size);
                file.read((char*)mem.get(), statbuf.st_size);
                ccb(std::format(locale("成功将文件 \"{}\" 加载到内存"),path),10);
                return MountDB_memory<EnableCallback>(mem,progresscallback);
            }
        }
    }
}

/**
 * @brief 数据管理器模块
 * 
 */
module;

#include <cassert>
#include <cstdint>
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
#include <spanstream>
#define ccb(a,b) if constexpr (EnableCallback) {\
progresscallback(a,b);\
}

module Engine.Utils.Data.DataManager:fuctions;

import Engine.Utils.Data.DataManager;
import Engine.Basics.Memory.MemoryStream;

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
             * @param buffsize 整个内存块的大小
             * @return int 返回值
             */
            template<bool EnableCallback=false>
            int DataManager::MountDB_memory(const std::shared_ptr<uint8_t[]> mem,std::function<void(std::string,float)> progresscallback,size_t buffsize){
                std::unique_lock<std::shared_mutex> lock(mtx);

                ccb(std::string(locale("尝试读取DB_Header")),0);
                DB_Header header;
                Engine::Basics::Memory::MemoryStream ms(mem,buffsize);
                ms>>header;

                switch (header.version){
                    case __version:
                        break;
                    default:
                        char a=header.version;
                        Log([a](){return std::format(locale("不支持的DB版本: {}"),a);});
                        ccb(std::string(locale("加载失败")),0);
                        return 2;
                }
                try{
                    /**
                     * @brief 循环读取条目
                     *
                     */
                    for (int i=0;i<header.numberOfEntrys;i++){
                        ccb(std::format(locale("正在加载数据条目 {} / {}"),i+1,header.numberOfEntrys),float(i+1)/header.numberOfEntrys*100);
                        DB_DataEntry_Header entryheader;
                        ms>>entryheader;
                        std::shared_ptr<uint8_t[]> namep=std::make_shared<uint8_t[]>(entryheader.NameSize);
                        std::shared_ptr<uint8_t[]> datap=std::make_shared<uint8_t[]>(entryheader.Size);
                        Engine::Basics::Memory::MemoryBlock name{.block=namep,.size=entryheader.NameSize};
                        Engine::Basics::Memory::MemoryBlock data{.block=datap,.size=entryheader.Size};
                        ms>>name;
                        ms>>data;
                        std::string name_s=std::string((const char*)namep.get());
                        InsertEntry(name_s, std::make_shared<DataEntry>(new DataEntry(name_s,data.size,entryheader.Type,data.block)));
                    }
                }catch(...){
                    Log(std::string(locale("加载数据库时发生错误，可能是由于内存中的数据格式不正确导致的")));
                    ccb(std::string(locale("加载失败")),0);
                    return 3;
                }
                ccb(std::string(locale("加载完成")),100);
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

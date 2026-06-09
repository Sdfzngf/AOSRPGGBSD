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
            int DataManager::InsertEntry(const std::string& key, std::shared_ptr<DataEntry> entry) {
                std::unique_lock lock(mtx);
                Data[key] = std::move(entry);
                return 0;
            }

            std::shared_ptr<DataEntry> DataManager::GetEntry(const std::string& key) const {
                std::shared_lock lock(mtx);
                auto it = Data.find(key);
                if (it != Data.end())
                    return it->second;
                return nullptr;
            }

            bool DataManager::RemoveEntry(const std::string& key) {
                std::unique_lock lock(mtx);
                return Data.erase(key) > 0;
            }

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

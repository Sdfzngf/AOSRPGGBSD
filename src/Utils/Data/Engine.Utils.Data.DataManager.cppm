/**
 * @brief 数据管理器模块，注释内容较多，建议查看函数实现部分的注释
 * 
 */
module;

#include <cassert>
#include <memory>
#include <unordered_map>
#include <shared_mutex>
#include <sys/stat.h>
#include <functional>

export module Engine.Utils.Data.DataManager;

import Engine.Utils.Logger;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DB;
import Engine.i18n;

using Engine::Utils::Logger::Log;
using Engine::i18n::locale;
using Engine::Utils::Data::DB_Header;

export namespace Engine {
    namespace Utils {
        namespace Data {
            /**
             * @brief 当前DB版本号
             * 
             */
            const uint8_t __version=1;

            class DataManager {
            private:
                //数据条目容器
                std::unordered_map<std::string, std::shared_ptr<DataEntry>> Data;
                //读写锁
                mutable std::shared_mutex mtx;
            public:
                int InsertEntry(const std::string& key, std::shared_ptr<DataEntry> entry);

                std::shared_ptr<DataEntry> GetEntry(const std::string& key) const;

                bool RemoveEntry(const std::string& key);

                template<bool EnableCallback=false>
                int MountDB_memory(const std::shared_ptr<uint8_t[]> mem,std::function<void(std::string,float)> progresscallback=0,size_t buffsize=0);

                template<bool EnableCallback=false>
                int MountDB(std::string path,std::function<void(std::string,float)> progresscallback=0);
            };
        }
    }
}

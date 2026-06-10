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
#include <fstream>
#include <cstdint>
#include <cstring>
#include <format>
#include <mutex>
#define ccb(a,b) if constexpr (EnableCallback) {\
progresscallback(a,b);\
}

export module Engine.Utils.Data.DataManager;

import Engine.Utils.Logger;
import Engine.Utils.Data.DataEntry;
import Engine.Utils.Data.DB;
import Engine.i18n;
import Engine.Basics.Memory.MemoryStream;

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
                int InsertEntry(std::shared_ptr<DataEntry> entry);

                std::shared_ptr<DataEntry> GetEntry(const std::string& key) const;

                bool RemoveEntry(const std::string& key);

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
                int MountDB_memory(const std::shared_ptr<uint8_t[]> mem,std::function<void(std::string,float)> progresscallback=0,size_t buffsize=0){
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
                int MountDB(std::string path,std::function<void(std::string,float)> progresscallback=0){
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
                    file.close();
                    return MountDB_memory<EnableCallback>(mem,progresscallback);
                }

                /**
                 * @brief 保存数据库到文件
                 *
                 * @tparam EnableCallback 是否启用回调函数，默认为false
                 * @param path 数据库文件路径
                 * @param progresscallback 进度回调函数
                 * @return int 返回值
                 */
                template<bool EnableCallback=false>
                int SaveDB(std::string path,std::string desc,std::function<void(std::string,float)> progresscallback=0){
                    std::unique_lock<std::shared_mutex> lock(mtx);
                    ccb("尝试保存当前数据库",1);
                    std::fstream file(path,std::ios::binary|std::ios::out|std::ios::trunc);
                    if(!file){
                        Log([path](){return std::format(locale("无法打开文件\"{}\""),path);});
                        return 1;
                    }
                    DB_Header dbh;
                    if (desc.length()>=503){
                        Log([path](){return std::format(locale("DB文件的描述过长\"{}\""),path);});
                        return 2;
                    }
                    auto fsize=Data.size();
                    dbh.SetDesc(desc.c_str(), desc.length());
                    dbh.numberOfEntrys=fsize;
                    dbh.version=__version;
                    file.write((char*)&dbh, sizeof(dbh));
                    int i=1;
                    for(const auto& pai:Data){
                        ccb(std::format(locale("保存数据 {} / {}"),i,dbh.numberOfEntrys),float(i+1)/dbh.numberOfEntrys*100);
                        DB_DataEntry_Header entryh;
                        entryh.NameSize=pai.first.size();
                        entryh.Size=pai.second.get()->GetSize();
                        entryh.Type=pai.second.get()->Type;
                        std::string nn=pai.first;
                        Log([&nn,&i,&dbh](){return std::format(locale("保存数据条目\"{}\"，进度 {} / {}"),nn,i,dbh.numberOfEntrys);});
                        file.write((const char*)&entryh, sizeof(entryh));
                        file.write(nn.c_str(), entryh.NameSize);
                        file.write((const char*)pai.second.get()->Data.load().get(), entryh.Size);
                    }
                    file.close();
                    ccb(std::string(locale("保存完成")),100);
                    return 0;
                }
            };
        }
    }
}

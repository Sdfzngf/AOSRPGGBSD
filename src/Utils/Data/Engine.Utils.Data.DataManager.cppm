/**
 * @brief 数据管理器模块，注释内容较多，建议查看函数实现部分的注释
 *
 */
module;

#include <cassert>
#include <cstdint>
#include <cstring>
#include <format>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <sys/stat.h>
#include <unordered_map>
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

class DataManager {
private:
    // 数据条目容器
    std::unordered_map<std::string, std::shared_ptr<DataEntry>> Data;
    // 读写锁
    mutable std::shared_mutex mtx;

public:
    auto InsertEntry(const std::string& key, std::shared_ptr<DataEntry> entry) -> int;
    auto InsertEntry(std::shared_ptr<DataEntry> entry) -> int;

    auto GetEntry(const std::string& key) const -> std::shared_ptr<DataEntry>;

    auto RemoveEntry(const std::string& key) -> bool;

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
        ms >> header;
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
        try {
            /**
             * @brief 循环读取条目
             *
             */
            for (int i = 0; i < header.numberOfEntrys; i++) {
                ccb(std::format(locale("正在加载数据条目 {} / {}"), i + 1, header.numberOfEntrys), float(i + 1) / header.numberOfEntrys * 100);
                DB_DataEntry_Header entryheader;
                ms >> entryheader;
                std::shared_ptr<uint8_t[]> namep = std::make_shared<uint8_t[]>(entryheader.NameSize);
                std::shared_ptr<uint8_t[]> datap = std::make_shared<uint8_t[]>(entryheader.Size);
                Engine::Basics::Memory::MemoryBlock name { .block = namep, .size = entryheader.NameSize };
                Engine::Basics::Memory::MemoryBlock data { .block = datap, .size = entryheader.Size };
                ms >> name;
                ms >> data;
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
    auto MountDB(std::string path, std::function<void(std::string, float)> progresscallback = nullptr) -> int
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

        stat(path.c_str(), &statbuf);
        std::shared_ptr<uint8_t[]> mem = std::make_shared<uint8_t[]>(statbuf.st_size);
        file.read(reinterpret_cast<char*>(mem.get()), statbuf.st_size);
        ccb(std::format(locale("成功将文件 \"{}\" 加载到内存"), path), 10);
        Log([&path]() -> std::string { return std::format(locale("成功将文件 \"{}\" 加载到内存"), path); });
        file.close();
        return MountDB_memory<EnableCallback>(mem, statbuf.st_size, progresscallback);
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
    auto SaveDB(std::string path, const std::string& desc, const std::function<void(std::string, float)>& progresscallback = nullptr) -> int
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
            entryh.NameSize = pai.first.size();
            entryh.Size = pai.second.get()->GetSize();
            entryh.Type = pai.second.get()->Type;
            std::string nn = pai.first;
            Log([&nn, &i, &dbh]() -> std::string { return std::format(locale("保存数据条目\"{}\"，进度 {} / {}"), nn, i, dbh.numberOfEntrys); });
            file.write(reinterpret_cast<const char*>(&entryh), sizeof(entryh));
            file.write(nn.c_str(), entryh.NameSize);
            file.write(reinterpret_cast<const char*>(pai.second.get()->Data.load().get()), entryh.Size);
        }
        file.close();
        ccb(std::string(locale("保存完成")), 100);
        Log([]() -> std::string { return std::string(locale("保存完成")); });
        return 0;
    }
};
}

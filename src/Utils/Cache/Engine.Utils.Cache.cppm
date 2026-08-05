module;

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

export module Engine.Utils.Cache;

import Engine.Basics.sha256;
import Engine.i18n;
import Engine.Utils.Logger;
import Engine.Basics.Memory.MemoryStream;

using Engine::Basics::sha256::sha256id;
using Engine::Utils::Logger::Log;

export namespace Engine::Utils::Cache {
class CacheManager {
private:
    std::string cachedir = "./.Cache";
    mutable std::shared_mutex mtx;

    auto EnsureDir(const std::string& path) -> int
    {
        std::filesystem::path filePath(path);
        std::filesystem::path parentDir = filePath.parent_path();
        if (!parentDir.empty()) {
            std::error_code dirEc;
            std::filesystem::create_directories(parentDir, dirEc);
            if (dirEc) {
                Log([&]() -> std::string {
                    return Engine::i18n::fmt("无法创建目录 \"{}\": {}", parentDir.string(), dirEc.message());
                });
                return 1;
            }
        }
        return 0;
    }

public:
    auto Init(const std::string& dir) -> int
    {
        cachedir = dir;
        while (cachedir.back() != '/') {
            cachedir.pop_back();
        }
        return EnsureDir(cachedir);
    }
    auto CreateCache(const Engine::Basics::Memory::MemoryBlock& block, const sha256id& sha) -> int
    {
        std::string fpath = cachedir + "/" + sha[0, 2] + "/";
        EnsureDir(fpath);
        fpath.append(sha);
        std::fstream file(fpath, std::ios::trunc | std::ios::in | std::ios::out | std::ios::binary);
        if (!file) {
            return 1;
        }
        Engine::Basics::Memory::MemoryStream ms(block.block, block.size);
        ms >> file;
        file.close();
        return 0;
    }
    auto LoadCache(const sha256id& sha) -> Engine::Basics::Memory::MemoryBlock
    {
        std::string fpath = cachedir;
        fpath += "/";
        fpath += sha[0, 2];
        fpath += "/";
        fpath += sha.getlstr();
        std::ifstream file(fpath, std::ios::in | std::ios::binary);
        if (!file) {
            return { .block = nullptr, .size = 0 };
        }

        file.seekg(0, std::ios::end);
        uint64_t size = file.tellg();
        file.seekg(0, std::ios::beg);

        if (size == 0) {
            return { .block = nullptr, .size = 0 };
        }
        std::shared_ptr<uint8_t[]> buffer = std::make_shared<uint8_t[]>(size);
        file.read(reinterpret_cast<char*>(buffer.get()), size); // NOLINT

        if (!file) {
            return { .block = nullptr, .size = 0 };
        }

        return { .block = buffer, .size = size };
    }
};
}

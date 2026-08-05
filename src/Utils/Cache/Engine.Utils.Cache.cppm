module;

#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <string>

export module Engine.Utils.Cache;

import Engine.Basics.sha256;
import Engine.i18n;
import Engine.Utils.Logger;

using Engine::Utils::Logger::Log;

export namespace Engine::Utils::Cache {
class CacheManager {
private:
    std::string cachedir = "./.Cache/";
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
        return EnsureDir(cachedir);
    }
    auto CreateCache(const std::shared_ptr<uint8_t[]>& mem, int size) -> int
    {

        return 0;
    }
};
}

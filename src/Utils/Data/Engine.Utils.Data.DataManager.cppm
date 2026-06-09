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
            const uint8_t __version=1;

            class DataManager {
            private:
                std::unordered_map<std::string, std::shared_ptr<DataEntry>> Data;
                mutable std::shared_mutex mtx;

            public:
                int InsertEntry(const std::string& key, std::shared_ptr<DataEntry> entry);

                std::shared_ptr<DataEntry> GetEntry(const std::string& key) const;

                bool RemoveEntry(const std::string& key);

                int MountDB_memory(const std::shared_ptr<uint8_t[]> mem);

                int MountDB(std::string path);
            };
        }
    }
}

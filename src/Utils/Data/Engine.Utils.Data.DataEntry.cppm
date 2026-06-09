/**
 * @brief 数据条目类
 * 
 */
module;

#include <cassert>
#include <cstdint>
#include <memory>
#include <atomic>
#include <shared_mutex>
#include <mutex>
#include <cstring>

export module Engine.Utils.Data.DataEntry;

import Engine.Basics.Memory;
import Engine.Utils.Logger;
import Engine.Utils.Data.DB;
import Engine.Utils.Data.DataEntry.EntryType;

export namespace Engine {
    namespace Utils {
        namespace Data {
            /**
             * @brief 数据条目结构，包含数据的大小、类型、名称和数据本体等信息，并提供线程安全的读写访问接口
             * 
             */
            struct DataEntry {
                std::atomic<uint32_t> Size{0};//大小
                std::atomic<uint32_t> Type{0};//类型
                std::atomic<uint8_t>  NameSize{0};//名称大小
                std::atomic<std::shared_ptr<const std::string>> Name{nullptr};//名称
                std::atomic<std::shared_ptr<uint8_t[]>> Data;//数据
                mutable std::shared_mutex DataMutex;//锁
                // 设置数据
                void New(uint32_t size){
                    std::unique_lock lock(DataMutex);
                    Size=size;
                    Data.store(std::make_shared<uint8_t[]>(size));
                }
                // 只读访问回调（共享锁）
                template <typename F>
                auto Read(F&& func) const {
                    std::shared_lock lock(DataMutex);
                    std::shared_ptr<uint8_t[]> dataPtr = Data.load(std::memory_order_acquire);
                    if (!dataPtr) {
                        throw std::runtime_error("Engine::Utils::Data::DataEntry::Read(F&& func)::dataPtr: Nullptr");
                    }
                    return func(dataPtr.get(),this);
                }

                // 读写访问回调（独占锁）
                template <typename F>
                auto Write(F&& func) {
                    std::unique_lock lock(DataMutex);
                    auto dataPtr = Data.load(std::memory_order_acquire);
                    if (!dataPtr) {
                        throw std::runtime_error("Engine::Utils::Data::DataEntry::Write(F&& func)::dataPtr: Nullptr");
                    }
                    return func(dataPtr.get(),this);
                }

                uint32_t GetSize() const {
                    return Size.load();
                }
            };
            
            /**
             * @brief 压力一个Entry?!!
             * 
             */
            void __test_entry(){
                Engine::Utils::Data::DataEntry ent;
                ent.Size=sizeof(Engine::Utils::Data::DB_Header);
                auto sz=ent.GetSize();
                ent.New(sizeof(Engine::Utils::Data::DB_Header));
                ent.Write([](uint8_t* data,const Engine::Utils::Data::DataEntry* entry) {
                    Engine::Utils::Data::DB_Header* a=new Engine::Utils::Data::DB_Header();
                    a->SetDesc("NMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLNMSLSB", 503);
                    a->SetDesc("a", 2,false);
                    memcpy(data, a, sizeof(Engine::Utils::Data::DB_Header));
                });
                ent.Read([sz](uint8_t* data,const Engine::Utils::Data::DataEntry* entry) {
                    Engine::Basics::Memory::dump_hex(data,sizeof(Engine::Utils::Data::DB_Header));
                });
            }
        }
    }
}

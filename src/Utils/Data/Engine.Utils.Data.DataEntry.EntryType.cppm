/**
 * @brief 数据条目类型枚举，看不懂诗人我吃
 *
 */
module;

#include <cstdint>

export module Engine.Utils.Data.DataEntry.EntryType;

export namespace Engine::Engine::Utils::Data {
enum class EntryType : uint8_t {
    Binary = 0,
    String = 1,
    List = 2,
    Script = 3,
    png = 4,
    svg = 5,
    wav = 6,
    Font = 7
};
}

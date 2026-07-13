/**
 * @brief 内存操作模块
 *
 */
module;

#include <cstdint>
#include <print>

export module Engine.Basics.Memory;

export namespace Engine::Basics::Memory {
auto dump_hex(const uint8_t* buf, uint32_t size) -> void
{
    for (uint32_t i = 0; i < size; i += 16) {
        std::print("{:08X}: ", i);

        for (int j = 0; j < 16; j++) {
            if (i + j < size) {
                std::print("{:02X} ", buf[i + j]);
            } else {
                std::print("   ");
            }
        }
        std::print(" ");

        for (int j = 0; j < 16; j++) {
            if (i + j < size) {
                uint8_t ch = buf[i + j];
                char c = ((static_cast<unsigned int>(ch) - ' ') < 127u - ' ') ? static_cast<char>(ch) : '.';
                std::print("{}", c);
            }
        }
        std::print("\n");
    }
}

}

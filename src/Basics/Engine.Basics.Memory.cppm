/**
 * @brief 内存操作模块
 *
 */
module;

#include <array>
#include <cstdint>
#include <format>
#include <ios>
#include <memory>
#include <sstream>
#include <string>
#include <utility>

export module Engine.Basics.Memory;

import Engine.Utils.Logger;

using Engine::Utils::Logger::Log;

export namespace Engine::Basics::Memory {
template <typename T>
inline auto _ths_affix_rt(std::string& _result_, T _affix_) -> void
{
    _result_.append(_affix_);
}

template <auto _case_ = std::uppercase, typename T = const char*, typename T2 = const char*>
auto to_hex_string(const std::shared_ptr<uint8_t[]>& data, uint32_t size, T prefix = "", T2 suffix = "") -> std::string
{
    if (!data || size <= 0) {
        return { };
    }
    std::string result;
    result.reserve(static_cast<std::string::size_type>(size) * static_cast<std::string::size_type>(2)); // NOLINT
    if constexpr (_case_ == std::uppercase) {
        static const std::array<char, 17> hex = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' }; // NOLINT
        for (unsigned int i = 0; std::cmp_less(i, size); ++i) {
            uint8_t byte = data[i]; // NOLINT
            _ths_affix_rt(result, prefix);
            result.push_back(hex.at(byte >> 4));
            result.push_back(hex.at(byte & 0x0F));
            _ths_affix_rt(result, suffix);
        }
    } else if constexpr (_case_ == std::nouppercase) { // NOLINT
        static const std::array<char, 17> hex = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f' }; // NOLINT
        for (unsigned int i = 0; std::cmp_less(i, size); ++i) {
            uint8_t byte = data[i]; // NOLINT
            _ths_affix_rt(result, prefix);
            result.push_back(hex.at(byte >> 4));
            result.push_back(hex.at(byte & 0x0F));
            _ths_affix_rt(result, suffix);
        }
    } else {
        static_assert(std::is_same_v<decltype(_case_), void>, "template err");
    }
    return result;
}

auto dump_hex(const uint8_t* buf, uint32_t size) -> void
{
    Log("\033[94m┌──────────┬──────────────────────────────────────────────────┬──────────────────┐\033[0m");
    Log("\033[94m│\033[36m  Offset  \033[94m│ "
        "\033[31m00 \033[91m01 \033[32m02 \033[92m03 \033[33m04 \033[93m05 \033[34m06 \033[94m07  "
        "\033[31m08 \033[91m0A \033[32m09 \033[92m0A \033[33m0B \033[93m0C \033[34m0D \033[94m0E "
        "\033[94m│\033[36m    ASCII or .    \033[94m│\033[0m");
    Log("\033[94m├──────────┼──────────────────────────────────────────────────┼──────────────────┤\033[0m");
    for (uint32_t i = 0; std::cmp_less(i, size); i += 16) {
        std::stringstream ss;
        ss << std::format("\033[94m│\033[0m {:08X} \033[94m│\033[0m ", i);

        for (int j = 0; j < 16; j++) {
            if (i + j < size) {
                ss << std::format("\033[{}m{:02X} \033[0m", (j % 2 == 0) ? (31 + ((j / 2) % 4)) : (91 + ((j / 2) % 4)), buf[i + j]);
                if (j == 7) {
                    ss << std::format(" ");
                }
            } else {
                ss << std::format("   ");
                if (j == 7) {
                    ss << std::format(" ");
                }
            }
        }
        ss << std::format("\033[94m│\033[0m ");

        for (int j = 0; std::cmp_less(j, 16); j++) {
            if (i + j < size) {
                uint8_t ch = buf[i + j];
                char c = ((static_cast<unsigned int>(ch) - ' ') < 127u - ' ') ? static_cast<char>(ch) : '.';
                ss << std::format("{}", c);
            }
        }
        ss << std::format(" \033[94m│\033[0m");
        Log(ss.str());
        Log("\033[94m├──────────┼──────────────────────────────────────────────────┼──────────────────┤\033[0m");
    }
    Log("\033[94m│\033[36m  Offset  \033[94m│ "
        "\033[31m00 \033[91m01 \033[32m02 \033[92m03 \033[33m04 \033[93m05 \033[34m06 \033[94m07  "
        "\033[31m08 \033[91m0A \033[32m09 \033[92m0A \033[33m0B \033[93m0C \033[34m0D \033[94m0E "
        "\033[94m│\033[36m    ASCII or .    \033[94m│\033[0m");
    Log("\033[94m└──────────┴──────────────────────────────────────────────────┴──────────────────┘\033[0m");
}
}

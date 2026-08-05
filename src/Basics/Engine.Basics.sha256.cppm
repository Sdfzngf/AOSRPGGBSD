module;

#include <cstdint>
#include <cstring>
#include <iomanip>
#include <ios>
#include <memory>

export module Engine.Basics.sha256;

import Engine.Basics.Memory;
import Engine.Utils.Logger;

using Engine::Utils::Logger::Log;

// SHA-256 常量 K[0..63]
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

export namespace Engine::Basics::sha256 {

// 辅助函数
inline auto rotr(uint32_t x, int n) -> uint32_t
{
    return (x >> n) | (x << (32 - n));
}

inline auto shr(uint32_t x, int n) -> uint32_t
{
    return x >> n;
}

inline auto Ch(uint32_t x, uint32_t y, uint32_t z) -> uint32_t
{
    return (x & y) ^ (~x & z);
}

inline auto Maj(uint32_t x, uint32_t y, uint32_t z) -> uint32_t
{
    return (x & y) ^ (x & z) ^ (y & z);
}

inline auto Sigma0(uint32_t x) -> uint32_t
{
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

inline auto Sigma1(uint32_t x) -> uint32_t
{
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

inline auto sigma0(uint32_t x) -> uint32_t
{
    return rotr(x, 7) ^ rotr(x, 18) ^ shr(x, 3);
}

inline auto sigma1(uint32_t x) -> uint32_t
{
    return rotr(x, 17) ^ rotr(x, 19) ^ shr(x, 10);
}

/**
 * 计算 SHA-256 哈希值
 * @param data 指向输入数据的共享指针（可为空）
 * @param size 输入数据的字节数
 * @return 包含 32 字节哈希值的 shared_ptr
 */
auto sha256(const std::shared_ptr<uint8_t[]>& data, uint32_t size) -> std::shared_ptr<uint8_t[]>
{
    // 1. 消息填充
    uint64_t bit_len = static_cast<uint64_t>(size) * 8;
    uint32_t total_len = size + 1; // 先加 1 字节 (0x80)
    while ((total_len % 64) != 56) { // 填充至 56 字节（448 位）
        ++total_len;
    }
    total_len += 8; // 最后 8 字节存放原始位长

    // 分配填充后的缓冲区并初始化为 0
    auto padded = std::make_shared<uint8_t[]>(total_len);
    std::memset(padded.get(), 0, total_len);

    // 复制原始数据（若 data 有效且 size>0）
    if (data && size > 0) {
        std::memcpy(padded.get(), data.get(), size);
    }
    padded[size] = 0x80; // 追加 '1' 位

    // 在最后 8 字节以大端序写入原始位长
    for (int i = 0; i < 8; ++i) {
        padded[total_len - 8 + i] = static_cast<uint8_t>(bit_len >> (56 - 8 * i));
    }

    // 2. 初始化哈希值（大端序）
    uint32_t H[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // 3. 按 64 字节块处理
    for (uint32_t offset = 0; offset < total_len; offset += 64) {
        uint32_t w[64];

        // 将当前块转换为 16 个大端序 32 位字
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(padded[offset + 4 * i]) << 24) | (static_cast<uint32_t>(padded[offset + 4 * i + 1]) << 16) | (static_cast<uint32_t>(padded[offset + 4 * i + 2]) << 8) | (static_cast<uint32_t>(padded[offset + 4 * i + 3])); // NOLINT
        }

        // 扩展消息调度
        for (int i = 16; i < 64; ++i) {
            w[i] = sigma1(w[i - 2]) + w[i - 7] + sigma0(w[i - 15]) + w[i - 16]; // NOLINT
        }

        // 工作变量
        uint32_t a = H[0], b = H[1], c = H[2], d = H[3];
        uint32_t e = H[4], f = H[5], g = H[6], h = H[7];

        // 64 轮压缩
        for (int i = 0; i < 64; ++i) {
            uint32_t T1 = h + Sigma1(e) + Ch(e, f, g) + K[i] + w[i]; // NOLINT
            uint32_t T2 = Sigma0(a) + Maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + T1;
            d = c;
            c = b;
            b = a;
            a = T1 + T2;
        }

        // 更新哈希值
        H[0] += a;
        H[1] += b;
        H[2] += c;
        H[3] += d;
        H[4] += e;
        H[5] += f;
        H[6] += g;
        H[7] += h;
    }

    // 4. 输出结果（大端序）
    auto result = std::shared_ptr<uint8_t[]>(new uint8_t[32]);
    for (int i = 0; i < 8; ++i) {
        result[4 * i] = static_cast<uint8_t>(H[i] >> 24); // NOLINT
        result[4 * i + 1] = static_cast<uint8_t>(H[i] >> 16); // NOLINT
        result[4 * i + 2] = static_cast<uint8_t>(H[i] >> 8); // NOLINT
        result[4 * i + 3] = static_cast<uint8_t>(H[i]); // NOLINT
    }
    return result;
}

auto sha256str(const std::string& text) -> std::shared_ptr<uint8_t[]>
{
    std::shared_ptr<uint8_t[]> data = std::make_shared<uint8_t[]>(text.size());
    std::memcpy(data.get(), text.data(), text.size());
    return sha256(data, text.size());
}

template <auto _case_ = std::uppercase, typename T = const char*, typename T2 = const char*>
auto sha256_s(const std::shared_ptr<uint8_t[]>& data, uint32_t size, T prefix = "", T2 suffix = "") -> std::string
{
    return ::Engine::Basics::Memory::to_hex_string<_case_, T, T2>(sha256(data, size), 32, prefix, suffix);
}

template <auto _case_ = std::uppercase, typename T = const char*, typename T2 = const char*>
auto sha256str_s(const std::string& text, T prefix = "", T2 suffix = "") -> std::string
{
    return ::Engine::Basics::Memory::to_hex_string<_case_, T, T2>(sha256str(text), 32, prefix, suffix);
}

auto _test_sha256() -> int
{
    std::shared_ptr<uint8_t[]> result = sha256str("元神驱动");
    Engine::Basics::Memory::dump_hex(result.get(), 32);
    std::string text = "[\\x";
    Log(sha256str_s("元神驱动"));
    Log(sha256str_s<std::uppercase>("元神驱动"));
    Log(sha256str_s<std::nouppercase>("元神驱动"));
    // Log(sha256str_s<std::noskipws>("元神驱动"));//err
    Log(sha256str_s<std::nouppercase>("元神驱动", text, "]"));
    return 0;
}

}

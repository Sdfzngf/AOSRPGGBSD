module;

#include <cstddef>
#include <cstdint>

module Engine.Basics.TextUtils:functions;

export namespace Engine::Basics::TextUtils {
/**
 * @brief 判断字节序列是否看起来是有效的 UTF-8。
 * @param data 字节序列起始地址。
 * @param len 字节序列长度。
 * @return true 如果序列看起来是 UTF-8；否则返回 false。
 */
auto IsLikelyUtf8(const uint8_t* data, std::size_t len) -> bool
{
    std::size_t i = 0;
    while (i < len) {
        uint8_t c = data[i];
        if ((c & 0x80) == 0) {
            // ASCII
            ++i;
            continue;
        }
        std::size_t need = 0;
        if ((c & 0xE0) == 0xC0) {
            // 拒绝过长编码：0xC0-0xC1（本应编码为单字节 ASCII）
            if (c <= 0xC1)
                return false;
            need = 1;
        } else if ((c & 0xF0) == 0xE0) {
            need = 2;
        } else if ((c & 0xF8) == 0xF0) {
            // 拒绝码点 > 0x10FFFF（UTF-8 上限）
            if (c > 0xF4)
                return false;
            need = 3;
        } else
            return false;
        if (i + need >= len)
            return false;

        // 拒绝过长编码的第二个字节不合法
        if (need >= 2 && c == 0xE0 && (data[i + 1] & 0xE0) == 0x80)
            return false;
        if (need >= 3 && c == 0xF0 && (data[i + 1] & 0xF0) == 0x80)
            return false;

        for (std::size_t j = 1; j <= need; ++j) {
            if ((data[i + j] & 0xC0) != 0x80)
                return false;
        }
        i += need + 1;
    }
    return true;
}

/**
 * @brief 判断数据中可打印字符的比例是否足够高。
 * @param data 字节序列起始地址。
 * @param len 字节序列长度。
 * @param threshold 可打印比例阈值，默认使用 kPrintableThreshold。
 * @return true 如果可打印比例达到阈值；否则返回 false。
 */
auto IsMostlyPrintable(const uint8_t* data, std::size_t len, double threshold) -> bool
{
    std::size_t printable = 0;
    for (std::size_t i = 0; i < len; ++i) {
        unsigned char ch = data[i];
        if (ch == '\n' || ch == '\r' || ch == '\t' || (ch >= 32 && ch <= 126))
            ++printable;
    }
    return len == 0 ? false : (static_cast<double>(printable) / static_cast<double>(len) >= threshold);
}

/**
 * @brief 将类型编号转换为显示名称。
 * @param idx 类型编号。
 * @return 对应的类型名称。
 */
auto TypeNameFromIndex(uint32_t idx) -> const char*
{
    switch (idx) {
    case 0:
        return "Binary";
    case 1:
        return "String";
    case 2:
        return "List";
    case 3:
        return "Script";
    case 4:
        return "PNG";
    case 5:
        return "SVG";
    default:
        return "Unknown";
    }
}

/**
 * @brief 将类型编号转换为 ANSI 颜色码。
 * @param idx 类型编号。
 * @return 对应的 ANSI 颜色码。
 */
auto TypeColorFromIndex(uint32_t idx) -> const char*
{
    switch (idx) {
    case 0:
        return "\x1b[33m"; // yellow
    case 1:
        return "\x1b[32m"; // green
    case 2:
        return "\x1b[36m"; // cyan
    case 3:
        return "\x1b[35m"; // magenta
    default:
        return "\x1b[0m"; // reset
    }
}

} // namespace Engine::Basics::TextUtils

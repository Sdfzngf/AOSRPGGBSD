module;

#include <cstddef>
#include <cstdint>

export module Engine.Basics.TextUtils;

export namespace Engine::Basics::TextUtils {

/// @brief 从条目中最多读取并展示的字节数。
inline constexpr std::size_t kMaxShowBytes = 4096;

/// @brief hex 摘要显示的最大字节数。
inline constexpr std::size_t kHexSummaryBytes = 64;

/// @brief 判定为可读文本所需的最小可打印字节比例。
inline constexpr double kPrintableThreshold = 0.9;

/**
 * @brief 判断字节序列是否看起来是有效的 UTF-8。
 * @param data 字节序列起始地址。
 * @param len 字节序列长度。
 * @return true 如果序列看起来是 UTF-8；否则返回 false。
 */
auto IsLikelyUtf8(const uint8_t* data, std::size_t len) -> bool;

/**
 * @brief 判断数据中可打印字符的比例是否足够高。
 * @param data 字节序列起始地址。
 * @param len 字节序列长度。
 * @param threshold 可打印比例阈值，默认使用 kPrintableThreshold。
 * @return true 如果可打印比例达到阈值；否则返回 false。
 */
auto IsMostlyPrintable(const uint8_t* data, std::size_t len, double threshold = kPrintableThreshold) -> bool;

/**
 * @brief 将类型编号转换为显示名称。
 * @param idx 类型编号。
 * @return 对应的类型名称。
 */
auto TypeNameFromIndex(uint32_t idx) -> const char*;

/**
 * @brief 将类型编号转换为 ANSI 颜色码。
 * @param idx 类型编号。
 * @return 对应的 ANSI 颜色码。
 */
auto TypeColorFromIndex(uint32_t idx) -> const char*;

} // namespace Engine::Basics::TextUtils

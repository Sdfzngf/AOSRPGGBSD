/**
 * @brief 数据库模块
 *
 */
module;

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>

export module Engine.Utils.Data.DB;

import Engine.Utils.Logger;

export namespace Engine::Utils::Data {
/**
 * @brief 数据文件的头部结构
 *
 */
struct DB_Header {
    uint8_t version { 1 }; // 版本号，当前版本为1
    uint32_t numberOfEntrys { 0 }; // 数据条目数量
    uint8_t desc[503] { }; // 描述信息
    uint8_t end_flag { 1 }; // 结束标志

    /**
    * @brief 设置头部描述信息。
    * @param text 描述字符串。
    * @param size 描述字符串长度。
    * @param format 是否将剩余字节补零。
    * @return 0 表示成功；1 表示长度非法或输入为空。
     */
    auto SetDesc(const char* text, size_t size, bool format = true) -> uint8_t
    {
        if (size > 503 || (text == nullptr && size != 0)) {
            return 1;
        }
        memcpy(desc, text, size);

        if (format && (503 - size) > 0) {
            memset(reinterpret_cast<void*>(desc + size), '\0', 503 - size);
        }
        return 0;
    }
};

/**
 * @brief 数据条目头部。
 *
 * 保存条目名称长度、数据长度和数据类型，后面紧跟名称字节和数据字节。
 */
struct DB_DataEntry_Header {
    uint8_t NameSize { 0 }; // 名称大小
    uint32_t Size { 0 }; // 大小
    uint32_t Type { 0 }; // 类型
    // Data//数据：包含名称与真正的数据
};
}

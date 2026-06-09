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

export namespace Engine {
    namespace Utils {
        namespace Data {
            /**
             * @brief 数据文件的头部结构
             * 
             */
            struct DB_Header {
                uint8_t version{1};//版本号，当前版本为1
                int numberOfEntrys{0};//数据条目数量
                uint8_t desc[503];//描述信息
                uint8_t end_flag{1};//结束标志

                /**
                 * @brief 设置描述信息
                 * 
                 * @param text 描述信息
                 * @param size 描述字符串大小
                 * @param format 是否对已有数据进行格式化
                 * @return uint8_t 返回值，如果size超过503则返回1，否则返回0
                 */
                uint8_t SetDesc(const char* text, size_t size,bool format=true) {
                    if(size>503){
                        return 1;
                    }
                    memcpy(desc, text, size);

                    if(format && (503-size)>0){
                        memset((void*)(desc+size),'\0',503-size);
                    }
                    return 0;
                }
            };

            struct DB_DataEntry_Header {
                uint8_t  NameSize{0};//名称大小
                uint32_t Size{0};//大小
                uint32_t Type{0};//类型
                //Data//数据：包含名称与真正的数据
            };
        }
    }
}

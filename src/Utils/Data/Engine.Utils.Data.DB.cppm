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
            struct DB_Header {
                uint8_t version{1};
                int numberOfEntrys{0};
                uint8_t desc[503];
                uint8_t end_flag{1};

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
        }
    }
}

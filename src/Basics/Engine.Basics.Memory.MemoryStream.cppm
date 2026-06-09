module;

#include <cstdint>
#include <cstring>
#include <type_traits>

export module Engine.Basics.Memory.MemoryStream;

export namespace Engine {
    namespace Basics {
        namespace Memory {
            enum class StreamStat{
                GOOD=0,
                EOFF=1,
                FAIL=2,
                EMPTY=3
            };

            /**
             * @brief 不线程安全的内存流
             *
             */
            class MemoryStream{
            private:
                uint8_t* buffer=nullptr;
                uint64_t pointer=0;
                uint64_t buffersize=0;
                StreamStat stat=StreamStat::EMPTY;
                int CheckStat(unsigned int rightsize){
                    switch (stat) {
                        case StreamStat::EMPTY:
                            return 4;
                            break;
                        case StreamStat::FAIL:
                            return 1;
                            break;
                        case StreamStat::EOFF:
                            return 2;
                            break;
                        default:
                            break;
                    }
                    if (pointer+rightsize>=buffersize){
                        stat=StreamStat::EOFF;
                        return 3;
                    }
                    if(!buffer){
                        stat=StreamStat::FAIL;
                        return 1;
                    }
                    return 0;
                }
            public:
                MemoryStream(uint8_t* buffer,uint64_t buffersize){
                    this->buffer=buffer;
                    this->buffersize=buffersize;
                    stat=StreamStat::GOOD;
                }
                MemoryStream(){
                }

                MemoryStream(const MemoryStream&) = delete;
                MemoryStream& operator=(const MemoryStream&) = delete;
                ~MemoryStream() = default;

                template<typename T>
                int operator<<(T right){
                    unsigned int rightsize=0;
                    if constexpr (std::is_same_v<const char*, T>){
                        rightsize=strlen(right);
                    }else {
                        rightsize=sizeof(right);
                    }

                    int result=CheckStat(rightsize);
                    if(result!=0)return result;

                    if constexpr (std::is_pointer_v<T>) {
                        memcpy(buffer+pointer,right,rightsize);
                    }else{
                        memcpy(buffer+pointer,&right,rightsize);
                    }
                    pointer+=rightsize;
                    if(pointer==buffersize)stat=StreamStat::EOFF;
                    return 0;
                }

                template<typename T>
                int operator>>(T right){
                    if constexpr (std::is_pointer_v<T>) {
                        static_assert(false, ">>不支持指针类型");
                    }
                    unsigned int rightsize=sizeof(right);

                    int result=CheckStat(rightsize);
                    if(result!=0)return result;

                    memcpy(&right,buffer+pointer,rightsize);
                    if(pointer==buffersize)stat=StreamStat::EOFF;
                    return 0;
                }

                int SetPointer(uint64_t pointer){
                    switch (stat) {
                        case StreamStat::EMPTY:
                            return 4;
                            break;
                        case StreamStat::FAIL:
                            return 1;
                            break;
                        default:
                            break;
                    }
                    return 0;
                }

                uint64_t Size(){
                    return buffersize;
                }
            };
        }
    }
}

module;

#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

export module Engine.Basics.Memory.MemoryStream;

export namespace Engine::Basics::Memory {
enum class StreamStat : uint8_t {
    GOOD = 0,
    EOFF = 1,
    FAIL = 2,
    EMPTY = 3
};
struct MemoryBlock {
    std::shared_ptr<uint8_t[]> block;
    uint64_t size;
};

/**
 * @brief 线程不安全的内存流
 *
 */
class MemoryStream {
private:
    std::shared_ptr<uint8_t[]> buffer = nullptr;
    uint64_t pointer = 0;
    uint64_t buffersize = 0;
    StreamStat stat = StreamStat::EMPTY;
    auto CheckStat(unsigned int rightsize) -> int
    {
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
        if (pointer + rightsize > buffersize) {
            stat = StreamStat::EOFF;
            return 3;
        }
        if (!buffer) {
            stat = StreamStat::FAIL;
            return 1;
        }
        return 0;
    }

public:
    MemoryStream(const std::shared_ptr<uint8_t[]>& buffer, uint64_t buffersize)
        : buffer(buffer)
        , buffersize(buffersize)
        , stat(StreamStat::GOOD)
    {
    }
    MemoryStream(const MemoryBlock& block)
        : buffer(block.block)
        , buffersize(block.size)
        , stat(StreamStat::GOOD)
    {
    }

    MemoryStream(const MemoryStream&) = delete;
    auto operator=(const MemoryStream&) -> MemoryStream& = delete;
    MemoryStream(MemoryStream&&) = delete;
    ~MemoryStream() = default;
    auto operator=(MemoryStream&&) -> MemoryStream& = delete;

    template <typename T>
    auto operator<<(T right) -> int
    {
        unsigned int rightsize = 0;
        if constexpr (std::is_same_v<const char*, T>) {
            rightsize = strlen(right);
        } else if constexpr (std::is_same_v<MemoryBlock, T>) {
            rightsize = right.size;
        } else if constexpr (std::is_same_v<std::shared_ptr<uint8_t[]>, T>) {
            static_assert(false, "不要用来操作智能指针");
        } else {
            rightsize = sizeof(right);
        }

        int result = CheckStat(rightsize);
        if (result != 0)
            return result;

        if constexpr (std::is_pointer_v<T>) {
            memcpy(buffer.get() + pointer, right, rightsize);
        } else if constexpr (std::is_same_v<MemoryBlock, T>) {
            memcpy(buffer.get() + pointer, right.block.get(), rightsize);
        } else {
            memcpy(buffer.get() + pointer, &right, rightsize);
        }
        pointer += rightsize;
        if (pointer == buffersize)
            stat = StreamStat::EOFF;
        return 0;
    }

    template <typename T>
    auto operator>>(T& right) -> int
    {
        unsigned int rightsize = 0;
        if constexpr (std::is_pointer_v<T>) {
            static_assert(false, ">>不支持指针类型");
        } else if constexpr (std::is_same_v<std::shared_ptr<uint8_t[]>, T>) {
            static_assert(false, "不要用来操作智能指针");
        } else if constexpr (std::is_same_v<MemoryBlock, T>) {
            rightsize = right.size;
        } else {
            rightsize = sizeof(right);
        }

        int result = CheckStat(rightsize);
        if (result != 0)
            return result;

        if constexpr (std::is_same_v<MemoryBlock, T>) {
            memcpy(right.block.get(), buffer.get() + pointer, rightsize);
        } else {
            memcpy(&right, buffer.get() + pointer, rightsize);
        }
        pointer += rightsize;
        if (pointer == buffersize)
            stat = StreamStat::EOFF;
        else
            stat = StreamStat::GOOD;
        return 0;
    }

    auto SetPointer(uint64_t pointer) -> int
    {
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
        this->pointer = pointer;
        if (this->pointer > buffersize) {
            stat = StreamStat::EOFF;
            return 3;
        }
        stat = this->pointer == buffersize ? StreamStat::EOFF : StreamStat::GOOD;
        return 0;
    }

    auto Size() -> uint64_t { return buffersize; }
};
} // namespace Engine::Basics::Memory

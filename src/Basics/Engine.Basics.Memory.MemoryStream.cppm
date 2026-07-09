/**
 * @brief 内存流
 *
 */
module;

#include <cstdint>
#include <cstring>
#include <memory>
#include <type_traits>

export module Engine.Basics.Memory.MemoryStream;

export namespace Engine::Basics::Memory {

// 内存流状态
enum class StreamStat : uint8_t {
    GOOD = 0,
    EOFF = 1,
    FAIL = 2,
    EMPTY = 3 // 如果内存流未初始化
};

// 内存块
struct MemoryBlock {
    std::shared_ptr<uint8_t[]> block; // 内存块
    uint64_t size; // 内存块大小
};

/**
 * @brief 线程不安全的内存流
 *
 */
class MemoryStream {
private:
    std::shared_ptr<uint8_t[]> buffer = nullptr; // 指向缓冲区的智能指针
    uint64_t pointer = 0; // 指针位置
    uint64_t buffersize = 0; // 缓冲区大小
    StreamStat stat = StreamStat::EMPTY; // 状态

    /**
     * @brief 检查状态
     * @param rightsize 右值大小
        * @return 0 表示可继续读写；非 0 表示当前状态或边界检查失败。
     */
    auto CheckStat(uint64_t rightsize) -> int
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
        if (!buffer) {
            stat = StreamStat::FAIL;
            return 1;
        }
        if (pointer > buffersize || rightsize > buffersize - pointer) {
            stat = StreamStat::EOFF;
            return 3;
        }
        return 0;
    }

public:
    /**
     * @brief 构造一个指向缓冲区的内存流。
     * @param buffer 共享缓冲区。
     * @param buffersize 缓冲区大小。
     */
    MemoryStream(const std::shared_ptr<uint8_t[]>& buffer, uint64_t buffersize)
        : buffer(buffer)
        , buffersize(buffersize)
        , stat(StreamStat::GOOD)
    {
    }
    /**
     * @brief 使用 MemoryBlock 构造一个内存流。
     * @param block 内存块。
     */
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

    /**
     * @brief 将数据写入缓冲区。
     * @tparam T 写入类型。
     * @param right 待写入数据。
     * @return 0 表示成功；非 0 表示失败。
     */
    template <typename T>
    auto operator<<(T right) -> int
    {
        uint64_t rightsize = 0;
        if constexpr (std::is_same_v<const char*, T>) {
            if (!right)
                return 1;
            rightsize = strlen(right);
        } else if constexpr (std::is_same_v<MemoryBlock, T>) {
            if (right.size > 0 && !right.block)
                return 1;
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

    /**
     * @brief 从缓冲区读取数据。
     * @tparam T 读取类型。
     * @param right 读取目标。
     * @return 0 表示成功；非 0 表示失败。
     */
    template <typename T>
    auto operator>>(T& right) -> int
    {
        uint64_t rightsize = 0;
        if constexpr (std::is_pointer_v<T>) {
            static_assert(false, ">>不支持指针类型");
        } else if constexpr (std::is_same_v<std::shared_ptr<uint8_t[]>, T>) {
            static_assert(false, "不要用来操作智能指针");
        } else if constexpr (std::is_same_v<MemoryBlock, T>) {
            if (right.size > 0 && !right.block)
                return 1;
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

    /**
     * @brief 设置当前读写指针。
     * @param pointer 新的指针位置。
     * @return 0 表示成功；非 0 表示越界或状态异常。
     */
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

    /**
     * @brief 获取缓冲区总大小。
     * @return 缓冲区大小。
     */
    [[nodiscard]] auto Size() -> uint64_t { return buffersize; }

    /**
     * @brief 获取当前位置到缓冲区末尾的剩余字节数。
     * @return 剩余字节数。
     */
    [[nodiscard]] auto Remaining() const -> uint64_t
    {
        if (pointer >= buffersize)
            return 0;
        return buffersize - pointer;
    }
};
} // namespace Engine::Basics::Memory

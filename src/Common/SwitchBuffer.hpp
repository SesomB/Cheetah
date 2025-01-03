#include <array>
#include <atomic>
#include <cstddef>
#include <cassert>

template <typename T, std::size_t SIZE>
class SwitchBuffer
{
    static_assert(SIZE >= 2, "SwitchBuffer must have at least 2 buffers");

private:
    std::array<T, SIZE> m_Buffers{};
    std::atomic<int> m_ActiveBufferIndex{0}; // Atomic for thread-safe access
    std::atomic<bool> m_BufferReady{false};  // Flag to indicate readiness of the buffer

public:
    inline T &GetActiveBuffer()
    {
        return m_Buffers[m_ActiveBufferIndex.load(std::memory_order_acquire)];
    }

    inline const T &GetActiveBuffer() const
    {
        return m_Buffers[m_ActiveBufferIndex.load(std::memory_order_acquire)];
    }

    inline T &GetInactiveBuffer()
    {
        return m_Buffers[(m_ActiveBufferIndex.load(std::memory_order_acquire) + 1) % SIZE];
    }

    inline const T &GetInactiveBuffer() const
    {
        return m_Buffers[(m_ActiveBufferIndex.load(std::memory_order_acquire) + 1) % SIZE];
    }

    void PopulateInactiveBuffer(const T &data)
    {
        auto &inactiveBuffer = GetInactiveBuffer();
        inactiveBuffer = data; // Update the inactive buffer
        m_BufferReady.store(true, std::memory_order_release);
    }

    inline void SetNextBuffer()
    {
        if (m_BufferReady.load(std::memory_order_acquire))
        {
            m_ActiveBufferIndex.store((m_ActiveBufferIndex.load(std::memory_order_relaxed) + 1) % SIZE, std::memory_order_release);
            m_BufferReady.store(false, std::memory_order_release); // Reset readiness flag
        }
    }

    bool IsBufferReady() const
    {
        return m_BufferReady.load(std::memory_order_acquire);
    }
};

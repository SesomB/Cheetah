#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <utility> // std::forward, std::index_sequence

template <typename T, std::size_t BUFFERS_COUNT>
class MultiBuffer
{
    static_assert(BUFFERS_COUNT >= 2, "Need at least two buffers");

    /* ---------- single slot ------------------------------------------------ */
    class alignas(64) Buffer
    {
      public:
        T data;
        alignas(64) std::atomic<bool> valid{true};

        constexpr Buffer() = default;

        template <typename... Args>
        constexpr Buffer(Args &&... args)
            : data(std::forward<Args>(args)...)
            , valid(true)
        {
        }
    };

    /* ---------- build N identical slots ------------------------------------ */
    template <std::size_t... I, typename... Args>
    static constexpr std::array<Buffer, BUFFERS_COUNT> make_array(std::index_sequence<I...>, Args &&... args)
    {
        return {(static_cast<void>(I), Buffer{std::forward<Args>(args)...})...};
    }

    std::array<Buffer, BUFFERS_COUNT> m_Buffers;

    /* ---------- active-index ------------------------------------------------ */
    alignas(64) std::atomic<std::size_t> m_Active{0};

    std::size_t nextIndex(std::size_t i) const noexcept
    {
        return (i + 1) % BUFFERS_COUNT;
    }

  public:
    /* construct every slot from the *same* argument list */
    template <typename... Args>
    constexpr explicit MultiBuffer(Args &&... args)
        : m_Buffers(make_array(std::make_index_sequence<BUFFERS_COUNT>{}, std::forward<Args>(args)...))
    {
    }

    constexpr MultiBuffer() = default;
    ~MultiBuffer() = default;

    /* --------- fast paths --------------------------------------------------- */
    /// Consumers call this – always the latest published data
    const T *read() const noexcept
    {
        const auto &buffer = m_Buffers[m_Active.load(std::memory_order_acquire)];
        if (buffer.valid.load(std::memory_order_relaxed) == false)
            return nullptr;
        return &(buffer.data);
    }

    /// Producers call this, fill the object, then `publish()`
    T &write() noexcept
    {
        auto &buffer = m_Buffers[nextIndex(m_Active.load(std::memory_order_relaxed))];
        buffer.valid.store(false, std::memory_order_release);
        return buffer.data;
    }

    /// After `write()` is fully populated, call this to flip the active slot
    void publish() noexcept
    {
        std::size_t newIdx = nextIndex(m_Active.load(std::memory_order_relaxed));
        m_Active.store(newIdx, std::memory_order_release);
        auto &buffer = m_Buffers[m_Active.load(std::memory_order_acquire)];
        buffer.valid.store(true, std::memory_order_release);
    }

    std::size_t activeIndex() const noexcept
    {
        return m_Active.load();
    }

    inline auto &getBuffersForDebug()
    {
        return m_Buffers;
    }
};

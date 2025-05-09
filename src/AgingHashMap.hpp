#pragma once

#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include <rte_hash.h>
#include <rte_jhash.h>
#include <rte_lcore.h>
#include <rte_mempool.h>
#include <rte_timer.h>

template <typename TKey, typename TValue, std::size_t Capacity = (1 << 16) - 1, typename ExpireFn = void,
          unsigned MempoolCache = RTE_MEMPOOL_CACHE_MAX_SIZE,
          uint32_t ExtraFlags = RTE_HASH_EXTRA_FLAGS_EXT_TABLE | RTE_HASH_EXTRA_FLAGS_RW_CONCURRENCY>
class AgingHashMap
{

  private:
    uint64_t m_TtlTicks;
    rte_mempool *m_Mempool{nullptr};
    rte_hash *m_Hash{nullptr};
    std::conditional_t<std::is_void<ExpireFn>::value, char, ExpireFn>
        m_ExpireFunction; // Only define this member if ExpireFn is not void
    int8_t m_SocketId;

    static_assert(std::is_trivially_copyable<TKey>::value, "TKey must be trivially copyable for rte_hash");

    struct Entry
    {
        TKey key;
        TValue value;
        rte_timer timer;
        AgingHashMap *parent;
    };

  public:
    /// @param base_name unique prefix for mempool+hash names
    /// @param ttl_sec   how many seconds until an entry expires
    /// @param socket_id which DPDK socket to allocate on
    template <typename F = ExpireFn, typename = std::enable_if_t<!std::is_void<F>::value>>
    AgingHashMap(const std::string &base_name, F expire_func, uint64_t ttl_sec = 3, int socket_id = SOCKET_ID_ANY)
        : m_TtlTicks(ttl_sec * rte_get_timer_hz())
        , m_ExpireFunction(expire_func)
        , m_SocketId(socket_id)
    {
        init(base_name);
    }

    template <typename F = ExpireFn, typename = std::enable_if_t<std::is_void<F>::value>>
    AgingHashMap(const std::string &base_name, uint64_t ttl_sec = 3, int socket_id = SOCKET_ID_ANY)
        : m_TtlTicks(ttl_sec * rte_get_timer_hz())
        , m_SocketId(socket_id)

    {
        init(base_name);
    }

    ~AgingHashMap()
    {
        // stop & destroy all entries
        clear();
        if (m_Hash)
            rte_hash_free(m_Hash);
        if (m_Mempool)
            rte_mempool_free(m_Mempool);
    }

    // non-copyable
    AgingHashMap(const AgingHashMap &) = delete;
    AgingHashMap &operator=(const AgingHashMap &) = delete;

    // movable
    AgingHashMap(AgingHashMap &&other) noexcept
        : m_SocketId(other.m_SocketId)
        , m_TtlTicks(other.m_TtlTicks)
        , m_Mempool(other.m_Mempool)
        , m_Hash(other.m_Hash)
    {
        if constexpr (!std::is_void<ExpireFn>::value)
            m_ExpireFunction = other.m_ExpireFunction;
        other.m_Mempool = nullptr;
        other.m_Hash = nullptr;
        reassignParents();
    }

    AgingHashMap &operator=(AgingHashMap &&other) noexcept
    {
        if (this != &other)
        {
            // clean up existing
            clear();
            if (m_Hash)
                rte_hash_free(m_Hash);
            if (m_Mempool)
                rte_mempool_free(m_Mempool);

            // move from o
            m_SocketId = other.m_SocketId;
            m_TtlTicks = other.m_TtlTicks;
            m_Mempool = other.m_Mempool;
            m_Hash = other.m_Hash;
            if constexpr (!std::is_void<ExpireFn>::value)
                m_ExpireFunction = other.m_ExpireFunction;

            other.m_Mempool = nullptr;
            other.m_Hash = nullptr;
            reassignParents();
        }
        return *this;
    }

    /// Must be driven periodically on one lcore
    void manageTimers()
    {
        rte_timer_manage();
    }

    // /// Insert or update (reset TTL). Returns pointer to value or nullptr on failure.
    // TValue *try_emplace(const TKey &key, const TValue &value)
    // {
    //     Entry *entry = nullptr;
    //     // existing?
    //     if (rte_hash_lookup_data(m_Hash, &key, (void **)&entry) >= 0)
    //     {
    //         entry->value = value;
    //         scheduleTimer(entry);
    //         return &entry->value;
    //     }

    //     // allocate a slot
    //     if (rte_mempool_get(m_Mempool, (void **)&entry) < 0)
    //         return nullptr;

    //     // placement-new + init
    //     new (&entry->value) TValue(value);
    //     entry->key = key;
    //     entry->parent = this;
    //     rte_timer_init(&entry->timer);

    //     // insert into hash (nullptr for bucket)
    //     if (rte_hash_add_key_data(m_Hash, &entry->key, entry) < 0)
    //     {
    //         entry->value.~TValue();
    //         rte_mempool_put(m_Mempool, entry);
    //         return nullptr;
    //     }

    //     scheduleTimer(entry);
    //     return &entry->value;
    // }

    /// rvalue overload (move)
    // template <typename T>
    // TValue *try_emplace(const TKey &key, T &&value)
    // {
    //     static_assert(std::is_same<std::decay_t<T>, TValue>::value, "try_emplace() requires TValue-compatible type");

    //     Entry *entry = nullptr;
    //     // 1) existing?
    //     if (rte_hash_lookup_data(m_Hash, &key, (void **)&entry) >= 0)
    //     {
    //         entry->value = std::forward<T>(value); // move‐assign
    //         scheduleTimer(entry);
    //         return &entry->value;
    //     }
    //     // 2) allocate slot
    //     if (rte_mempool_get(m_Mempool, (void **)&entry) < 0)
    //         return nullptr;

    //     // 3) placement‐new + move‐construct
    //     new (&entry->value) TValue(std::forward<T>(value));
    //     entry->key = key;
    //     entry->parent = this;
    //     rte_timer_init(&entry->timer);
    //     // 4) insert
    //     if (rte_hash_add_key_data(m_Hash, &entry->key, entry) < 0)
    //     {
    //         entry->value.~TValue();
    //         rte_mempool_put(m_Mempool, entry);
    //         return nullptr;
    //     }
    //     scheduleTimer(entry);
    //     return &entry->value;
    // }

    template <typename... Args>
    TValue *try_emplace(const TKey &key, Args &&... args)
    {
        static_assert(std::is_constructible<TValue, Args...>::value,
                      "TValue must be constructible from provided arguments");

        Entry *entry = nullptr;

        if (rte_hash_lookup_data(m_Hash, &key, (void **)&entry) >= 0)
        {
            scheduleTimer(entry);
            return &entry->value;
        }

        if (rte_mempool_get(m_Mempool, (void **)&entry) < 0)
            return nullptr;

        new (&entry->value) TValue(std::forward<Args>(args)...);
        entry->key = key;
        entry->parent = this;
        rte_timer_init(&entry->timer);

        if (rte_hash_add_key_data(m_Hash, &entry->key, entry) < 0)
        {
            entry->value.~TValue();
            rte_mempool_put(m_Mempool, entry);
            return nullptr;
        }

        scheduleTimer(entry);
        return &entry->value;
    }

    /// Lookup without extending TTL
    template <bool ExtendTTL = false>
    TValue *lookup(const TKey &key) const noexcept
    {
        Entry *entry = nullptr;
        if ((rte_hash_lookup_data(m_Hash, &key, (void **)&entry) >= 0))
        {
            if constexpr (ExtendTTL)
                scheduleTimer(entry);
            return &entry->value;
        }
        return nullptr;
        // return (rte_hash_lookup_data(m_Hash, &key, (void **)&entry) >= 0) ? &entry->value : nullptr;
    }

    /// Remove immediately. Returns true if an entry was erased.
    bool erase(const TKey &key)
    {
        Entry *entry = nullptr;
        if (rte_hash_lookup_data(m_Hash, &key, (void **)&entry) < 0)
            return false;
        if (rte_hash_del_key(m_Hash, &key) < 0)
            return false;

        rte_timer_stop(&entry->timer);
        entry->value.~TValue();
        rte_mempool_put(m_Mempool, entry);
        return true;
    }

    std::size_t size() const noexcept
    {
        return rte_hash_count(m_Hash);
    }

    /// Clears all entries but keeps the same mempool+hash
    void clear()
    {
        const void *key = nullptr;
        void *value = nullptr;
        uint32_t next = 0;
        while (rte_hash_iterate(m_Hash, &key, &value, &next) >= 0)
        {
            auto *entry = static_cast<Entry *>(value);
            rte_timer_stop(&entry->timer);
            entry->value.~TValue();
            rte_mempool_put(m_Mempool, entry);
        }
        rte_hash_reset(m_Hash);
    }

  private:
    static void expire_cb(rte_timer *expired_timer, void *arg)
    {
        auto *entry = static_cast<Entry *>(arg);
        bool to_erase = true;

        if constexpr (!std::is_void<ExpireFn>::value)
            to_erase = entry->parent->m_ExpireFunction(entry->key, entry->value);

        if (to_erase)
            entry->parent->erase(entry->key);
    }

    void init(const std::string &base_name)
    {
        const std::string poolName = base_name + "_mp";
        const std::string tableName = base_name + "_ht";

        if (rte_timer_subsystem_init() < 0)
            throw std::runtime_error("rte_timer_subsystem_init failed");

        // 1) create mempool for Entry objects
        m_Mempool = rte_mempool_create(poolName.c_str(), Capacity, sizeof(Entry), MempoolCache, 0, nullptr, nullptr,
                                       nullptr, nullptr, m_SocketId, 0);
        if (!m_Mempool)
            throw std::bad_alloc();

        // 2) create hash table (external-data + RW concurrency)
        rte_hash_parameters params = {
            .name = tableName.c_str(),
            .entries = Capacity,
            .reserved = 0,
            .key_len = sizeof(TKey),
            .hash_func = rte_jhash,
            .hash_func_init_val = 0,
            .socket_id = m_SocketId,
            .extra_flag = ExtraFlags,
        };

        m_Hash = rte_hash_create(&params);
        if (!m_Hash)
        {
            rte_mempool_free(m_Mempool);
            throw std::bad_alloc();
        }
    }

    void scheduleTimer(Entry *entry) const
    {
        const uint32_t lcore = rte_lcore_id();
        rte_timer_reset(&entry->timer, m_TtlTicks,
                        SINGLE, // one-shot
                        lcore, &AgingHashMap::expire_cb, entry);
    }

    /// After a move, update each Entry’s parent pointer to `this`
    void reassignParents()
    {
        const void *key = nullptr;
        void *value = nullptr;
        uint32_t next = 0;
        while (rte_hash_iterate(m_Hash, &key, &value, &next) >= 0)
        {
            static_cast<Entry *>(value)->parent = this;
        }
    }
};

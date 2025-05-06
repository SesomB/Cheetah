#include <iostream>
#include <rte_hash.h>
#include <rte_jhash.h>
#include <rte_lcore.h>
#include <rte_mempool.h>
#include <rte_timer.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

template <typename TKey, typename TValue, std::size_t Capacity = 1 << 16,
          unsigned MempoolCache = RTE_MEMPOOL_CACHE_MAX_SIZE,
          uint32_t ExtraFlags = RTE_HASH_EXTRA_FLAGS_EXT_TABLE        // external-data
                                | RTE_HASH_EXTRA_FLAGS_RW_CONCURRENCY // thread-safe
          >
class MyAgingHashMap
{
    static_assert(std::is_trivially_copyable<TKey>::value, "TKey must be trivially copyable for rte_hash");

    struct Entry
    {
        TKey key;
        TValue value;
        rte_timer timer;
        MyAgingHashMap *parent;
    };

  public:
    /// @param base_name   Unique name for this map (used internally for mempool/table)
    /// @param ttl_sec     Time-to-live in seconds (default 3s)
    /// @param socket_id   DPDK socket (default ANY)
    MyAgingHashMap(std::string base_name, uint64_t ttl_sec = 3, int socket_id = SOCKET_ID_ANY)
        : m_SocketId(socket_id)
        , m_TtlTicks(ttl_sec * rte_get_timer_hz())
        , m_PoolName(base_name + "_mp")
        , m_TableName(base_name + "_ht")
    {
        // 1) init timer subsystem (safe to call multiple times)
        rte_timer_subsystem_init();

        // 2) create mempool for Entry objects
        m_Mempool = rte_mempool_create(m_PoolName.c_str(), Capacity, sizeof(Entry), MempoolCache, 0, nullptr, nullptr,
                                       nullptr, nullptr, m_SocketId, 0);
        if (!m_Mempool)
            throw std::bad_alloc();

        // 3) create the hash table mapping TKey→Entry*
        rte_hash_parameters params = {.name = m_TableName.c_str(),
                                      .entries = Capacity,
                                      .reserved = 0,
                                      .key_len = sizeof(TKey),
                                      .hash_func = rte_jhash,
                                      .hash_func_init_val = 0,
                                      .socket_id = m_SocketId,
                                      .extra_flag = ExtraFlags};
        m_Hash = rte_hash_create(&params);
        if (!m_Hash)
        {
            rte_mempool_free(m_Mempool);
            throw std::bad_alloc();
        }
    }

    ~MyAgingHashMap()
    {
        clear(); // stops timers, destroys all entries
        if (m_Hash)
            rte_hash_free(m_Hash);
        if (m_Mempool)
            rte_mempool_free(m_Mempool);
    }

    MyAgingHashMap(const MyAgingHashMap &) = delete;
    MyAgingHashMap &operator=(const MyAgingHashMap &) = delete;

    MyAgingHashMap(MyAgingHashMap &&o) noexcept
        : m_SocketId(o.m_SocketId)
        , m_TtlTicks(o.m_TtlTicks)
        , m_PoolName(std::move(o.m_PoolName))
        , m_TableName(std::move(o.m_TableName))
        , m_Mempool(o.m_Mempool)
        , m_Hash(o.m_Hash)
    {
        o.m_Mempool = nullptr;
        o.m_Hash = nullptr;
    }
    MyAgingHashMap &operator=(MyAgingHashMap &&o) noexcept
    {
        if (this != &o)
        {
            this->~MyAgingHashMap();
            m_SocketId = o.m_SocketId;
            m_TtlTicks = o.m_TtlTicks;
            m_PoolName = std::move(o.m_PoolName);
            m_TableName = std::move(o.m_TableName);
            m_Mempool = o.m_Mempool;
            m_Hash = o.m_Hash;
            o.m_Mempool = o.m_Hash = nullptr;
        }
        return *this;
    }

    /// Should be called periodically (entry.g. once per loop on your main lcore)
    void manageTimers()
    {
        rte_timer_manage();
    }

    /// Insert or update an entry, resetting its TTL.
    /// Returns pointer to the stored value, or nullptr on OOM/hash-fail.
    TValue *try_emplace(const TKey &key, const TValue &value)
    {
        Entry *entry = nullptr;

        // 1) existing?
        if (rte_hash_lookup_data(m_Hash, &key, (void **)&entry) >= 0)
        {
            // update value + reset timer
            entry->value = value;
            resetTimer(entry);
            return &entry->value;
        }

        // 2) allocate new Entry
        if (rte_mempool_get(m_Mempool, (void **)&entry) < 0)
            return nullptr;

        // placement-new for value & init fields
        new (&entry->value) TValue(value);
        entry->key = key;
        entry->parent = this;
        rte_timer_init(&entry->timer);

        // 3) add to hash
        if (rte_hash_add_key_data(m_Hash, &entry->key, entry) < 0)
        {
            entry->value.~TValue();
            rte_mempool_put(m_Mempool, entry);
            return nullptr;
        }

        // 4) schedule expiration
        resetTimer(entry);
        return &entry->value;
    }
    /// Insert or update an entry, resetting its TTL.
    /// Returns pointer to the stored value, or nullptr on OOM/hash-fail.
    TValue *try_emplace(const TKey &key, const TValue &&value)
    {
        Entry *entry = nullptr;

        // 1) existing?
        if (rte_hash_lookup_data(m_Hash, &key, (void **)&entry) >= 0)
        {
            // update value + reset timer
            entry->value = value;
            resetTimer(entry);
            return &entry->value;
        }

        // 2) allocate new Entry
        if (rte_mempool_get(m_Mempool, (void **)&entry) < 0)
            return nullptr;

        // placement-new for value & init fields
        *reinterpret_cast<TValue *>(&entry->value) = std::move(value);
        // new (&entry->value) TValue(value);
        entry->key = key;
        entry->parent = this;
        rte_timer_init(&entry->timer);

        // 3) add to hash
        if (rte_hash_add_key_data(m_Hash, &entry->key, entry) < 0)
        {
            entry->value.~TValue();
            rte_mempool_put(m_Mempool, entry);
            return nullptr;
        }

        // 4) schedule expiration
        resetTimer(entry);
        return &entry->value;
    }
    /// Lookup without extending TTL
    TValue *get(const TKey &key) const noexcept
    {
        Entry *entry = nullptr;
        if (rte_hash_lookup_data(m_Hash, &key, (void **)&entry) >= 0)
            return &entry->value;
        return nullptr;
    }

    /// Erase immediately (stops timer, destroys value, frees slot)
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

    /// Stop & clear all entries but keep the same pool & table
    void clear()
    {
        // void *iter = nullptr;
        // const void *k = nullptr;
        // void *d = nullptr;
        // while (rte_hash_iterate(hash_, &iter, &k, &d) >= 0)
        // {
        //     auto *entry = static_cast<Entry *>(d);
        //     rte_timer_stop(&entry->timer);
        //     entry->value.~TValue();
        //     rte_mempool_put(mempool_, entry);
        // }
        rte_hash_reset(m_Hash);
    }

  private:
    int m_SocketId;
    uint64_t m_TtlTicks;
    std::string m_PoolName, m_TableName;
    rte_mempool *m_Mempool{nullptr};
    rte_hash *m_Hash{nullptr};

    static void expire_cb(rte_timer *t, void *arg)
    {
        Entry *entry = static_cast<Entry *>(arg);
        std::cout << entry->key << " Expired !" << std::endl;
        // erase will stop the timer again (no-op) and free the slot
        entry->parent->erase(entry->key);
    }

    void resetTimer(Entry *entry)
    {
        // schedule a one-shot timer on our current lcore
        unsigned lcore = rte_lcore_id();
        rte_timer_reset(&entry->timer, m_TtlTicks, SINGLE, lcore, &MyAgingHashMap::expire_cb, entry);
    }
};

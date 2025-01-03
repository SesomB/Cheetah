#include "TrackBucket.hpp"
#include "TrackDescriptor.hpp"
#include <FiveTuple.hpp>
#include <array>
#include <new>
#include <rte_malloc.h>
#include <rte_mempool.h>

class FlowTable
{
  private:
    static constexpr int TRACK_DESCRIPTIONS_POOL_SIZE = (1 << 24) - 1;
    TrackBucket **m_HashBuckets;
    rte_mempool *m_TrackDescriptorsPool;
    rte_mempool *m_TrackBucketsPool;

  public:
    FlowTable();
    ~FlowTable();

    TrackDescriptor *lookup(const uint32_t hash, const FiveTuple &fiveTuple);
    bool insert(const uint32_t hash, const FiveTuple &fiveTuple, const uint16_t matchedRuleId);
};

FlowTable::FlowTable()
    : m_TrackDescriptorsPool(nullptr)
{
    m_HashBuckets = reinterpret_cast<TrackBucket **>(rte_zmalloc(NULL, sizeof(TrackBucket *) * 1 << 24, 64));
    if (m_HashBuckets == nullptr)
    {
        throw std::bad_alloc();
    }

    // Init mempool
    m_TrackDescriptorsPool = rte_mempool_create("TrackDescriptorsPool", TRACK_DESCRIPTIONS_POOL_SIZE,
                                                sizeof(TrackDescriptor), 0, 0, NULL, NULL, NULL, NULL, 0, 0);
    if (m_TrackDescriptorsPool == nullptr)
    {
        throw std::bad_alloc();
    }

    m_TrackBucketsPool = rte_mempool_create("TrackBucketsPool", TRACK_DESCRIPTIONS_POOL_SIZE, sizeof(TrackBucket), 0, 0,
                                            NULL, NULL, NULL, NULL, 0, 0);
    if (m_TrackBucketsPool == nullptr)
    {
        throw std::bad_alloc();
    }
}

FlowTable::~FlowTable()
{
    if (m_TrackDescriptorsPool)
        rte_mempool_free(m_TrackDescriptorsPool);
    if (m_TrackBucketsPool)
        rte_mempool_free(m_TrackBucketsPool);
    if (m_HashBuckets)
        rte_free(m_HashBuckets);
}

TrackDescriptor *FlowTable::lookup(const uint32_t hash, const FiveTuple &fiveTuple)
{
    const uint32_t RSS24MSBs = hash >> 8;

    TrackBucket *lookup_result = m_HashBuckets[RSS24MSBs];

    if (lookup_result == nullptr)
        return nullptr;
}

bool FlowTable::insert(const uint32_t hash, const FiveTuple &fiveTuple, const uint16_t matchedRuleId)
{
    const uint32_t RSS24MSBs = hash >> 8;
    const uint32_t RSS8LSBs = hash & 0xff;

    TrackBucket *lookup_result = m_HashBuckets[RSS24MSBs];

    if (lookup_result == nullptr) // insert new
    {
        TrackBucket *newTrackBucketPtr = nullptr;
        TrackDescriptor *newTrackDescriptor = nullptr;

        if (rte_mempool_get(m_TrackDescriptorsPool, (void **)&newTrackDescriptor) != 0)
            return false;
        if (rte_mempool_get(m_TrackBucketsPool, (void **)&newTrackBucketPtr) != 0)
            return false;

        newTrackDescriptor->mLastSeen = rte_rdtsc();
        newTrackDescriptor->mMatchedRules[0] = std::make_pair(matchedRuleId, 100);

        newTrackBucketPtr->mFiveTuple = fiveTuple;
        newTrackBucketPtr->mNext = nullptr;
        newTrackBucketPtr->mPrev = nullptr;
        newTrackBucketPtr->mRSS8LSBs = (hash & (0xff));
        newTrackBucketPtr->mTrackDescriptor = newTrackDescriptor;

        m_HashBuckets[RSS24MSBs] = newTrackBucketPtr;
        return true;
    }

    TrackDescriptor *trackDescriptor = nullptr;
    for (; lookup_result; lookup_result = lookup_result->mNext)
    {
        if ((lookup_result->mRSS8LSBs == RSS8LSBs) && (lookup_result->mFiveTuple == fiveTuple))
        {
            trackDescriptor = lookup_result->mTrackDescriptor;
            break;
        }
    }
}
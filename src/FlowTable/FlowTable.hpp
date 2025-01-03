#pragma once
#include "FiveTuple.hpp"
#include "TrackBucket.hpp"
#include "TrackDescriptor.hpp"
#include <cstdint>
#include <new>
#include <rte_malloc.h>
#include <rte_mempool.h>

class FlowTable
{
  private:
    static constexpr int TRACK_DESCRIPTIONS_POOL_SIZE = (1 << 22) - 1;
    static constexpr int TRACK_BUCKETS_POOL_SIZE = (1 << 22) - 1;

    TrackBucket **m_HashBuckets;
    rte_mempool *m_TrackDescriptorsPool;
    rte_mempool *m_TrackBucketsPool;

  public:
    explicit FlowTable()
        : m_HashBuckets(nullptr)
        , m_TrackDescriptorsPool(nullptr)
        , m_TrackBucketsPool(nullptr)
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

        m_TrackBucketsPool = rte_mempool_create("TrackBucketsPool", TRACK_BUCKETS_POOL_SIZE, sizeof(TrackBucket), 0, 0,
                                                NULL, NULL, NULL, NULL, 0, 0);
        if (m_TrackBucketsPool == nullptr)
        {
            throw std::bad_alloc();
        }
    }

    ~FlowTable() noexcept
    {
        if (m_TrackDescriptorsPool)
            rte_mempool_free(m_TrackDescriptorsPool);
        if (m_TrackBucketsPool)
            rte_mempool_free(m_TrackBucketsPool);
        if (m_HashBuckets)
            rte_free(m_HashBuckets);
    }

    TrackDescriptor *lookup(const uint32_t hash, const FiveTuple &fiveTuple) noexcept
    {
        const uint32_t RSS24MSBs = hash >> 8;

        TrackBucket *current_track_bucket = m_HashBuckets[RSS24MSBs];

        FiveTuple fiveTupleRev = !fiveTuple;

        for (; current_track_bucket; current_track_bucket = current_track_bucket->mNext)
        {
            if ((current_track_bucket->mRSS8LSBs == (hash & 0xff)) &&
                ((current_track_bucket->mFiveTuple == fiveTuple) || (current_track_bucket->mFiveTuple == fiveTupleRev)))
            {
                return current_track_bucket->mTrackDescriptor;
            }
        }

        return nullptr;
    }

    bool delete_entry(const uint32_t hash, TrackDescriptor *trackDescriptor)
    {
        if (trackDescriptor == nullptr)
            return false;

        const uint32_t RSS24MSBs = hash >> 8;

        TrackBucket *trackBucket = trackDescriptor->mParentTrackBucket;

        if (trackBucket == nullptr)
            [[unlikely]] return false;

        // If the track bucket is the first in the list
        if (trackBucket->mPrev == nullptr)
        {
            // If the track bucket is the only one in the list
            if (trackBucket->mNext == nullptr)
            {
                m_HashBuckets[RSS24MSBs] = nullptr;
            }
            else
            {
                m_HashBuckets[RSS24MSBs] = trackBucket->mNext;
                trackBucket->mNext->mPrev = nullptr;
            }
        }
        else
        {
            trackBucket->mPrev->mNext = trackBucket->mNext;
            if (trackBucket->mNext != nullptr)
                trackBucket->mNext->mPrev = trackBucket->mPrev;
        }
        rte_mempool_put(m_TrackDescriptorsPool, trackDescriptor);
        rte_mempool_put(m_TrackBucketsPool, trackBucket);
        return true;
    }

    bool insert(const uint32_t hash, const FiveTuple &fiveTuple, const uint16_t matchedRuleId)
    {
        const uint32_t RSS24MSBs = hash >> 8;
        const uint8_t RSS8LSBs = hash & 0xff;

        TrackBucket *current_track_bucket = m_HashBuckets[RSS24MSBs];

        if (current_track_bucket == nullptr) // insert new track bucket + track descriptor
        {
            TrackBucket *newTrackBucketPtr = nullptr;
            TrackDescriptor *newTrackDescriptor = nullptr;

            if (rte_mempool_get(m_TrackDescriptorsPool, (void **)&newTrackDescriptor) != 0)
                return false;
            if (rte_mempool_get(m_TrackBucketsPool, (void **)&newTrackBucketPtr) != 0)
                return false;

            new (newTrackDescriptor) TrackDescriptor{.mParentTrackBucket = newTrackBucketPtr,
                                                     .mLastSeen = rte_rdtsc(),
                                                     .mMatchedRules = {std::make_pair(matchedRuleId, 100)}};

            new (newTrackBucketPtr) TrackBucket{.mFiveTuple = fiveTuple,
                                                .mPrev = nullptr,
                                                .mNext = nullptr,
                                                .mTrackDescriptor = newTrackDescriptor,
                                                .mRSS8LSBs = RSS8LSBs};

            m_HashBuckets[RSS24MSBs] = newTrackBucketPtr;
            return true;
        }

        FiveTuple fiveTupleRev = !fiveTuple;

        // Traverse the linked list of track buckets to find a matching bucket
        // based on RSS8LSBs and FiveTuple. If found, return the associated track descriptor.
        for (; current_track_bucket; current_track_bucket = current_track_bucket->mNext)
        {
            if ((current_track_bucket->mRSS8LSBs == RSS8LSBs) &&
                ((current_track_bucket->mFiveTuple == fiveTuple) || (current_track_bucket->mFiveTuple == fiveTupleRev)))
            {
                return current_track_bucket->mTrackDescriptor;
            }

            // We reached the last element in the list
            if (current_track_bucket->mNext == nullptr)
                break;
        }

        // if no track bucket found, insert new track bucket + track descriptor
        TrackBucket *newTrackBucketPtr = nullptr;
        TrackDescriptor *newTrackDescriptor = nullptr;

        if (rte_mempool_get(m_TrackDescriptorsPool, (void **)&newTrackDescriptor) != 0)
            return false;
        if (rte_mempool_get(m_TrackBucketsPool, (void **)&newTrackBucketPtr) != 0)
            return false;

        new (newTrackDescriptor) TrackDescriptor{.mParentTrackBucket = newTrackBucketPtr,
                                                 .mLastSeen = rte_rdtsc(),
                                                 .mMatchedRules = {std::make_pair(matchedRuleId, 100)}};

        new (newTrackBucketPtr) TrackBucket{.mFiveTuple = fiveTuple,
                                            .mPrev = current_track_bucket,
                                            .mNext = nullptr,
                                            .mTrackDescriptor = newTrackDescriptor,
                                            .mRSS8LSBs = RSS8LSBs};

        current_track_bucket->mNext = newTrackBucketPtr;

        return true;
    }
};

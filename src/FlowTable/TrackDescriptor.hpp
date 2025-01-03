#pragma once

#include <array>
#include <stdint.h>
#include <tuple>

class TrackBucket;

class TrackDescriptor
{
  public:
    TrackBucket *mParentTrackBucket;
    uint64_t mLastSeen;
    std::array<std::tuple<uint16_t, uint16_t>, 16> mMatchedRules;
};

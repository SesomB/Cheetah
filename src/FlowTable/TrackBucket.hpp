#pragma once
#include "FiveTuple.hpp"
#include "TrackDescriptor.hpp"

class TrackBucket
{
  public:
    FiveTuple mFiveTuple;
    TrackBucket *mPrev;
    TrackBucket *mNext;
    TrackDescriptor *mTrackDescriptor;
    uint8_t mRSS8LSBs;
};

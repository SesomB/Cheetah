#include "FiveTuple.hpp"
#include "TrackDescriptor.hpp"

class TrackBucket
{
  public:
    FiveTuple mFiveTuple;
    uint8_t mRSS8LSBs;
    TrackBucket *mPrev;
    TrackBucket *mNext;
    TrackDescriptor *mTrackDescriptor;
};

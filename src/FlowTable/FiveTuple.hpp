#pragma once
#include <stdint.h>

class FiveTuple
{
  public:
    uint32_t mSourceAddress;
    uint32_t mDestinationAddress;
    uint16_t mSourcePort;
    uint16_t mDestinationPort;
    uint8_t mProtocol;

    bool operator==(const FiveTuple &other) const
    {
        return mSourceAddress == other.mSourceAddress && mDestinationAddress == other.mDestinationAddress &&
               mSourcePort == other.mSourcePort && mDestinationPort == other.mDestinationPort &&
               mProtocol == other.mProtocol;
    }

    FiveTuple operator!() const
    {
        return FiveTuple{.mSourceAddress = mDestinationAddress,
                         .mDestinationAddress = mSourceAddress,
                         .mSourcePort = mDestinationPort,
                         .mDestinationPort = mSourcePort,
                         .mProtocol = mProtocol};
    }
};

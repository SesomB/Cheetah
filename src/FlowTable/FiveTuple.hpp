#include <stdint.h>

class FiveTuple
{
  public:
    uint32_t mSourceAddress;
    uint32_t mDestinationAddress;
    uint16_t mSourcePort;
    uint16_t mDestinationPort;
    uint8_t mProtocol;
};

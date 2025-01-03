#include <array>
#include <stdint.h>
#include <tuple>

class TrackDescriptor
{
  public:
    uint32_t mLastSeen;
    std::array<std::tuple<uint16_t, uint16_t>, 16> mMatchedRules;
};

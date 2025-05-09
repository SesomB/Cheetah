#pragma once

#include "AgingHashMap.hpp"
#include "FlowTable/FiveTuple.hpp"

class NewTrackDescriptor
{
  public:
    NewTrackDescriptor() = default;
    ~NewTrackDescriptor() = default;
};

class NewFlowTable
{
  private:
    static constexpr int MAX_TRACKED_SESSIONS = (1 << 22) - 1;
    AgingHashMap<FiveTuple, NewTrackDescriptor, MAX_TRACKED_SESSIONS> m_Hashmap;

  public:
    NewFlowTable();
    ~NewFlowTable() = default;
};
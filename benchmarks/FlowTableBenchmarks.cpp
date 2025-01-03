#include "FlowTable/FlowTable.hpp"
#include "FlowTable/TrackDescriptor.hpp"
#include <benchmark/benchmark.h>
#include <unistd.h>

// Example benchmark for the add function
void BM_CheetahAddition(benchmark::State &state)
{

    FlowTable *mFlowTable = new FlowTable();

    uint32_t hash1 = 84812345;
    FiveTuple fiveTuple{.mSourceAddress = 0xc0a80000,
                        .mDestinationAddress = 0x08080808,
                        .mSourcePort = 12345,
                        .mDestinationPort = 80,
                        .mProtocol = 0};

    int n = state.range(0);

    mFlowTable->insert(hash1, fiveTuple, 10);
    fiveTuple.mProtocol++;
    mFlowTable->insert(hash1, fiveTuple, 10);
    fiveTuple.mProtocol += 2;

    TrackDescriptor *lookup_result = nullptr;
    bool result = false;

    for (auto _ : state)
    {
        result = mFlowTable->insert(hash1, fiveTuple, 10);

        // Pause timing
        // state.PauseTiming();
        result = mFlowTable->delete_entry(hash1, mFlowTable->lookup(hash1, fiveTuple));
        // state.ResumeTiming();

        benchmark::DoNotOptimize(result);
    }

    lookup_result = mFlowTable->lookup(hash1, fiveTuple);
    delete mFlowTable;
}

// Register the benchmark
BENCHMARK(BM_CheetahAddition)->RangeMultiplier(4)->Range(1 << 10, 1 << 20);

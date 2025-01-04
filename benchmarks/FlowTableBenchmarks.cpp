#include "FlowTable/FlowTable.hpp"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <iostream>
#include <unistd.h>

static void BM_FlowTableInsertion(benchmark::State &state)
{
    // 1. Generate random data once, outside the measurement loop.
    std::vector<uint32_t> hashes(state.range(0));
    for (int i = 0; i < state.range(0); ++i)
    {
        hashes[i] = static_cast<uint32_t>(rand());
    }

    FiveTuple fiveTuple{0xc0a80000, 0x08080808, 12345, 80, 16};

    // 2. Construct the FlowTable once (expensive operation).
    FlowTable *flowTable = new FlowTable();

    // std::cout << "Inserting " << hashes.size() << " flows into the table" << std::endl;
    // 3. Benchmark loop
    for (auto _ : state)
    {
        // 4. Insert flows into the pre-constructed table
        for (auto hash : hashes)
        {
            bool result = flowTable->insert(hash, fiveTuple, 10);

            state.PauseTiming();
            if (!result)
                std::cout << "Failed to insert flow with hash: " << hash << std::endl;
            flowTable->delete_entry(hash, flowTable->lookup(hash, fiveTuple));
            state.ResumeTiming();
            benchmark::DoNotOptimize(result);
        }
    }
    delete flowTable;
}

static void BM_FlowTableTraverseList(benchmark::State &state)
{
    const uint32_t hash = 0x12345678;

    // 1. Generate random five tuples once, outside the measurement loop.
    int range = state.range(0);
    std::vector<FiveTuple> fiveTuplesVec(range);
    for (int i = 0; i < range; ++i)
    {
        FiveTuple tmp_fiveTuple{.mSourceAddress = static_cast<uint32_t>(rand()),
                                .mDestinationAddress = static_cast<uint32_t>(rand()),
                                .mSourcePort = static_cast<uint16_t>(rand() % 65535),
                                .mDestinationPort = static_cast<uint16_t>(rand() % 65535),
                                .mProtocol = static_cast<uint8_t>(rand() % 256)};

        fiveTuplesVec[i] = tmp_fiveTuple;
    }

    // 2. Construct the FlowTable once (expensive operation).
    FlowTable *flowTable = new FlowTable();

    // 4. Insert flows into the pre-constructed table
    for (auto &fiveTuple : fiveTuplesVec)
    {
        bool result = flowTable->insert(hash, fiveTuple, 10);
        if (!result)
            std::cout << "Failed to insert flow with hash: " << hash << std::endl;
    }

    // std::cout << "Inserting " << hashes.size() << " flows into the table" << std::endl;
    // 3. Benchmark loop

    auto fiveTupleBench = fiveTuplesVec.back();

    for (auto _ : state)
    {
        // // 4. Insert flows into the pre-constructed table
        // for (auto &fiveTuple : fiveTuplesVec)
        // {
        bool result = flowTable->lookup(hash, fiveTupleBench);
        state.PauseTiming();
        if (!result)
            std::cout << "Failed to lookup flow with hash: " << hash << std::endl;
        state.ResumeTiming();

        benchmark::DoNotOptimize(result);
        // }
    }
    delete flowTable;
}

// Example registration
// BENCHMARK(BM_FlowTableInsertion)->RangeMultiplier(2)->Range(1, 64);
BENCHMARK(BM_FlowTableTraverseList)->RangeMultiplier(2)->Range(1, 256);
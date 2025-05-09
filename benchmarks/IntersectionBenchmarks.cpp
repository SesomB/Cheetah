#include "FlowTable/FlowTable.hpp"
#include <benchmark/benchmark.h>
#include <cstdint>
#include <iostream>
#include <unistd.h>

template <size_t BIT_COUNT>
class MyBitset
{
  public:
    static constexpr uint32_t BLOCKS_COUNT = BIT_COUNT / 64;

  private:
    std::array<uint64_t, BLOCKS_COUNT> m_Data;

  public:
    MyBitset()
    {
        clear();
    }
    ~MyBitset() = default;

    inline void clear(const bool value = false)
    {
        const uint64_t block_data = (value) ? ~0ULL : 0ULL;
        m_Data.fill(block_data);
    }

    inline constexpr uint64_t &getBlock(const uint32_t index)
    {
        return m_Data[index];
    }
};

using RulesBitset = MyBitset<65536>;

static void IM_Test(benchmark::State &state)
{
    const auto intersectionTable = new std::array<RulesBitset *, 10>();
    intersectionTable->fill(nullptr);

    for (uint32_t i = 0; i < intersectionTable->size(); ++i)
    {
        intersectionTable->at(i) = new RulesBitset();
    }

    for (auto _ : state)
    {
        // for (uint32_t i = 0; i < RulesBitset::BLOCKS_COUNT; i++)
        // {
        //     uint64_t result = ~0ULL;
        //     for (const auto &bitset : *intersectionTable)
        //     {
        //         result &= bitset->getBlock(i);
        //     }
        //     benchmark::DoNotOptimize(result);
        // }
        RulesBitset bitsetBuffer;
        for (const auto &bitset : *intersectionTable)
        {
            for (uint32_t i = 0; i < RulesBitset::BLOCKS_COUNT; ++i)
            {
                bitsetBuffer.getBlock(i) &= bitset->getBlock(i);
            }
            // result &= bitset->getBlock(i);
        }

        // for (uint32_t i = 0; i < RulesBitset::BLOCKS_COUNT; i++)
        // {
        //     uint64_t result = ~0ULL;
        //     for (const auto &bitset : *intersectionTable)
        //     {
        //         result &= bitset->getBlock(i);
        //     }
        //     benchmark::DoNotOptimize(result);
        // }
        benchmark::DoNotOptimize(bitsetBuffer);
    }

    for (uint32_t i = 0; i < intersectionTable->size(); ++i)
    {
        auto &intersectionBitset = intersectionTable->at(i);
        if (intersectionBitset)
        {
            delete intersectionBitset;
            intersectionBitset = nullptr;
        }
    }
    delete intersectionTable;
}

// Example registration
// BENCHMARK(BM_FlowTableInsertion)->RangeMultiplier(2)->Range(1, 64);
BENCHMARK(IM_Test)->RangeMultiplier(2)->Range(1, 256);
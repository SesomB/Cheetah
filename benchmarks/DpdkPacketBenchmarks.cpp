#include "Common/Bitmap/Bitmap.hpp"
#include <benchmark/benchmark.h>
#include <unistd.h>

// Example benchmark for the add function
void BM_CheetahAddition(benchmark::State &state)
{
    int n = state.range(0);

    for (auto _ : state)
    {
        Bitmap<1 << 16, 20> aa;
        auto value = aa.OperateAND();
    }
}

// Register the benchmark
BENCHMARK(BM_CheetahAddition)->RangeMultiplier(4)->Range(1 << 10, 1 << 20);

#include <benchmark/benchmark.h>
#include <iostream>
#include <rte_eal.h>

int main(int argc, char **argv)
{
    // const char *args[] = {"-l", "0", NULL};
    // int ret = rte_eal_init(2, (char **)args);
    // if (ret < 0)
    // {
    //     std::cerr << "Failed to initialize EAL" << std::endl;
    //     return 1;
    // }

    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv))
        return 1;

    auto all_benchmarks_return_code = benchmark::RunSpecifiedBenchmarks();

    // rte_eal_cleanup();
    return 0;
}
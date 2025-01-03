#include <cassert>
#include <rte_eal.h>
#include <spdlog/spdlog.h>

int main(int argc, char *argv[])
{
    rte_eal_init(argc, argv);
    spdlog::info("Hello, World!");
    return rte_eal_cleanup();
}

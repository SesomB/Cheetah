#include "MyAgingHashMap.hpp"
#include "rte_hash.h"
#include "rte_jhash.h"
#include "rte_mempool.h"
#include <cassert>
#include <rte_eal.h>
#include <spdlog/spdlog.h>

class CustomData
{
  private:
    std::string m_Data;

  public:
    CustomData(const std::string &data)
        : m_Data(data)
    {
        std::cout << "Custom Data Called Constructor" << std::endl;
    };
    ~CustomData()
    {
        std::cout << "Custom Data Called Destructor" << m_Data << std::endl;
    };

    CustomData(CustomData &&other) = delete;

    CustomData &operator=(const CustomData &other) = delete;

    CustomData &operator=(CustomData &&other) noexcept = delete;
};

int main(int argc, char *argv[])
{
    rte_eal_init(argc, argv);

    MyAgingHashMap<uint32_t, CustomData> a("aaaaa");

    a.try_emplace(10, CustomData("hello_world"));
    rte_delay_ms(1000);
    a.try_emplace(12, CustomData("hello_world22"));
    while (true)
    {
        a.manageTimers();
    }
    return rte_eal_cleanup();
}

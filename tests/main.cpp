#include <gtest/gtest.h>
#include <iostream>
#include <rte_eal.h>

int main(int argc, char **argv)
{
    const char *args[] = {"-l", "0", NULL};
    int ret = rte_eal_init(2, (char **)args);
    if (ret < 0)
    {
        std::cerr << "Failed to initialize EAL" << std::endl;
        return 1;
    }

    testing::InitGoogleTest(&argc, argv);
    auto all_tests_return_code = RUN_ALL_TESTS();

    rte_eal_cleanup();

    return all_tests_return_code;
}

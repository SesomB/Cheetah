#include "PumaSDK/DpdkPacket.hpp"
#include <iostream>

using namespace PumaSDK;

int main(int argc, char const *argv[])
{
    rte_mbuf *mbuf = nullptr;
    DpdkPacket myPacket(mbuf);

    std::cout << "Hello World !" << std::endl;
    return 0;
}

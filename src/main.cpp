#include "AgingHashMap.hpp"
#include <cassert>
#include <chrono>
#include <iostream>
#include <rte_eal.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <thread>

class Person
{
  private:
    std::string name;
    int age;

  public:
    // Constructor
    Person() = default;
    Person(const std::string &name, int age)
        : name(name)
        , age(age)
    {
        std::cout << "[+] Constructor called" << std::endl;
    }

    // Destructor
    ~Person()
    {
        std::cout << "[-] Destructor called" << std::endl;
    }

    // Copy Constructor
    Person(const Person &other)
        : name(other.name)
        , age(other.age)
    {
        std::cout << "[<->] Copy constructor called" << std::endl;
    }

    // Copy Assignment Operator
    Person &operator=(const Person &other)
    {
        std::cout << "[<=>] Copy assignment operator called" << std::endl;
        if (this != &other)
        {
            name = other.name;
            age = other.age;
        }
        return *this;
    }

    // Move Constructor
    Person(Person &&other) noexcept
        : name(std::move(other.name))
        , age(other.age)
    {
        std::cout << "[<<-] Move constructor called" << std::endl;
    }

    // Move Assignment Operator
    Person &operator=(Person &&other) noexcept
    {
        std::cout << "[<<==] Move assignment operator called" << std::endl;
        if (this != &other)
        {
            name = std::move(other.name);
            age = other.age;
        }
        return *this;
    }

    // Utility function
    void print() const
    {
        std::cout << "Name: " << name << ", Age: " << age << std::endl;
    }
};

bool expire_cb(const uint32_t &key, Person &value)
{
    std::cout << "Expiring ";
    value.print();
    std::cout << std::endl;
    return true;
}

using PersonCallbackFunction = bool (*)(const uint32_t &, Person &);

int main(int argc, char *argv[])
{
    rte_eal_init(argc, argv);
    AgingHashMap<uint32_t, Person, 65535, PersonCallbackFunction> hashmap("with_cb", expire_cb);

    // Person alice("Alice", 30);
    hashmap.try_emplace(4, "alice", 30);
    // AgingHashMap<uint32_t, Person> hashmap2("with_cb");

    // auto hashmap = new AgingHashMap<uint32_t, Person>("person_ahm");
    // for (int i = 0; i < 1; ++i)
    // {
    //     std::ostringstream oss;
    //     oss << "person_" << std::to_string(i);
    //     const std::string person_name = oss.str();
    //     // Person p{person_name, i};
    //     hashmap.try_emplace(i, Person{person_name, i});
    //     // hashmap.try_emplace(i, p);
    // }

    std::cout << "Loop Begin..." << std::endl;
    // hashmap.clear();

    while (true)
    {
        hashmap.manageTimers();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (Person *z = hashmap.lookup<true>(3))
        {
            z->print();
        }
        std::cout << "Elements: " << hashmap.size() << std::endl;
    }
    return rte_eal_cleanup();
}

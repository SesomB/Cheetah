#include "Common/MultiBuffer/MultiBuffer.hpp"
#include "spdlog/spdlog.h"
#include <chrono>
#include <gtest/gtest.h>
#include <sstream>
#include <string>
#include <thread>
class Person
{
  private:
    std::string m_Name;
    unsigned int m_Age;

  public:
    Person() = default;

    Person(const std::string &name, const unsigned int age)
        : m_Name(name)
        , m_Age(age){};

    ~Person() = default;

    inline const std::string &getName() const
    {
        return m_Name;
    }

    inline void setName(const std::string &name)
    {
        m_Name = name;
    }

    inline unsigned int getAge() const
    {
        return m_Age;
    }

    inline void setAge(unsigned int age)
    {
        m_Age = age;
    }
};

TEST(MultiBufferTests, HelloFunction)
{
    bool threadRunning = true;

    MultiBuffer<Person, 3> PersonBuffers{};

    // Setup First Buffer
    Person &buffer = PersonBuffers.write();
    buffer.setName("Moshe");
    buffer.setAge(27);
    PersonBuffers.publish();

    std::thread printerThread([&]() {
        while (threadRunning)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            const auto personPtr = PersonBuffers.read();
            if (!personPtr)
            {
                // spdlog::info("{} | Invalid Current Person Buffer", fmt::ptr(personPtr));
                continue;
            }
            const auto &person = *personPtr;
            // spdlog::info("Current Person Buffer: Name: {} | Age: {}", person.getName(), person.getAge());
        }
    });

    std::thread debugThread([&]() {
        while (threadRunning)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            const auto &buffers = PersonBuffers.getBuffersForDebug();
            const auto activeIndex = PersonBuffers.activeIndex();
            std::ostringstream oss;
            oss << fmt::format("Active: {} | ", fmt::ptr(&(buffers[activeIndex])));
            for (auto &buffer : buffers)
            {
                oss << fmt::format("Buffer ({}).valid = {} | ", fmt::ptr(&buffer), buffer.valid);
            }
            spdlog::info(oss.str());
        }
    });

    while (true)
    {

        std::this_thread::sleep_for(std::chrono::milliseconds(1500));

        buffer = PersonBuffers.write();
        buffer.setName("Asaf");
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        buffer.setAge(23);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        PersonBuffers.publish();

        std::this_thread::sleep_for(std::chrono::milliseconds(1500));
    }

    threadRunning = false;
    printerThread.join();
    debugThread.join();
}

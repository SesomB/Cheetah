#include "FlowTable/FlowTable.hpp"
#include <gtest/gtest.h>

// The fixture for testing class Foo.
class FlowTableTests : public testing::Test
{
  protected:
    // You can remove any or all of the following functions if their bodies would
    // be empty.

    FlowTableTests()
    {
        m_FlowTable = new FlowTable();
    }

    ~FlowTableTests() override
    {
        if (m_FlowTable)
            delete m_FlowTable;
    }

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:

    void SetUp() override
    {
        // Code here will be called immediately after the constructor (right
        // before each test).
    }

    void TearDown() override
    {
        // Code here will be called immediately after each test (right
        // before the destructor).
    }

    FlowTable *m_FlowTable = nullptr;
};

// Tests that the Foo::Bar() method does Abc.
TEST_F(FlowTableTests, LookupsAndInserts)
{
    ASSERT_NE(m_FlowTable, nullptr);

    uint32_t hash1 = 84812345;
    FiveTuple fiveTuple1{.mSourceAddress = 0xc0a80000,
                         .mDestinationAddress = 0x08080808,
                         .mSourcePort = 12345,
                         .mDestinationPort = 80,
                         .mProtocol = 6};

    ASSERT_EQ(fiveTuple1, fiveTuple1);

    FiveTuple fiveTuple2{.mSourceAddress = 0xc0a80000,
                         .mDestinationAddress = 0x08180808,
                         .mSourcePort = 12345,
                         .mDestinationPort = 80,
                         .mProtocol = 6};
    ASSERT_EQ(fiveTuple2, fiveTuple2);

    ASSERT_FALSE(m_FlowTable->lookup(hash1, fiveTuple1));
    for (int i = 0; i < 100; ++i)
    {
        ASSERT_TRUE(m_FlowTable->insert(hash1, fiveTuple1, 100));
    }

    ASSERT_TRUE(m_FlowTable->lookup(hash1, fiveTuple1));
    ASSERT_EQ(std::get<0>(m_FlowTable->lookup(hash1, fiveTuple1)->mMatchedRules[0]), 100);
}

TEST_F(FlowTableTests, Deletions)
{
    ASSERT_NE(m_FlowTable, nullptr);

    uint32_t hash1 = 84812345;
    FiveTuple fiveTuple{.mSourceAddress = 0xc0a80000,
                        .mDestinationAddress = 0x08080808,
                        .mSourcePort = 12345,
                        .mDestinationPort = 80,
                        .mProtocol = 0};

    FiveTuple fiveTupleRev = !fiveTuple;

    ASSERT_EQ(fiveTuple, fiveTuple);
    ASSERT_FALSE(m_FlowTable->lookup(hash1, fiveTuple));

    // Populating the list
    for (int i = 0; i < 10; ++i)
    {
        ASSERT_TRUE(m_FlowTable->insert(hash1, fiveTuple, 100));
        fiveTuple.mProtocol++;
    }

    TrackDescriptor *lookup_result = nullptr;

    // Deleting the first in the list
    fiveTuple.mProtocol = 0;
    lookup_result = m_FlowTable->lookup(hash1, fiveTuple);
    ASSERT_TRUE(lookup_result);
    ASSERT_TRUE(m_FlowTable->delete_entry(hash1, lookup_result));
    lookup_result = m_FlowTable->lookup(hash1, fiveTuple);
    ASSERT_FALSE(lookup_result);

    // Deleting the last in the list
    fiveTuple.mProtocol = 9;
    lookup_result = m_FlowTable->lookup(hash1, fiveTuple);
    ASSERT_TRUE(lookup_result);
    ASSERT_TRUE(m_FlowTable->delete_entry(hash1, lookup_result));
    lookup_result = m_FlowTable->lookup(hash1, fiveTuple);
    ASSERT_FALSE(lookup_result);

    // Deleting the only one in the list
    fiveTuple.mProtocol = 3;
    lookup_result = m_FlowTable->lookup(hash1, fiveTuple);
    ASSERT_TRUE(lookup_result);
    ASSERT_TRUE(m_FlowTable->delete_entry(hash1, lookup_result));
    lookup_result = m_FlowTable->lookup(hash1, fiveTuple);
    ASSERT_FALSE(lookup_result);

    // Deleting the only one in the list (REVERSED)
    fiveTupleRev.mProtocol = 6;
    lookup_result = m_FlowTable->lookup(hash1, fiveTupleRev);
    ASSERT_TRUE(lookup_result);
    ASSERT_TRUE(m_FlowTable->delete_entry(hash1, lookup_result));
    lookup_result = m_FlowTable->lookup(hash1, fiveTupleRev);
    ASSERT_FALSE(lookup_result);

    // Deleting a non-existent entry
    fiveTuple.mProtocol = 10;
    lookup_result = m_FlowTable->lookup(hash1, fiveTuple);
    ASSERT_FALSE(lookup_result);
    ASSERT_FALSE(m_FlowTable->delete_entry(hash1, lookup_result));
    lookup_result = m_FlowTable->lookup(hash1, fiveTuple);
    ASSERT_FALSE(lookup_result);
}
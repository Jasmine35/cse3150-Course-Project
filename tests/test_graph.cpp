
#include <gtest/gtest.h>
#include <fstream>
#include "graph/GraphParser.h"
#include "graph/Graph.h"

TEST(GraphTest, ParserReadsRelationships) {
    // write a tiny fake CAIDA file
    std::ofstream f("test_input.txt");
    f << "# this is a comment\n";
    f << "1|2|-1\n";   // AS1 is provider of AS2
    f << "3|4|0\n";    // AS3 and AS4 are peers
    f << "bad line\n"; // malformed, should be skipped
    f.close();

    auto relationships = GraphParser::parse("test_input.txt");

    EXPECT_EQ(relationships.size(), 2);

    EXPECT_EQ(relationships[0].asn1_, 1);
    EXPECT_EQ(relationships[0].asn2_, 2);
    EXPECT_EQ(relationships[0].type_, -1);

    EXPECT_EQ(relationships[1].asn1_, 3);
    EXPECT_EQ(relationships[1].asn2_, 4);
    EXPECT_EQ(relationships[1].type_, 0);
}


TEST(GraphTest, BuildsProviderCustomerRelationship) {
    std::ofstream f("test_pc.txt");
    f << "1|2|-1\n"; // AS1 provider of AS2
    f.close();

    Graph g("test_pc.txt");
    EXPECT_EQ(g.size(), 2);

    AS* as1 = g.getAS(1);
    AS* as2 = g.getAS(2);

    ASSERT_NE(as1, nullptr);
    ASSERT_NE(as2, nullptr);

    EXPECT_TRUE(as1->customers_.count(2));
    EXPECT_TRUE(as2->providers_.count(1));
}

TEST(GraphTest, BuildsPeerRelationship) {
    std::ofstream f("test_peer.txt");
    f << "3|4|0\n"; // AS3 and AS4 are peers
    f.close();

    Graph g("test_peer.txt");

    AS* as3 = g.getAS(3);
    AS* as4 = g.getAS(4);

    EXPECT_TRUE(as3->peers_.count(4));
    EXPECT_TRUE(as4->peers_.count(3));
}

TEST(GraphTest, CycleDetectionThrows) {
    std::ofstream f("test_cycle.txt");
    f << "1|2|-1\n"; // AS1 provider of AS2
    f << "2|1|-1\n"; // AS2 provider of AS1 — cycle!
    f.close();

    EXPECT_THROW(Graph g("test_cycle.txt"), std::runtime_error);
}

TEST(GraphTest, PeerCycleIsAccepted) {
    std::ofstream f("test_peer_cycle.txt");
    f << "5|6|0\n"; // peers — this bidirectional relationship is fine
    f.close();

    EXPECT_NO_THROW(Graph g("test_peer_cycle.txt"));
}

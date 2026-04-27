#include <gtest/gtest.h>
#include <fstream>
#include "bgp/BGP.h"
#include "bgp/Announcement.h"
#include "graph/Graph.h"
#include "bgp/ROV.h"

TEST(PropagationTest, ReceiveAndStoreAnnouncement) {
    BGP bgp;
    Announcement ann("1.2.0.0/16", {1}, 1, Relationship::ORIGIN);

    bgp.receiveAnnouncement(ann);
    bgp.processQueue();

    EXPECT_TRUE(bgp.hasPrefix("1.2.0.0/16"));
    const Announcement* stored = bgp.getAnnouncement("1.2.0.0/16");
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->nextHop_, 1);
}

TEST(PropagationTest, QueueClearedAfterProcessing) {
    BGP bgp;
    Announcement ann("2.3.0.0/16", {2}, 2, Relationship::CUSTOMER);

    bgp.receiveAnnouncement(ann);
    bgp.processQueue();
    // processing again should not change anything
    bgp.processQueue();

    EXPECT_TRUE(bgp.hasPrefix("2.3.0.0/16"));
}

TEST(PropagationTest, FirstAnnouncementAlwaysStored) {
    BGP bgp;
    Announcement ann("3.4.0.0/16", {3}, 3, Relationship::PEER);

    bgp.receiveAnnouncement(ann);
    bgp.processQueue();

    EXPECT_TRUE(bgp.hasPrefix("3.4.0.0/16"));
}

//testing graph flattening and propogation
// helper to build a small graph file
void writeGraphFile(const std::string& filename,
                    const std::string& content) {
    std::ofstream f(filename);
    f << content;
    f.close();
}

TEST(PropagationTest, AnnouncementReachesCustomer) {
    // AS1 is provider of AS2
    writeGraphFile("test_prop.txt", "1|2|-1\n");

    Graph g("test_prop.txt");

    Announcement ann("1.2.0.0/16", {1}, 1, Relationship::ORIGIN);
    g.seedAnnouncement(1, ann);
    g.propagate();

    // AS2 should have received the announcement
    AS* as2 = g.getAS(2);
    ASSERT_NE(as2, nullptr);
    EXPECT_TRUE(as2->policy_->hasPrefix("1.2.0.0/16"));
}

TEST(PropagationTest, AnnouncementReachesProvider) {
    // AS1 is provider of AS2 - seed at AS2, should travel up to AS1
    writeGraphFile("test_prop2.txt", "1|2|-1\n");

    Graph g("test_prop2.txt");

    Announcement ann("2.3.0.0/16", {2}, 2, Relationship::ORIGIN);
    g.seedAnnouncement(2, ann);
    g.propagate();

    AS* as1 = g.getAS(1);
    ASSERT_NE(as1, nullptr);
    EXPECT_TRUE(as1->policy_->hasPrefix("2.3.0.0/16"));
}

TEST(PropagationTest, AnnouncementReachesPeer) {
    // AS1 and AS2 are peers
    writeGraphFile("test_prop3.txt", "1|2|0\n");

    Graph g("test_prop3.txt");

    Announcement ann("3.4.0.0/16", {1}, 1, Relationship::ORIGIN);
    g.seedAnnouncement(1, ann);
    g.propagate();

    AS* as2 = g.getAS(2);
    ASSERT_NE(as2, nullptr);
    EXPECT_TRUE(as2->policy_->hasPrefix("3.4.0.0/16"));
}

TEST(PropagationTest, CustomerBeatsProvider) {
    // AS1 -- provider --> AS3
    // AS2 -- provider --> AS3
    // AS3 peers with both AS1 and AS2
    // seed same prefix at AS1 and AS2
    // AS3 should prefer the customer relationship
    writeGraphFile("test_tiebreak1.txt",
        "1|3|-1\n"  // AS1 is provider of AS3
        "2|3|-1\n"  // AS2 is provider of AS3
    );

    Graph g("test_tiebreak1.txt");

    // seed at AS3 itself (origin) and at AS1 (so AS3 receives from provider)
    Announcement fromOrigin("10.0.0.0/8", {3}, 3, Relationship::ORIGIN);
    g.seedAnnouncement(3, fromOrigin);
    g.propagate();

    // AS1 and AS2 should have received from customer (AS3)
    AS* as1 = g.getAS(1);
    AS* as2 = g.getAS(2);
    ASSERT_NE(as1, nullptr);
    ASSERT_NE(as2, nullptr);

    const Announcement* ann1 = as1->policy_->getAnnouncement("10.0.0.0/8");
    const Announcement* ann2 = as2->policy_->getAnnouncement("10.0.0.0/8");
    ASSERT_NE(ann1, nullptr);
    ASSERT_NE(ann2, nullptr);

    // both providers received from customer - correct relationship
    EXPECT_EQ(ann1->recvFrom_, Relationship::CUSTOMER);
    EXPECT_EQ(ann2->recvFrom_, Relationship::CUSTOMER);
}

TEST(PropagationTest, ShorterPathWins) {
    BGP bgp(99); // owner ASN 99

    // longer path from peer - after prepending owner(99) will be [99, 10, 20, 1] = size 4
    Announcement longer("5.0.0.0/8", {10, 20, 1}, 10, Relationship::PEER);
    // shorter path from peer - after prepending owner(99) will be [99, 20, 1] = size 3
    Announcement shorter("5.0.0.0/8", {20, 1}, 20, Relationship::PEER);

    bgp.receiveAnnouncement(longer);
    bgp.processQueue();

    bgp.receiveAnnouncement(shorter);
    bgp.processQueue();

    const Announcement* stored = bgp.getAnnouncement("5.0.0.0/8");
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->asPath_.size(), 3); // shorter path won, size 3 after prepend
    EXPECT_EQ(stored->nextHop_, 20);      // confirm it's the shorter announcement

}

TEST(PropagationTest, LowerNextHopWinsOnTie) {
    BGP bgp;

    // same relationship, same path length, different next hops
    Announcement higher("6.0.0.0/8", {100, 1}, 100, Relationship::PEER);
    Announcement lower("6.0.0.0/8", {50, 1}, 50, Relationship::PEER);

    bgp.receiveAnnouncement(higher);
    bgp.processQueue();

    bgp.receiveAnnouncement(lower);
    bgp.processQueue();

    const Announcement* stored = bgp.getAnnouncement("6.0.0.0/8");
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->nextHop_, 50); // lower next hop won
}

TEST(PropagationTest, OriginAlwaysWins) {
    BGP bgp;

    // receive from customer first
    Announcement fromCustomer("7.0.0.0/8", {5, 1}, 5, Relationship::CUSTOMER);
    bgp.receiveAnnouncement(fromCustomer);
    bgp.processQueue();

    // then receive origin - should replace customer
    Announcement origin("7.0.0.0/8", {1}, 1, Relationship::ORIGIN);
    bgp.receiveAnnouncement(origin);
    bgp.processQueue();

    const Announcement* stored = bgp.getAnnouncement("7.0.0.0/8");
    ASSERT_NE(stored, nullptr);
    EXPECT_EQ(stored->recvFrom_, Relationship::ORIGIN);
}

TEST(PropagationTest, ROVDropsInvalidAnnouncement) {
    ROV rov(10);

    // invalid announcement should be dropped
    Announcement invalid("8.0.0.0/8", {666}, 666, Relationship::CUSTOMER, true);
    rov.receiveAnnouncement(invalid);
    rov.processQueue();

    EXPECT_FALSE(rov.hasPrefix("8.0.0.0/8"));
}

TEST(PropagationTest, ROVAcceptsValidAnnouncement) {
    ROV rov(10);

    // valid announcement should be accepted normally
    Announcement valid("9.0.0.0/8", {777}, 777, Relationship::CUSTOMER, false);
    rov.receiveAnnouncement(valid);
    rov.processQueue();

    EXPECT_TRUE(rov.hasPrefix("9.0.0.0/8"));
}

TEST(PropagationTest, ROVBlocksHijackFromReaching) {
    // AS4 deploys ROV - should block AS666's hijack
    writeGraphFile("test_rov.txt",
        "4|666|-1\n"
        "4|3|-1\n"
        "777|4|0\n"
    );

    Graph g("test_rov.txt");
    g.loadROVASNs("../data/test_rov_asns.txt");

    Announcement legitimate("1.2.0.0/16", {777}, 777, Relationship::ORIGIN);
    g.seedAnnouncement(777, legitimate);

    Announcement hijack("1.2.0.0/16", {666}, 666, Relationship::ORIGIN, true);
    g.seedAnnouncement(666, hijack);

    g.propagate();

    // AS4 deploys ROV so should only have 777's announcement
    AS* as4 = g.getAS(4);
    ASSERT_NE(as4, nullptr);
    const Announcement* ann = as4->policy_->getAnnouncement("1.2.0.0/16");
    ASSERT_NE(ann, nullptr);
    EXPECT_EQ(ann->nextHop_, 777); // legitimate wins because hijack was dropped
}

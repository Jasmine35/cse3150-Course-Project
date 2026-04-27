/* This is the entry point
 * parses the command-line argyements (paths to CAIDA file, announcements CSV, ROV ASNs file, output path)
 * orchestrates the full pipeline
 * build graph -> load ROV ASNs -> seed announcements -> propagate -> dump CSV
 * handles cycle-detection error exit code
 */

#include <iostream>
#include <fstream>
#include <string>
#include "graph/Graph.h"
#include "bgp/Announcement.h"

int main(int argc, char* argv[]) {
    /*// check the file opens at all
    std::ifstream test("/Users/jasmine/Downloads/20161001.as-rel2.txt");
    if (!test.is_open()) {
        std::cout << "Could not open file" << std::endl;
        return 1;
    }

    // print the first 5 lines so we can see the format
    // print the first 5 NON-comment lines so we can see the real format
    std::string line;
    int count = 0;
    while (std::getline(test, line) && count < 5) {
        if (!line.empty() && line[0] != '#') {
            std::cout << "Data line: [" << line << "]" << std::endl;
            count++;
        }
    }
    test.close();

    // now try loading the graph
    Graph g("/Users/jasmine/Downloads/20161001.as-rel2.txt");
    std::cout << "Loaded " << g.size() << " AS nodes" << std::endl;*/

   /* // build the bgpsimulator.com homepage example
    // write it to a temp file
    std::ofstream f("../data/homepage_test.txt");
    f << "4|666|-1\n";   // AS4 provider of AS666
    f << "4|3|-1\n";     // AS4 provider of AS3
    f << "777|4|0\n";    // AS777 peers with AS4
    f.close();

    Graph g("../data/homepage_test.txt");

    // AS777 is legitimate owner
    Announcement legitimate("1.2.0.0/16", {777}, 777, Relationship::ORIGIN);
    g.seedAnnouncement(777, legitimate);

    // AS666 hijacks
    Announcement hijack("1.2.0.0/16", {666}, 666, Relationship::ORIGIN);
    g.seedAnnouncement(666, hijack);

    g.propagate();

    // print what each AS thinks the best path is
    uint32_t asns[] = {3, 4, 666, 777};
    for (uint32_t asn : asns) {
        AS* as = g.getAS(asn);
        if (as == nullptr) continue;
        const Announcement* ann = as->policy_->getAnnouncement("1.2.0.0/16");
        if (ann == nullptr) {
            std::cout << "AS " << asn << ": no announcement" << std::endl;
            continue;
        }
        std::cout << "AS " << asn << ": path=";
        for (size_t i = 0; i < ann->asPath_.size(); i++) {
            if (i > 0) std::cout << " ";
            std::cout << ann->asPath_[i];
        }
        std::cout << " recvFrom=";
        switch (ann->recvFrom_) {
            case Relationship::ORIGIN:   std::cout << "ORIGIN"; break;
            case Relationship::CUSTOMER: std::cout << "CUSTOMER"; break;
            case Relationship::PEER:     std::cout << "PEER"; break;
            case Relationship::PROVIDER: std::cout << "PROVIDER"; break;
        }
        std::cout << std::endl;
    }*/

    // build the bgpsimulator.com homepage example
    /*std::ofstream f("../data/homepage_test.txt");
    f << "4|666|-1\n";
    f << "4|3|-1\n";
    f << "777|4|0\n";
    f.close();

    Graph g("../data/homepage_test.txt");

    // make AS 4 deploy ROV
    g.loadROVASNs("../data/test_rov_asns.txt");

    // AS777 legitimate announcement
    Announcement legitimate("1.2.0.0/16", {777}, 777, Relationship::ORIGIN);
    g.seedAnnouncement(777, legitimate);

    // AS666 hijack - marked as rov_invalid
    Announcement hijack("1.2.0.0/16", {666}, 666, Relationship::ORIGIN, true);
    g.seedAnnouncement(666, hijack);

    g.propagate();

    // print results
    uint32_t asns[] = {3, 4, 666, 777};
    for (uint32_t asn : asns) {
        AS* as = g.getAS(asn);
        if (as == nullptr) continue;
        const Announcement* ann = as->policy_->getAnnouncement("1.2.0.0/16");
        if (ann == nullptr) {
            std::cout << "AS " << asn << ": no announcement" << std::endl;
            continue;
        }
        std::cout << "AS " << asn << ": path=";
        for (size_t i = 0; i < ann->asPath_.size(); i++) {
            if (i > 0) std::cout << " ";
            std::cout << ann->asPath_[i];
        }
        std::cout << " recvFrom=";
        switch (ann->recvFrom_) {
            case Relationship::ORIGIN:   std::cout << "ORIGIN"; break;
            case Relationship::CUSTOMER: std::cout << "CUSTOMER"; break;
            case Relationship::PEER:     std::cout << "PEER"; break;
            case Relationship::PROVIDER: std::cout << "PROVIDER"; break;
        }
        std::cout << std::endl;
    }*/


    //testing the simulator on the real/given datasets
    if (argc < 5) {
        std::cerr << "Usage: bgp_simulator <caida_file> <anns_csv> <rov_asns_file> <output_csv>" << std::endl;
        return 1;
    }

    std::string caidaFile  = argv[1];
    std::string annsFile   = argv[2];
    std::string rovFile    = argv[3];
    std::string outputFile = argv[4];

    std::cout << "Loading graph..." << std::endl;
    Graph g(caidaFile);
    std::cout << "Loaded " << g.size() << " AS nodes" << std::endl;

    AS* as15830 = g.getAS(15830);
    AS* as25 = g.getAS(25);
    AS* as27 = g.getAS(27);

    if (as15830) std::cout << "AS 15830 rank=" << as15830->propagation_rank_ << std::endl;
    if (as25)    std::cout << "AS 25 rank="    << as25->propagation_rank_    << std::endl;
    if (as27)    std::cout << "AS 27 rank="    << as27->propagation_rank_    << std::endl;

    if (as15830) {
        std::cout << "AS 15830 providers: ";
        for (auto p : as15830->providers_) std::cout << p << " ";
        std::cout << std::endl;
        std::cout << "AS 15830 customers: ";
        for (auto c : as15830->customers_) std::cout << c << " ";
        std::cout << std::endl;
        std::cout << "AS 15830 peers: ";
        for (auto p : as15830->peers_) std::cout << p << " ";
        std::cout << std::endl;
    }

    std::cout << "Loading ROV ASNs..." << std::endl;
    g.loadROVASNs(rovFile);

    std::cout << "Seeding announcements..." << std::endl;
    auto seedAnns = GraphParser::parseAnnouncements(annsFile);
    for (const auto& seed : seedAnns) {
        Announcement ann(
            seed.prefix_,
            {seed.seedAsn_},
            seed.seedAsn_,
            Relationship::ORIGIN,
            seed.rovInvalid_
        );
        g.seedAnnouncement(seed.seedAsn_, ann);
        std::cout << "  Seeded AS " << seed.seedAsn_
                  << " prefix " << seed.prefix_
                  << " rov_invalid=" << seed.rovInvalid_ << std::endl;
    }

    std::cout << "Propagating..." << std::endl;
    g.propagate();

    std::cout << "Dumping CSV..." << std::endl;
    g.dumpCSV(outputFile);

    std::cout << "Done." << std::endl;
    return 0;
}



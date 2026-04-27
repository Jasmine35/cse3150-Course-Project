/* This is the entry point
 * parses the command-line argyements (paths to CAIDA file, announcements CSV, ROV ASNs file, output path)
 * orchestrates the full pipeline
 * build graph -> load ROV ASNs -> seed announcements -> propagate -> dump CSV
 * handles cycle-detection error exit code
 * Usage: bgp_simulator <caida_file> <anns_csv> <rov_asns_file> <output_csv>
 */

#include <iostream>
#include <fstream>
#include <string>
#include "graph/Graph.h"
#include "bgp/Announcement.h"

//  Helpers

static void printAnnouncement(Graph& g, uint32_t asn, const std::string& prefix) {
    AS* as = g.getAS(asn);
    if (as == nullptr) return;
    const Announcement* ann = as->policy_->getAnnouncement(prefix);
    if (ann == nullptr) {
        std::cout << "  AS " << asn << ": no announcement" << std::endl;
        return;
    }
    std::cout << "  AS " << asn << ": path=";
    for (size_t i = 0; i < ann->asPath_.size(); i++) {
        if (i > 0) std::cout << " ";
        std::cout << ann->asPath_[i];
    }
    std::cout << " recvFrom=";
    switch (ann->recvFrom_) {
        case Relationship::ORIGIN:   std::cout << "ORIGIN";   break;
        case Relationship::CUSTOMER: std::cout << "CUSTOMER"; break;
        case Relationship::PEER:     std::cout << "PEER";     break;
        case Relationship::PROVIDER: std::cout << "PROVIDER"; break;
    }
    std::cout << std::endl;
}

static void writeHomepageTopology(const std::string& path) {
    std::ofstream f(path);
    f << "4|666|-1\n";   // AS4 is provider of AS666
    f << "4|3|-1\n";     // AS4 is provider of AS3
    f << "777|4|0\n";    // AS777 peers with AS4
    f.close();
}

// File open + format check

static void phase1_fileCheck(const std::string& caidaFile) {
    std::cout << "\n========== Phase 1: File Open + Format Check ==========" << std::endl;

    std::ifstream test(caidaFile);
    if (!test.is_open()) {
        std::cerr << "  [FAIL] Could not open file: " << caidaFile << std::endl;
        return;
    }
    std::cout << "  [OK] Opened: " << caidaFile << std::endl;

    std::string line;
    int count = 0;
    std::cout << "  First 5 non-comment data lines:" << std::endl;
    while (std::getline(test, line) && count < 5) {
        if (!line.empty() && line[0] != '#') {
            std::cout << "    [" << line << "]" << std::endl;
            count++;
        }
    }
    test.close();
    std::cout << "  Phase 1 complete." << std::endl;
}

// Homepage example, no ROV

static void phase2_homepageNoROV() {
    std::cout << "\n========== Phase 2: Homepage Example (no ROV) ==========" << std::endl;

    const std::string topoPath = "../data/homepage_test.txt";
    const std::string prefix   = "1.2.0.0/16";
    writeHomepageTopology(topoPath);

    Graph g(topoPath);

    Announcement legitimate(prefix, {777}, 777, Relationship::ORIGIN);
    g.seedAnnouncement(777, legitimate);

    Announcement hijack(prefix, {666}, 666, Relationship::ORIGIN);
    g.seedAnnouncement(666, hijack);

    g.propagate();

    std::cout << "  Results for prefix " << prefix << ":" << std::endl;
    for (uint32_t asn : {3u, 4u, 666u, 777u})
        printAnnouncement(g, asn, prefix);

    std::cout << "  Phase 2 complete." << std::endl;
}

// Homepage example, with ROV

static void phase3_homepageWithROV() {
    std::cout << "\n========== Phase 3: Homepage Example (with ROV) ==========" << std::endl;

    const std::string topoPath = "../data/homepage_test.txt";
    const std::string rovPath  = "../data/test_rov_asns.txt";
    const std::string prefix   = "1.2.0.0/16";
    writeHomepageTopology(topoPath);

    Graph g(topoPath);
    g.loadROVASNs(rovPath);   // AS4 deploys ROV

    Announcement legitimate(prefix, {777}, 777, Relationship::ORIGIN);
    g.seedAnnouncement(777, legitimate);

    // Marked rov_invalid so ROV-deploying ASes will drop it
    Announcement hijack(prefix, {666}, 666, Relationship::ORIGIN, /*rov_invalid=*/true);
    g.seedAnnouncement(666, hijack);

    g.propagate();

    std::cout << "  Results for prefix " << prefix << ":" << std::endl;
    for (uint32_t asn : {3u, 4u, 666u, 777u})
        printAnnouncement(g, asn, prefix);

    std::cout << "  Phase 3 complete." << std::endl;
}

// Real CAIDA datasets, full pipeline

static void phase4_realDatasets(const std::string& caidaFile,
                                 const std::string& annsFile,
                                 const std::string& rovFile,
                                 const std::string& outputFile) {
    std::cout << "\n========== Phase 4: Real CAIDA Datasets ==========" << std::endl;

    std::cout << "  Loading graph..." << std::endl;
    Graph g(caidaFile);
    std::cout << "  Loaded " << g.size() << " AS nodes." << std::endl;

    // Spot-check a few ASes
    for (uint32_t probe : {15830u, 25u, 27u}) {
        AS* as = g.getAS(probe);
        if (!as) continue;
        std::cout << "  AS " << probe << " rank=" << as->propagation_rank_
                  << "  providers=" << as->providers_.size()
                  << "  customers=" << as->customers_.size()
                  << "  peers="     << as->peers_.size()    << std::endl;
    }

    std::cout << "  Loading ROV ASNs from " << rovFile << "..." << std::endl;
    g.loadROVASNs(rovFile);

    std::cout << "  Seeding announcements from " << annsFile << "..." << std::endl;
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
        std::cout << "    Seeded AS " << seed.seedAsn_
                  << "  prefix=" << seed.prefix_
                  << "  rov_invalid=" << seed.rovInvalid_ << std::endl;
    }

    std::cout << "  Propagating..." << std::endl;
    g.propagate();

    std::cout << "  Dumping CSV to " << outputFile << "..." << std::endl;
    g.dumpCSV(outputFile);

    std::cout << "  Phase 4 complete." << std::endl;
}

int main(int argc, char* argv[]){

    if (argc < 5) {
        std::cerr << "Usage: bgp_simulator <caida_file> <anns_csv> <rov_asns_file> <output_csv>"
                  << std::endl;
        return 1;
    }

    const std::string caidaFile  = argv[1];
    const std::string annsFile   = argv[2];
    const std::string rovFile    = argv[3];
    const std::string outputFile = argv[4];

    phase1_fileCheck(caidaFile);
    phase2_homepageNoROV();
    phase3_homepageWithROV();
    phase4_realDatasets(caidaFile, annsFile, rovFile, outputFile);

    std::cout << "\nAll phases complete." << std::endl;

    return 0;
}

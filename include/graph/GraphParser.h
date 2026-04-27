/* delcares parser that reads raw CAIDA file and hands off the relationship data to Graph
 * keeping parsing logic here so it keeps the file-format details of the graph logic
 *
 * should have one static method (or a simple class with one public method) that
 * 1. takes a filename
 * 2. reads the file line by line
 * 3. returns a list of relationship records (vector of structs, each holding two ASNs and a relationship type)
 * Keep parsing completely separate from graph construction (easier to test)
 */

# pragma once

# include <cstdint>
# include <string>
# include <vector>

// represents a single relationship from the CAIDA file

struct ASRelationship{
    uint32_t asn1_;
    uint32_t asn2_;
    int type_;
};

struct SeedAnnouncement {
    uint32_t seedAsn_;
    std::string prefix_;
    bool rovInvalid_;
};


class GraphParser{
public:
    static std::vector<ASRelationship> parse(const std::string& filename);
    static std::vector<ASRelationship> parseFromString(const std::string& content);

    static std::vector<SeedAnnouncement> parseAnnouncements(const std::string& filename);
    static std::vector<SeedAnnouncement> parseAnnouncementsFromString(const std::string& content);
};

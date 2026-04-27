/* implements actual file reading
 * parses each line of CAIDA file
 * extracts 2 ASNs and relationship type
 * calls to Graph to register each edge
 */


# include "graph/GraphParser.h"
# include <fstream>
# include <sstream>
# include <stdexcept>

static std::vector<ASRelationship> parseTopologyStream(std::istream& in) {
    std::vector<ASRelationship> relationships;
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(ss, token, '|')) tokens.push_back(token);
        if (tokens.size() < 3) continue;
        ASRelationship r;
        r.asn1_ = static_cast<uint32_t>(std::stoul(tokens[0]));
        r.asn2_ = static_cast<uint32_t>(std::stoul(tokens[1]));
        r.type_ = std::stoi(tokens[2]);
        if (r.type_ == -1 || r.type_ == 0) relationships.push_back(r);
    }
    return relationships;
}

std::vector<ASRelationship> GraphParser::parse(const std::string& filename){
    std::ifstream file(filename);
    if (!file.is_open()) throw std::runtime_error("Could not open file: " + filename);
    return parseTopologyStream(file);
}

std::vector<ASRelationship> GraphParser::parseFromString(const std::string& content) {
    std::istringstream ss(content);
    return parseTopologyStream(ss);
}

static std::vector<SeedAnnouncement> parseAnnouncementsStream(std::istream& in) {
    std::vector<SeedAnnouncement> announcements;
    std::string line;
    bool firstLine = true;
    while (std::getline(in, line)) {
        if (firstLine) { firstLine = false; continue; }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(ss, token, ',')) tokens.push_back(token);
        if (tokens.size() < 3) continue;
        SeedAnnouncement ann;
        ann.seedAsn_ = static_cast<uint32_t>(std::stoul(tokens[0]));
        ann.prefix_ = tokens[1];
        ann.rovInvalid_ = (tokens[2] == "True" || tokens[2] == "true");
        announcements.push_back(ann);
    }
    return announcements;
}

std::vector<SeedAnnouncement> GraphParser::parseAnnouncements(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) throw std::runtime_error("Could not open announcements file: " + filename);
    return parseAnnouncementsStream(file);
}

std::vector<SeedAnnouncement> GraphParser::parseAnnouncementsFromString(const std::string& content) {
    std::istringstream ss(content);
    return parseAnnouncementsStream(ss);
}

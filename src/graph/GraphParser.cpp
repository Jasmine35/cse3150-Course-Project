/* implements actual file reading
 * parses each line of CAIDA file
 * extracts 2 ASNs and relationship type
 * calls to Graph to register each edge
 */


# include "graph/GraphParser.h"
# include <fstream>
# include <sstream>
# include <stdexcept>

std::vector<ASRelationship> GraphParser::parse(const std::string& filename){
    std::vector<ASRelationship> relationships;

    std::ifstream file(filename);

    if(!file.is_open()){
        throw std::runtime_error("Could not open file: " + filename);
    }

    std::string line;
    while(std::getline(file, line)){
        //skip all empty lines and commented lines
        if(line.empty() || line[0] == '#'){
            continue;
        }

        std::stringstream ss(line);

        std::string token;

        std::vector<std::string> tokens;

        while(std::getline(ss, token, '|')){
            tokens.push_back(token);
        }

        if(tokens.size() < 3){
            continue; // skip if missing any info
        }

        ASRelationship r;
        r.asn1_ = static_cast<uint32_t>(std::stoul(tokens[0]));
        r.asn2_ = static_cast<uint32_t>(std::stoul(tokens[1]));
        r.type_ = std::stoi(tokens[2]);

        if(r.type_ == -1 || r.type_ == 0){
            relationships.push_back(r);
        }
    }

    return relationships;
}

std::vector<SeedAnnouncement> GraphParser::parseAnnouncements(const std::string& filename) {
    std::vector<SeedAnnouncement> announcements;

    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open announcements file: " + filename);
    }

    std::string line;
    bool firstLine = true;
    while (std::getline(file, line)) {
        // skip header line
        if (firstLine) { firstLine = false; continue; }

        // strip \r
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string token;
        std::vector<std::string> tokens;
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() < 3) continue;

        SeedAnnouncement ann;
        ann.seedAsn_ = static_cast<uint32_t>(std::stoul(tokens[0]));
        ann.prefix_ = tokens[1];
        ann.rovInvalid_ = (tokens[2] == "True" || tokens[2] == "true");
        announcements.push_back(ann);
    }

    return announcements;
}

#include <emscripten/bind.h>
#include <string>
#include "graph/Graph.h"
#include "graph/GraphParser.h"
#include "bgp/Announcement.h"

static std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else                out += c;
    }
    return out;
}

// topologyContent  : full text of a CAIDA relationship file (pipe-delimited)
// annsContent      : full text of an announcements CSV (seed_asn,prefix,rov_invalid)
// rovAsnsContent   : newline-separated list of ASNs that deploy ROV (may be empty)
// targetAsn        : ASN whose RIB to return
// returns JSON: { "asn": N, "results": [ { "prefix": "...", "as_path": [...], "recv_from": "..." }, ... ] }
//         or   { "error": "..." }
std::string runSimulation(
    const std::string& topologyContent,
    const std::string& annsContent,
    const std::string& rovAsnsContent,
    uint32_t targetAsn
) {
    try {
        auto relationships = GraphParser::parseFromString(topologyContent);
        Graph graph(relationships);

        if (!rovAsnsContent.empty())
            graph.loadROVASNsFromString(rovAsnsContent);

        auto seedAnns = GraphParser::parseAnnouncementsFromString(annsContent);
        for (const auto& sa : seedAnns) {
            Announcement ann(sa.prefix_, {sa.seedAsn_}, sa.seedAsn_,
                             Relationship::ORIGIN, sa.rovInvalid_);
            graph.seedAnnouncement(sa.seedAsn_, ann);
        }

        graph.propagate();
        return graph.getResultsAsJSON(targetAsn);
    } catch (const std::exception& e) {
        return "{\"error\":\"" + jsonEscape(e.what()) + "\"}";
    }
}

EMSCRIPTEN_BINDINGS(bgp_module) {
    emscripten::function("runSimulation", &runSimulation);
}

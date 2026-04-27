/* declares Graph class
 * reponsisble for owning all AS nodes, beuilding neighbor relationsips, running cycle detetction, computing propagatuin ranks
 * I used smart pointers for this. The only tradeoff is that I can't copy an AS node, but that should mirror the actaul functionality since I shouldn't copy a whole AS node
 */

# pragma once

# include <cstdint>
# include <memory>
# include <unordered_map>
# include <unordered_set>
# include <vector>
# include <string>

# include "graph/AS.h"
# include "graph/GraphParser.h"

class Graph{
public:
    //builds the graph using the CAIDA
    explicit Graph(const std::string& filename);

    //returna a RAW pointer to the AS, or nullptr is not found
    AS* getAS(uint32_t asn);

    //return the total number of AS nodes in the graph
    size_t size() const;

    void assignRanks();
    const std::vector<std::vector<AS*>>& getRanks() const;

    void seedAnnouncement(uint32_t asn, const Announcement& ann);

    void propagate();

    void dumpCSV(const std::string& filename) const;

    void loadROVASNs(const std::string& filename);

private:
    //use hash map to keep track of nodes (ASN = key, unique ptr = nodes)
    std::unordered_map<uint32_t, std::unique_ptr<AS>> nodes_;

    // creates an AS node in the graph if it already doesn't exist
    void ensureAS(uint32_t asn);

    //creates the relartionship from the parsed records
    void buildRelationships(const std::vector<ASRelationship>& relationships);

    //runs DFS cycle detection on the provider/customer edges (doesn't need to be checked for the peer-peer edges)
    //throws std::runtime_error if a cycle is found
    void detectCycles();

    //DFS helper dunction: returns true if a cycle is detected
    bool dfs(uint32_t asn, std::unordered_set<uint32_t>& visited, std::unordered_set<uint32_t>& inStack);

    std::vector<std::vector<AS*>> rankedASes_;

    void propagateUp();
    void propagateAcrossPeers();
    void propagateDown();

    // sends all local RIB announcements from one AS to a set of neighbors
    void sendTo(AS* sender,
                const std::unordered_set<uint32_t>& neighborAsns,
                Relationship recvFromRelationship);
};

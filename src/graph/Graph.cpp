/* implements graph construction - wiring up all neighbor pointers
 * cycle detecttion alg (DFS on provider/customer edges)
 * BFS rank assignment that flattens DAG into vector of vectors
 */

#include "graph/Graph.h"
#include <stdexcept>
#include <queue>
#include <fstream>
#include <sstream>
#include "bgp/Announcement.h"
#include "bgp/ROV.h"

Graph::Graph(const std::string& filename) {
    auto relationships = GraphParser::parse(filename);
    buildRelationships(relationships);
    detectCycles();
    assignRanks();
}

Graph::Graph(const std::vector<ASRelationship>& relationships) {
    buildRelationships(relationships);
    detectCycles();
    assignRanks();
}

AS* Graph::getAS(uint32_t asn) {
    auto it = nodes_.find(asn);
    if (it == nodes_.end()) {
        return nullptr;
    }
    return it->second.get(); // .get() extracts the raw pointer from unique_ptr
}

size_t Graph::size() const {
    return nodes_.size();
}

void Graph::ensureAS(uint32_t asn) {
    // only insert if this ASN doesn't already have a node
    if (nodes_.find(asn) == nodes_.end()) {
        nodes_[asn] = std::make_unique<AS>(asn);
    }
}

void Graph::buildRelationships(const std::vector<ASRelationship>& relationships){
    for (const auto& r : relationships) {
        // make sure both ASes exist before wiring them up
        ensureAS(r.asn1_);
        ensureAS(r.asn2_);

        if (r.type_ == -1) {
            // asn1 is the provider, asn2 is the customer
            nodes_[r.asn1_]->customers_.insert(r.asn2_);
            nodes_[r.asn2_]->providers_.insert(r.asn1_);
        } else if (r.type_ == 0) {
            // both are peers of each other
            nodes_[r.asn1_]->peers_.insert(r.asn2_);
            nodes_[r.asn2_]->peers_.insert(r.asn1_);
        }
    }
}

void Graph::detectCycles() {
    std::unordered_set<uint32_t> visited;
    std::unordered_set<uint32_t> inStack;

    for (const auto& pair : nodes_) {
        uint32_t asn = pair.first;
        if (visited.find(asn) == visited.end()) {
            if (dfs(asn, visited, inStack)) {
                throw std::runtime_error(
                    "Cycle detected in provider/customer relationships"
                );
            }
        }
    }
}

bool Graph::dfs(uint32_t asn,
                std::unordered_set<uint32_t>& visited,
                std::unordered_set<uint32_t>& inStack) {

    visited.insert(asn);
    inStack.insert(asn);

    // only follow customer edges - peer edges are intentionally ignored
    for (uint32_t customer : nodes_[asn]->customers_) {
        if (visited.find(customer) == visited.end()) {
            if (dfs(customer, visited, inStack)) {
                return true;
            }
        } else if (inStack.find(customer) != inStack.end()) {
            // found a node that's already in our current DFS path = cycle
            return true;
        }
    }

    inStack.erase(asn); // done with this node, remove from current path
    return false;
}

void Graph::assignRanks() {
    // start with all ASes that have no customers (leaves)
    std::queue<uint32_t> toProcess;

    for (auto& [asn, as] : nodes_) {
        if (as->customers_.empty()) {
            as->propagation_rank_ = 0;
            toProcess.push(asn);
        }
    }

    // BFS upward through providers
    while (!toProcess.empty()) {
        uint32_t asn = toProcess.front();
        toProcess.pop();

        AS* current = nodes_[asn].get();

        for (uint32_t providerAsn : current->providers_) {
            AS* provider = nodes_[providerAsn].get();
            int newRank = current->propagation_rank_ + 1;

            if (provider->propagation_rank_ < newRank) {
                provider->propagation_rank_ = newRank;
                toProcess.push(providerAsn);
            }
        }
    }

    // find the maximum rank
    int maxRank = 0;
    for (auto& [asn, as] : nodes_) {
        maxRank = std::max(maxRank, as->propagation_rank_);
    }

    // build the ranked vector of vectors
    rankedASes_.resize(maxRank + 1);
    for (auto& [asn, as] : nodes_) {
        if (as->propagation_rank_ >= 0) {
            rankedASes_[as->propagation_rank_].push_back(as.get());
        }
    }
}

const std::vector<std::vector<AS*>>& Graph::getRanks() const {
    return rankedASes_;
}

void Graph::seedAnnouncement(uint32_t asn, const Announcement& ann) {
    AS* as = getAS(asn);
    if (as == nullptr) return;

    //store directly into RIB - bypassing processQueue because origin announcements already have the correct path set
    as -> policy_ -> seedDirectly(ann);
}

void Graph::propagate() {
    propagateUp();
    propagateAcrossPeers();
    propagateDown();
}

void Graph::sendTo(AS* sender,
                   const std::unordered_set<uint32_t>& neighborAsns,
                   Relationship recvFromRelationship) {

    for (uint32_t neighborAsn : neighborAsns) {
        AS* neighbor = getAS(neighborAsn);
        if (neighbor == nullptr) continue;

        // send every announcement in the local RIB to this neighbor
        for (auto& [prefix, ann] : sender->policy_->getRIB()) {
            // create a modified copy for this hop
            Announcement outgoing = ann;
            outgoing.nextHop_ = sender->asn_;
            outgoing.recvFrom_ = recvFromRelationship;
            // prepend this AS to the path - UPDATE - take this out bevause we don't want to prepend AND postpend
            //outgoing.asPath_.insert(outgoing.asPath_.begin(), sender->asn_);

            neighbor->policy_->receiveAnnouncement(outgoing);
        }
    }
}

void Graph::propagateUp() {
    // go from rank 0 upward
    for (int rank = 0; rank < (int)rankedASes_.size(); rank++) {
        // first process any queued announcements at this rank
        for (AS* as : rankedASes_[rank]) {
            as->policy_->processQueue();
        }
        // then send upward to providers
        for (AS* as : rankedASes_[rank]) {
            sendTo(as, as->providers_, Relationship::CUSTOMER);
        }
    }
}

void Graph::propagateAcrossPeers() {
    // CRITICAL: all ASes send first, then all ASes process
    // doing it in one loop would let announcements travel multiple hops

    // phase 1 - everyone sends to their peers
    for (auto& [asn, as] : nodes_) {
        sendTo(as.get(), as->peers_, Relationship::PEER);
    }

    // phase 2 - everyone processes what they received
    for (auto& [asn, as] : nodes_) {
        as->policy_->processQueue();
    }
}

void Graph::propagateDown() {
    // go from highest rank downward
    for (int rank = (int)rankedASes_.size() - 1; rank >= 0; rank--) {
        // send downward to customers
        for (AS* as : rankedASes_[rank]) {
            sendTo(as, as->customers_, Relationship::PROVIDER);
        }
        // then process at the rank below (if it exists)
        if (rank > 0) {
            for (AS* as : rankedASes_[rank - 1]) {
                as->policy_->processQueue();
            }
        }
    }
}

void Graph::dumpCSV(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) {
        throw std::runtime_error("Could not open output file: " + filename);
    }

    out << "asn,prefix,as_path\r\n";

    for (auto& [asn, as] : nodes_) {
        for (auto& [prefix, ann] : as->policy_->getRIB()) {
            out << asn << "," << prefix << ",\"(";

            for (size_t i = 0; i < ann.asPath_.size(); i++) {
                if (i > 0) out << ", ";
                out << ann.asPath_[i];
            }

            // trailing comma only for single-element paths (Python tuple format)
            if (ann.asPath_.size() == 1) {
                out << ",";
            }

            out << ")\"\r\n";
        }
    }
}

static void applyROVFromStream(Graph& graph, std::istream& in) {
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;
        while (!line.empty() && (line.back() == '\r' || line.back() == ' '))
            line.pop_back();
        try {
            uint32_t asn = static_cast<uint32_t>(std::stoul(line));
            AS* as = graph.getAS(asn);
            if (as != nullptr)
                as->policy_ = std::make_unique<ROV>(asn);
        } catch (...) { continue; }
    }
}

void Graph::loadROVASNs(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Could not open ROV file: " + filename);
    applyROVFromStream(*this, file);
}

void Graph::loadROVASNsFromString(const std::string& content) {
    std::istringstream ss(content);
    applyROVFromStream(*this, ss);
}

static std::string relToString(Relationship r) {
    switch (r) {
        case Relationship::ORIGIN:   return "ORIGIN";
        case Relationship::CUSTOMER: return "CUSTOMER";
        case Relationship::PEER:     return "PEER";
        case Relationship::PROVIDER: return "PROVIDER";
    }
    return "UNKNOWN";
}

std::string Graph::getResultsAsJSON(uint32_t targetAsn) const {
    auto it = nodes_.find(targetAsn);
    if (it == nodes_.end())
        return "{\"error\":\"AS " + std::to_string(targetAsn) + " not found in topology\"}";

    const auto& rib = it->second->policy_->getRIB();
    std::string json = "{\"asn\":" + std::to_string(targetAsn) + ",\"results\":[";
    bool first = true;
    for (const auto& [prefix, ann] : rib) {
        if (!first) json += ",";
        first = false;
        json += "{\"prefix\":\"" + prefix + "\",\"as_path\":[";
        for (size_t i = 0; i < ann.asPath_.size(); i++) {
            if (i > 0) json += ",";
            json += std::to_string(ann.asPath_[i]);
        }
        json += "],\"recv_from\":\"" + relToString(ann.recvFrom_) + "\"}";
    }
    json += "]}";
    return json;
}

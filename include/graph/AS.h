/* declares AS struct/class
 * holds the ADN and 3 sets of neighbors, providers, customers, peers
 * stores propogation rank once graph is flattened
 */
# pragma once
# include <cstdint>
# include <unordered_set>
# include <memory>
# include "bgp/BGP.h"

struct AS{
    uint32_t asn_; // make the type uint32_t to limit it to 32 and make it be positive

    //providers of the node - keeeping track of their asn numbers
    std::unordered_set<uint32_t> providers_;

    //customers of the node
    std::unordered_set<uint32_t> customers_;

    //peers of the node
    std::unordered_set<uint32_t> peers_;

    int propagation_rank_ = -1;


    //every AS gets a BGP Polciy by default
    std::unique_ptr<Policy> policy_ = std::make_unique<BGP>(asn_);

    AS(uint32_t asn) : asn_(asn){}

};

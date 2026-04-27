/* declares Announcement struct
 * hold specific prefix (like 8.8.0/24) as AS-path (vector of ASNS)
 * next-hop ASN
 * recieved-from relationship (origin/customer/peer/provider)
 * row_invalid boolean */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

// represents the relationship an announcement was received from - 4 options
enum class Relationship {
    ORIGIN,    // the AS that originated the announcement - always preferred
    CUSTOMER,  // received from a customer
    PEER,      // received from a peer
    PROVIDER   // received from a provider
};

struct Announcement {
    std::string prefix_;           // e.g. "1.2.0.0/16"
    std::vector<uint32_t> asPath_; // list of ASNs the announcement has traveled through
    uint32_t nextHop_;             // ASN this announcement just came from
    Relationship recvFrom_;        // relationship type it was received from
    bool rovInvalid_ = false;      // true if this announcement is a hijack

    //announcement constructor as an initializer list
    Announcement(
        const std::string& prefix,
        const std::vector<uint32_t>& asPath,
        uint32_t nextHop,
        Relationship recvFrom,
        bool rovInvalid = false
    ) : prefix_(prefix),
        asPath_(asPath),
        nextHop_(nextHop),
        recvFrom_(recvFrom),
        rovInvalid_(rovInvalid) {}
};
